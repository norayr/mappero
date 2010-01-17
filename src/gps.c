/*
 * Copyright (C) 2006, 2007 John Costigan.
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

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>
#include <errno.h>

#ifndef LEGACY
#    include <hildon/hildon-note.h>
#    include <hildon/hildon-banner.h>
#else
#    include <hildon-widgets/hildon-note.h>
#    include <hildon-widgets/hildon-banner.h>
#endif

#include "types.h"
#include "data.h"
#include "defines.h"

#include "display.h"
#include "gps.h"

#ifdef HAVE_LIBGPSBT
#    include "gpsbt.h"
#endif

#ifdef HAVE_LIBGPSMGR
#    include "gpsmgr.h"
#endif

#include "path.h"
#include "util.h"

#include <location/location-gpsd-control.h>
#include <location/location-gps-device.h>

static LocationGPSDControl *gpsd_control = NULL;
static LocationGPSDevice *gps_device = NULL;

static volatile GThread *_gps_thread = NULL;
static GMutex *_gps_init_mutex = NULL;

static gint _gmtoffset = 0;

#define MACRO_PARSE_INT(tofill, str) { \
    gchar *error_check; \
    (tofill) = strtol((str), &error_check, 10); \
    if(error_check == (str)) \
    { \
        g_printerr("Line %d: Failed to parse string as int: %s\n", \
                __LINE__, str); \
        MACRO_BANNER_SHOW_INFO(_window, \
                _("Invalid NMEA input from receiver!")); \
        return; \
    } \
}
#define MACRO_PARSE_FLOAT(tofill, str) { \
    gchar *error_check; \
    (tofill) = g_ascii_strtod((str), &error_check); \
    if(error_check == (str)) \
    { \
        g_printerr("Failed to parse string as float: %s\n", str); \
        MACRO_BANNER_SHOW_INFO(_window, \
                _("Invalid NMEA input from receiver!")); \
        return; \
    } \
}

static void update_satellite_info(LocationGPSDeviceSatellite *satellite,
                                  int satellite_index)
{
    if (satellite_index >= MAX_SATELLITES)
        return;

    _gps_sat[satellite_index].prn	= satellite->prn;
    _gps_sat[satellite_index].elevation	= satellite->elevation / 100;
    _gps_sat[satellite_index].azimuth	= satellite->azimuth;

    _gps_sat[satellite_index].snr	= satellite->signal_strength;
    _gps.satforfix[satellite_index]	= satellite->in_use ?
        satellite->prn : 0;
}

static void
on_gps_changed(LocationGPSDevice *device, MapController *controller)
{
    int i;
    gboolean newly_fixed = FALSE;

    /* ignore any signals arriving while gps is disabled */
    if (!_enable_gps)
        return;

    if (device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET) {
        _gps.lat = device->fix->latitude;
        _gps.lon = device->fix->longitude;
    }

    if (device->fix->fields & LOCATION_GPS_DEVICE_SPEED_SET) {
        _gps.speed = device->fix->speed;
    }

    if (device->fix->fields & LOCATION_GPS_DEVICE_TRACK_SET) {
        _gps.heading = device->fix->track;
        if (map_controller_get_auto_rotate(controller))
            map_controller_set_rotation(controller, _gps.heading);
    }

    /* fetch timestamp from gps if available, otherwise create one. */
    if (device->fix->fields & LOCATION_GPS_DEVICE_TIME_SET) {
        _pos.time = (time_t)device->fix->time;
    } else {
        _pos.time = time(NULL);
    }

    /* fetch altitude in meters from gps */
    if (device->fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET) {
        _pos.altitude = (gint)device->fix->altitude;
    }

    /*
     * Horizontal accuracy, liblocation provides the value in
     * centimeters
     */
    _gps.hdop = device->fix->eph / 100;

    /* Vertical inaccuracy, in meters */
    _gps.vdop = device->fix->epv;

    /* Translate data into integers. */
    latlon2unit(_gps.lat, _gps.lon, _pos.unitx, _pos.unity);

    /* We consider a fix only if the geocoordinates are given */
    if (device->status >= LOCATION_GPS_DEVICE_STATUS_FIX)
    {
        _gps.fix = device->fix->mode;

        /* Data is valid. */
        if (_gps_state < RCVR_FIXED)
        {
            newly_fixed = TRUE;
            set_conn_state(RCVR_FIXED);
        }

        /* Add the point to the track only if it's not too inaccurate: if the
         * uncertainty is greater than 200m, don't add it (TODO: this should be
         * a configuration option). */
        if (_gps.hdop < 200 &&
            (track_add(_pos.time, newly_fixed) || newly_fixed))
	{
	    /* Move mark to new location. */
	    map_refresh_mark(FALSE);
	}
	else
	{
	    map_move_mark();
	}
    } else {
        track_insert_break(FALSE);
        _gps.speed = 0;
        _gps.fix = 0;
        set_conn_state(RCVR_UP);
        map_move_mark();
    }

    for(i = 0; device->satellites && i < device->satellites->len; i++)
        update_satellite_info((LocationGPSDeviceSatellite *)
                              device->satellites->pdata[i], i);
    _gps.satinuse = device->satellites_in_use;
    _gps.satinview = device->satellites_in_view;

    if(_gps_info)
        gps_display_data();

    if(_satdetails_on)
        gps_display_details();
}

/**
 * Set the connection state.  This function controls all connection-related
 * banners.
 */
void
set_conn_state(ConnState new_conn_state)
{
    printf("%s(%d)\n", __PRETTY_FUNCTION__, new_conn_state);

    switch(_gps_state = new_conn_state)
    {
        case RCVR_OFF:
        case RCVR_FIXED:
            if(_fix_banner)
            {
                gtk_widget_destroy(_fix_banner);
                _fix_banner = NULL;
            }
            break;
        case RCVR_DOWN:
            if(_fix_banner)
            {
                gtk_widget_destroy(_fix_banner);
                _fix_banner = NULL;
            }
            break;
        case RCVR_UP:
            if(!_fix_banner)
            {
                _fix_banner = hildon_banner_show_progress(
                        _window, NULL, _("Establishing GPS fix"));
            }
            break;
        default: ; /* to quell warning. */
    }
    map_force_redraw();

    vprintf("%s(): return\n", __PRETTY_FUNCTION__);
}

/**
 * Disconnect from the receiver.  This method cleans up any and everything
 * that might be associated with the receiver.
 */
void
rcvr_disconnect()
{
    printf("%s()\n", __PRETTY_FUNCTION__);

    location_gpsd_control_stop(gpsd_control);

    if(_window)
        set_conn_state(RCVR_OFF);
    vprintf("%s(): return\n", __PRETTY_FUNCTION__);
}

/**
 * Connect to the receiver.
 * This method assumes that _fd is -1 and _channel is NULL.  If unsure, call
 * rcvr_disconnect() first.
 * Since this is an idle function, this function returns whether or not it
 * should be called again, which is always FALSE.
 */
gboolean
rcvr_connect()
{
    printf("%s(%d)\n", __PRETTY_FUNCTION__, _gps_state);

    if(_enable_gps && _gps_state == RCVR_OFF)
    {
        set_conn_state(RCVR_DOWN);

	location_gpsd_control_start(gpsd_control);
    }

    vprintf("%s(): return FALSE\n", __PRETTY_FUNCTION__);
    return FALSE;
}

void
reset_bluetooth()
{
    printf("%s()\n", __PRETTY_FUNCTION__);
    if(system("/usr/bin/sudo -l | grep -q '/usr/sbin/hciconfig  *hci0  *reset'"
            " && sudo /usr/sbin/hciconfig hci0 reset"))
        popup_error(_window,
                _("An error occurred while trying to reset the bluetooth "
                "radio.\n\n"
                "Did you make sure to modify\nthe /etc/sudoers file?"));
    else if(_gps_state != RCVR_OFF)
    {
        rcvr_connect();
    }
    vprintf("%s(): return\n", __PRETTY_FUNCTION__);
}

void
gps_init()
{
    printf("%s()\n", __PRETTY_FUNCTION__);

    if (!gpsd_control)
    {
        MapController *controller = map_controller_get_instance();
	gpsd_control = location_gpsd_control_get_default();

	gps_device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
	g_signal_connect (gps_device, "changed",
			  G_CALLBACK(on_gps_changed), controller);
    }

    _gps_init_mutex = g_mutex_new();

    /* Fix a stupid PATH bug in libgps. */
    {
        gchar *path_env = getenv("PATH");
        gchar *new_path = g_strdup_printf("%s:%s", path_env, "/usr/sbin");
        setenv("PATH", new_path, 1);
        g_free(new_path);
    }

    /* set _gpsoffset */
    {   
        time_t time1;
        struct tm time2;
        time1 = time(NULL);
        localtime_r(&time1, &time2);
        _gmtoffset = time2.tm_gmtoff;
    }

    vprintf("%s(): return FALSE\n", __PRETTY_FUNCTION__);
}

void
gps_destroy(gboolean last)
{
    static GThread* tmp = NULL;
    printf("%s()\n", __PRETTY_FUNCTION__);

    if(!last)
    {
        if(_gps_thread)
        {
            tmp = (GThread*)_gps_thread;
            _gps_thread = NULL;
        }
    }
    else if(tmp)
        g_thread_join(tmp);

    vprintf("%s(): return FALSE\n", __PRETTY_FUNCTION__);
}

