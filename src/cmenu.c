/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * POI and GPS-Info code originally written by Cezary Jackiewicz.
 *
 * Default map data provided by http://www.openstreetmap.org/
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

#include <dialog.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#ifndef LEGACY
#    include <hildon/hildon-note.h>
#    include <hildon/hildon-banner.h>
#else
#    include <hildon-widgets/hildon-note.h>
#    include <hildon-widgets/hildon-banner.h>
#endif

#include "data.h"
#include "defines.h"

#include "cmenu.h"
#include "display.h"
#include "menu.h"
#include "path.h"
#include "poi.h"
#include "route.h"
#include "util.h"

static MapPoint _cmenu_unit;

static void
cmenu_show_latlon(const MapPoint *p)
{
  MapGeo lat, lon;
  gint tmp_degformat = _degformat;
  gint fallback_deg_format = _degformat;
  gchar buffer[80], tmp1[LL_FMT_LEN], tmp2[LL_FMT_LEN];
  
  unit2latlon(p->x, p->y, lat, lon);
  
  // Check that the current coord system supports the select position
  if(!coord_system_check_lat_lon (lat, lon, &fallback_deg_format))
  {
  	_degformat = fallback_deg_format;
  }
      
  format_lat_lon(lat, lon, tmp1, tmp2);
  

  if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
  {
	  snprintf(buffer, sizeof(buffer),
	            "%s: %s\n"
	          "%s: %s",
	          DEG_FORMAT_ENUM_TEXT[_degformat].long_field_1, tmp1,
	          DEG_FORMAT_ENUM_TEXT[_degformat].long_field_2, tmp2);
  }
  else
  {
	  snprintf(buffer, sizeof(buffer),
  	            "%s: %s",
  	          DEG_FORMAT_ENUM_TEXT[_degformat].long_field_1, tmp1);
  }
  
  MACRO_BANNER_SHOW_INFO(_window, buffer);

  _degformat = tmp_degformat;
}

static void
cmenu_clip_latlon(const MapPoint *p)
{
    gchar buffer[80];
    MapGeo lat, lon;

    unit2latlon(p->x, p->y, lat, lon);

    snprintf(buffer, sizeof(buffer), "%.06f,%.06f", lat, lon);

    gtk_clipboard_set_text(
            gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), buffer, -1);
}

static void
cmenu_route_to(const MapPoint *p)
{
    gchar buffer[80];
    gchar strlat[32];
    gchar strlon[32];
    MapGeo lat, lon;

    unit2latlon(p->x, p->y, lat, lon);

    g_ascii_formatd(strlat, 32, "%.06f", lat);
    g_ascii_formatd(strlon, 32, "%.06f", lon);
    snprintf(buffer, sizeof(buffer), "%s, %s", strlat, strlon);

    route_download(buffer);
}

static void
cmenu_distance_to(const MapPoint *p)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gchar buffer[80];
    MapGeo lat, lon;

    unit2latlon(p->x, p->y, lat, lon);

    snprintf(buffer, sizeof(buffer), "%s: %.02f %s", _("Distance"),
            calculate_distance(gps->lat, gps->lon, lat, lon)
              * UNITS_CONVERT[_units],
            UNITS_ENUM_TEXT[_units]);
    MACRO_BANNER_SHOW_INFO(_window, buffer);
}

static void
cmenu_add_route(const MapPoint *p)
{
    MapController *controller = map_controller_get_instance();

    map_path_append_unit(map_route_get_path(), p);
    route_find_nearest_point();
    map_controller_refresh_paths(controller);
}

static gboolean
cmenu_cb_loc_show_latlon(GtkMenuItem *item)
{
    MapGeo lat, lon;

    unit2latlon(_cmenu_unit.x, _cmenu_unit.y, lat, lon);

    latlon_dialog(lat, lon);

    return TRUE;
}

static gboolean
cmenu_cb_loc_route_to(GtkMenuItem *item)
{
    cmenu_route_to(&_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_download_poi(GtkMenuItem *item)
{
    poi_download_dialog(&_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_browse_poi(GtkMenuItem *item)
{
    poi_browse_dialog(&_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_distance_to(GtkMenuItem *item)
{
    cmenu_distance_to(&_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_add_route(GtkMenuItem *item)
{
    cmenu_add_route(&_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_add_way(GtkMenuItem *item)
{
    route_add_way_dialog(&_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_add_poi(GtkMenuItem *item)
{
    poi_add_dialog(_window, &_cmenu_unit);

    return TRUE;
}

static gboolean
cmenu_cb_loc_set_gps(GtkMenuItem *item)
{
    MapController *controller = map_controller_get_instance();

    map_controller_set_gps_position(controller, &_cmenu_unit);

    return TRUE;
}

static void
cmenu_way_delete(MapPathWayPoint *way)
{
    gchar buffer[BUFFER_SIZE];
    GtkWidget *confirm;

    snprintf(buffer, sizeof(buffer), "%s:\n%s\n",
             _("Confirm delete of waypoint"), way->desc);
    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window), buffer);

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm)))
    {
        MapController *controller = map_controller_get_instance();
        MapPathPoint *pdel_start, *pdel_end;
        MapLineIter line;
        MapPath *route = map_route_get_path();
        gint num_del;

        /* Delete surrounding route data, too (from pdel_start inclusive till
         * pdel_end exclusive). */

        /* Don't delete beyond line boundaries */
        map_path_line_iter_from_point(route, way->point, &line);
        pdel_start = map_path_line_first(&line);
        pdel_end = pdel_start + map_path_line_len(&line);

        if (way > route->whead && way[-1].point + 1 > pdel_start)
            pdel_start = way[-1].point + 1;

        if(way < route->wtail && way[1].point < pdel_end)
            pdel_end = way[1].point;

#if 0
        /* If pdel_end is set to _route.tail, and if _route.tail is a
         * non-zero point, then delete _route.tail. */
        if(pdel_end == _route.tail && pdel_end->unit.y)
            pdel_end++; /* delete _route.tail too */
        /* else, if *both* endpoints are zero points, delete one. */
        else if(!pdel_start->unit.y && !pdel_end->unit.y)
            pdel_start--;

        /* Delete BETWEEN pdel_start and pdel_end, exclusive. */
        num_del = pdel_end - pdel_start - 1;

        memmove(pdel_start + 1, pdel_end,
                (_route.tail - pdel_end + 1) * sizeof(MapPathPoint));
        _route.tail -= num_del;
#endif
        map_path_remove_range(route, pdel_start, pdel_end);
        num_del = pdel_end - pdel_start;

        /* Remove waypoint and move/adjust subsequent waypoints. */
        g_free(way->desc);
        while(way++ != route->wtail)
        {
            way[-1] = *way;
            way[-1].point -= num_del;
        }
        route->wtail--;

        route_find_nearest_point();
        map_controller_refresh_paths(controller);
    }
    gtk_widget_destroy(confirm);
}

void
map_menu_point_map(const MapPoint *p)
{
    GtkWidget *dialog, *button;
    MapController *controller;
    MapDialog *dlg;
    gint response;
    enum {
        SHOW_LATLON = 0,
        DISTANCE_TO,
        ROUTE_TO,
        DOWNLOAD_POI,
        BROWSE_POI,
        ADD_ROUTE,
        ADD_WAYPOINT,
        ADD_POI,
        GPS_LOCATION,
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Map Point"),
                            map_controller_get_main_window(controller),
                            FALSE);
    dlg = (MapDialog *)dialog;

    _cmenu_unit = *p;

    button = map_dialog_create_button(dlg, _("Show Position"), SHOW_LATLON);

    button = map_dialog_create_button(dlg, _("Show Distance to"), DISTANCE_TO);

    button = map_dialog_create_button(dlg, _("Find route to..."), ROUTE_TO);

    button = map_dialog_create_button(dlg, _("Download POI..."), DOWNLOAD_POI);

    button = map_dialog_create_button(dlg, _("Browse POI..."), BROWSE_POI);

    button = map_dialog_create_button(dlg, _("Add Route Point"), ADD_ROUTE);

    button = map_dialog_create_button(dlg, _("Add Waypoint..."), ADD_WAYPOINT);

    button = map_dialog_create_button(dlg, _("Add POI..."), ADD_POI);

    button = map_dialog_create_button(dlg, _("Set as GPS Location"),
                                      GPS_LOCATION);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (response)
    {
    case SHOW_LATLON:
        cmenu_cb_loc_show_latlon(NULL); break;
    case DISTANCE_TO:
        cmenu_cb_loc_distance_to(NULL); break;
    case ROUTE_TO:
        cmenu_cb_loc_route_to(NULL); break;
    case DOWNLOAD_POI:
        cmenu_cb_loc_download_poi(NULL); break;
    case BROWSE_POI:
        cmenu_cb_loc_browse_poi(NULL); break;
    case ADD_ROUTE:
        cmenu_cb_loc_add_route(NULL); break;
    case ADD_WAYPOINT:
        cmenu_cb_loc_add_way(NULL); break;
    case ADD_POI:
        cmenu_cb_loc_add_poi(NULL); break;
    case GPS_LOCATION:
        cmenu_cb_loc_set_gps(NULL); break;
    }
}

void
map_menu_point_waypoint(MapPathWayPoint *way)
{
    GtkWidget *dialog, *button;
    MapController *controller;
    MapDialog *dlg;
    gint response;
    enum {
        SHOW_LATLON = 0,
        SHOW_DESC,
        CLIP_LATLON,
        CLIP_DESC,
        ROUTE_TO,
        DISTANCE_TO,
        DELETE,
        ADD_POI,
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Waypoint"),
                            map_controller_get_main_window(controller),
                            FALSE);
    dlg = (MapDialog *)dialog;

    button = map_dialog_create_button(dlg, _("Show Position"), SHOW_LATLON);

    button = map_dialog_create_button(dlg, _("Show Description"), SHOW_DESC);

    button = map_dialog_create_button(dlg, _("Copy Position"), CLIP_LATLON);

    button = map_dialog_create_button(dlg, _("Copy Description"), CLIP_DESC);

    button = map_dialog_create_button(dlg, _("Show Distance to"), DISTANCE_TO);

    button = map_dialog_create_button(dlg, _("Find route to..."), ROUTE_TO);

    button = map_dialog_create_button(dlg, _("Delete..."), DELETE);

    button = map_dialog_create_button(dlg, _("Add POI..."), ADD_POI);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (response)
    {
    case SHOW_LATLON:
        cmenu_show_latlon(&way->point->unit); break;
    case SHOW_DESC:
        MACRO_BANNER_SHOW_INFO(_window, way->desc); break;
    case CLIP_LATLON:
        cmenu_clip_latlon(&way->point->unit); break;
    case CLIP_DESC:
        gtk_clipboard_set_text(
                gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), way->desc, -1);
        break;
    case ROUTE_TO:
        cmenu_route_to(&way->point->unit); break;
    case DISTANCE_TO:
        route_show_distance_to(way->point); break;
    case DELETE:
        cmenu_way_delete(way); break;
    case ADD_POI:
        poi_add_dialog(_window, &way->point->unit); break;
    }
}

void
map_menu_point_poi(PoiInfo *poi)
{
    GtkWidget *dialog, *button;
    MapController *controller;
    MapDialog *dlg;
    gint response;
    MapPoint point;
    enum {
        SHOW_EDIT = 0,
        DISTANCE_TO,
        ROUTE_TO,
        ADD_ROUTE,
        ADD_WAYPOINT,
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("POI"),
                            map_controller_get_main_window(controller),
                            FALSE);
    dlg = (MapDialog *)dialog;

    button = map_dialog_create_button(dlg, _("View/Edit..."), SHOW_EDIT);
    button = map_dialog_create_button(dlg, _("Show Distance to"), DISTANCE_TO);
    button = map_dialog_create_button(dlg, _("Find route to..."), ROUTE_TO);
    button = map_dialog_create_button(dlg, _("Add Route Point"), ADD_ROUTE);
    button = map_dialog_create_button(dlg, _("Add Waypoint..."), ADD_WAYPOINT);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    /* the POI coordinates in units are needed for most of the handlers, so
     * let's calculate them here */
    latlon2unit(poi->lat, poi->lon, point.x, point.y);

    switch (response)
    {
    case SHOW_EDIT:
        poi_view_dialog(_window, poi); break;
    case DISTANCE_TO:
        cmenu_distance_to(&point); break;
    case ROUTE_TO:
        cmenu_route_to(&point); break;
    case ADD_ROUTE:
        cmenu_add_route(&point); break;
    case ADD_WAYPOINT:
        route_add_way_dialog(&point); break;
    }
}

static void
map_menu_point_select(const MapPoint *p, MapPathWayPoint *wp, GtkTreeModel *model)
{
    GtkWidget *dialog, *button;
    MapController *controller;
    MapDialog *dlg;
    PoiInfo poi;
    gint response;
    enum {
        POINT_MAP,
        POINT_WAYPOINT,
        POINT_POI
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Select point"),
                            map_controller_get_main_window(controller),
                            TRUE);
    dlg = (MapDialog *)dialog;

    button = map_dialog_create_button(dlg, _("Map Point"), POINT_MAP);
    if (wp)
        button = map_dialog_create_button(dlg, _("Waypoint"), POINT_WAYPOINT);
    if (model)
        button = map_dialog_create_button(dlg, _("POI"), POINT_POI);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (response) {
    case POINT_MAP:
        map_menu_point_map(p); break;
    case POINT_WAYPOINT:
        map_menu_point_waypoint(wp); break;
    case POINT_POI:
        if (poi_run_select_dialog(model, &poi))
        {
            map_menu_point_poi(&poi);
            g_free(poi.label);
            g_free(poi.desc);
            g_free(poi.clabel);
        }
        break;
    }
}

void
map_menu_point(const MapPoint *p, MapArea *area)
{
    GtkTreeModel *model;
    MapPathWayPoint *way;

    /* check whether a waypoint is nearby */
    way = find_nearest_waypoint(p);

    /* check whether some POI is nearby */
    model = poi_get_model_for_area(area);

    /* if we have any waypoint or POI, first open a dialog for the user to
     * select which one he wanted to pick */
    if (way || model)
    {
        map_menu_point_select(p, way, model);
        if (model)
            g_object_unref(model);
    }
    else
        map_menu_point_map(p);
}

