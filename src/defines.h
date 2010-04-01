/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAEMO_MAPPER_DEFINES
#define MAEMO_MAPPER_DEFINES

#include <libintl.h>

#include "tile_source.h"

#define _(String) gettext(String)
#define H_(String) dgettext("hildon-libs", String)

#define BOUND(x, a, b) { \
    if((x) < (a)) \
        (x) = (a); \
    else if((x) > (b)) \
        (x) = (b); \
}

#define PI   ((MapGeo)3.14159265358979323846)

#ifdef USE_DOUBLES_FOR_LATLON
#define GSIN(x) sin(x)
#define GCOS(x) cos(x)
#define GASIN(x) asin(x)
#define GTAN(x) tan(x)
#define GATAN(x) atan(x)
#define GATAN2(x, y) atan2(x, y)
#define GEXP(x) exp(x)
#define GLOG(x) log(x)
#define GPOW(x, y) pow(x, y)
#define GSQTR(x) sqrt(x)
#else
#define GSIN(x) sinf(x)
#define GCOS(x) cosf(x)
#define GASIN(x) asinf(x)
#define GTAN(x) tanf(x)
#define GATAN(x) atanf(x)
#define GATAN2(x, y) atan2f(x, y)
#define GEXP(x) expf(x)
#define GLOG(x) logf(x)
#define GPOW(x, y) powf(x, y)
#define GSQTR(x) sqrtf(x)
#endif

#define EARTH_RADIUS (6378.13701) /* Kilometers */

/* mean circumference, in meters */
#define EARTH_CIRCUMFERENCE (40041470)

/* BT dbus service location */
#define BASE_PATH                "/org/bluez"
#define BASE_INTERFACE           "org.bluez"
//#define ADAPTER_PATH             BASE_PATH
#define ADAPTER_INTERFACE        BASE_INTERFACE ".Adapter"
#define MANAGER_PATH             BASE_PATH
#define MANAGER_INTERFACE        BASE_INTERFACE ".Manager"
#define ERROR_INTERFACE          BASE_INTERFACE ".Error"
#define SECURITY_INTERFACE       BASE_INTERFACE ".Security"
#define RFCOMM_INTERFACE         BASE_INTERFACE ".RFCOMM"
#define BLUEZ_DBUS               BASE_INTERFACE

#define LIST_ADAPTERS            "ListAdapters"
#define LIST_BONDINGS            "ListBondings"
//#define CREATE_BONDING           "CreateBonding"
#define GET_REMOTE_NAME          "GetRemoteName"
#define GET_REMOTE_SERVICE_CLASSES "GetRemoteServiceClasses"

#define BTCOND_PATH              "/com/nokia/btcond/request"
#define BTCOND_BASE              "com.nokia.btcond"
#define BTCOND_INTERFACE         BTCOND_BASE ".request"
#define BTCOND_REQUEST           BTCOND_INTERFACE
#define BTCOND_CONNECT           "rfcomm_connect"
#define BTCOND_DISCONNECT        "rfcomm_disconnect"
#define BTCOND_DBUS              BTCOND_BASE


/** MAX_ZOOM defines the largest map zoom level we will download.
 * (MAX_ZOOM - 1) is the largest map zoom level that the user can zoom to.
 */
#define MIN_ZOOM (0)
#define MAX_ZOOM (20)

#define TILE_SIZE_PIXELS (256)
#define TILE_HALFDIAG_PIXELS (181)
#define TILE_SIZE_P2 (8)

#define ARRAY_CHUNK_SIZE (1024)

#define BUFFER_SIZE (2048)

#define GPSD_PORT_DEFAULT (2947)

#define NUM_DOWNLOAD_THREADS (4)
#define WORLD_SIZE_UNITS (2 << (MAX_ZOOM + TILE_SIZE_P2))

/* This gets more and more wrong as the latitude increases */
#define METRES_TO_UNITS(m) ((gint64)(m) * WORLD_SIZE_UNITS / EARTH_CIRCUMFERENCE)

#define HOURGLASS_SEPARATION (7)

/* (im)precision of a finger tap, in screen pixels */
#define TOUCH_RADIUS    25

#define deg2rad(deg) ((deg) * (PI / 180.0))
#define rad2deg(rad) ((rad) * (180.0 / PI))

#define tile2pixel(TILE) ((TILE) << TILE_SIZE_P2)
#define pixel2tile(PIXEL) ((PIXEL) >> TILE_SIZE_P2)
#define tile2unit(TILE) ((TILE) << (TILE_SIZE_P2 + _zoom))
#define unit2tile(unit) ((unit) >> (TILE_SIZE_P2 + _zoom))
#define tile2zunit(TILE, ZOOM) ((TILE) << (TILE_SIZE_P2 + (ZOOM)))
#define unit2ztile(unit, ZOOM) ((unit) >> (TILE_SIZE_P2 + (ZOOM)))

#define pixel2unit(PIXEL) ((PIXEL) << _zoom)
#define unit2pixel(PIXEL) ((PIXEL) >> _zoom)
#define pixel2zunit(PIXEL, ZOOM) ((PIXEL) << (ZOOM))
#define unit2zpixel(PIXEL, ZOOM) ((PIXEL) >> (ZOOM))

/* Pans are done 64 pixels at a time. */
#define PAN_PIXELS (64)
#define ROTATE_DEGREES (30)

#define INITIAL_DOWNLOAD_RETRIES (3)

#define CONFIG_DIR_NAME "~/.maemo-mapper/"
#define CONFIG_PATH_DB_FILE "paths.db"

#define CACHE_BASE_DIR "~/MyDocs/.maps/"

#define REPO_DEFAULT_NAME "OpenStreet"
#define REPO_DEFAULT_TYPE "XYZ_INV"
#define REPO_DEFAULT_CACHE_DIR "OpenStreetMap I"
#define REPO_DEFAULT_FILE_EXT "png"
#define REPO_DEFAULT_MAP_URI "http://tile.openstreetmap.org/%0d/%d/%d.png"
#define REPO_DEFAULT_DL_ZOOM_STEPS (2)
#define REPO_DEFAULT_VIEW_ZOOM_STEPS (1)
#define REPO_DEFAULT_MIN_ZOOM (4)
#define REPO_DEFAULT_MAX_ZOOM (20)

#define XML_DATE_FORMAT "%FT%T"

#define HELP_ID_PREFIX "help_maemomapper_"
#define HELP_ID_INTRO HELP_ID_PREFIX"intro"
#define HELP_ID_GETSTARTED HELP_ID_PREFIX"getstarted"
#define HELP_ID_ABOUT HELP_ID_PREFIX"about"
#define HELP_ID_SETTINGS HELP_ID_PREFIX"settings"
#define HELP_ID_NEWREPO HELP_ID_PREFIX"newrepo"
#define HELP_ID_REPOMAN HELP_ID_PREFIX"repoman"
#define HELP_ID_MAPMAN HELP_ID_PREFIX"mapman"
#define HELP_ID_DOWNROUTE HELP_ID_PREFIX"downroute"
#define HELP_ID_DOWNPOI HELP_ID_PREFIX"downpoi"
#define HELP_ID_BROWSEPOI HELP_ID_PREFIX"browsepoi"
#define HELP_ID_POILIST HELP_ID_PREFIX"poilist"
#define HELP_ID_POICAT HELP_ID_PREFIX"poicat"

#define latlon2unit(lat, lon, unitx, unity) \
    (tile_source_get_primary_type()->latlon_to_unit(lat, lon, &unitx, &unity))
#define unit2latlon(unitx, unity, lat, lon) \
    (tile_source_get_primary_type()->unit_to_latlon(unitx, unity, &lat, &lon))


#define SQUARE(n) ((n) * (n))

#define DISTANCE_SQUARED(a, b) \
   ((guint64)((((gint64)(b).x)-(a).x)*(((gint64)(b).x)-(a).x))\
  + (guint64)((((gint64)(b).y)-(a).y)*(((gint64)(b).y)-(a).y)))

#define MACRO_QUEUE_DRAW_AREA()

#define UNBLANK_SCREEN(MOVING, APPROACHING_WAYPOINT) { \
    /* Check if we need to unblank the screen. */ \
    switch(_unblank_option) \
    { \
        case UNBLANK_NEVER: \
            break; \
        case UNBLANK_WAYPOINT: \
            if(APPROACHING_WAYPOINT) \
            { \
                DEBUG("Unblanking screen..."); \
                osso_display_blanking_pause(_osso); \
            } \
            break; \
        default: \
        case UNBLANK_FULLSCREEN: \
            if(!_fullscreen) \
                break; \
        case UNBLANK_WHEN_MOVING: \
            if(!(MOVING)) \
                break; \
        case UNBLANK_WITH_GPS: \
            DEBUG("Unblanking screen..."); \
            osso_display_blanking_pause(_osso); \
    } \
}

#define LL_FMT_LEN 20
#define lat_format(A, B) deg_format((A), (B), 'S', 'N')
#define lon_format(A, B) deg_format((A), (B), 'W', 'E')

#define TRACKS_MASK 0x00000001
#define ROUTES_MASK 0x00000002

#define MACRO_BANNER_SHOW_INFO(A, S) { \
    gchar *my_macro_buffer = g_markup_printf_escaped( \
            "<span size='%s'>%s</span>", \
            INFO_FONT_ENUM_TEXT[_info_font_size], (S)); \
    hildon_banner_show_information_with_markup(A, NULL, my_macro_buffer); \
    g_free(my_macro_buffer); \
}

#define _voice_synth_path "/usr/bin/flite"

#define STR_EMPTY(s) (!s || s[0] == '\0')

/* Sanitize SQLite's sqlite3_column_text: */
#define sqlite3_column_str(stmt, col) \
    ((const gchar *)sqlite3_column_text(stmt, col))

#ifdef ENABLE_DEBUG

#ifdef g_mutex_lock
#undef g_mutex_lock
#endif
#define g_mutex_lock(mutex) \
    G_STMT_START { \
        struct timespec ts0, ts1; \
        long ms_diff; \
        clock_gettime(CLOCK_MONOTONIC, &ts0); \
        G_THREAD_CF (mutex_lock,     (void)0, (mutex)); \
        clock_gettime(CLOCK_MONOTONIC, &ts1); \
        ms_diff = (ts1.tv_sec - ts0.tv_sec) * 1000 + \
                  (ts1.tv_nsec - ts0.tv_nsec) / 1000000; \
        if (ms_diff > 300) \
            g_warning("%s: %ld wait for %s", G_STRLOC, ms_diff, #mutex); \
    } G_STMT_END

#endif /* ENABLE_DEBUG */

#endif /* ifndef MAEMO_MAPPER_DEFINES */
