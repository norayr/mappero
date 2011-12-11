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

#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libosso.h>
#include <glib.h>
#include "dbus/dbus.h"

#define DBUS_API_SUBJECT_TO_CHANGE

#include "types.h"
#include "data.h"
#include "defines.h"

#include "display.h"
#include "dbus-ifc.h"

#include <mappero/debug.h>

static DBusConnection *_bus = NULL;

/***********************
 * BELOW: DBUS METHODS *
 ***********************/

static gint
dbus_ifc_cb_default(const gchar *interface, const gchar *method,
        GArray *args, gpointer data, osso_rpc_t *retval)
{
    DEBUG("%s", method);

    /* TODO: study this code and figure out if this callback can be removed */
    retval->type = DBUS_TYPE_INVALID;

    return OSSO_OK;
}

static gboolean
dbus_ifc_set_view_position_idle(SetViewPositionArgs *args)
{
    MapPoint center;
    DEBUG("%f, %f, %d, %f", args->new_lat, args->new_lon,
          args->new_zoom, args->new_viewing_angle);

    {
        MapController *controller = map_controller_get_instance();
        map_controller_disable_auto_center(controller);

        latlon2unit(args->new_lat, args->new_lon, center.x, center.y);

        map_center_unit_full(center, args->new_zoom, args->new_viewing_angle);
    }

    g_free(args);

    return FALSE;
}

static gint
dbus_ifc_handle_set_view_center(GArray *args, osso_rpc_t *retval)
{
    DEBUG("");
    SetViewPositionArgs *svca = g_new(SetViewPositionArgs, 1);

    /* Argument 3: int: zoom.  Get this first because we might need it to
     * calculate the next latitude and/or longitude. */
    if(args->len >= 3
            && g_array_index(args, osso_rpc_t, 2).type == DBUS_TYPE_INT32)
    {
        svca->new_zoom = CLAMP(g_array_index(args, osso_rpc_t, 2).value.i,
                MIN_ZOOM, MAX_ZOOM);
    }
    else
        svca->new_zoom = _next_zoom;

    if(args->len < 2)
    {
        /* Latitude and/or Longitude are not defined.  Calculate next. */
        MapPoint new_center = map_calc_new_center(svca->new_zoom);
        unit2latlon(new_center.x, new_center.y,
                svca->new_lat, svca->new_lon);
    }

    /* Argument 1: double: latitude. */
    if(args->len >= 1
        && g_array_index(args, osso_rpc_t, 0).type == DBUS_TYPE_DOUBLE)
    {
        svca->new_lat = CLAMP(g_array_index(args, osso_rpc_t, 0).value.d,
                -90.0, 90.0);
    }

    /* Argument 2: double: longitude. */
    if(args->len >= 2
            && g_array_index(args, osso_rpc_t, 1).type == DBUS_TYPE_DOUBLE)
    {
        svca->new_lon = CLAMP(g_array_index(args, osso_rpc_t, 1).value.d,
                -180.0, 180.0);
    }

    /* Argument 4: double: viewing angle. */
    if(args->len >= 4
            && g_array_index(args, osso_rpc_t, 3).type == DBUS_TYPE_DOUBLE)
    {
        svca->new_viewing_angle
            = (gint)(g_array_index(args, osso_rpc_t, 2).value.d + 0.5) % 360;
    }
    else
        svca->new_viewing_angle = _next_map_rotate_angle;

    g_idle_add((GSourceFunc)dbus_ifc_set_view_position_idle, svca);
    retval->type = DBUS_TYPE_INVALID;

    return OSSO_OK;
}

static gint
dbus_ifc_controller(const gchar *interface, const gchar *method,
        GArray *args, gpointer data, osso_rpc_t *retval)
{
    DEBUG("%s", method);

    /* Method: set_view_center */
    if(!strcmp(method, MM_DBUS_METHOD_SET_VIEW_POSITION))
        return dbus_ifc_handle_set_view_center(args, retval);

    return OSSO_ERROR;
}

/***********************
 * ABOVE: DBUS METHODS *
 ***********************/


/***********************
 * BELOW: DBUS SIGNALS *
 ***********************/

void
dbus_ifc_fire_view_position_changed(
        MapPoint new_center, gint new_zoom, gdouble new_viewing_angle)
{
    DBusMessage *message = NULL;
    MapGeo new_lat, new_lon;
    DEBUG("%d, %d, %d, %f", new_center.x, new_center.y,
          new_zoom, new_viewing_angle);

    unit2latlon(new_center.x, new_center.y, new_lat, new_lon);

    if(NULL == (message = dbus_message_new_signal(MM_DBUS_PATH,
                    MM_DBUS_INTERFACE, MM_DBUS_SIGNAL_VIEW_POSITION_CHANGED))
            || !dbus_message_append_args(message,
                DBUS_TYPE_DOUBLE, &new_lat,
                DBUS_TYPE_DOUBLE, &new_lon,
                DBUS_TYPE_INT32, &new_zoom,
                DBUS_TYPE_DOUBLE, &new_viewing_angle,
                DBUS_TYPE_INVALID)
            || !dbus_connection_send(_bus, message, NULL))
    {
        g_printerr("Error sending view_position_changed signal.\n");
    }

    if(message)
        dbus_message_unref(message);
}

void
dbus_ifc_fire_view_dimensions_changed(
        gint new_view_width_pixels, gint new_view_height_pixels)
{
    DBusMessage *message = NULL;
    DEBUG("%d, %d", new_view_width_pixels, new_view_height_pixels);

    if(NULL == (message = dbus_message_new_signal(MM_DBUS_PATH,
                    MM_DBUS_INTERFACE, MM_DBUS_SIGNAL_VIEW_DIMENSIONS_CHANGED))
            || !dbus_message_append_args(message,
                DBUS_TYPE_INT32, &new_view_width_pixels,
                DBUS_TYPE_INT32, &new_view_height_pixels,
                DBUS_TYPE_INVALID)
            || !dbus_connection_send(_bus, message, NULL))
    {
        g_printerr("Error sending view_dimensions_changed signal.\n");
    }

    if(message)
        dbus_message_unref(message);
}

/***********************
 * ABOVE: DBUS SIGNALS *
 ***********************/

void
dbus_ifc_init()
{
    DBusError error;

    if(OSSO_OK != osso_rpc_set_default_cb_f(_osso, dbus_ifc_cb_default, NULL))
        g_printerr("osso_rpc_set_default_cb_f failed.\n");

    if(OSSO_OK != osso_rpc_set_cb_f(_osso, MM_DBUS_SERVICE,
                MM_DBUS_PATH, MM_DBUS_INTERFACE, dbus_ifc_controller, NULL))
        g_printerr("osso_rpc_set_cb_f failed.\n");

    dbus_error_init(&error);
    if(NULL == (_bus = dbus_bus_get(DBUS_BUS_SESSION, &error)))
    {
        g_printerr("Error getting session bus: %s: %s\n",
                (dbus_error_is_set(&error)
                 ? error.name : "<no error message>"),
                (dbus_error_is_set(&error)
                 ? error.message : ""));
    }
}