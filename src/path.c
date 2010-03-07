/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * POI and GPS-Info code originally written by Cezary Jackiewicz.
 *
 * Default map data provided by http://www.openstreetmap.org/
 *
 * This file is part of Maemo Mapper.
 *
 * Maemo Mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Maemo Mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Maemo Mapper.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sqlite3.h>

#include <hildon/hildon-check-button.h>
#include <hildon/hildon-entry.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-sound.h>

#include "types.h"
#include "data.h"
#include "debug.h"
#include "defines.h"
#include "dialog.h"
#include "display.h"
#include "error.h"
#include "gpx.h"
#include "main.h"
#include "path.h"
#include "poi.h"
#include "router.h"
#include "util.h"
#include "screen.h"

/* uncertainty below which a GPS position is considered to be precise enough 
 * to be added to the track (in metres) */
#define MAX_UNCERTAINTY 200

/* minimum distance the GPS position must have from the previous track point in
 * order to be registered in the track (in metres) */
#define MIN_TRACK_DISTANCE 10

typedef union {
    gint field;
    struct {
        gchar zoom;
        gint16 altitude;
    } s;
} FieldMix;

typedef struct {
    GtkWindow *dialog;
    GtkWidget *autoroute;
    GtkWidget *origin;
    GtkWidget *destination;
    GtkWidget *btn_swap;
    HildonTouchSelector *router_selector;
    gint origin_row_gps;
    gint origin_row_route;
    gint origin_row_other;
    gint origin_row_active;

    gboolean replace;

    MapLocation from;
    MapLocation to;

    GPtrArray *routers;
    gboolean is_computing;
} RouteDownloadInfo;
#define ROUTE_DOWNLOAD_INFO     "rdi"
#define MAP_RESPONSE_OPTIONS    2

/** Data used during the asynchronous automatic route downloading operation. */
typedef struct {
    gboolean enabled;
    gboolean in_progress;
    MapRouter *router;
    MapLocation dest;
} AutoRouteDownloadData;

static AutoRouteDownloadData _autoroute_data;

/* _near_point is the route point to which we are closest. */
static Point *_near_point = NULL;

/* _next_way is what we currently interpret to be the next waypoint. */
static WayPoint *_next_way;
static gint64 _next_way_dist_squared = INT64_MAX;

/* _next_wpt is the route point immediately following _next_way. */
static Point *_next_wpt = NULL;
static gint64 _next_wpt_dist_squared = INT64_MAX;

static gfloat _initial_distance_from_waypoint = -1.f;
static WayPoint *_initial_distance_waypoint = NULL;

static sqlite3 *_path_db = NULL;
static sqlite3_stmt *_track_stmt_select = NULL;
static sqlite3_stmt *_track_stmt_delete_path = NULL;
static sqlite3_stmt *_track_stmt_delete_way = NULL;
static sqlite3_stmt *_track_stmt_insert_path = NULL;
static sqlite3_stmt *_track_stmt_insert_way = NULL;
static sqlite3_stmt *_route_stmt_select = NULL;
static sqlite3_stmt *_route_stmt_delete_path = NULL;
static sqlite3_stmt *_route_stmt_delete_way = NULL;
static sqlite3_stmt *_route_stmt_insert_path = NULL;
static sqlite3_stmt *_route_stmt_insert_way = NULL;
static sqlite3_stmt *_path_stmt_trans_begin = NULL;
static sqlite3_stmt *_path_stmt_trans_commit = NULL;
static sqlite3_stmt *_path_stmt_trans_rollback = NULL;

static gchar *_last_spoken_phrase;

static MapRouter *
get_selected_router(RouteDownloadInfo *rdi)
{
    gint idx;

    idx = hildon_touch_selector_get_active (rdi->router_selector, 0);
    g_assert(idx >= 0 && idx < rdi->routers->len);

    return g_ptr_array_index(rdi->routers, idx);
}

static GPtrArray *
get_routers(MapController *controller)
{
    const GSList *list;
    static GPtrArray *routers = NULL;

    /* FIXME: This function assumes the routers' list will never change at
     * runtime. */
    if (routers) return routers;

    routers = g_ptr_array_sized_new(4);
    for (list = map_controller_list_plugins(controller); list != NULL;
         list = list->next)
    {
        MapRouter *router = list->data;

        if (!MAP_IS_ROUTER(router)) continue;

        /* FIXME: We should at least weakly reference the routers */
        g_ptr_array_add(routers, router);
    }

    return routers;
}

void
path_resize(Path *path, gint size)
{
    if(path->head + size != path->cap)
    {
        Point *old_head = path->head;
        WayPoint *curr;
        path->head = g_renew(Point, old_head, size);
        path->cap = path->head + size;
        if(path->head != old_head)
        {
            path->tail = path->head + (path->tail - old_head);

            /* Adjust all of the waypoints. */
            for(curr = path->whead - 1; curr++ != path->wtail; )
                curr->point = path->head + (curr->point - old_head);
        }
    }
}

void
path_wresize(Path *path, gint wsize)
{
    if(path->whead + wsize != path->wcap)
    {
        WayPoint *old_whead = path->whead;
        path->whead = g_renew(WayPoint, old_whead, wsize);
        path->wtail = path->whead + (path->wtail - old_whead);
        path->wcap = path->whead + wsize;
    }
}

static void
read_path_from_db(Path *path, sqlite3_stmt *select_stmt)
{
    MACRO_PATH_INIT(*path);
    while(SQLITE_ROW == sqlite3_step(select_stmt))
    {
        const gchar *desc;
        FieldMix mix;

        MACRO_PATH_INCREMENT_TAIL(*path);
        path->tail->unit.x = sqlite3_column_int(select_stmt, 0);
        path->tail->unit.y = sqlite3_column_int(select_stmt, 1);
        path->tail->time = sqlite3_column_int(select_stmt, 2);
        mix.field = sqlite3_column_int(select_stmt, 3);
        path->tail->zoom = mix.s.zoom;
        path->tail->altitude = mix.s.altitude;

        desc = (const gchar *)sqlite3_column_text(select_stmt, 4);
        if(desc)
        {
            MACRO_PATH_INCREMENT_WTAIL(*path);
            path->wtail->point = path->tail;
            path->wtail->desc = g_strdup(desc);
        }
    }
    sqlite3_reset(select_stmt);

    /* If the last point isn't null, then add another null point. */
    if(path->tail->unit.y)
    {
        MACRO_PATH_INCREMENT_TAIL(*path);
        *path->tail = _point_null;
    }

    map_path_optimize(path);
}

/* Returns the new next_update_index. */
static gint
write_path_to_db(Path *path,
        sqlite3_stmt *delete_path_stmt,
        sqlite3_stmt *delete_way_stmt,
        sqlite3_stmt *insert_path_stmt,
        sqlite3_stmt *insert_way_stmt,
        gint index_last_saved)
{
    Point *curr;
    WayPoint *wcurr;
    gint num;
    gboolean success = TRUE;
    DEBUG("%d", index_last_saved);

    /* Start transaction. */
    sqlite3_step(_path_stmt_trans_begin);
    sqlite3_reset(_path_stmt_trans_begin);

    if(index_last_saved == 0)
    {
        /* Replace the whole thing, so delete the table first. */
        if(SQLITE_DONE != sqlite3_step(delete_way_stmt)
                || SQLITE_DONE != sqlite3_step(delete_path_stmt))
        {
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%s\n%s",
                    _("Failed to write to path database. "
                        "Tracks and routes may not be saved."),
                    sqlite3_errmsg(_path_db));
            popup_error(_window, buffer);
            success = FALSE;
        }
        sqlite3_reset(delete_way_stmt);
        sqlite3_reset(delete_path_stmt);
    }

    for(num = index_last_saved, curr = path->head + num, wcurr = path->whead;
            success && ++curr <= path->tail; ++num)
    {
        FieldMix mix;

        /* If this is the last point, and it is null, don't write it. */
        if(curr == path->tail && !curr->unit.y)
            break;

        mix.field = 0;
        mix.s.altitude = curr->altitude;
        mix.s.zoom = curr->zoom;

        /* Insert the path point. */
        if(SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 1, curr->unit.x)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 2, curr->unit.y)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 3, curr->time)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 4, mix.field)
        || SQLITE_DONE != sqlite3_step(insert_path_stmt))
        {
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%s\n%s",
                    _("Failed to write to path database. "
                        "Tracks and routes may not be saved."),
                    sqlite3_errmsg(_path_db));
            popup_error(_window, buffer);
            success = FALSE;
        }
        sqlite3_reset(insert_path_stmt);

        /* Now, check if curr is a waypoint. */
        if(success && wcurr <= path->wtail && wcurr->point == curr)
        {
            gint num = sqlite3_last_insert_rowid(_path_db);
            if(SQLITE_OK != sqlite3_bind_int(insert_way_stmt, 1, num)
            || SQLITE_OK != sqlite3_bind_text(insert_way_stmt, 2, wcurr->desc,
                -1, SQLITE_STATIC)
            || SQLITE_DONE != sqlite3_step(insert_way_stmt))
            {
                gchar buffer[BUFFER_SIZE];
                snprintf(buffer, sizeof(buffer), "%s\n%s",
                        _("Failed to write to path database. "
                            "Tracks and routes may not be saved."),
                        sqlite3_errmsg(_path_db));
                popup_error(_window, buffer);
                success = FALSE;
            }
            sqlite3_reset(insert_way_stmt);
            wcurr++;
        }
    }
    if(success)
    {
        sqlite3_step(_path_stmt_trans_commit);
        sqlite3_reset(_path_stmt_trans_commit);
        return num;
    }
    else
    {
        sqlite3_step(_path_stmt_trans_rollback);
        sqlite3_reset(_path_stmt_trans_rollback);
        return index_last_saved;
    }
}

void
path_save_route_to_db()
{
    if(_path_db)
    {
        write_path_to_db(&_route,
                          _route_stmt_delete_path,
                          _route_stmt_delete_way,
                          _route_stmt_insert_path,
                          _route_stmt_insert_way,
                          0);
    }
}

static void
path_save_track_to_db()
{
    if(_path_db)
    {
        write_path_to_db(&_track,
                          _track_stmt_delete_path,
                          _track_stmt_delete_way,
                          _track_stmt_insert_path,
                          _track_stmt_insert_way,
                          0);
    }
}

static void
path_update_track_in_db()
{
    if(_path_db)
    {
        _track_index_last_saved = write_path_to_db(&_track,
                          _track_stmt_delete_path,
                          _track_stmt_delete_way,
                          _track_stmt_insert_path,
                          _track_stmt_insert_way,
                          _track_index_last_saved);
    }
}

/**
 * Updates _near_point, _next_way, and _next_wpt.  If quick is FALSE (as
 * it is when this function is called from route_find_nearest_point), then
 * the entire list (starting from _near_point) is searched.  Otherwise, we
 * stop searching when we find a point that is farther away.
 */
static gboolean
route_update_nears(gboolean quick)
{
    MapController *controller = map_controller_get_instance();
    gboolean ret = FALSE;
    Point *curr, *near;
    WayPoint *wcurr, *wnext;
    gint64 near_dist_squared;
    DEBUG("%d", quick);

    /* If we have waypoints (_next_way != NULL), then determine the "next
     * waypoint", which is defined as the waypoint after the nearest point,
     * UNLESS we've passed that waypoint, in which case the waypoint after
     * that waypoint becomes the "next" waypoint. */
    if(_next_way)
    {
        /* First, set near_dist_squared with the new distance from
         * _near_point. */
        near = _near_point;
        near_dist_squared = DISTANCE_SQUARED(_pos.unit, near->unit);

        /* Now, search _route for a closer point.  If quick is TRUE, then we'll
         * only search forward, only as long as we keep finding closer points.
         */
        for(curr = _near_point; curr++ != _route.tail; )
        {
            if(curr->unit.y)
            {
                gint64 dist_squared = DISTANCE_SQUARED(_pos.unit, curr->unit);
                if(dist_squared <= near_dist_squared)
                {
                    near = curr;
                    near_dist_squared = dist_squared;
                }
                else if(quick)
                    break;
            }
        }

        /* Update _near_point. */
        _near_point = near;

        for(wnext = wcurr = _next_way; wcurr < _route.wtail; wcurr++)
        {
            if(wcurr->point < near
            /* Okay, this else if expression warrants explanation.  If the
             * nearest track point happens to be a waypoint, then we want to
             * check if we have "passed" that waypoint.  To check this, we
             * test the distance from _pos to the waypoint and from _pos to
             * _next_wpt, and if the former is increasing and the latter is
             * decreasing, then we have passed the waypoint, and thus we
             * should skip it.  Note that if there is no _next_wpt, then
             * there is no next waypoint, so we do not skip it in that case. */
                || (wcurr->point == near && quick
                    && (_next_wpt
                     && (DISTANCE_SQUARED(_pos.unit, near->unit) >
                                                     _next_way_dist_squared &&
                         DISTANCE_SQUARED(_pos.unit, _next_wpt->unit)
                                                   < _next_wpt_dist_squared))))
            {
                wnext = wcurr + 1;
            }
            else
                break;
        }

        if(wnext == _route.wtail && (wnext->point < near
                || (wnext->point == near && quick
                    && (_next_wpt
                     && (DISTANCE_SQUARED(_pos.unit, near->unit) >
                                                     _next_way_dist_squared &&
                         DISTANCE_SQUARED(_pos.unit, _next_wpt->unit)
                                                 < _next_wpt_dist_squared)))))
        {
            _next_way = NULL;
            _next_wpt = NULL;
            _next_way_dist_squared = INT64_MAX;
            _next_wpt_dist_squared = INT64_MAX;
            ret = TRUE;
        }
        /* Only update _next_way (and consequently _next_wpt) if _next_way is
         * different, and record that fact for return. */
        else
        {
            if(!quick || _next_way != wnext)
            {
                _next_way = wnext;
                _next_wpt = wnext->point;
                if(_next_wpt == _route.tail)
                    _next_wpt = NULL;
                else
                {
                    while(!(++_next_wpt)->unit.y)
                    {
                        if(_next_wpt == _route.tail)
                        {
                            _next_wpt = NULL;
                            break;
                        }
                    }
                }
                ret = TRUE;
            }
            _next_way_dist_squared =
                DISTANCE_SQUARED(_pos.unit, wnext->point->unit);
            if(_next_wpt)
                _next_wpt_dist_squared =
                    DISTANCE_SQUARED(_pos.unit, _next_wpt->unit);
        }
    }

    map_controller_set_next_waypoint(controller, _next_way);

    return ret;
}

/**
 * Reset the _near_point data by searching the entire route for the nearest
 * route point and waypoint.
 */
void
route_find_nearest_point()
{
    /* Initialize _near_point to first non-zero point. */
    _near_point = _route.head;
    while(!_near_point->unit.y && _near_point != _route.tail)
        _near_point++;

    /* Initialize _next_way. */
    if(_route.wtail < _route.whead)
        _next_way = NULL;
    else
        /* We have at least one waypoint. */
        _next_way = _route.whead;
    _next_way_dist_squared = INT64_MAX;

    /* Initialize _next_wpt. */
    _next_wpt = NULL;
    _next_wpt_dist_squared = INT64_MAX;

    route_update_nears(FALSE);
}

/**
 * Calculates the distance from the current GPS location to the given point,
 * following the route.  If point is NULL, then the distance is shown to the
 * next waypoint.
 */
gboolean
route_calc_distance_to(const Point *point, gfloat *distance)
{
    gdouble lat1, lon1, lat2, lon2;
    gfloat sum = 0.0;

    /* If point is NULL, use the next waypoint. */
    if(point == NULL && _next_way)
        point = _next_way->point;

    /* If point is still NULL, return an error. */
    if(point == NULL)
    {
        return FALSE;
    }

    unit2latlon(_pos.unit.x, _pos.unit.y, lat1, lon1);
    if(point > _near_point)
    {
        Point *curr;
        /* Skip _near_point in case we have already passed it. */
        for(curr = _near_point + 1; curr <= point; ++curr)
        {
            if(curr->unit.y)
            {
                unit2latlon(curr->unit.x, curr->unit.y, lat2, lon2);
                sum += calculate_distance(lat1, lon1, lat2, lon2);
                lat1 = lat2;
                lon1 = lon2;
            }
        }
    }
    else if(point < _near_point)
    {
        Point *curr;
        /* Skip _near_point in case we have already passed it. */
        for(curr = _near_point - 1; curr >= point; --curr)
        {
            if(curr->unit.y)
            {
                unit2latlon(curr->unit.x, curr->unit.y, lat2, lon2);
                sum += calculate_distance(lat1, lon1, lat2, lon2);
                lat1 = lat2;
                lon1 = lon2;
            }
        }
    }
    else
    {
        /* Waypoint _is_ the nearest point. */
        unit2latlon(_near_point->unit.x, _near_point->unit.y, lat2, lon2);
        sum += calculate_distance(lat1, lon1, lat2, lon2);
    }

    *distance = sum;
    return TRUE;
}

/**
 * Show the distance from the current GPS location to the given point,
 * following the route.  If point is NULL, then the distance is shown to the
 * next waypoint.
 */
gboolean
route_show_distance_to(Point *point)
{
    gchar buffer[80];
    gfloat sum;

    if (!route_calc_distance_to(point, &sum))
        return FALSE;

    snprintf(buffer, sizeof(buffer), "%s: %.02f %s", _("Distance"),
            sum * UNITS_CONVERT[_units], UNITS_ENUM_TEXT[_units]);
    MACRO_BANNER_SHOW_INFO(_window, buffer);

    return TRUE;
}

void
route_show_distance_to_next()
{
    if(!route_show_distance_to(NULL))
    {
        MACRO_BANNER_SHOW_INFO(_window, _("There is no next waypoint."));
    }
}

void
route_show_distance_to_last()
{
    if(_route.head != _route.tail)
    {
        /* Find last non-zero point. */
        Point *p;
        for(p = _route.tail; !p->unit.y; p--) { }
        route_show_distance_to(p);
    }
    else
    {
        MACRO_BANNER_SHOW_INFO(_window, _("The current route is empty."));
    }
}

static void
track_show_distance_from(Point *point)
{
    gchar buffer[80];
    gdouble lat1, lon1, lat2, lon2;
    gdouble sum = 0.0;
    Point *curr;
    unit2latlon(_pos.unit.x, _pos.unit.y, lat1, lon1);

    /* Skip _track.tail because that should be _pos. */
    for(curr = _track.tail; curr > point; --curr)
    {
        if(curr->unit.y)
        {
            unit2latlon(curr->unit.x, curr->unit.y, lat2, lon2);
            sum += calculate_distance(lat1, lon1, lat2, lon2);
            lat1 = lat2;
            lon1 = lon2;
        }
    }

    snprintf(buffer, sizeof(buffer), "%s: %.02f %s", _("Distance"),
            sum * UNITS_CONVERT[_units], UNITS_ENUM_TEXT[_units]);
    MACRO_BANNER_SHOW_INFO(_window, buffer);
}

void
track_show_distance_from_last()
{
    /* Find last zero point. */
    if(_track.head != _track.tail)
    {
        Point *point;
        /* Find last zero point. */
        for(point = _track.tail; point->unit.y; point--) { }
        track_show_distance_from(point);
    }
    else
    {
        MACRO_BANNER_SHOW_INFO(_window, _("The current track is empty."));
    }
}

void
track_show_distance_from_first()
{
    /* Find last zero point. */
    if(_track.head != _track.tail)
        track_show_distance_from(_track.head);
    else
    {
        MACRO_BANNER_SHOW_INFO(_window, _("The current track is empty."));
    }
}

static void
auto_calculate_route_cb(MapRouter *router, Path *path, const GError *error)
{
    MapController *controller = map_controller_get_instance();

    DEBUG("called (error = %p)", error);

    if (G_UNLIKELY(error))
        cancel_autoroute();
    else
    {
        map_path_merge(path, &_route, MAP_PATH_MERGE_POLICY_REPLACE);
        path_save_route_to_db();

        /* Find the nearest route point, if we're connected. */
        route_find_nearest_point();

        map_controller_refresh_paths(controller);
    }

    _autoroute_data.in_progress = FALSE;
}

/**
 * This function is periodically run to download updated auto-route data
 * from the route source.
 */
static gboolean
auto_route_dl_idle()
{
    MapRouterQuery rq;

    memset(&rq, 0, sizeof(rq));
    rq.from.point = _pos.unit;
    rq.to = _autoroute_data.dest;

    map_router_calculate_route(_autoroute_data.router, &rq,
        (MapRouterCalculateRouteCb)auto_calculate_route_cb, NULL);

    return FALSE;
}

void
path_reset_route()
{
    route_find_nearest_point();
}

void
map_path_route_step(const MapGpsData *gps, gboolean newly_fixed)
{
    MapController *controller = map_controller_get_instance();
    gboolean show_directions = TRUE;
    gint announce_thres_unsquared;
    gboolean refresh_panel = FALSE;
    gboolean moving = FALSE;
    gboolean approaching_waypoint = FALSE;
    gboolean late = FALSE, out_of_route = FALSE;
    Point pos = _point_null;

    announce_thres_unsquared = (20+gps->speed) * _announce_notice_ratio*32;

    /* Check if we are late, with a tolerance of 3 minutes */
    if (_near_point && _near_point->time != 0 && pos.time != 0 &&
        pos.time > _near_point->time + 60 * 3)
        late = TRUE;
    DEBUG("Late: %d", late);

    {
        refresh_panel = TRUE;

        /* Update the nearest-waypoint data. */
        if(_route.head != _route.tail
                && (newly_fixed ? (route_find_nearest_point(), TRUE)
                                : route_update_nears(TRUE)))
        {
            /* FIXME: rewrite this condition */
        }

        /* Calculate distance to route. (point to line) */
        if(_near_point)
        {
            gint route_x1, route_x2, route_y1, route_y2;
            gint64 route_dist_squared_1 = INT64_MAX;
            gint64 route_dist_squared_2 = INT64_MAX;
            gfloat slope;

            route_x1 = _near_point->unit.x;
            route_y1 = _near_point->unit.y;

            /* Try previous point first. */
            if(_near_point != _route.head && _near_point[-1].unit.y)
            {
                route_x2 = _near_point[-1].unit.x;
                route_y2 = _near_point[-1].unit.y;
                slope = (gfloat)(route_y2 - route_y1)
                    / (gfloat)(route_x2 - route_x1);

                if(route_x1 == route_x2)
                {
                    /* Vertical line special case. */
                    route_dist_squared_1 = (pos.unit.x - route_x1)
                        * (pos.unit.x - route_x1);
                }
                else
                {
                    route_dist_squared_1 = abs((slope * pos.unit.x)
                        - pos.unit.y + (route_y1 - (slope * route_x1)));
                    route_dist_squared_1 =
                        route_dist_squared_1 * route_dist_squared_1
                        / ((slope * slope) + 1);
                }
            }
            if(_near_point != _route.tail && _near_point[1].unit.y)
            {
                route_x2 = _near_point[1].unit.x;
                route_y2 = _near_point[1].unit.y;
                slope = (gfloat)(route_y2 - route_y1)
                    / (gfloat)(route_x2 - route_x1);

                if(route_x1 == route_x2)
                {
                    /* Vertical line special case. */
                    route_dist_squared_2 = (pos.unit.x - route_x1)
                        * (pos.unit.x - route_x1);
                }
                else
                {
                    route_dist_squared_2 = abs((slope * pos.unit.x)
                        - pos.unit.y + (route_y1 - (slope * route_x1)));
                    route_dist_squared_2 =
                        route_dist_squared_2 * route_dist_squared_2
                        / ((slope * slope) + 1);
                }
            }

            /* Check if our distance from the route is large. */
            if(MIN(route_dist_squared_1, route_dist_squared_2)
                    > (2000 * 2000))
            {
                out_of_route = TRUE;
            }
        }

        /* Keep the display on. */
        moving = TRUE;
    }

    /* check if we need to recalculate the route */
    if (late || out_of_route)
    {
        /* Prevent announcments from occurring. */
        announce_thres_unsquared = INT_MAX;

        if(_autoroute_data.enabled && !_autoroute_data.in_progress)
        {
            MACRO_BANNER_SHOW_INFO(_window,
                                   _("Recalculating directions..."));
            _autoroute_data.in_progress = TRUE;
            show_directions = FALSE;
            g_idle_add((GSourceFunc)auto_route_dl_idle, NULL);
        }
        else
        {
            /* Reset the route to try and find the nearest point.*/
            path_reset_route();
        }
    }

    if(_initial_distance_waypoint
           && (_next_way != _initial_distance_waypoint
           ||  _next_way_dist_squared > (_initial_distance_from_waypoint
                                       * _initial_distance_from_waypoint)))
    {
        /* We've moved on to the next waypoint, or we're really far from
         * the current waypoint. */
        if(_waypoint_banner)
        {
            gtk_widget_destroy(_waypoint_banner);
            _waypoint_banner = NULL;
        }
        _initial_distance_from_waypoint = -1.f;
        _initial_distance_waypoint = NULL;
    }

    /* Check if we should announce upcoming waypoints. */
    if(_enable_announce
            && (_initial_distance_waypoint || _next_way_dist_squared
                < (announce_thres_unsquared * announce_thres_unsquared)))
    {
        if(show_directions)
        {
            if(!_initial_distance_waypoint)
            {
                /* First time we're close enough to this waypoint. */
                if(_enable_voice
                        /* And that we haven't already announced it. */
                        && strcmp(_next_way->desc, _last_spoken_phrase))
                {
                    g_free(_last_spoken_phrase);
                    _last_spoken_phrase = g_strdup(_next_way->desc);
                    if(!fork())
                    {
                        /* We are the fork child.  Synthesize the voice. */
                        hildon_play_system_sound(
                            "/usr/share/sounds/ui-information_note.wav");
                        sleep(1);
                        DEBUG("%s %s", _voice_synth_path, _last_spoken_phrase);
                        execl(_voice_synth_path, basename(_voice_synth_path),
                                "-t", _last_spoken_phrase, (char *)NULL);
                        /* No good?  Try to launch it with /bin/sh */
                        execl("/bin/sh", "sh", _voice_synth_path,
                                "-t", _last_spoken_phrase, (char *)NULL);
                        /* Still no good? Oh well... */
                        exit(0);
                    }
                }
                _initial_distance_from_waypoint
                    = sqrtf(_next_way_dist_squared);
                _initial_distance_waypoint = _next_way;
                if(_next_wpt && _next_wpt->unit.y != 0)
                {
                    /* Create a banner for us the show progress. */
                    _waypoint_banner = hildon_banner_show_progress(
                            _window, NULL, _next_way->desc);
                }
                else
                {
                    /* This is the last point in a segment, i.e.
                     * "Arrive at ..." - just announce. */
                    MACRO_BANNER_SHOW_INFO(_window, _next_way->desc);
                }
            }
            else if(_waypoint_banner);
            {
                /* We're already close to this waypoint. */
                gdouble fraction = 1.f - (sqrtf(_next_way_dist_squared)
                        / _initial_distance_from_waypoint);
                BOUND(fraction, 0.f, 1.f);
                hildon_banner_set_fraction(
                        HILDON_BANNER(_waypoint_banner), fraction);
            }
        }
        approaching_waypoint = TRUE;
    }
    else if(_next_way_dist_squared > 2 * (_initial_distance_from_waypoint
                                        * _initial_distance_from_waypoint))
    {
        /* We're too far away now - destroy the banner. */
    }

    UNBLANK_SCREEN(moving, approaching_waypoint);

    if (refresh_panel)
    {
        MapScreen *screen = map_controller_get_screen(controller);
        map_screen_refresh_panel(screen);
    }
}

/**
 * Returns TRUE if the point needs to be added to the path.
 */
static gboolean
point_is_significant(const MapGpsData *gps, const Path *path)
{
    gint xdiff, ydiff, min_distance_units;

    /* check if the track is empty or was in a break */
    if (!path->tail->unit.y)
        return TRUE;

    /* check how much time has passed since last update */
    if (gps->time - path->tail->time > 60)
        return TRUE;

    /* Have we moved enough? */
    xdiff = abs(gps->unit.x - path->tail->unit.x);
    ydiff = abs(gps->unit.y - path->tail->unit.y);

    /* check if the distances are obviously big enough */
    if (xdiff >= METRES_TO_UNITS(300) ||
        ydiff >= METRES_TO_UNITS(300))
        return TRUE;

    /* Euristics to compute the minimum distance: we use MIN_TRACK_DISTANCE +
     * a fraction of the error. */
    min_distance_units = METRES_TO_UNITS(MIN_TRACK_DISTANCE + gps->hdop / 3);
    if (SQUARE(xdiff) + SQUARE(ydiff) >= SQUARE(min_distance_units))
        return TRUE;

    return FALSE;
}

/**
 * Add a point to the _track list.
 */
void
map_path_track_update(const MapGpsData *gps)
{
    MapController *controller = map_controller_get_instance();
    Point pos = _point_null;
    gboolean must_add = FALSE;

    DEBUG("%d, %d, %d (fix = %d)", (guint)gps->time,
          gps->unit.x, gps->unit.y, gps->fields & MAP_GPS_LATLON);

    if (gps->fields & MAP_GPS_LATLON)
    {
        /* Add the point to the track only if it's not too inaccurate: if the
         * uncertainty is greater than 200m, don't add it (TODO: this should be
         * a configuration option). */
        if (gps->hdop < MAX_UNCERTAINTY && point_is_significant(gps, &_track))
        {
            MapScreen *screen = map_controller_get_screen(controller);

            must_add = TRUE;

            pos.unit = gps->unit;
            if (gps->fields & MAP_GPS_ALTITUDE)
                pos.altitude = gps->altitude;

            /* Draw the line immediately */
            map_screen_track_append(screen, &pos);
        }
    }
    else
    {
        /* insert a break, if there isn't one already */
        if (_track.tail->unit.y != 0)
            must_add = TRUE;
    }
    pos.time = gps->time;

    if (must_add)
    {
        MACRO_PATH_INCREMENT_TAIL(_track);
        *_track.tail = pos;
        map_path_optimize(&_track);
    }

    /* Maybe update the track database. */
    {
        static time_t last_track_db_update = 0;
        if (!gps->time || (gps->time - last_track_db_update > 60
                && _track.tail - _track.head + 1 > _track_index_last_saved))
        {
            path_update_track_in_db();
            last_track_db_update = gps->time;
        }
    }
}

void
track_clear()
{
    GtkWidget *confirm;

    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window),
                            _("Really clear the track?"));

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm))) {
        MapController *controller = map_controller_get_instance();
        MACRO_PATH_FREE(_track);
        MACRO_PATH_INIT(_track);
        path_save_track_to_db();
        map_controller_refresh_paths(controller);
    }

    gtk_widget_destroy(confirm);
}

void
track_insert_break(gboolean temporary)
{
    if(_track.tail->unit.y)
    {
        MACRO_PATH_INCREMENT_TAIL(_track);
        *_track.tail = _point_null;

        /* To mark a "waypoint" in a track, we'll add a (0, 0) point and then
         * another instance of the most recent track point. */
        if(temporary)
        {
            MACRO_PATH_INCREMENT_TAIL(_track);
            *_track.tail = _track.tail[-2];
        }
    }

    /* Update the track database. */
    path_update_track_in_db();
}

/**
 * Cancel the current auto-route.
 */
void
cancel_autoroute()
{
    DEBUG("");

    if(_autoroute_data.enabled)
    {
        g_free(_autoroute_data.dest.address);
        g_object_unref(_autoroute_data.router);
        /* this also sets the enabled flag to FALSE */
        memset(&_autoroute_data, 0, sizeof(_autoroute_data));
    }
}

gboolean
autoroute_enabled()
{
    return _autoroute_data.enabled;
}

WayPoint *
find_nearest_waypoint(gint unitx, gint unity)
{
    WayPoint *wcurr;
    WayPoint *wnear;
    gint64 nearest_squared;
    MapPoint pos = { unitx, unity };
    DEBUG("");

    wcurr = wnear = _route.whead;
    if(wcurr && wcurr <= _route.wtail)
    {
        nearest_squared = DISTANCE_SQUARED(pos, wcurr->point->unit);

        wnear = _route.whead;
        while(++wcurr <=  _route.wtail)
        {
            gint64 test_squared = DISTANCE_SQUARED(pos, wcurr->point->unit);
            if(test_squared < nearest_squared)
            {
                wnear = wcurr;
                nearest_squared = test_squared;
            }
        }

        /* Only use the waypoint if it is within a 6*_draw_width square drawn
         * around the position. This is consistent with select_poi(). */
        if(abs(unitx - wnear->point->unit.x) < pixel2unit(3 * _draw_width)
            && abs(unity - wnear->point->unit.y) < pixel2unit(3 * _draw_width))
            return wnear;
    }

    return NULL;
}

static void
on_origin_changed_other(HildonPickerButton *button, RouteDownloadInfo *rdi)
{
    GtkEntryCompletion *completion;
    GtkWidget *dialog;
    GtkWidget *entry;
    gboolean chose = FALSE;

    /* if the "Other" option is chosen then ask the user to enter a location */
    if (hildon_picker_button_get_active(button) != rdi->origin_row_other)
        return;

    dialog = gtk_dialog_new_with_buttons
        (_("Origin"), rdi->dialog, GTK_DIALOG_MODAL,
         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
         NULL);

    entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(_loc_model));
    gtk_entry_completion_set_text_column(completion, 0);
    gtk_entry_set_completion(GTK_ENTRY(entry), completion);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry,
                       FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar *origin;

        origin = gtk_entry_get_text(GTK_ENTRY(entry));
        if (!STR_EMPTY(origin))
        {
            hildon_button_set_value(HILDON_BUTTON(button), origin);
            chose = TRUE;
        }
    }

    if (!chose)
        hildon_picker_button_set_active(button, 0);

    gtk_widget_destroy(dialog);
}

static void
on_origin_changed_gps(HildonPickerButton *button, RouteDownloadInfo *rdi)
{
    gboolean enable;

    /* enable autoroute only if the GPS option is chosen */
    enable = (hildon_picker_button_get_active(button) == rdi->origin_row_gps);

    gtk_widget_set_sensitive(rdi->autoroute, enable);
}

static void
on_router_selector_changed(HildonPickerButton *button, RouteDownloadInfo *rdi)
{
    MapRouter *router;

    router = get_selected_router(rdi);
    g_assert(router != NULL);

    gtk_dialog_set_response_sensitive(GTK_DIALOG(rdi->dialog),
                                      MAP_RESPONSE_OPTIONS,
                                      map_router_has_options(router));
}

static void
calculate_route_cb(MapRouter *router, Path *path, const GError *error,
                   GtkDialog **p_dialog)
{
    GtkDialog *dialog = *p_dialog;
    MapController *controller = map_controller_get_instance();
    RouteDownloadInfo *rdi;
    GtkTreeIter iter;
    const gchar *from, *to;

    DEBUG("called (error = %p)", error);
    g_slice_free(GtkDialog *, p_dialog);
    if (!dialog)
    {
        /* the dialog has been canceled while dowloading the route */
        DEBUG("Route dialog canceled");
        return;
    }

    g_object_remove_weak_pointer(G_OBJECT(dialog), (gpointer)p_dialog);

    hildon_gtk_window_set_progress_indicator(GTK_WINDOW(dialog), FALSE);
    rdi = g_object_get_data(G_OBJECT(dialog), ROUTE_DOWNLOAD_INFO);
    rdi->is_computing = FALSE;

    if (G_UNLIKELY(error))
    {
        map_error_show(GTK_WINDOW(dialog), error);
        return;
    }

    /* Cancel any autoroute that might be occurring. */
    cancel_autoroute();

    map_path_merge(path, &_route,
                   rdi->replace ? MAP_PATH_MERGE_POLICY_REPLACE :
                   MAP_PATH_MERGE_POLICY_APPEND);
    path_save_route_to_db();

    /* Find the nearest route point, if we're connected. */
    route_find_nearest_point();

    map_controller_refresh_paths(controller);

    if (hildon_check_button_get_active
        (HILDON_CHECK_BUTTON(rdi->autoroute)))
    {
        _autoroute_data.router = g_object_ref(router);
        _autoroute_data.dest = rdi->to;
        _autoroute_data.dest.address = g_strdup(rdi->to.address);
        _autoroute_data.enabled = TRUE;
    }

    /* Save Origin in Route Locations list if not from GPS. */
    from = rdi->from.address;
    if (from != NULL &&
        !g_slist_find_custom(_loc_list, from, (GCompareFunc)strcmp))
    {
        _loc_list = g_slist_prepend(_loc_list, g_strdup(from));
        gtk_list_store_insert_with_values(_loc_model, &iter,
                INT_MAX, 0, from, -1);
    }

    /* Save Destination in Route Locations list. */
    to = rdi->to.address;
    if (to != NULL &&
        !g_slist_find_custom(_loc_list, to, (GCompareFunc)strcmp))
    {
        _loc_list = g_slist_prepend(_loc_list, g_strdup(to));
        gtk_list_store_insert_with_values(_loc_model, &iter,
                INT_MAX, 0, to, -1);
    }

    gtk_dialog_response(dialog, GTK_RESPONSE_CLOSE);
}

static void
on_dialog_response(GtkWidget *dialog, gint response, RouteDownloadInfo *rdi)
{
    MapController *controller = map_controller_get_instance();
    MapRouterQuery rq;
    MapRouter *router;
    GtkWidget **p_dialog;
    const gchar *from = NULL, *to = NULL;

    if (response == MAP_RESPONSE_OPTIONS)
    {
        router = get_selected_router(rdi);
        map_router_run_options_dialog(router, GTK_WINDOW(dialog));
        return;
    }

    if (response != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    if (rdi->is_computing) return;

    memset(&rq, 0, sizeof(rq));

    router = get_selected_router(rdi);
    map_controller_set_default_router(controller, router);

    rdi->origin_row_active =
        hildon_picker_button_get_active(HILDON_PICKER_BUTTON(rdi->origin));
    if (rdi->origin_row_active == rdi->origin_row_gps)
    {
        rq.from.point = _pos.unit;
    }
    else if (rdi->origin_row_active == rdi->origin_row_route)
    {
        Point *p;

        /* Use last non-zero route point. */
        for(p = _route.tail; !p->unit.y; p--) { }

        rq.from.point = p->unit;
    }
    else
    {
        from = hildon_button_get_value(HILDON_BUTTON(rdi->origin));
        if (STR_EMPTY(from))
        {
            popup_error(dialog, _("Please specify a start location."));
            return;
        }
    }

    to = gtk_entry_get_text(GTK_ENTRY(rdi->destination));
    if (STR_EMPTY(to))
    {
        popup_error(dialog, _("Please specify an end location."));
        return;
    }

    rq.from.address = g_strdup(from);
    rq.to.address = g_strdup(to);
    rq.parent = GTK_WINDOW(dialog);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdi->btn_swap)))
    {
        MapLocation tmp;
        tmp = rq.from;
        rq.from = rq.to;
        rq.to = tmp;
    }

    rdi->from = rq.from;
    rdi->to = rq.to;

    rdi->replace = (rdi->origin_row_active == rdi->origin_row_gps);

    hildon_gtk_window_set_progress_indicator(GTK_WINDOW(dialog), TRUE);

    /* weak pointer trick to prevent crashes if the callback is invoked
     * after the dialog is destroyed. */
    p_dialog = g_slice_new(GtkWidget *);
    *p_dialog = dialog;
    g_object_add_weak_pointer(G_OBJECT(dialog), (gpointer)p_dialog);
    rdi->is_computing = TRUE;
    map_router_calculate_route(router, &rq,
                               (MapRouterCalculateRouteCb)calculate_route_cb,
                               p_dialog);
}

static void
route_download_info_free(RouteDownloadInfo *rdi)
{
    g_free(rdi->from.address);
    g_free(rdi->to.address);
    g_slice_free(RouteDownloadInfo, rdi);
}

/**
 * Display a dialog box to the user asking them to download a route.  The
 * "From" and "To" textfields may be initialized using the first two
 * parameters.  The third parameter, if set to TRUE, will cause the "Use GPS
 * Location" checkbox to be enabled, which automatically sets the "From" to the
 * current GPS position (this overrides any value that may have been passed as
 * the "To" initializer).
 * None of the passed strings are freed - that is left to the caller, and it is
 * safe to free either string as soon as this function returns.
 */
gboolean
route_download(gchar *to)
{
    MapController *controller = map_controller_get_instance();
    GtkWidget *dialog;
    MapDialog *dlg;
    GtkWidget *label;
    GtkWidget *widget;
    GtkWidget *hbox;
    GtkEntryCompletion *to_comp;
    HildonTouchSelector *origin_selector;
    RouteDownloadInfo *rdi;
    gint i;
    gint active_origin_row, row;
    MapRouter *router, *default_router;

    DEBUG("");
    conic_recommend_connected();

    dialog = map_dialog_new(_("Download Route"), GTK_WINDOW(_window), TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          _("Options"), MAP_RESPONSE_OPTIONS);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);

    rdi = g_slice_new0(RouteDownloadInfo);
    g_object_set_data_full(G_OBJECT(dialog), ROUTE_DOWNLOAD_INFO, rdi,
                           (GDestroyNotify)route_download_info_free);

    dlg = (MapDialog *)dialog;
    rdi->dialog = GTK_WINDOW(dialog);

    /* Destination. */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       label = gtk_label_new(_("Destination")),
                       FALSE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5f);
    rdi->destination = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_box_pack_start(GTK_BOX(hbox), rdi->destination,
                       TRUE, TRUE, 0);
    map_dialog_add_widget(dlg, hbox);


    /* Origin. */
    origin_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    row = 0;
    rdi->origin_row_gps = row++;
    hildon_touch_selector_append_text(origin_selector, _("Use GPS Location"));
    /* Use "End of Route" by default if they have a route. */
    if(_route.head != _route.tail)
    {
        hildon_touch_selector_append_text(origin_selector, _("Use End of Route"));
        rdi->origin_row_route = row++;
        rdi->origin_row_other = row++;
        active_origin_row = rdi->origin_row_route;
    }
    else
    {
        rdi->origin_row_other = row++;
        rdi->origin_row_route = -1;
        active_origin_row = (_pos.unit.x != 0 && _pos.unit.y != 0) ? rdi->origin_row_gps : rdi->origin_row_other;
    }
    hildon_touch_selector_append_text(origin_selector, _("Other..."));
    hildon_touch_selector_set_active(origin_selector, 0, active_origin_row);

    rdi->origin =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Origin"),
                     "touch-selector", origin_selector,
                     "xalign", 0.0,
                     NULL);
    g_signal_connect(rdi->origin, "value-changed",
                     G_CALLBACK(on_origin_changed_other), rdi);
    map_dialog_add_widget(dlg, rdi->origin);

    /* Auto. */
    rdi->autoroute = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    if (active_origin_row != rdi->origin_row_gps)
        gtk_widget_set_sensitive(rdi->autoroute, FALSE);
    gtk_button_set_label(GTK_BUTTON(rdi->autoroute), _("Auto-Update"));
    g_signal_connect(rdi->origin, "value-changed",
                     G_CALLBACK(on_origin_changed_gps), rdi);
    map_dialog_add_widget(dlg, rdi->autoroute);

    /* Swap button. */
    rdi->btn_swap = gtk_toggle_button_new_with_label("Swap");
    hildon_gtk_widget_set_theme_size(rdi->btn_swap, HILDON_SIZE_FINGER_HEIGHT);
    map_dialog_add_widget(dlg, rdi->btn_swap);

    /* Router */
    widget = hildon_touch_selector_new_text();
    rdi->router_selector = HILDON_TOUCH_SELECTOR(widget);

    rdi->routers = get_routers(controller);
    default_router = map_controller_get_default_router(controller);
    for (i = 0; i < rdi->routers->len; i++)
    {
        router = g_ptr_array_index(rdi->routers, i);
        hildon_touch_selector_append_text(rdi->router_selector,
                                          map_router_get_name(router));
        if (router == default_router)
            hildon_touch_selector_set_active(rdi->router_selector, 0, i);
    }
    router = get_selected_router(rdi);
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), MAP_RESPONSE_OPTIONS,
                                      router != NULL &&
                                      map_router_has_options(router));

    widget = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                          "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                          "size", HILDON_SIZE_FINGER_HEIGHT,
                          "title", _("Router"),
                          "touch-selector", rdi->router_selector,
                          "xalign", 0.0,
                          NULL);
    g_signal_connect(widget, "value-changed",
                     G_CALLBACK(on_router_selector_changed), rdi);
    map_dialog_add_widget(dlg, widget);

    /* Set up auto-completion. */
    to_comp = gtk_entry_completion_new();
    gtk_entry_completion_set_model(to_comp, GTK_TREE_MODEL(_loc_model));
    gtk_entry_completion_set_text_column(to_comp, 0);
    gtk_entry_set_completion(GTK_ENTRY(rdi->destination), to_comp);

    /* Initialize fields. */
    if(to)
        gtk_entry_set_text(GTK_ENTRY(rdi->destination), to);

    gtk_widget_show_all(dialog);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_dialog_response), rdi);

    return TRUE;
}

void
route_add_way_dialog(gint unitx, gint unity)
{
    gdouble lat, lon;
    gchar tmp1[LL_FMT_LEN], tmp2[LL_FMT_LEN], *p_latlon;
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *label_lat_lon = NULL;
    static GtkWidget *txt_scroll = NULL;
    static GtkWidget *txt_desc = NULL;
    static int last_deg_format = 0;
    
    unit2latlon(unitx, unity, lat, lon);
    
    gint fallback_deg_format = _degformat;
    
    if(!coord_system_check_lat_lon (lat, lon, &fallback_deg_format))
    {
    	last_deg_format = _degformat;
    	_degformat = fallback_deg_format;
    	
    	if(dialog != NULL) gtk_widget_destroy(dialog);
    	dialog = NULL;
    }
    else if(_degformat != last_deg_format)
    {
    	last_deg_format = _degformat;
    	
		if(dialog != NULL) gtk_widget_destroy(dialog);
    	dialog = NULL;
    }
    
    
    if(dialog == NULL)
    {
        dialog = gtk_dialog_new_with_buttons(_("Add Waypoint"),
                GTK_WINDOW(_window), GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                NULL);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                table = gtk_table_new(2, 2, FALSE), TRUE, TRUE, 0);

        
        
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        	p_latlon = g_strdup_printf("%s, %s",
        			DEG_FORMAT_ENUM_TEXT[_degformat].short_field_1,
        			DEG_FORMAT_ENUM_TEXT[_degformat].short_field_2);
        else
        	p_latlon = g_strdup_printf("%s", DEG_FORMAT_ENUM_TEXT[_degformat].short_field_1);
        
        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(p_latlon),
                0, 1, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

        g_free(p_latlon);
        
        gtk_table_attach(GTK_TABLE(table),
                label_lat_lon = gtk_label_new(""),
                1, 2, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label_lat_lon), 0.0f, 0.5f);
        

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Description")),
                0, 1, 1, 2, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

        txt_scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(txt_scroll),
                                       GTK_SHADOW_IN);
        gtk_table_attach(GTK_TABLE(table),
                txt_scroll,
                1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 2, 4);

        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(txt_scroll),
                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

        txt_desc = gtk_text_view_new ();
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txt_desc), GTK_WRAP_WORD);

        gtk_container_add(GTK_CONTAINER(txt_scroll), txt_desc);
        gtk_widget_set_size_request(GTK_WIDGET(txt_scroll), 400, 60);
    }

    format_lat_lon(lat, lon, tmp1, tmp2);
    
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	p_latlon = g_strdup_printf("%s, %s", tmp1, tmp2);
    else
    	p_latlon = g_strdup_printf("%s", tmp1);
    
    
    gtk_label_set_text(GTK_LABEL(label_lat_lon), p_latlon);
    g_free(p_latlon);
    
    gtk_text_buffer_set_text(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_desc)), "", 0);

    gtk_widget_show_all(dialog);

    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        GtkTextBuffer *tbuf;
        GtkTextIter ti1, ti2;
        gchar *desc;

        tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_desc));
        gtk_text_buffer_get_iter_at_offset(tbuf, &ti1, 0);
        gtk_text_buffer_get_end_iter(tbuf, &ti2);
        desc = gtk_text_buffer_get_text(tbuf, &ti1, &ti2, TRUE);

        if(*desc)
        {
            /* There's a description.  Add a waypoint. */
            MACRO_PATH_INCREMENT_TAIL(_route);
            _route.tail->unit.x = unitx;
            _route.tail->unit.y = unity;
            _route.tail->time = 0;
            _route.tail->altitude = 0;

            MACRO_PATH_INCREMENT_WTAIL(_route);
            _route.wtail->point = _route.tail;
            _route.wtail->desc
                = gtk_text_buffer_get_text(tbuf, &ti1, &ti2, TRUE);
        }
        else
        {
            GtkWidget *confirm;

            g_free(desc);

            confirm = hildon_note_new_confirmation(GTK_WINDOW(dialog),
                    _("Creating a \"waypoint\" with no description actually "
                        "adds a break point.  Is that what you want?"));

            if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm)))
            {
                /* There's no description.  Add a break by adding a (0, 0)
                 * point (if necessary), and then the ordinary route point. */
                if(_route.tail->unit.y)
                {
                    MACRO_PATH_INCREMENT_TAIL(_route);
                    *_route.tail = _point_null;
                }

                MACRO_PATH_INCREMENT_TAIL(_route);
                _route.tail->unit.x = unitx;
                _route.tail->unit.y = unity;
                _route.tail->time = 0;
                _route.tail->altitude = 0;


                gtk_widget_destroy(confirm);
            }
            else
            {
                gtk_widget_destroy(confirm);
                continue;
            }
        }

        route_find_nearest_point();
        break;
    }
    gtk_widget_hide(dialog);
    
    _degformat = last_deg_format;
}

WayPoint*
path_get_next_way()
{
    return _next_way;
}

void
path_init()
{
    gchar *settings_dir;

    /* Initialize settings_dir. */
    settings_dir = gnome_vfs_expand_initial_tilde(CONFIG_DIR_NAME);
    g_mkdir_with_parents(settings_dir, 0700);

    /* Open path database. */
    {   
        gchar *path_db_file;

        path_db_file = gnome_vfs_uri_make_full_from_relative(
                settings_dir, CONFIG_PATH_DB_FILE);

        if(!path_db_file || SQLITE_OK != sqlite3_open(path_db_file, &_path_db)
        /* Open worked. Now create tables, failing if they already exist. */
        || (sqlite3_exec(_path_db,
                    "create table route_path ("
                    "num integer primary key, "
                    "unitx integer, "
                    "unity integer, "
                    "time integer, "
                    "altitude integer)"
                    ";"
                    "create table route_way ("
                    "route_point primary key, "
                    "description text)"
                    ";"
                    "create table track_path ("
                    "num integer primary key, "
                    "unitx integer, "
                    "unity integer, "
                    "time integer, "
                    "altitude integer)"
                    ";"
                    "create table track_way ("
                    "track_point primary key, "
                    "description text)",
                    NULL, NULL, NULL), FALSE) /* !! Comma operator !! */
            /* Create prepared statements - failure here is bad! */
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "select unitx, unity, time, altitude, description "
                    "from route_path left join route_way on "
                    "route_path.num = route_way.route_point "
                    "order by route_path.num",
                    -1, &_route_stmt_select, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "select unitx, unity, time, altitude, description "
                    "from track_path left join track_way on "
                    "track_path.num = track_way.track_point "
                    "order by track_path.num",
                    -1, &_track_stmt_select, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from route_path",
                    -1, &_route_stmt_delete_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from route_way",
                    -1, &_route_stmt_delete_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into route_path "
                    "(num, unitx, unity, time, altitude) "
                    "values (NULL, ?, ?, ?, ?)",
                    -1, &_route_stmt_insert_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into route_way (route_point, description) "
                    "values (?, ?)",
                    -1, &_route_stmt_insert_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from track_path",
                    -1, &_track_stmt_delete_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from track_way",
                    -1, &_track_stmt_delete_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into track_path "
                    "(num, unitx, unity, time, altitude) "
                    "values (NULL, ?, ?, ?, ?)",
                    -1, &_track_stmt_insert_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into track_way (track_point, description) "
                    "values (?, ?)",
                    -1, &_track_stmt_insert_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db, "begin transaction",
                    -1, &_path_stmt_trans_begin, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db, "commit transaction",
                    -1, &_path_stmt_trans_commit, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db, "rollback transaction",
                    -1, &_path_stmt_trans_rollback, NULL))
        {   
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%s\n%s",
                    _("Failed to open path database. "
                        "Tracks and routes will not be saved."),
                    sqlite3_errmsg(_path_db));
            sqlite3_close(_path_db);
            _path_db = NULL;
            popup_error(_window, buffer);
        }
        else
        {   
            read_path_from_db(&_route, _route_stmt_select);
            read_path_from_db(&_track, _track_stmt_select);
            _track_index_last_saved = _track.tail - _track.head - 1;
        }
        g_free(path_db_file);
    }

    g_free(settings_dir);

    _last_spoken_phrase = g_strdup("");
}

void
path_destroy()
{
    /* Save paths. */
    if(_track.tail->unit.y)
        track_insert_break(FALSE);
    path_update_track_in_db();
    path_save_route_to_db();

    if(_path_db)
    {   
        sqlite3_close(_path_db);
        _path_db = NULL;
    }

    MACRO_PATH_FREE(_track);
    MACRO_PATH_FREE(_route);
}

void
map_path_optimize(Path *path)
{
    Point *curr, *prev;
    gint tolerance = 3 + _draw_width;

    /* for every point, set the zoom level at which the point must be rendered
     */

    if (path->points_optimized == 0 && path->head != path->tail)
    {
        path->head->zoom = SCHAR_MAX;
        path->points_optimized = 1;
    }

    for (curr = path->head + path->points_optimized;
         curr <= path->tail; curr++)
    {
        gint dx, dy, dmax, zoom;

        prev = curr - 1;
        if (curr->unit.y == 0 || prev->unit.y == 0)
        {
            curr->zoom = MAX_ZOOM;
            continue;
        }

        dx = curr->unit.x - prev->unit.x;
        dy = curr->unit.y - prev->unit.y;
        dmax = MAX(ABS(dx), ABS(dy));

        for (zoom = 0; dmax > tolerance << zoom; zoom++);

        /* We got the zoom level for this point, supposing that the previous
         * one is always drawn.
         * But we need go back in the path to find the last point which is
         * _surely_ drawn when this point is; that is, we look for the last
         * point having a zoom value bigger than that of the current point. */

        while (zoom >= prev->zoom)
        {
            Point *prev_before;
            /* going back is safe (we don't risk going past the head) because
             * the first point will always have zoom set to 127 */
            for (prev_before = prev; prev->zoom <= zoom; prev--);

            if (prev == prev_before) break;

            /* now, find the distance between these two points */
            dx = curr->unit.x - prev->unit.x;
            dy = curr->unit.y - prev->unit.y;
            dmax = MAX(ABS(dx), ABS(dy));

            for (; dmax > tolerance << zoom; zoom++);
        }

        curr->zoom = zoom;
    }

    path->points_optimized = path->tail - path->head;
}

void
map_path_merge(Path *src_path, Path *dest_path, MapPathMergePolicy policy)
{
    map_path_optimize(src_path);

    if (policy != MAP_PATH_MERGE_POLICY_REPLACE
        && dest_path->head != dest_path->tail)
    {
        Point *src_first;
        Path *src, *dest;

        if (policy == MAP_PATH_MERGE_POLICY_APPEND)
        {
            /* Append to current path. Make sure last path point is zero. */
            if(dest_path->tail->unit.y != 0)
            {
                MACRO_PATH_INCREMENT_TAIL((*dest_path));
                *dest_path->tail = _point_null;
            }
            src = src_path;
            dest = dest_path;
        }
        else
        {
            /* Prepend to current route. */
            src = dest_path;
            dest = src_path;
        }

        /* Find src_first non-zero point. */
        for(src_first = src->head - 1; src_first++ != src->tail; )
            if(src_first->unit.y)
                break;

        /* Append route points from src to dest. */
        if(src->tail >= src_first)
        {
            WayPoint *curr;
            gint num_dest_points = dest->tail - dest->head + 1;
            gint num_src_points = src->tail - src_first + 1;

            /* Adjust dest->tail to be able to fit src route data
             * plus room for more route data. */
            path_resize(dest,
                    num_dest_points + num_src_points + ARRAY_CHUNK_SIZE);

            memcpy(dest->tail + 1, src_first,
                    num_src_points * sizeof(Point));

            dest->tail += num_src_points;

            /* Append waypoints from src to dest->. */
            path_wresize(dest, (dest->wtail - dest->whead)
                    + (src->wtail - src->whead) + 2 + ARRAY_CHUNK_SIZE);
            for(curr = src->whead - 1; curr++ != src->wtail; )
            {
                (++(dest->wtail))->point = dest->head + num_dest_points
                    + (curr->point - src_first);
                dest->wtail->desc = curr->desc;
            }

        }

        /* Kill old route - don't use MACRO_PATH_FREE(), because that
         * would free the string desc's that we just moved to data.route. */
        g_free(src->head);
        g_free(src->whead);
        if (policy == MAP_PATH_MERGE_POLICY_PREPEND)
            (*dest_path) = *dest;
    }
    else
    {
        MACRO_PATH_FREE((*dest_path));
        /* Overwrite with data.route. */
        (*dest_path) = *src_path;
        path_resize(dest_path,
                dest_path->tail - dest_path->head + 1 + ARRAY_CHUNK_SIZE);
        path_wresize(dest_path,
                dest_path->wtail - dest_path->whead + 1 + ARRAY_CHUNK_SIZE);
    }
}

