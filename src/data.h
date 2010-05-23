/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2009-2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAEMO_MAPPER_DATA_H
#define MAEMO_MAPPER_DATA_H

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <libosso.h>
#include "controller.h"
#include "types.h"

/* Constants regarding enums and defaults. */
extern gchar *UNITS_ENUM_TEXT[UNITS_ENUM_COUNT];
extern gfloat UNITS_CONVERT[UNITS_ENUM_COUNT];
extern gchar *UNITS_SMALL_TEXT[UNITS_ENUM_COUNT];
extern gfloat UNITS_SMALL_CONVERT[UNITS_ENUM_COUNT];
extern gchar *UNBLANK_ENUM_TEXT[UNBLANK_ENUM_COUNT];
extern gchar *INFO_FONT_ENUM_TEXT[INFO_FONT_ENUM_COUNT];
extern gchar *COLORABLE_GCONF[COLORABLE_ENUM_COUNT];
extern GdkColor COLORABLE_DEFAULT[COLORABLE_ENUM_COUNT];
extern const CoordFormatSetup DEG_FORMAT_ENUM_TEXT[DEG_FORMAT_ENUM_COUNT];
extern gchar *SPEED_LOCATION_ENUM_TEXT[SPEED_LOCATION_ENUM_COUNT];

/** The main GtkContainer of the application. */
extern GtkWidget *_window;

/** The main OSSO context of the application. */
extern osso_context_t *_osso;

/** The controller object of the application. */
extern MapController *_controller;

/** The widget that provides the visual display of the map. */
extern GtkWidget *_w_map;

/** The context menu for the map. */
extern GtkMenu *_map_cmenu;

extern gint _map_offset_devx;
extern gint _map_offset_devy;

extern gint _map_rotate_angle;

extern GtkWidget *_text_lat;
extern GtkWidget *_text_lon;
extern GtkWidget *_text_speed;
extern GtkWidget *_text_alt;
extern GtkWidget *_sat_panel;
extern GtkWidget *_text_time;
extern GtkWidget *_heading_panel;

/** GPS data. */
extern MapPathPoint _pos;
extern const MapPathPoint _point_null;

extern GpsSatelliteData _gps_sat[MAX_SATELLITES];
extern gboolean _satdetails_on;

extern gboolean _is_first_time;

/** VARIABLES FOR MAINTAINING STATE OF THE CURRENT VIEW. */

/** The "zoom" level defines the resolution of a pixel, from 0 to MAX_ZOOM.
 * Each pixel in the current view is exactly (1 << _zoom) "units" wide. */
extern gint _zoom; /* zoom level, from 0 to MAX_ZOOM. */
extern MapPoint _center; /* current center location, X. */

extern gint _next_zoom;
extern MapPoint _next_center;
extern gint _next_map_rotate_angle;
extern GdkPixbuf *_redraw_wait_icon;
extern GdkRectangle _redraw_wait_bounds;

/** CACHED SCREEN INFORMATION THAT IS DEPENDENT ON THE CURRENT VIEW. */
extern gint _view_width_pixels;
extern gint _view_height_pixels;
extern gint _view_halfwidth_pixels;
extern gint _view_halfheight_pixels;


/** The current track and route. */
extern MapPath _track;

/** THE GdkGC OBJECTS USED FOR DRAWING. */
extern GdkGC *_gc[COLORABLE_ENUM_COUNT];
extern GdkColor _color[COLORABLE_ENUM_COUNT];

/** BANNERS. */
extern GtkWidget *_fix_banner;
extern GtkWidget *_waypoint_banner;
extern GtkWidget *_download_banner;

/** DOWNLOAD PROGRESS. */
extern gboolean _conic_is_connected;
extern volatile gint _num_downloads;
extern gint _curr_download;
extern GHashTable *_mut_exists_table;
extern GTree *_mut_priority_tree;
extern GMutex *_mut_priority_mutex;
extern GThreadPool *_mut_thread_pool;
extern GThreadPool *_mrt_thread_pool;
extern gboolean _refresh_map_after_download;

/** CONFIGURATION INFORMATION. */
extern ConnState _gps_state;
extern gchar *_route_dir_uri;
extern gchar *_track_file_uri;
extern CenterMode _center_mode;
extern gboolean _center_rotate;
extern gboolean _fullscreen;
extern gboolean _enable_gps;
extern gboolean _enable_tracking;
extern gboolean _gps_info;

extern gint _route_dl_radius;
extern gchar *_poi_dl_url;
extern gint _show_paths;
extern gboolean _show_zoomlevel;
extern gboolean _show_scale;
extern gboolean _show_comprose;
extern gboolean _show_velvec;
extern gboolean _show_poi;
extern gboolean _auto_download;
extern gint _auto_download_precache;
extern gint _lead_ratio;
extern gboolean _lead_is_fixed;
extern gint _center_ratio;
extern gint _draw_width;
extern gint _rotate_sens;
extern gint _ac_min_speed;
extern gboolean _enable_announce;
extern gint _announce_notice_ratio;
extern gboolean _enable_voice;
extern GSList *_loc_list;
extern GtkListStore *_loc_model;
extern UnitType _units;
extern gint _degformat;
extern gboolean _speed_limit_on;
extern gint _speed_limit;
extern gboolean _speed_excess;
extern SpeedLocation _speed_location;
extern UnblankOption _unblank_option;
extern InfoFontSize _info_font_size;

/** POI */
extern gchar *_poi_db_filename;
extern gchar *_poi_db_dirname;
extern gint _poi_zoom;
extern gboolean _poi_enabled;


/*********************
 * BELOW: MENU ITEMS *
 *********************/

/* Menu items for the "Maps" submenu. */
extern GtkWidget *_menu_layers_submenu;

/*********************
 * ABOVE: MENU ITEMS *
 *********************/

#endif /* ifndef MAEMO_MAPPER_DATA_H */

