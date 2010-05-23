/*
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

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif
#include "controller.h"

#include "controller-priv.h"
#include "data.h"

#include <hildon/hildon-gtk.h>
#include <hildon/hildon-gtk.h>
#include <libosso.h>
#include <mappero/debug.h>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>
#include <string.h>

#define MCE_MATCH_RULE "type='signal',interface='" MCE_SIGNAL_IF "',member='" MCE_DEVICE_ORIENTATION_SIG "'"

static void
set_orientation(const gchar *mode)
{
    HildonPortraitFlags flags = HILDON_PORTRAIT_MODE_SUPPORT;

    DEBUG("Setting orientation to %s", mode);
    if (strcmp(mode, "portrait") == 0)
        flags += HILDON_PORTRAIT_MODE_REQUEST;
    hildon_gtk_window_set_portrait_flags(GTK_WINDOW(_window), flags);
}

static DBusHandlerResult
dbus_handle_mce_message(DBusConnection *conn, DBusMessage *msg, gpointer data)
{
    DBusMessageIter iter;
    const gchar *mode = NULL;

    if (dbus_message_is_signal(msg, MCE_SIGNAL_IF,
                               MCE_DEVICE_ORIENTATION_SIG)) {
        if (dbus_message_iter_init(msg, &iter)) {
            dbus_message_iter_get_basic(&iter, &mode);
            set_orientation(mode);
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
accelerometer_start()
{
    DBusError error;
    char *mode;
    DBusMessage *message, *reply;

    /* Get the system DBus connection */
    DBusConnection *conn = osso_get_sys_dbus_connection(_osso);

    /* Add the callback, which should be called, once the device is rotated */
    dbus_bus_add_match(conn, MCE_MATCH_RULE, NULL);
    dbus_connection_add_filter(conn, dbus_handle_mce_message, NULL, NULL);

    /* Get the current orientation */
    message = dbus_message_new_method_call(MCE_SERVICE,
                                           MCE_REQUEST_PATH,
                                           MCE_REQUEST_IF,
                                           MCE_ACCELEROMETER_ENABLE_REQ);

    dbus_error_init(&error);
    reply = dbus_connection_send_with_reply_and_block(conn, message,
                                                      -1, &error);
    dbus_message_unref(message); 

    if (dbus_message_get_args(reply, NULL,
                              DBUS_TYPE_STRING, &mode,
                              DBUS_TYPE_INVALID)) {
        set_orientation(mode);
    }
    dbus_message_unref(reply); 
}

static void
accelerometer_stop()
{
    DBusConnection *conn = osso_get_sys_dbus_connection(_osso);
    DBusMessage *message;

    dbus_bus_remove_match(conn, MCE_MATCH_RULE, NULL);
    dbus_connection_remove_filter(conn, dbus_handle_mce_message, NULL);

    message = dbus_message_new_method_call(MCE_SERVICE,
                                           MCE_REQUEST_PATH,
                                           MCE_REQUEST_IF,
                                           MCE_ACCELEROMETER_DISABLE_REQ);
    dbus_message_set_no_reply(message, TRUE);

    dbus_connection_send(conn, message, NULL);
    dbus_message_unref(message); 
}

void
map_controller_set_orientation(MapController *self, MapOrientation orientation)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (orientation == self->priv->orientation) return;

    if (orientation == MAP_ORIENTATION_AUTO)
        accelerometer_start();
    else {
        accelerometer_stop();
        
        set_orientation(orientation == MAP_ORIENTATION_LANDSCAPE ?
                        "landscape" : "portrait");
    }

    self->priv->orientation = orientation;
}

MapOrientation
map_controller_get_orientation(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), 0);
    return self->priv->orientation;
}

