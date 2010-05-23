/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Mappero.
 *
 * Mappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif
#define _GNU_SOURCE
#include "route.h"

#include "controller.h"
#include "data.h"
#include "dialog.h"
#include "error.h"
#include "main.h"
#include "navigation.h"
#include "util.h"

#include <hildon/hildon-banner.h>
#include <hildon/hildon-check-button.h>
#include <hildon/hildon-entry.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#include <mappero/debug.h>
#include <mappero/router.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct _MapRoute {
    MapPath path;
    gfloat distance_to_next_waypoint;
};

MapRoute _route;
MapRoute *_p_route = &_route;

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

static MapRouteNearInfo _near_info;

void
map_route_clear()
{
    MapController *controller = map_controller_get_instance();

    map_route_destroy();
    map_path_init(&_route.path);
    path_save_route_to_db();
    route_find_nearest_point();
    map_controller_refresh_paths(controller);
}

void
map_route_destroy()
{
    cancel_autoroute();
    map_path_unset(&_route.path);
}

void
map_route_path_changed()
{
    route_find_nearest_point();
    _route.distance_to_next_waypoint = -1;
}

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
path_save_route_to_db()
{
    map_path_save_route(&_route.path);
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
    const MapGpsData *gps;
    MapScreen *screen;
    gboolean changed;
    DEBUG("%d", quick);

    gps = map_controller_get_gps_data(controller);
    changed =
        map_path_update_near_info(&_route.path, &gps->unit, &_near_info, quick);

    /* The cached distance to the next waypoint is no longer valid */
    _route.distance_to_next_waypoint = -1;

    screen = map_controller_get_screen(controller);
    map_screen_refresh_panel(screen);
    /* TODO: refresh paths only if the waypoint has changed, not the near point
     */
    if (changed)
        map_controller_refresh_paths(controller);

    return changed;
}

/**
 * Reset the _near_point data by searching the entire route for the nearest
 * route point and waypoint.
 */
void
route_find_nearest_point()
{
    memset(&_near_info, 0, sizeof(_near_info));

    route_update_nears(FALSE);
}

/**
 * Calculates the distance from the current GPS location to the given point,
 * following the route.  If point is NULL, then the distance is shown to the
 * next waypoint.
 */
gboolean
route_calc_distance_to(const MapPathPoint *point, gfloat *distance)
{
    MapController *controller = map_controller_get_instance();
    MapGeo lat2, lon2;
    MapPathPoint *near_point;
    const MapGpsData *gps;
    gfloat sum = 0.0;

    /* If point is NULL, use the next waypoint. */
    if (point == NULL)
    {
        const MapPathWayPoint *next = map_route_get_next_waypoint();
        if (next)
            point = next->point;
    }

    /* If point is still NULL, return an error. */
    if(point == NULL)
    {
        return FALSE;
    }

    near_point = map_path_first(&_route.path) + _near_info.p_near;
    if (point > near_point)
    {
        MapPathPoint *curr;

        /* we skip near_point here, because near_point->distance is the
         * distance to the previous point and we don't care about that. */
        for (curr = near_point + 1; curr <= point; ++curr)
        {
            sum += curr->distance;
        }
    }
    else if (point < near_point)
    {
        MapPathPoint *curr;
        /* Going backwards, the near point is the one before the next */
        near_point--;
        for (curr = near_point; curr > point; --curr)
        {
            sum += curr->distance;
        }
    }

    /* sum the distance to near_point */
    gps = map_controller_get_gps_data(controller);
    unit2latlon(near_point->unit.x, near_point->unit.y, lat2, lon2);
    sum += calculate_distance(gps->lat, gps->lon, lat2, lon2);

    *distance = sum;
    return TRUE;
}

/**
 * Show the distance from the current GPS location to the given point,
 * following the route.  If point is NULL, then the distance is shown to the
 * next waypoint.
 */
gboolean
route_show_distance_to(MapPathPoint *point)
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

static void
map_route_take_path(MapPath *path, MapPathMergePolicy policy)
{
    MapController *controller = map_controller_get_instance();

    map_path_merge(path, &_route.path, policy);
    path_save_route_to_db();

    /* Find the nearest route point, if we're connected. */
    route_find_nearest_point();

    map_controller_refresh_paths(controller);
}

static void
auto_calculate_route_cb(MapRouter *router, MapPath *path, const GError *error)
{
    DEBUG("called (error = %p)", error);

    if (G_UNLIKELY(error))
        cancel_autoroute();
    else
    {
        map_route_take_path(path, MAP_PATH_MERGE_POLICY_REPLACE);
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

/* Checks whether the point p lies near the segment p0-p1 or p0-p2. This is
 * implemented by checking whether p is inside the ellipse having foci in p0
 * and p1 and having @distance as the distance between each focus and the
 * closest point on the major axis. Same for p0-p2. */
static gboolean
point_near_segments(const MapPoint *p, const MapPoint *p0, const MapPoint *p1,
                    const MapPoint *p2, guint distance)
{
    gint p_p0, p_p1, p_p2, p0_p1, p0_p2;

    p_p0 = sqrtf(DISTANCE_SQUARED(*p, *p0));
    if (p1 != NULL)
    {
        p_p1 = sqrtf(DISTANCE_SQUARED(*p, *p1));
        p0_p1 = sqrtf(DISTANCE_SQUARED(*p0, *p1));
        /* the ellipse diameter is the distance between the two foci plus 2
         * times @distance */
        if (p_p0 + p_p1 <= p0_p1 + 2 * distance)
            return TRUE;
    }
    if (p2 != NULL)
    {
        p_p2 = sqrtf(DISTANCE_SQUARED(*p, *p2));
        p0_p2 = sqrtf(DISTANCE_SQUARED(*p0, *p2));
        if (p_p0 + p_p2 <= p0_p2 + 2 * distance)
            return TRUE;
    }

    return FALSE;
}

void
map_path_route_step(const MapGpsData *gps, gboolean newly_fixed)
{
    gfloat announce_distance;
    gboolean approaching_waypoint = FALSE;
    gboolean late = FALSE, out_of_route = FALSE;
    gfloat distance = 0;
    const MapPathWayPoint *next_way;
    MapPathPoint *near_point = NULL;

    /* if we don't have a route to follow, nothing to do */
    if (map_path_len(&_route.path) == 0) return;

    /* Update the nearest-waypoint data. */
    if (newly_fixed)
        route_find_nearest_point();
    else
        route_update_nears(TRUE);

    if (_near_info.p_near < map_path_len(&_route.path))
        near_point = map_path_first(&_route.path) + _near_info.p_near;

    /* Check if we are late, with a tolerance of 3 minutes */
    if (near_point && near_point->time != 0 && gps->time != 0 &&
        gps->time > near_point->time + 60 * 3)
        late = TRUE;
    DEBUG("Late: %d", late);

    if (!late) /* if we are late, we can skip this distance check */
    {
        /* TODO: do this check only if we have actually moved */

        /* Calculate distance to route. (point to line) */
        if (near_point)
        {
            const MapPoint *n1 = NULL, *n2 = NULL;
            gint max_distance;

            /* Try previous point first. */
            if (near_point != map_path_first(&_route.path) &&
                near_point[-1].unit.y)
                n1 = &near_point[-1].unit;
            if (near_point != map_path_last(&_route.path) &&
                near_point[1].unit.y)
                n2 = &near_point[1].unit;

            /* Check if our distance from the route is large. */
            max_distance = METRES_TO_UNITS(100 + gps->hdop);
            out_of_route =
                !point_near_segments(&gps->unit, &near_point->unit, n1, n2,
                                     max_distance);
            DEBUG("out_of_route: %d (max distance = %d)",
                  out_of_route, max_distance);
        }
    }

    /* check if we need to recalculate the route */
    if (late || out_of_route)
    {
        if(_autoroute_data.enabled && !_autoroute_data.in_progress)
        {
            MACRO_BANNER_SHOW_INFO(_window,
                                   _("Recalculating directions..."));
            _autoroute_data.in_progress = TRUE;
            g_idle_add((GSourceFunc)auto_route_dl_idle, NULL);
        }
        else
        {
            /* Reset the route to try and find the nearest point.*/
            path_reset_route();
        }
    }

    next_way = map_route_get_next_waypoint();
    distance = map_route_get_distance_to_next_waypoint();

    /* this variable is measured in kilometres: */
    announce_distance = 0.02 /* 20 metres */
        * _announce_notice_ratio; /* this settings varies between 1 and 20 */

    /* if the speed (which is stored in km/h) is high, let's give two
     * seconds more: */
    if (gps->fields & MAP_GPS_SPEED && gps->speed > 20)
        announce_distance += gps->speed * (2 / 3600.0);

    /* next_way will be NULL if we passed the last waypoint */
    if (next_way)
        approaching_waypoint =
            distance <= announce_distance && !out_of_route;
    else
        approaching_waypoint = FALSE;

    map_navigation_set_alert(approaching_waypoint, next_way, distance);

    UNBLANK_SCREEN(FALSE, approaching_waypoint);
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

MapPathWayPoint *
find_nearest_waypoint(const MapPoint *point)
{
    MapController *controller = map_controller_get_instance();
    MapPathWayPoint *wcurr;
    MapPathWayPoint *wnear;
    gint64 nearest_squared;
    gint radius_unit, zoom;
    DEBUG("");

    zoom = map_controller_get_zoom(controller);
    radius_unit = pixel2zunit(TOUCH_RADIUS, zoom);

    wcurr = wnear = _route.path.whead;
    if (wcurr && wcurr <= _route.path.wtail)
    {
        nearest_squared = DISTANCE_SQUARED(*point, wcurr->point->unit);

        wnear = _route.path.whead;
        while (++wcurr <= _route.path.wtail)
        {
            gint64 test_squared = DISTANCE_SQUARED(*point, wcurr->point->unit);
            if(test_squared < nearest_squared)
            {
                wnear = wcurr;
                nearest_squared = test_squared;
            }
        }

        /* Only use the waypoint if it is within a 6*_draw_width square drawn
         * around the position. This is consistent with select_poi(). */
        if (abs(point->x - wnear->point->unit.x) < radius_unit
            && abs(point->y - wnear->point->unit.y) < radius_unit)
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
calculate_route_cb(MapRouter *router, MapPath *path, const GError *error,
                   GtkDialog **p_dialog)
{
    GtkDialog *dialog = *p_dialog;
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

    map_route_take_path(path,
                        rdi->replace ? MAP_PATH_MERGE_POLICY_REPLACE :
                        MAP_PATH_MERGE_POLICY_APPEND);

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
        MapPathPoint *p = map_path_last(&_route.path);

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

    dialog = map_dialog_new(_("Find route"), GTK_WINDOW(_window), TRUE);
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
    if (map_path_len(&_route.path) > 0)
    {
        hildon_touch_selector_append_text(origin_selector, _("Use End of Route"));
        rdi->origin_row_route = row++;
        rdi->origin_row_other = row++;
    }
    else
    {
        rdi->origin_row_other = row++;
        rdi->origin_row_route = -1;
    }
    active_origin_row = (_pos.unit.x != 0 && _pos.unit.y != 0) ?
        rdi->origin_row_gps : rdi->origin_row_other;
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
route_add_way_dialog(const MapPoint *point)
{
    MapController *controller = map_controller_get_instance();
    MapGeo lat, lon;
    gchar tmp1[LL_FMT_LEN], tmp2[LL_FMT_LEN], *p_latlon;
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *label_lat_lon = NULL;
    static GtkWidget *txt_scroll = NULL;
    static GtkWidget *txt_desc = NULL;
    static int last_deg_format = 0;
    
    unit2latlon(point->x, point->y, lat, lon);
    
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
            MapPathPoint *p_in_path = map_path_append_unit(&_route.path, point);

            map_path_make_waypoint(&_route.path, p_in_path,
                gtk_text_buffer_get_text(tbuf, &ti1, &ti2, TRUE));
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
                map_path_append_break(&_route.path);

                map_path_append_unit(&_route.path, point);

                gtk_widget_destroy(confirm);
            }
            else
            {
                gtk_widget_destroy(confirm);
                continue;
            }
        }

        route_find_nearest_point();
        map_controller_refresh_paths(controller);
        break;
    }
    gtk_widget_hide(dialog);
    
    _degformat = last_deg_format;
}

MapPathWayPoint *
map_route_get_next_waypoint()
{
    MapPathWayPoint *wp;
    wp = _route.path.whead + _near_info.wp_next;
    return (wp <= _route.path.wtail) ? wp : NULL;
}

gfloat
map_route_get_distance_to_next_waypoint()
{
    if (_route.distance_to_next_waypoint < 0)
        route_calc_distance_to(NULL, &_route.distance_to_next_waypoint);
    return _route.distance_to_next_waypoint;
}

