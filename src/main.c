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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dbus/dbus-glib.h>
#include <gst/gst.h>
#include <locale.h>

#include <gconf/gconf-client.h>

#ifdef MAEMO_CHANGES /* probably not the best macro to check for here */
#    include <device_symbols.h>
#endif

#ifdef CONIC
#    include <conicconnection.h>
#    include <conicconnectionevent.h>
#endif

#ifndef LEGACY
#    include <hildon/hildon-program.h>
#    include <hildon/hildon-banner.h>
#else
#    include <hildon-widgets/hildon-program.h>
#    include <hildon-widgets/hildon-banner.h>
#endif

#include "types.h"
#include "data.h"
#include "debug.h"
#include "defines.h"

#include "cmenu.h"
#include "controller.h"
#include "dbus-ifc.h"
#include "display.h"
#include "gpx.h"
#include "main.h"
#include "maps.h"
#include "menu.h"
#include "path.h"
#include "poi.h"
#include "screen.h"
#include "settings.h"
#include "util.h"

#include <clutter-gtk/clutter-gtk.h>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>


static void osso_cb_hw_state(osso_hw_state_t *state, gpointer data);

static HildonProgram *_program = NULL;

#ifdef CONIC
static ConIcConnection *_conic_conn = NULL;
static gboolean _conic_is_connecting = FALSE;
static gboolean _conic_conn_failed = FALSE;
static GMutex *_conic_connection_mutex = NULL;
static GCond *_conic_connection_cond = NULL;
#endif

static DBusHandlerResult
dbus_handle_mce_message(DBusConnection *conn, DBusMessage *msg, gpointer data)
{
    MapController *controller = MAP_CONTROLLER(data);
    DBusMessageIter iter;
    const gchar *display_status = NULL;

    if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, MCE_DISPLAY_SIG)) {
        if (dbus_message_iter_init(msg, &iter)) {
            dbus_message_iter_get_basic(&iter, &display_status);
            map_controller_display_status_changed(controller, display_status);
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
mce_init()
{
    MapController *controller = map_controller_get_instance();
    DBusError error;
    char *display_status;
    DBusConnection *conn;
    DBusMessage *message, *reply;

    g_return_if_fail(_osso != NULL);

    /* Get the system DBus connection */
    conn = osso_get_sys_dbus_connection(_osso);

#define MCE_MATCH_DISPLAY "type='signal',interface='" MCE_SIGNAL_IF "',member='" MCE_DISPLAY_SIG "'"
    /* Add the callback, which should be called, once the device is rotated */
    dbus_bus_add_match(conn, MCE_MATCH_DISPLAY, NULL);
    dbus_connection_add_filter(conn, dbus_handle_mce_message, controller, NULL);

    /* Get the current display state */
    message = dbus_message_new_method_call(MCE_SERVICE,
                                           MCE_REQUEST_PATH,
                                           MCE_REQUEST_IF,
                                           MCE_DISPLAY_STATUS_GET);

    dbus_error_init(&error);
    reply = dbus_connection_send_with_reply_and_block(conn, message,
                                                      -1, &error);
    dbus_message_unref(message);

    if (dbus_message_get_args(reply, NULL,
                              DBUS_TYPE_STRING, &display_status,
                              DBUS_TYPE_INVALID)) {
        map_controller_display_status_changed(controller, display_status);
    }
    dbus_message_unref(reply);
}

/**
 * maemo_mapper_init_late:
 *
 * Put inside this function all initializations that can be delayed a bit
 */
static gboolean
maemo_mapper_init_late(gpointer unused)
{
    mce_init();
    path_init_late();

    gps_show_info(); /* hides info, if necessary. */

    return FALSE;
}

#ifdef CONIC
static void
conic_conn_event(ConIcConnection *connection, ConIcConnectionEvent *event)
{
    ConIcConnectionStatus status;
    DEBUG("");

    /* we are only interested in CONNECTED and DISCONNECTED */
    status = con_ic_connection_event_get_status(event);
    if (status != CON_IC_STATUS_CONNECTED &&
        status != CON_IC_STATUS_DISCONNECTED)
        return;

    g_mutex_lock(_conic_connection_mutex);

    if((_conic_is_connected = (status == CON_IC_STATUS_CONNECTED)))
    {
        /* We're connected. */
        _conic_conn_failed = FALSE;
        if(_download_banner != NULL)
            gtk_widget_show(_download_banner);

        /* if we didn't ask for the connection ourselves, queue a map redraw */
        if (!_conic_is_connecting)
        {
            MapController *controller = map_controller_get_instance();
            MapScreen *screen;

            screen = map_controller_get_screen(controller);
            map_screen_refresh_tiles(screen);
        }
    }
    else
    {
        /* We're not connected. */
        /* Mark as a failed connection, if we had been trying to connect. */
        _conic_conn_failed = _conic_is_connecting;
        if(_download_banner != NULL)
            gtk_widget_hide(_download_banner);
    }

    _conic_is_connecting = FALSE; /* No longer trying to connect. */
    g_cond_broadcast(_conic_connection_cond);
    g_mutex_unlock(_conic_connection_mutex);
}
#endif

void
conic_recommend_connected()
{
    DEBUG("");

#if defined(__arm__) && defined(CONIC)
    g_mutex_lock(_conic_connection_mutex);
    if(!_conic_is_connected && !_conic_is_connecting)
    {
        /* Fire up a connection request. */
        con_ic_connection_connect(_conic_conn, CON_IC_CONNECT_FLAG_NONE);
        _conic_is_connecting = TRUE;
    }
    g_mutex_unlock(_conic_connection_mutex);
#endif
}

gboolean
conic_ensure_connected()
{
    DEBUG("");

#if defined(__arm__) && defined(CONIC)
    if (_window && !_conic_is_connected)
    {   
        g_mutex_lock(_conic_connection_mutex);
        /* If we're not connected, and if we're not connecting, and if we're
         * not in the wake of a connection failure, then try to connect. */
        if(!_conic_is_connected && !_conic_is_connecting &&!_conic_conn_failed)
        {
            /* Fire up a connection request. */
            con_ic_connection_connect(_conic_conn, CON_IC_CONNECT_FLAG_NONE);
            _conic_is_connecting = TRUE;
        }
        g_cond_wait(_conic_connection_cond, _conic_connection_mutex);
        g_mutex_unlock(_conic_connection_mutex);
    }
#else
    _conic_is_connected = TRUE;
#endif

    return _window && _conic_is_connected;
}

/**
 * Save state and destroy all non-UI elements created by this program in
 * preparation for exiting.
 */
static void
maemo_mapper_destroy()
{
    /* _program and widgets have already been destroyed. */
    _window = NULL;

    path_destroy();

    settings_save();

    poi_destroy();

    g_mutex_lock(_mut_priority_mutex);
    _mut_priority_tree = g_tree_new((GCompareFunc)mut_priority_comparefunc);
    g_mutex_unlock(_mut_priority_mutex);

    /* Allow remaining downloads to finish. */
#ifdef CONIC
    g_mutex_lock(_conic_connection_mutex);
    g_cond_broadcast(_conic_connection_cond);
    g_mutex_unlock(_conic_connection_mutex);
#endif
    g_thread_pool_free(_mut_thread_pool, TRUE, TRUE);
}

/**
 * Initialize everything required in preparation for calling gtk_main().
 */
static void
maemo_mapper_init(gint argc, gchar **argv)
{
    GtkWidget *hbox;

    /* Set enum-based constants. */
    UNITS_ENUM_TEXT[UNITS_KM] = _("km");
    UNITS_ENUM_TEXT[UNITS_MI] = _("mi.");
    UNITS_ENUM_TEXT[UNITS_NM] = _("n.m.");

    UNITS_SMALL_TEXT[UNITS_KM] = _("m");
    UNITS_SMALL_TEXT[UNITS_MI] = _("y.");
    UNITS_SMALL_TEXT[UNITS_NM] = _("n.m.");

    UNBLANK_ENUM_TEXT[UNBLANK_WITH_GPS] = _("When Receiving Any GPS Data");
    UNBLANK_ENUM_TEXT[UNBLANK_WHEN_MOVING] = _("When Moving");
    UNBLANK_ENUM_TEXT[UNBLANK_FULLSCREEN] = _("When Moving (Full Screen Only)");
    UNBLANK_ENUM_TEXT[UNBLANK_WAYPOINT] = _("When Approaching a Waypoint");
    UNBLANK_ENUM_TEXT[UNBLANK_NEVER] = _("Never");

    INFO_FONT_ENUM_TEXT[INFO_FONT_XXSMALL] = "xx-small";
    INFO_FONT_ENUM_TEXT[INFO_FONT_XSMALL] = "x-small";
    INFO_FONT_ENUM_TEXT[INFO_FONT_SMALL] = "small";
    INFO_FONT_ENUM_TEXT[INFO_FONT_MEDIUM] = "medium";
    INFO_FONT_ENUM_TEXT[INFO_FONT_LARGE] = "large";
    INFO_FONT_ENUM_TEXT[INFO_FONT_XLARGE] = "x-large";
    INFO_FONT_ENUM_TEXT[INFO_FONT_XXLARGE] = "xx-large";

    SPEED_LOCATION_ENUM_TEXT[SPEED_LOCATION_TOP_LEFT] = _("Top-Left");
    SPEED_LOCATION_ENUM_TEXT[SPEED_LOCATION_TOP_RIGHT] = _("Top-Right");
    SPEED_LOCATION_ENUM_TEXT[SPEED_LOCATION_BOTTOM_RIGHT] = _("Bottom-Right");
    SPEED_LOCATION_ENUM_TEXT[SPEED_LOCATION_BOTTOM_LEFT] = _("Bottom-Left");

    /* Set up track array (must be done before config). */
    memset(&_track, 0, sizeof(_track));
    /* initialisation of paths is done in path_init() */

    _mut_priority_mutex = g_mutex_new();

#ifdef CONIC
    _conic_connection_mutex = g_mutex_new();
    _conic_connection_cond = g_cond_new();
#endif

    /* Initialize _program. */
    _program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("Mappero");

    /* Initialize _window. */
    _window = GTK_WIDGET(hildon_window_new());
    hildon_program_add_window(_program, HILDON_WINDOW(_window));

    _controller = g_object_new(MAP_TYPE_CONTROLLER, NULL);

    /* Create and add widgets and supporting data. */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(_window), hbox);

    _w_map = (GtkWidget *)map_controller_get_screen(_controller);
    gtk_box_pack_start(GTK_BOX(hbox), _w_map, TRUE, TRUE, 0);

    gtk_widget_show_all(hbox);

    _mut_exists_table = g_hash_table_new(
            mut_exists_hashfunc, mut_exists_equalfunc);
    _mut_priority_tree = g_tree_new(mut_priority_comparefunc);

    _mut_thread_pool = g_thread_pool_new(
            (GFunc)thread_proc_mut, NULL, NUM_DOWNLOAD_THREADS, FALSE, NULL);

    /* Connect signals. */
    g_signal_connect(G_OBJECT(_window), "destroy",
            G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show(_window);
    menu_init();
    path_init();
    poi_db_connect();
    display_init();
    dbus_ifc_init();
    
#ifdef CONIC
    _conic_conn = con_ic_connection_new();
    g_object_set(_conic_conn, "automatic-connection-events", TRUE, NULL);
    g_signal_connect(G_OBJECT(_conic_conn), "connection-event",
            G_CALLBACK(conic_conn_event), NULL);
#endif

    osso_hw_set_event_cb(_osso, NULL, osso_cb_hw_state, NULL);

    /* Lets go fullscreen if so requested in saved config */
    if (_fullscreen) {
      gtk_window_fullscreen(GTK_WINDOW(_window));
    }
}

static gboolean
osso_cb_hw_state_idle(osso_hw_state_t *state)
{
    MapController *controller = map_controller_get_instance();

    DEBUG("inact=%d, save=%d, shut=%d, memlow=%d, state=%d",
          state->system_inactivity_ind,
          state->save_unsaved_data_ind, state->shutdown_ind,
          state->memory_low_ind, state->sig_device_mode_ind);

    if(state->shutdown_ind)
    {
        maemo_mapper_destroy();
        exit(1);
    }

    if(state->save_unsaved_data_ind)
        settings_save();

    if (map_controller_get_device_active(controller) != !state->system_inactivity_ind)
        map_controller_set_device_active(controller, !state->system_inactivity_ind);

    g_free(state);

    return FALSE;
}

static void
osso_cb_hw_state(osso_hw_state_t *state, gpointer data)
{
    DEBUG("");
    osso_hw_state_t *state_copy = g_new(osso_hw_state_t, 1);
    memcpy(state_copy, state, sizeof(osso_hw_state_t));
    g_idle_add((GSourceFunc)osso_cb_hw_state_idle, state_copy);
}

gint
main(gint argc, gchar *argv[])
{
    TIME_START();

    /* Initialize localization. */
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    g_thread_init(NULL);

    map_debug_init();

    /* Initialize _osso. */
    _osso = osso_initialize("com.gnuite.maemo_mapper", VERSION, TRUE, NULL);
    if(!_osso)
    {
        g_printerr("osso_initialize failed.\n");
        return 1;
    }

    map_set_fast_mode(TRUE);
    gtk_clutter_init(&argc, &argv);
    gst_init(&argc, &argv);

    /* Init gconf. */
    g_type_init();
    gconf_init(argc, argv, NULL);

    /* Init Gnome-VFS. */
    gnome_vfs_init();

#ifdef ENABLE_DEBUG
    /* This is just some helpful DBUS testing code. */
    if(argc >= 3)
    {
        /* Try to set the center to a new lat/lon. */
        GError *error = NULL;
        gchar *error_check;
        gdouble lat, lon;
        DBusGConnection *bus;
        DBusGProxy *proxy;
        
        bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
        if(!bus || error)
        {
            g_printerr("Error: %s\n", error->message);
            return -1;
        }

        proxy = dbus_g_proxy_new_for_name(bus,
                        MM_DBUS_SERVICE, MM_DBUS_PATH, MM_DBUS_INTERFACE);

        lat = g_ascii_strtod((argv[1]), &error_check);
        if(error_check == argv[1])
        {
            g_printerr("Failed to parse string as float: %s\n", argv[1]);
            return 1;
        }

        lon = g_ascii_strtod((argv[2]), &error_check);
        if(error_check == argv[2])
        {
            g_printerr("Failed to parse string as float: %s\n", argv[2]);
            return 2;
        }

        error = NULL;
        if(argc >= 4)
        {
            /* We are specifying a zoom. */
            gint zoom;

            zoom = g_ascii_strtod((argv[3]), &error_check);
            if(error_check == argv[3])
            {
                g_printerr("Failed to parse string as integer: %s\n", argv[3]);
                return 3;
            }
            if(!dbus_g_proxy_call(proxy, MM_DBUS_METHOD_SET_VIEW_POSITION,
                        &error,
                        G_TYPE_DOUBLE, lat, G_TYPE_DOUBLE, lon,
                        G_TYPE_INT, zoom, G_TYPE_INVALID,
                        G_TYPE_INVALID)
                    || error)
            {
                g_printerr("Error: %s\n", error->message);
                return 4;
            }
        }
        else
        {
            /* Not specifying a zoom. */
            if(!dbus_g_proxy_call(proxy, MM_DBUS_METHOD_SET_VIEW_POSITION,
                        &error,
                        G_TYPE_DOUBLE, lat, G_TYPE_DOUBLE, lon, G_TYPE_INVALID,
                        G_TYPE_INVALID)
                    || error)
            {
                g_printerr("Error: %s\n", error->message);
                return -2;
            }
        }

        g_object_unref(proxy);
        dbus_g_connection_unref(bus);

        return 0;
    }
#endif

    g_idle_add_full(G_PRIORITY_LOW, maemo_mapper_init_late, NULL, NULL);

    maemo_mapper_init(argc, argv);

    TIME_STOP();

    gtk_main();

    maemo_mapper_destroy();

    osso_deinitialize(_osso);

    exit(0);
}

