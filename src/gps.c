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
#include "debug.h"
#include "defines.h"

#include "display.h"
#include "gps.h"
#include "path.h"
#include "util.h"

#include "controller-priv.h"

static void
load_settings(MapGpsData *gps, GConfClient *gconf_client)
{
    GConfValue *value;

    /* Get last saved latitude.  Default is 50.f. */
    value = gconf_client_get(gconf_client, GCONF_KEY_LAST_LAT, NULL);
    if (value)
    {
        gps->lat = gconf_value_get_float(value);
        gconf_value_free(value);
    }
    else
        gps->lat = 50.f;

    /* Get last saved longitude.  Default is 0. */
    gps->lon = gconf_client_get_float(gconf_client, GCONF_KEY_LAST_LON, NULL);

    gps->speed = gconf_client_get_float(gconf_client,
                                        GCONF_KEY_LAST_SPEED, NULL);

    gps->heading = gconf_client_get_float(gconf_client,
                                          GCONF_KEY_LAST_HEADING, NULL);
}

static void update_satellite_info(LocationGPSDeviceSatellite *satellite,
                                  MapGpsData *gps, int satellite_index)
{
    if (satellite_index >= MAX_SATELLITES)
        return;

    _gps_sat[satellite_index].prn	= satellite->prn;
    _gps_sat[satellite_index].elevation	= satellite->elevation / 100;
    _gps_sat[satellite_index].azimuth	= satellite->azimuth;

    _gps_sat[satellite_index].snr	= satellite->signal_strength;
    gps->satforfix[satellite_index] = satellite->in_use ? satellite->prn : 0;
}

static void
on_gps_changed(LocationGPSDevice *device, MapController *controller)
{
    MapGpsData *gps = &controller->priv->gps_data;
    int i;
    gboolean newly_fixed = FALSE;

    /* ignore any signals arriving while gps is disabled */
    if (!_enable_gps)
        return;

    gps->fields = 0;

    if (device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET) {
        gps->lat = device->fix->latitude;
        gps->lon = device->fix->longitude;
        gps->fields |= MAP_GPS_LATLON;

        latlon2unit(gps->lat, gps->lon, gps->unit.x, gps->unit.y);
    }

    if (device->fix->fields & LOCATION_GPS_DEVICE_SPEED_SET) {
        gps->speed = device->fix->speed;
        gps->fields |= MAP_GPS_SPEED;
    }

    if (device->fix->fields & LOCATION_GPS_DEVICE_TRACK_SET) {
        gps->heading = device->fix->track;
        gps->fields |= MAP_GPS_HEADING;
        if (map_controller_get_auto_rotate(controller))
            map_controller_set_rotation(controller, gps->heading);
    }

    /* fetch timestamp from gps if available, otherwise create one. */
    if (device->fix->fields & LOCATION_GPS_DEVICE_TIME_SET) {
        gps->time = (time_t)device->fix->time;
    } else {
        gps->time = time(NULL);
    }
    _pos.time = gps->time;

    /* fetch altitude in meters from gps */
    if (device->fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET) {
        gps->altitude = (gint)device->fix->altitude;
        gps->fields |= MAP_GPS_ALTITUDE;

        _pos.altitude = gps->altitude;
    }

    /*
     * Horizontal accuracy, liblocation provides the value in
     * centimeters
     */
    gps->hdop = device->fix->eph / 100;

    /* Vertical inaccuracy, in meters */
    gps->vdop = device->fix->epv;

    _pos.unit = gps->unit;

    /* We consider a fix only if the geocoordinates are given */
    if (device->status >= LOCATION_GPS_DEVICE_STATUS_FIX)
    {
        gps->fix = device->fix->mode;

        /* Data is valid. */
        if (_gps_state < RCVR_FIXED)
        {
            newly_fixed = TRUE;
            set_conn_state(RCVR_FIXED);
        }

        /* Add the point to the track only if it's not too inaccurate: if the
         * uncertainty is greater than 200m, don't add it (TODO: this should be
         * a configuration option). */
        if (gps->hdop < 200)
            track_add(gps, newly_fixed);

        /* Move mark to new location. */
        map_controller_update_gps(controller);
    } else {
        track_insert_break(FALSE);
        gps->speed = 0;
        gps->fix = 0;
        set_conn_state(RCVR_UP);
        map_move_mark();
    }

    for(i = 0; device->satellites && i < device->satellites->len; i++)
        update_satellite_info((LocationGPSDeviceSatellite *)
                              device->satellites->pdata[i], gps, i);
    gps->satinuse = device->satellites_in_use;
    gps->satinview = device->satellites_in_view;

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
    DEBUG("%d", new_conn_state);

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
}

/**
 * Disconnect from the receiver.  This method cleans up any and everything
 * that might be associated with the receiver.
 */
void
map_controller_gps_disconnect(MapController *self)
{
    DEBUG("");

    location_gpsd_control_stop(self->priv->gpsd_control);

    if(_window)
        set_conn_state(RCVR_OFF);
}

void
map_controller_gps_connect(MapController *self)
{
    DEBUG("%d", _gps_state);

    if(_enable_gps && _gps_state == RCVR_OFF)
    {
        set_conn_state(RCVR_DOWN);

	location_gpsd_control_start(self->priv->gpsd_control);
    }
}

void
map_controller_gps_init(MapController *self, GConfClient *gconf_client)
{
    MapControllerPrivate *priv = self->priv;

    priv->gpsd_control = location_gpsd_control_get_default();
    priv->gps_device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
    g_signal_connect(priv->gps_device, "changed",
                     G_CALLBACK(on_gps_changed), self);

    load_settings(&priv->gps_data, gconf_client);

    latlon2unit(priv->gps_data.lat, priv->gps_data.lon,
                _pos.unit.x, _pos.unit.y);
}

void
map_controller_gps_dispose(MapController *self)
{
    MapControllerPrivate *priv = self->priv;

    if (priv->gps_device)
    {
        g_object_unref(priv->gps_device);
        priv->gps_device = NULL;
    }

    if (priv->gpsd_control)
    {
        g_object_unref(priv->gpsd_control);
        priv->gpsd_control = NULL;
    }
}

