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

#include "types.h"
#include "data.h"
#include "defines.h"

/* Constants regarding enums and defaults. */
gchar *UNITS_ENUM_TEXT[UNITS_ENUM_COUNT];
gchar *UNITS_SMALL_TEXT[UNITS_ENUM_COUNT];

/* UNITS_CONVERT, when multiplied, converts from kilometers. */
gfloat UNITS_CONVERT[] =
{
    1.0,		/* to kilometers */
    0.621371192,	/* to miles */
    0.539956803,	/* to nautical miles */
};
/* UNITS_SMALL_CONVERT, when multiplied, converts from the chosen unit to its
 * smaller unit used for close distances */
gfloat UNITS_SMALL_CONVERT[] =
{
    1000.0,             /* to metres */
    1760.0,             /* to yards */
    1.0,                /* unused */
};

gchar *UNBLANK_ENUM_TEXT[UNBLANK_ENUM_COUNT];
gchar *INFO_FONT_ENUM_TEXT[INFO_FONT_ENUM_COUNT];
gchar *COLORABLE_GCONF[COLORABLE_ENUM_COUNT];
GdkColor COLORABLE_DEFAULT[COLORABLE_ENUM_COUNT] =
{
    {0, 0x0000, 0x0000, 0xc000}, /* COLORABLE_MARK */
    {0, 0x6000, 0x6000, 0xf800}, /* COLORABLE_MARK_VELOCITY */
    {0, 0x8000, 0x8000, 0x8000}, /* COLORABLE_MARK_OLD */
    {0, 0xe000, 0x0000, 0x0000}, /* COLORABLE_TRACK */
    {0, 0xa000, 0x0000, 0x0000}, /* COLORABLE_TRACK_MARK */
    {0, 0x7000, 0x0000, 0x0000}, /* COLORABLE_TRACK_BREAK */
    {0, 0x0000, 0xa000, 0x0000}, /* COLORABLE_ROUTE */
    {0, 0x0000, 0x8000, 0x0000}, /* COLORABLE_ROUTE_WAY */
    {0, 0x0000, 0x6000, 0x0000}, /* COLORABLE_ROUTE_BREAK */
    {0, 0xd000, 0xd000, 0x0000}, /* COLORABLE_ROUTE_NEXT */
    {0, 0xa000, 0x0000, 0xa000}, /* COLORABLE_POI */
};
const CoordFormatSetup DEG_FORMAT_ENUM_TEXT[DEG_FORMAT_ENUM_COUNT] = {
    [DDPDDDDD] = {
        .name = "-dd.ddddd°",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [DD_MMPMMM] = {
        .name = "-dd°mm.mmm'",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [DD_MM_SSPS] = {
        .name = "-dd°mm'ss.s\"",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [DDPDDDDD_NSEW] = {
        .name = "dd.ddddd° S",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [DD_MMPMMM_NSEW] = {
        .name = "dd°mm.mmm' S",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [DD_MM_SSPS_NSEW] = {
        .name = "dd°mm'ss.s\" S",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [NSEW_DDPDDDDD] = {
        .name = "S dd.ddddd°",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [NSEW_DD_MMPMMM] = {
        .name = "S dd° mm.mmm'",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    [NSEW_DD_MM_SSPS] = {
        .name = "S dd° mm' ss.s\"",
        .short_field_1 = "Lat",
        .long_field_1 = "Latitude",
        .short_field_2 = "Lon",
        .long_field_2 = "Longitude",
        .field_2_in_use = TRUE,
    },
    // Used by Radio Amateurs
    [IARU_LOC] = {
        .name = "IARU Locator",
        .short_field_1 = "Locator",
        .long_field_1 = "Locator",
        .short_field_2 = "",
        .long_field_2 = "",
        .field_2_in_use = FALSE,
    },
    [UK_OSGB] = {
        .name = "OSGB X,Y Grid",
        .short_field_1 = "X",
        .long_field_1 = "OS X",
        .short_field_2 = "Y",
        .long_field_2 = "OS Y",
        .field_2_in_use = TRUE,
    },
    [UK_NGR] = {
        .name = "OSGB Landranger Grid (8)",
        .short_field_1 = "GR",
        .long_field_1 = "OS Grid",
        .short_field_2 = "",
        .long_field_2 = "",
        .field_2_in_use = FALSE,
    },
    [UK_NGR6] = {
        .name = "OSGB Landranger Grid (6)",
        .short_field_1 = "GR",
        .long_field_1 = "OS Grid",
        .short_field_2 = "",
        .long_field_2 = "",
        .field_2_in_use = FALSE,
    },
};
gchar *SPEED_LOCATION_ENUM_TEXT[SPEED_LOCATION_ENUM_COUNT];

/** The main GtkContainer of the application. */
GtkWidget *_window = NULL;

/** The main OSSO context of the application. */
osso_context_t *_osso = NULL;

/* The controller object of the application. */
MapController *_controller = NULL;

/** The widget that provides the visual display of the map. */
GtkWidget *_w_map = NULL;

/** The context menu for the map. */
GtkMenu *_map_cmenu = NULL;

gint _map_offset_devx;
gint _map_offset_devy;

gint _map_rotate_angle = 0;

GtkWidget *_text_lat = NULL;
GtkWidget *_text_lon = NULL;
GtkWidget *_text_speed = NULL;
GtkWidget *_text_alt = NULL;
GtkWidget *_sat_panel = NULL;
GtkWidget *_text_time = NULL;
GtkWidget *_heading_panel = NULL;

/** GPS data. */
MapPathPoint _pos = { { 0, 0 }, 0, 0, SHRT_MIN, 0};
const MapPathPoint _point_null = { { 0, 0 }, 0, SCHAR_MAX, 0, 0};

GpsSatelliteData _gps_sat[MAX_SATELLITES];
gboolean _satdetails_on = FALSE;

gboolean _is_first_time = FALSE;

/** VARIABLES FOR MAINTAINING STATE OF THE CURRENT VIEW. */

/** The "zoom" level defines the resolution of a pixel, from 0 to MAX_ZOOM.
 * Each pixel in the current view is exactly (1 << _zoom) "units" wide. */
gint _zoom = 3; /* zoom level, from 0 to MAX_ZOOM. */
MapPoint _center = {-1, -1}; /* current center location, X. */

gint _next_zoom = 3;
MapPoint _next_center = {-1, -1};
gint _next_map_rotate_angle = 0;
GdkPixbuf *_redraw_wait_icon = NULL;
GdkRectangle _redraw_wait_bounds = { 0, 0, 0, 0};

/** CACHED SCREEN INFORMATION THAT IS DEPENDENT ON THE CURRENT VIEW. */
gint _view_width_pixels = 0;
gint _view_height_pixels = 0;
gint _view_halfwidth_pixels = 0;
gint _view_halfheight_pixels = 0;

/** The current track and route. */
MapPath _track;
MapPath _route;

/** THE GdkGC OBJECTS USED FOR DRAWING. */
GdkGC *_gc[COLORABLE_ENUM_COUNT];
GdkColor _color[COLORABLE_ENUM_COUNT];

/** BANNERS. */
GtkWidget *_fix_banner = NULL;
GtkWidget *_waypoint_banner = NULL;
GtkWidget *_download_banner = NULL;

/** DOWNLOAD PROGRESS. */
gboolean _conic_is_connected = FALSE;
volatile gint _num_downloads = 0;
gint _curr_download = 0;
GHashTable *_mut_exists_table = NULL;
GTree *_mut_priority_tree = NULL;
GMutex *_mut_priority_mutex = NULL;
/* NOMORE gint _dl_errors = 0; */
GThreadPool *_mut_thread_pool = NULL;
GThreadPool *_mrt_thread_pool = NULL;

/* Need to refresh map after downloads finished. This is needed when during render task we find tile
   to download and we have something to draw on top of it. */
gboolean _refresh_map_after_download = FALSE;

/** CONFIGURATION INFORMATION. */
ConnState _gps_state;
gchar *_route_dir_uri = NULL;
gchar *_track_file_uri = NULL;
CenterMode _center_mode = CENTER_LEAD;
gboolean _center_rotate = TRUE;
gboolean _fullscreen = FALSE;
gboolean _enable_gps = FALSE;
gboolean _enable_tracking = TRUE;
gboolean _gps_info = FALSE;

gint _route_dl_radius = 4;
gchar *_poi_dl_url = NULL;
gint _show_paths = 0;
gboolean _show_zoomlevel = TRUE;
gboolean _show_scale = TRUE;
gboolean _show_comprose = TRUE;
gboolean _show_velvec = TRUE;
gboolean _show_poi = TRUE;
gboolean _auto_download = FALSE;
gint _auto_download_precache = 2;
gint _lead_ratio = 5;
gboolean _lead_is_fixed = FALSE;
gint _center_ratio = 5;
gint _draw_width = 5;
gint _rotate_sens = 5;
gint _ac_min_speed = 2;
gboolean _enable_announce = TRUE;
gint _announce_notice_ratio = 8;
gboolean _enable_voice = TRUE;
GSList *_loc_list;
GtkListStore *_loc_model;
UnitType _units = UNITS_KM;
gint _degformat = DDPDDDDD;
gboolean _speed_limit_on = FALSE;
gint _speed_limit = 100;
gboolean _speed_excess = FALSE;
SpeedLocation _speed_location = SPEED_LOCATION_TOP_RIGHT;
UnblankOption _unblank_option = UNBLANK_FULLSCREEN;
InfoFontSize _info_font_size = INFO_FONT_MEDIUM;
//----------------------


/** POI */
gchar *_poi_db_filename = NULL;
gchar *_poi_db_dirname = NULL;
gint _poi_zoom = 6;
gboolean _poi_enabled = FALSE;


/*********************
 * BELOW: MENU ITEMS *
 *********************/

/* Menu items for the "Maps" submenu. */
GtkWidget *_menu_layers_submenu = NULL;

/*********************
 * ABOVE: MENU ITEMS *
 *********************/
