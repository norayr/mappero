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
#include <sqlite3.h>

#include <hildon/hildon-note.h>

#include "types.h"
#include "data.h"
#include "debug.h"
#include "defines.h"
#include "display.h"
#include "gpx.h"
#include "path.h"
#include "poi.h"
#include "route.h"
#include "util.h"
#include "screen.h"

/* uncertainty below which a GPS position is considered to be precise enough 
 * to be added to the track (in metres) */
#define MAX_UNCERTAINTY 200

/* minimum distance the GPS position must have from the previous track point in
 * order to be registered in the track (in metres) */
#define MIN_TRACK_DISTANCE 10

typedef struct {
    gint idx_start;
} MapLine;

/* since the MapLine fits in a pointer, we directly store it in the GList
 * data field */
#define MAP_LINE(list)  ((MapLine *)(&(list->data)))
#define MAP_LINE_NEW(index) GINT_TO_POINTER(index)

typedef union {
    gint field;
    struct {
        gchar zoom;
        gint16 altitude;
    } s;
} FieldMix;

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

void
path_resize(Path *path, gint size)
{
    if(path->_head + size != path->_cap)
    {
        Point *old_head = path->_head;
        WayPoint *curr;
        path->_head = g_renew(Point, old_head, size);
        path->_cap = path->_head + size;
        if(path->_head != old_head)
        {
            path->_tail = path->_head + (path->_tail - old_head);

            /* Adjust all of the waypoints. */
            for(curr = path->whead - 1; curr++ != path->wtail; )
                curr->point = path->_head + (curr->point - old_head);
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

void
map_path_calculate_distances(Path *path)
{
    Point *curr, *first;
    gfloat total = 0;
    MapGeo lat, lon, last_lat, last_lon;
    MapLineIter line;
    gint n_points;

    n_points = map_path_len(path) - path->points_with_distance;
    if (n_points <= 0) return;

#ifdef ENABLE_DEBUG
    struct timespec ts0, ts1;
    long ms_diff;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts0);
#endif

    first = map_path_first(path) + path->points_with_distance;
    map_path_line_iter_from_point(path, first, &line);
    do
    {
        Point *start, *end;
        start = map_path_line_first(&line);
        end = start + map_path_line_len(&line);
        if (start < first)
        {
            /* we are in the middle of a line with already some distances
             * computed */
            start = first;
            last_lat = path->last_lat, last_lon = path->last_lon;
        }
        else if (start < end)
        {
            /* the first point of the line is treated specially */
            start->distance = 0;
            unit2latlon(start->unit.x, start->unit.y, last_lat, last_lon);
            start++;
        }
        else /* this line is empty */
        {
            last_lat = last_lon = 0;
        }

        for (curr = start; curr < end; curr++)
        {
            unit2latlon(curr->unit.x, curr->unit.y, lat, lon);
            curr->distance = calculate_distance(last_lat, last_lon, lat, lon);
            total += curr->distance;
            last_lat = lat;
            last_lon = lon;
        }
    }
    while (map_path_line_iter_next(&line));

    path->last_lat = last_lat;
    path->last_lon = last_lon;
    path->length += total;
    path->points_with_distance += n_points;

#ifdef ENABLE_DEBUG
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts1);
    ms_diff = (ts1.tv_sec - ts0.tv_sec) * 1000 +
        (ts1.tv_nsec - ts0.tv_nsec) / 1000000;
    DEBUG("%ld ms for %d points", ms_diff, n_points);
#endif
}

static void
read_path_from_db(Path *path, sqlite3_stmt *select_stmt)
{
    map_path_init(path);
    while(SQLITE_ROW == sqlite3_step(select_stmt))
    {
        const gchar *desc;
        FieldMix mix;
        Point pt;

        pt.unit.x = sqlite3_column_int(select_stmt, 0);
        pt.unit.y = sqlite3_column_int(select_stmt, 1);
        pt.time = sqlite3_column_int(select_stmt, 2);
        mix.field = sqlite3_column_int(select_stmt, 3);
        pt.zoom = mix.s.zoom;
        pt.altitude = mix.s.altitude;
        pt.distance = 0;

        desc = (const gchar *)sqlite3_column_text(select_stmt, 4);
        if (pt.unit.y != 0)
            map_path_append_point_with_desc(path, &pt, desc);
        else
            map_path_append_break(path);
    }
    sqlite3_reset(select_stmt);

    map_path_append_break(path);

    map_path_append_point_end(path);

    path->first_unsaved = map_path_len(path);
}

static gboolean
write_point_to_db(const Point *p, sqlite3_stmt *insert_path_stmt)
{
    FieldMix mix;
    gboolean success = TRUE;

    mix.field = 0;
    mix.s.altitude = p->altitude;
    mix.s.zoom = p->zoom;

    /* Insert the path point. */
    if (SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 1, p->unit.x)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 2, p->unit.y)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 3, p->time)
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
    return success;
}

/* Returns the new next_update_index. */
static gint
write_path_to_db(Path *path,
        sqlite3_stmt *delete_path_stmt,
        sqlite3_stmt *delete_way_stmt,
        sqlite3_stmt *insert_path_stmt,
        sqlite3_stmt *insert_way_stmt)
{
    Point *curr, *first;
    WayPoint *wcurr;
    gint saved = 0;
    gboolean success = TRUE;
    MapLineIter line;
    DEBUG("%d", path->first_unsaved);

    /* Start transaction. */
    sqlite3_step(_path_stmt_trans_begin);
    sqlite3_reset(_path_stmt_trans_begin);

    if (path->first_unsaved == 0)
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

    first = map_path_first(path) + path->first_unsaved;
    wcurr = path->whead;
    map_path_line_iter_from_point(path, first, &line);
    do
    {
        Point *start, *end;
        start = map_path_line_first(&line);
        end = start + map_path_line_len(&line);
        if (start >= first)
        {
            /* new line: write a break point */
            success = write_point_to_db(&_point_null, insert_path_stmt);
        }
        else
            start = first;

        for (curr = start; success && curr < end; curr++)
        {
            success = write_point_to_db(curr, insert_path_stmt);

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

            if (success) saved++;
        }
    }
    while (map_path_line_iter_next(&line));

    if(success)
    {
        sqlite3_step(_path_stmt_trans_commit);
        sqlite3_reset(_path_stmt_trans_commit);
        path->first_unsaved += saved;
        g_assert(path->first_unsaved <= map_path_len(path));
        return saved;
    }
    else
    {
        sqlite3_step(_path_stmt_trans_rollback);
        sqlite3_reset(_path_stmt_trans_rollback);
        return 0;
    }
}

void
map_path_save_route(Path *path)
{
    if(_path_db)
    {
        write_path_to_db(path,
                          _route_stmt_delete_path,
                          _route_stmt_delete_way,
                          _route_stmt_insert_path,
                          _route_stmt_insert_way);
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
                          _track_stmt_insert_way);
    }
}

/**
 * map_path_find_closest:
 * @path: a #Path.
 * @p: the reference #MapPoint.
 * @start: the index of starting #Point in @path.
 * @local: whether the search should be local only.
 *
 * Starting from @start, scans @path and returns the point which is closest to
 * @p. If @local is %TRUE, the search stops as soon as the distance is
 * increasing.
 *
 * Returns: the index of the closest #Point.
 */
static gint
map_path_find_closest(const Path *path, const MapPoint *p, gint start,
                      gboolean local)
{
    Point *curr, *near;
    gint64 near_dist_squared;

    if (start >= map_path_len(path)) return 0;

    /* First, set near_dist_squared with the new distance from
     * _near_point. */
    near = map_path_first(path) + start;
    near_dist_squared = DISTANCE_SQUARED(*p, near->unit);

    /* Now, search _route for a closer point.  If quick is TRUE, then we'll
     * only search forward, only as long as we keep finding closer points.
     */
    for (curr = near + 1; curr < map_path_end(path); curr++)
    {
        gint64 dist_squared = DISTANCE_SQUARED(*p, curr->unit);
        if (dist_squared <= near_dist_squared)
        {
            near = curr;
            near_dist_squared = dist_squared;
        }
        else if (local)
            break;
    }

    return near - map_path_first(path);
}

static inline void
compute_distances_to_near_and_following(const Path *path,
                                        const MapPoint *p, const Point *near,
                                        gint64 *dist_squared_near,
                                        gint64 *dist_squared_after_near)
{
    *dist_squared_near = DISTANCE_SQUARED(*p, near->unit);
    if (near < map_path_end(path))
    {
        const Point *after_near = near + 1;
        *dist_squared_after_near = DISTANCE_SQUARED(*p, after_near->unit);
    }
    else
        *dist_squared_after_near = -1;
}

/**
 * map_path_update_near_info:
 * @path: a #Path.
 * @p: the reference #MapPoint.
 * @ni: a #MapRouteNearInfo used as both input and output.
 * @local: whether the search should be local.
 *
 * Updates the near points information: finds the point in @path closest to @p,
 * and the next waypoint. If @local is %TRUE, only a local search is performed
 * (this should be set when the information in @ni was correct).
 *
 * Returns: %TRUE if the @ni has changed.
 */
gboolean
map_path_update_near_info(const Path *path, const MapPoint *p,
                          MapRouteNearInfo *ni, gboolean local)
{
    WayPoint *wcurr, *wnext;
    Point *near;
    gint p_near;
    gint64 dist_squared_near = -1, dist_squared_after_near;
    gboolean p_near_is_set = FALSE;
    gboolean changed = FALSE;

    if (map_path_len(path) == 0)
    {
        if (ni->p_near != 0 || ni->wp_next != 0)
        {
            ni->p_near = ni->wp_next = 0;
            ni->dist_squared_near = ni->dist_squared_after_near = -1;
            changed = TRUE;
        }
        return changed;
    }

    if (ni->p_near >= map_path_len(path))
        return FALSE;

    /* If we are doing a local search, try to follow the path without skipping
     * points: */
    if (local)
    {
        near = map_path_first(path) + ni->p_near;
        compute_distances_to_near_and_following(path, p, near,
                                                &dist_squared_near,
                                                &dist_squared_after_near);
        if (ni->dist_squared_near > 0)
        {
            /* if we are still approaching the near point, don't change it */
            if (dist_squared_near <= ni->dist_squared_near)
            {
                p_near = ni->p_near;
                p_near_is_set = TRUE;
            }
            else /* we are getting farther from the near point */
            {
                /* see if we are approaching the next one */
                if (ni->p_near + 1 < map_path_len(path) &&
                    dist_squared_after_near <= ni->dist_squared_after_near)
                {
                    p_near = ni->p_near + 1;
                    dist_squared_near = dist_squared_after_near;
                    dist_squared_after_near = -1;
                    p_near_is_set = TRUE;
                }
            }
        }

        /* update the internal data */
        ni->dist_squared_near = dist_squared_near;
        ni->dist_squared_after_near = dist_squared_after_near;
    }
    else
    {
        ni->dist_squared_near = -1;
        ni->dist_squared_after_near = -1;
    }

    if (!p_near_is_set)
        p_near = map_path_find_closest(path, p, ni->p_near, local);

    near = map_path_first(path) + p_near;

    if (p_near != ni->p_near)
    {
        ni->p_near = p_near;
        changed = TRUE;

        /* Determine the "next waypoint" */
        for (wcurr = path->whead + ni->wp_next;
             wcurr <= path->wtail && wcurr->point < near;
             wcurr++);

        wnext = wcurr;
        ni->wp_next = wnext - path->whead;
    }

    return changed;
}

/**
 * Returns TRUE if the point needs to be added to the path.
 */
static gboolean
point_is_significant(const MapGpsData *gps, const Path *path)
{
    gint xdiff, ydiff, min_distance_units;
    MapLineIter line;
    Point *prev;

    /* check if the track is empty */
    if (map_path_len(path) == 0)
        return TRUE;

    /* check if the track was in a break */
    map_path_line_iter_last(path, &line);
    if (map_path_line_len(&line) == 0)
        return TRUE;

    /* check how much time has passed since last update */
    prev = map_path_last(path);
    if (gps->time - prev->time > 60)
        return TRUE;

    /* Have we moved enough? */
    xdiff = abs(gps->unit.x - prev->unit.x);
    ydiff = abs(gps->unit.y - prev->unit.y);

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
            map_screen_refresh_panel(screen);

            if (_track.last_lat != 0 && _track.last_lon != 0)
            {
                pos.distance =
                    calculate_distance(_track.last_lat, _track.last_lon,
                                       gps->lat, gps->lon);
            }
            pos.time = gps->time;
            map_path_append_point_fast(&_track, &pos);
            _track.last_lat = gps->lat;
            _track.last_lon = gps->lon;
            _track.length += pos.distance;
            _track.points_with_distance++;
            map_path_optimize(&_track);
        }
    }
    else
    {
        /* insert a break */
        if (map_path_len(&_track) > 0)
            must_add = map_path_append_break(&_track);
    }

    /* Maybe update the track database. */
    {
        static time_t last_track_db_update = 0;
        if (!gps->time || (gps->time - last_track_db_update > 60
                           && map_path_len(&_track) > _track.first_unsaved))
        {
            path_save_track_to_db();
            last_track_db_update = gps->time;
        }
    }

    UNBLANK_SCREEN(must_add, FALSE);
}

void
track_clear()
{
    GtkWidget *confirm;

    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window),
                            _("Really clear the track?"));

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm))) {
        MapController *controller = map_controller_get_instance();
        map_path_unset(&_track);
        map_path_init(&_track);
        path_save_track_to_db();
        map_controller_refresh_paths(controller);
    }

    gtk_widget_destroy(confirm);
}

void
track_insert_break(gboolean temporary)
{
    if (!map_path_append_break(&_track))
        return;

    /* To mark a "waypoint" in a track, we'll add a break point and then
     * another instance of the most recent track point. */
    if (temporary)
    {
        map_path_append_point(&_track, map_path_last(&_track));
    }

    /* Update the track database. */
    path_save_track_to_db();
}

static sqlite3 *
open_db(const gchar *filename)
{
    sqlite3 *db;
    gboolean exists;

    if (!filename) return NULL;

    exists = g_file_test(filename, G_FILE_TEST_EXISTS);
    if (SQLITE_OK != sqlite3_open(filename, &db))
        return NULL;

    if (!exists)
    {
        sqlite3_exec(db,
                     "CREATE TABLE route_path ("
                     "num INTEGER PRIMARY KEY, "
                     "unitx INTEGER, "
                     "unity INTEGER, "
                     "time INTEGER, "
                     "altitude INTEGER)"
                     ";"
                     "CREATE TABLE route_way ("
                     "route_point PRIMARY KEY, "
                     "description TEXT)"
                     ";"
                     "CREATE TABLE track_path ("
                     "num INTEGER PRIMARY KEY, "
                     "unitx INTEGER, "
                     "unity INTEGER, "
                     "time INTEGER, "
                     "altitude INTEGER)"
                     ";"
                     "CREATE TABLE track_way ("
                     "track_point PRIMARY KEY, "
                     "description TEXT)",
                     NULL, NULL, NULL);
    }

    return db;
}

void
path_init()
{
    MapController *controller = map_controller_get_instance();
    gchar *settings_dir;

    /* Initialize settings_dir. */
    settings_dir = gnome_vfs_expand_initial_tilde(CONFIG_DIR_NAME);
    g_mkdir_with_parents(settings_dir, 0700);

    /* Open path database. */
    {   
        gchar *path_db_file;

        path_db_file = gnome_vfs_uri_make_full_from_relative(
                settings_dir, CONFIG_PATH_DB_FILE);

        _path_db = open_db(path_db_file);
        g_free(path_db_file);

        if (!_path_db
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
            read_path_from_db(map_route_get_path(), _route_stmt_select);
            map_route_path_changed();
            read_path_from_db(&_track, _track_stmt_select);
        }
    }

    g_free(settings_dir);

    map_controller_refresh_paths(controller);
}

void
path_destroy()
{
    /* Save paths. */
    map_path_append_break(&_track);
    path_save_track_to_db();
    path_save_route_to_db();

    if(_path_db)
    {   
        sqlite3_close(_path_db);
        _path_db = NULL;
    }

    map_path_unset(&_track);
    map_route_destroy();
}

void
map_path_optimize(Path *path)
{
    Point *curr, *prev;
    gint tolerance = 3 + _draw_width;

    /* for every point, set the zoom level at which the point must be rendered
     */

    if (map_path_len(path) == 0) return;

    curr = map_path_first(path) + path->points_optimized;
    if (path->points_optimized == 0)
    {
        curr->zoom = SCHAR_MAX;
        path->points_optimized = 1;
        curr++;
    }

    for (; curr < map_path_end(path); curr++)
    {
        gint dx, dy, dmax, zoom;

        prev = curr - 1;

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

    path->points_optimized = map_path_len(path);
}

void
map_path_merge(Path *src_path, Path *dest_path, MapPathMergePolicy policy)
{
    map_path_calculate_distances(src_path);
    map_path_optimize(src_path);

    DEBUG("src length %.2f, dest %.2f", src_path->length, dest_path->length);

    if (map_path_len(src_path) == 0)
    {
        if (policy == MAP_PATH_MERGE_POLICY_REPLACE)
        {
            map_path_unset(dest_path);
            map_path_init(dest_path);
        }
        return;
    }

    if (policy != MAP_PATH_MERGE_POLICY_REPLACE
        && map_path_len(dest_path) > 0)
    {
        Point *src_first;
        Path *src, *dest;
        gint num_dest_points;
        gint num_src_points;
        WayPoint *curr;
        GList *list;

        if (policy == MAP_PATH_MERGE_POLICY_APPEND)
        {
            /* Append to current path. */
            src = src_path;
            dest = dest_path;
        }
        else
        {
            /* Prepend to current route. */
            src = dest_path;
            dest = src_path;
        }

        src_first = map_path_first(src);

        /* Append route points from src to dest. */
        num_dest_points = map_path_len(dest);
        num_src_points = map_path_len(src);

        /* Adjust dest->tail to be able to fit src route data
         * plus room for more route data. */
        path_resize(dest,
                    num_dest_points + num_src_points + ARRAY_CHUNK_SIZE);

        memcpy(map_path_end(dest), src_first,
               num_src_points * sizeof(Point));

        dest->_tail += num_src_points;

        /* Append waypoints from src to dest->. */
        path_wresize(dest, (dest->wtail - dest->whead)
                     + (src->wtail - src->whead) + 2 + ARRAY_CHUNK_SIZE);
        for(curr = src->whead - 1; curr++ != src->wtail; )
        {
            (++(dest->wtail))->point =
                map_path_first(dest) + num_dest_points
                + (curr->point - src_first);
            dest->wtail->desc = curr->desc;
        }

        /* Adjust the indexes of the lines and concatenate the lists */
        /* First, remove the first line of the path, because it will be the
         * same as the last line of the destination path (if it was set,
         * otherwise the merge will unify the segments) */
        src->_lines = g_list_delete_link(src->_lines, src->_lines);
        for (list = src->_lines; list != NULL; list = list->next)
        {
            MapLine *line = MAP_LINE(list);
            line->idx_start += num_dest_points;
        }
        dest->_lines = g_list_concat(dest->_lines, src->_lines);

        dest->length += src->length;
        dest->last_lat = src->last_lat;
        dest->last_lon = src->last_lon;
        dest->points_with_distance = map_path_len(dest);

        /* Kill old route - don't use map_path_unset(), because that
         * would free the string desc's that we just moved to data.route. */
        g_free(src->_head);
        g_free(src->whead);
        if (policy == MAP_PATH_MERGE_POLICY_PREPEND)
            (*dest_path) = *dest;
    }
    else
    {
        map_path_unset(dest_path);
        /* Overwrite with data.route. */
        (*dest_path) = *src_path;
        path_resize(dest_path, map_path_len(dest_path) + ARRAY_CHUNK_SIZE);
        path_wresize(dest_path, map_path_len(dest_path) + ARRAY_CHUNK_SIZE);
    }
    DEBUG("total length: %.2f", dest_path->length);
}

void
map_path_remove_range(Path *path, Point *start, Point *end)
{
    g_warning("%s not implemented", G_STRFUNC);
}

guint
map_path_get_duration(const Path *path)
{
    MapLineIter line;
    guint duration = 0;

    map_path_line_iter_first(path, &line);
    do
    {
        Point *first, *last;
        first = map_path_line_first(&line);
        last = first + map_path_line_len(&line) - 1;
        if (last > first && first->time != 0 && last->time != 0)
            duration += last->time - first->time;
    }
    while (map_path_line_iter_next(&line));

    return duration;
}

void
map_path_append_point_end(Path *path)
{
    map_path_calculate_distances(path);
    map_path_optimize(path);
}

Point *
map_path_append_unit(Path *path, const MapPoint *p)
{
    MapController *controller = map_controller_get_instance();
    Point pt, *p_in_path;

    pt.unit = *p;
    pt.altitude = 0;
    pt.time = 0;
    pt.zoom = SCHAR_MAX;
    p_in_path = map_path_append_point(path, &pt);

    map_controller_refresh_paths(controller);
    return p_in_path;
}

/**
 * map_path_append_break:
 * @path: the #Path.
 *
 * Breaks the line. Returns %FALSE if the line was already broken.
 */
gboolean
map_path_append_break(Path *path)
{
    GList *list;
    MapLine *line;
    gint path_len;

    path_len = map_path_len(path);
    if (G_UNLIKELY(path_len == 0)) return FALSE;

    list = g_list_last(path->_lines);
    g_assert(list != NULL);

    line = MAP_LINE(list);
    if (line->idx_start == path_len) return FALSE;

    path->_lines = g_list_append(path->_lines, MAP_LINE_NEW(path_len));
    path->last_lon = path->last_lat = 0;
    return TRUE;
}

/**
 * map_path_end_is_break:
 * @path: the #Path.
 *
 * Returns %TRUE if the path ends with a break point.
 */
gboolean
map_path_end_is_break(const Path *path)
{
    GList *list;
    MapLine *line;
    gint path_len;

    path_len = map_path_len(path);
    if (path_len == 0) return TRUE;

    list = g_list_last(path->_lines);
    g_assert(list != NULL);

    line = MAP_LINE(list);
    return line->idx_start == path_len;
}

void
map_path_init(Path *path)
{
    path->_head = path->_tail = g_new(Point, ARRAY_CHUNK_SIZE);
    path->_cap = path->_head + ARRAY_CHUNK_SIZE;
    path->whead = g_new(WayPoint, ARRAY_CHUNK_SIZE);
    path->wtail = path->whead - 1;
    path->wcap = path->whead + ARRAY_CHUNK_SIZE;
    path->length = 0;
    path->last_lat = 0;
    path->last_lon = 0;
    path->points_with_distance = 0;
    path->points_optimized = 0;
    path->_lines = g_list_prepend(NULL, MAP_LINE_NEW(0));
    path->first_unsaved = 0;
}

void
map_path_unset(Path *path)
{
    if (path->_head) {
        WayPoint *curr;
        g_free(path->_head);
        path->_head = path->_tail = path->_cap = NULL;
        for (curr = path->whead - 1; curr++ != path->wtail; )
            g_free(curr->desc);
        g_free(path->whead);
        path->whead = path->wtail = path->wcap = NULL;
        g_list_free(path->_lines);
        path->_lines = NULL;
    }
}

void
map_path_line_iter_first(const Path *path, MapLineIter *iter)
{
    iter->path = path;
    iter->line = path->_lines;
    g_assert(iter->line != NULL);
}

void
map_path_line_iter_last(const Path *path, MapLineIter *iter)
{
    iter->path = path;
    iter->line = g_list_last(path->_lines);
    g_assert(iter->line != NULL);
}

void
map_path_line_iter_from_point(const Path *path, const Point *point,
                              MapLineIter *iter)
{
    MapLineIter line;

    g_assert(point >= path->_head);
    g_assert(point <= path->_tail);

    map_path_line_iter_first(path, &line);
    *iter = line;
    while (map_path_line_iter_next(&line) &&
           point >= map_path_line_first(&line))
        *iter = line;
}

gboolean
map_path_line_iter_next(MapLineIter *iter)
{
    g_assert(iter->path != NULL);
    iter->line = g_list_next(iter->line);
    return iter->line != NULL;
}

Point *
map_path_line_first(const MapLineIter *iter)
{
    return map_path_first(iter->path) + MAP_LINE(iter->line)->idx_start;
}

gint
map_path_line_len(const MapLineIter *iter)
{
    gint start, end;

    start = MAP_LINE(iter->line)->idx_start;
    if (iter->line->next)
        end = MAP_LINE(iter->line->next)->idx_start;
    else
        end = map_path_len(iter->path);
    g_assert(start <= end);
    return end - start;
}

