/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Max Lapan
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
#include <dbus/dbus-glib.h>
#include <gconf/gconf-client.h>
#include <glib.h>

#include <hildon/hildon-note.h>
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-number-editor.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-check-button.h>

#include "types.h"
#include "data.h"
#include "defines.h"
#include "dialog.h"

#include "gps.h"
#include "display.h"
#include "maps.h"
#include "marshal.h"
#include "poi.h"
#include "settings.h"
#include "util.h"

typedef struct _KeysDialogInfo KeysDialogInfo;
struct _KeysDialogInfo {
    GtkWidget *cmb[CUSTOM_KEY_ENUM_COUNT];
};

typedef struct _ColorsDialogInfo ColorsDialogInfo;
struct _ColorsDialogInfo {
    GtkWidget *col[COLORABLE_ENUM_COUNT];
};


/**
 * Save all configuration data to GCONF.
 */
void
settings_save()
{
    gchar *settings_dir;
    GConfClient *gconf_client;
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gchar buffer[16];

    /* Initialize settings_dir. */
    settings_dir = gnome_vfs_expand_initial_tilde(CONFIG_DIR_NAME);
    g_mkdir_with_parents(settings_dir, 0700);

    /* SAVE ALL GCONF SETTINGS. */

    gconf_client = gconf_client_get_default();
    if(!gconf_client)
    {
        popup_error(_window,
                _("Failed to initialize GConf.  Settings were not saved."));
        return;
    }

    /* Save GPS Receiver Type. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_GPS_RCVR_TYPE,
            GPS_RCVR_ENUM_TEXT[_gri.type], NULL);

    /* Save Bluetooth Receiver MAC. */
    if(_gri.bt_mac)
        gconf_client_set_string(gconf_client,
                GCONF_KEY_GPS_BT_MAC, _gri.bt_mac, NULL);
    else
        gconf_client_unset(gconf_client,
                GCONF_KEY_GPS_BT_MAC, NULL);

    /* Save GPSD Host. */
    if(_gri.gpsd_host)
        gconf_client_set_string(gconf_client,
                GCONF_KEY_GPS_GPSD_HOST, _gri.gpsd_host, NULL);
    else
        gconf_client_unset(gconf_client,
                GCONF_KEY_GPS_GPSD_HOST, NULL);

    /* Save GPSD Port. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_GPS_GPSD_PORT, _gri.gpsd_port, NULL);

    /* Save File Path. */
    if(_gri.file_path)
        gconf_client_set_string(gconf_client,
                GCONF_KEY_GPS_FILE_PATH, _gri.file_path, NULL);
    else
        gconf_client_unset(gconf_client,
                GCONF_KEY_GPS_FILE_PATH, NULL);

    /* GPS polling interval */
    gconf_client_set_int(gconf_client, GCONF_KEY_GPS_INTERVAL,
                         map_controller_gps_get_interval(controller), NULL);

    /* GPS power save */
    gconf_client_set_bool(gconf_client, GCONF_KEY_GPS_POWER_SAVE,
                          map_controller_gps_get_power_save(controller), NULL);

    /* Save Auto-Download. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_AUTO_DOWNLOAD,
            map_controller_get_auto_download(controller), NULL);

    /* Save Auto-Download Pre-cache. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_AUTO_DOWNLOAD_PRECACHE, _auto_download_precache, NULL);

    /* Save Auto-Center Sensitivity. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_CENTER_SENSITIVITY, _center_ratio, NULL);

    /* Save Auto-Center Lead Amount. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_LEAD_AMOUNT, _lead_ratio, NULL);

    /* Save Auto-Center Lead Fixed flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_LEAD_IS_FIXED, _lead_is_fixed, NULL);

    /* Save Auto-Center/Rotate Minimum Speed. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_AC_MIN_SPEED, _ac_min_speed, NULL);

    /* Save Draw Line Width. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_DRAW_WIDTH, _draw_width, NULL);

    /* Save Announce Flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_ENABLE_ANNOUNCE, _enable_announce, NULL);

    /* Save Announce Advance Notice Ratio. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_ANNOUNCE_NOTICE, _announce_notice_ratio, NULL);

    /* Save Enable Voice flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_ENABLE_VOICE, _enable_voice, NULL);

    /* Save fullscreen flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_FULLSCREEN, _fullscreen, NULL);

    /* Save Units. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_UNITS, UNITS_ENUM_TEXT[_units], NULL);

    /* Save Custom Key Actions. */
    {
        gint i;
        for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
            gconf_client_set_string(gconf_client,
                    CUSTOM_KEY_GCONF[i],
                    CUSTOM_ACTION_ENUM_TEXT[_action[i]], NULL);
    }

    /* Save Deg Format. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_DEG_FORMAT, DEG_FORMAT_ENUM_TEXT[_degformat].name , NULL);

    /* Save Speed Limit On flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SPEED_LIMIT_ON, _speed_limit_on, NULL);

    /* Save Speed Limit. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_SPEED_LIMIT, _speed_limit, NULL);

    /* Save Speed Location. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_SPEED_LOCATION,
            SPEED_LOCATION_ENUM_TEXT[_speed_location], NULL);

    /* Save Info Font Size. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_INFO_FONT_SIZE,
            INFO_FONT_ENUM_TEXT[_info_font_size], NULL);

    /* Save Unblank Option. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_UNBLANK_SIZE,
            UNBLANK_ENUM_TEXT[_unblank_option], NULL);

    /* Save last saved latitude. */
    gconf_client_set_float(gconf_client,
            GCONF_KEY_LAST_LAT, gps->lat, NULL);

    /* Save last saved longitude. */
    gconf_client_set_float(gconf_client,
            GCONF_KEY_LAST_LON, gps->lon, NULL);

    /* Save last saved altitude. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_LAST_ALT, _pos.altitude, NULL);

    /* Save last saved speed. */
    gconf_client_set_float(gconf_client,
            GCONF_KEY_LAST_SPEED, gps->speed, NULL);

    /* Save last saved heading. */
    gconf_client_set_float(gconf_client,
            GCONF_KEY_LAST_HEADING, gps->heading, NULL);

    /* Save last saved timestamp. */
    gconf_client_set_float(gconf_client,
            GCONF_KEY_LAST_TIME, _pos.time, NULL);

    /* Save last center point. */
    {
        MapGeo center_lat, center_lon;
        gint zoom;

        map_controller_get_center(controller, &_center);
        unit2latlon(_center.x, _center.y, center_lat, center_lon);

        /* Save last center latitude. */
        gconf_client_set_float(gconf_client,
                GCONF_KEY_CENTER_LAT, center_lat, NULL);

        /* Save last center longitude. */
        gconf_client_set_float(gconf_client,
                GCONF_KEY_CENTER_LON, center_lon, NULL);

        /* Save last view angle. */
        gconf_client_set_int(gconf_client,
                GCONF_KEY_CENTER_ANGLE, _map_rotate_angle, NULL);

        /* Save last Zoom Level. */
        zoom = map_controller_get_zoom(controller);
        gconf_client_set_int(gconf_client, GCONF_KEY_ZOOM, zoom, NULL);
    }

    /* Save Route Directory. */
    if(_route_dir_uri)
        gconf_client_set_string(gconf_client,
                GCONF_KEY_ROUTEDIR, _route_dir_uri, NULL);

    /* Repositories */
    map_controller_save_repositories(controller, gconf_client);

    /* Save Last Track File. */
    if(_track_file_uri)
        gconf_client_set_string(gconf_client,
                GCONF_KEY_TRACKFILE, _track_file_uri, NULL);

    /* Save Auto-Center Mode. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_AUTOCENTER_MODE, _center_mode, NULL);

    /* Save Auto-Center Rotate Flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_AUTOCENTER_ROTATE, _center_rotate, NULL);

    /* Save Show Zoom Level flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWZOOMLEVEL, _show_zoomlevel, NULL);

    /* Save Show Scale flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWSCALE, _show_scale, NULL);

    /* Save Show Compass Rose flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWCOMPROSE, _show_comprose, NULL);

    /* Save Show Tracks flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWTRACKS, _show_paths & TRACKS_MASK, NULL);

    /* Save Show Routes flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWROUTES, _show_paths & ROUTES_MASK, NULL);

    /* Save Show Velocity Vector flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWVELVEC, _show_velvec, NULL);

    /* Save Show POIs flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_SHOWPOIS, _show_poi, NULL);

    /* Save Enable GPS flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_ENABLE_GPS, _enable_gps, NULL);

    /* Save Enable Tracking flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_ENABLE_TRACKING, _enable_tracking, NULL);

    /* Save Route Locations. */
    gconf_client_set_list(gconf_client,
            GCONF_KEY_ROUTE_LOCATIONS, GCONF_VALUE_STRING, _loc_list, NULL);

    /* Save GPS Info flag. */
    gconf_client_set_bool(gconf_client,
            GCONF_KEY_GPS_INFO, _gps_info, NULL);

    /* Save Router engine name */
    {
        MapRouter *router;
        router = map_controller_get_default_router(controller);
        gconf_client_set_string(gconf_client,
                                GCONF_KEY_ROUTER_NAME,
                                map_router_get_name(router), NULL);
    }

    /* Save routers configurations */
    map_controller_save_routers_options(controller, gconf_client);

    /* Save Route Download Radius. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_ROUTE_DL_RADIUS, _route_dl_radius, NULL);

    /* Save POI Download URL Format. */
    gconf_client_set_string(gconf_client,
            GCONF_KEY_POI_DL_URL, _poi_dl_url, NULL);

    /* Save Colors. */
    {
        gint i;
        for(i = 0; i < COLORABLE_ENUM_COUNT; i++)
        {
            snprintf(buffer, sizeof(buffer), "#%02x%02x%02x",
                    _color[i].red >> 8,
                    _color[i].green >> 8,
                    _color[i].blue >> 8);
            gconf_client_set_string(gconf_client,
                    COLORABLE_GCONF[i], buffer, NULL);
        }
    }

    /* Save POI database. */
    if(_poi_db_filename)
        gconf_client_set_string(gconf_client,
                GCONF_KEY_POI_DB, _poi_db_filename, NULL);
    else
        gconf_client_unset(gconf_client, GCONF_KEY_POI_DB, NULL);

    /* Save Show POI below zoom. */
    gconf_client_set_int(gconf_client,
            GCONF_KEY_POI_ZOOM, _poi_zoom, NULL);

    /* orientation */
    gconf_client_set_int(gconf_client, GCONF_KEY_ORIENTATION,
                         map_controller_get_orientation(controller), NULL);

    gconf_client_clear_cache(gconf_client);
    g_object_unref(gconf_client);
    g_free(settings_dir);
}

static gboolean
settings_dialog_browse_forfile(GtkWidget *button)
{
    GtkWidget *dialog, *parent;

    parent = gtk_widget_get_toplevel(button);
    dialog = GTK_WIDGET(
            hildon_file_chooser_dialog_new(GTK_WINDOW(parent),
            GTK_FILE_CHOOSER_ACTION_OPEN));

    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
            hildon_button_get_value(HILDON_BUTTON(button)));

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        gchar *filename = gtk_file_chooser_get_filename(
                GTK_FILE_CHOOSER(dialog));
        hildon_button_set_value(HILDON_BUTTON(button), filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);

    return TRUE;
}

static gboolean
settings_dialog_hardkeys_reset(GtkWidget *widget, KeysDialogInfo *cdi)
{
    GtkWidget *confirm;

    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window),
            _("Reset all hardware keys to their original defaults?"));

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm)))
    {
        gint i;
        for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
            gtk_combo_box_set_active(GTK_COMBO_BOX(cdi->cmb[i]),
                    CUSTOM_KEY_DEFAULT[i]);
    }
    gtk_widget_destroy(confirm);

    return TRUE;
}

static gboolean
settings_dialog_hardkeys(GtkWindow *parent)
{
    gint i;
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static KeysDialogInfo bdi;
    static GtkWidget *btn_defaults = NULL;

    if(dialog == NULL)
    {
        dialog = gtk_dialog_new_with_buttons(_("Hardware Keys"),
                parent, GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                NULL);

        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
                btn_defaults = gtk_button_new_with_label(_("Reset...")));
        g_signal_connect(G_OBJECT(btn_defaults), "clicked",
                          G_CALLBACK(settings_dialog_hardkeys_reset), &bdi);

        gtk_dialog_add_button(GTK_DIALOG(dialog),
                GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                table = gtk_table_new(2, 9, FALSE), TRUE, TRUE, 0);
        for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
        {
            gint j;
            gtk_table_attach(GTK_TABLE(table),
                    label = gtk_label_new(""),
                    0, 1, i, i + 1, GTK_FILL, 0, 2, 1);
            gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
            gtk_label_set_markup(GTK_LABEL(label), CUSTOM_KEY_ICON[i]);
            gtk_table_attach(GTK_TABLE(table),
                    bdi.cmb[i] = gtk_combo_box_new_text(),
                    1, 2, i, i + 1, GTK_FILL, 0, 2, 1);
            for(j = 0; j < CUSTOM_ACTION_ENUM_COUNT; j++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(bdi.cmb[i]),
                        CUSTOM_ACTION_ENUM_TEXT[j]);
        }
    }

    /* Initialize contents of the combo boxes. */
    for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
        gtk_combo_box_set_active(GTK_COMBO_BOX(bdi.cmb[i]), _action[i]);

    gtk_widget_show_all(dialog);

OUTER_WHILE:
    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        /* Check for duplicates. */
        for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
        {
            gint j;
            for(j = i + 1; j < CUSTOM_KEY_ENUM_COUNT; j++)
            {
                if(gtk_combo_box_get_active(GTK_COMBO_BOX(bdi.cmb[i]))
                        == gtk_combo_box_get_active(GTK_COMBO_BOX(bdi.cmb[j])))
                {
                    GtkWidget *confirm;
                    gchar *buffer = g_strdup_printf("%s:\n    %s\n%s",
                        _("The following action is mapped to multiple keys"),
                        CUSTOM_ACTION_ENUM_TEXT[gtk_combo_box_get_active(
                            GTK_COMBO_BOX(bdi.cmb[i]))],
                        _("Continue?"));
                    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window),
                            buffer);

                    if(GTK_RESPONSE_OK != gtk_dialog_run(GTK_DIALOG(confirm)))
                    {
                        gtk_widget_destroy(confirm);
                        goto OUTER_WHILE;
                    }
                    gtk_widget_destroy(confirm);
                }
            }
        }
        for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
            _action[i] = gtk_combo_box_get_active(GTK_COMBO_BOX(bdi.cmb[i]));
        break;
    }

    gtk_widget_hide(dialog);

    return TRUE;
}

static gboolean
settings_dialog_colors_reset(GtkWidget *widget, ColorsDialogInfo *cdi)
{
    GtkWidget *confirm;

    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window),
            _("Reset all colors to their original defaults?"));

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm)))
    {
        gint i;
        for(i = 0; i < COLORABLE_ENUM_COUNT; i++)
        {
            hildon_color_button_set_color(
                    HILDON_COLOR_BUTTON(cdi->col[i]),
                    &COLORABLE_DEFAULT[i]);
        }
    }
    gtk_widget_destroy(confirm);

    return TRUE;
}

static gboolean
settings_dialog_colors(GtkWindow *parent)
{
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *btn_defaults = NULL;
    static ColorsDialogInfo cdi;

    if(dialog == NULL)
    {
        dialog = gtk_dialog_new_with_buttons(_("Colors"),
                parent, GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                NULL);

        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
                btn_defaults = gtk_button_new_with_label(_("Reset...")));
        g_signal_connect(G_OBJECT(btn_defaults), "clicked",
                          G_CALLBACK(settings_dialog_colors_reset), &cdi);

        gtk_dialog_add_button(GTK_DIALOG(dialog),
                GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                table = gtk_table_new(4, 3, FALSE), TRUE, TRUE, 0);

        /* GPS. */
        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("GPS")),
                0, 1, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_MARK] = hildon_color_button_new(),
                1, 2, 0, 1, 0, 0, 2, 4);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_MARK_VELOCITY] = hildon_color_button_new(),
                2, 3, 0, 1, 0, 0, 2, 4);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_MARK_OLD] = hildon_color_button_new(),
                3, 4, 0, 1, 0, 0, 2, 4);

        /* Track. */
        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Track")),
                0, 1, 1, 2, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_TRACK] = hildon_color_button_new(),
                1, 2, 1, 2, 0, 0, 2, 4);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_TRACK_MARK] = hildon_color_button_new(),
                2, 3, 1, 2, 0, 0, 2, 4);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_TRACK_BREAK] = hildon_color_button_new(),
                3, 4, 1, 2, 0, 0, 2, 4);

        /* Route. */
        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Route")),
                0, 1, 2, 3, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_ROUTE] = hildon_color_button_new(),
                1, 2, 2, 3, 0, 0, 2, 4);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_ROUTE_WAY] = hildon_color_button_new(),
                2, 3, 2, 3, 0, 0, 2, 4);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_ROUTE_BREAK] = hildon_color_button_new(),
                3, 4, 2, 3, 0, 0, 2, 4);

        /* POI. */
        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("POI")),
                0, 1, 3, 4, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                cdi.col[COLORABLE_POI] = hildon_color_button_new(),
                1, 2, 3, 4, 0, 0, 2, 4);
    }

    /* Initialize GPS. */
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK]),
            &_color[COLORABLE_MARK]);
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK_VELOCITY]),
            &_color[COLORABLE_MARK_VELOCITY]);
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK_OLD]),
            &_color[COLORABLE_MARK_OLD]);

    /* Initialize Track. */
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK]),
            &_color[COLORABLE_TRACK]);
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK_MARK]),
            &_color[COLORABLE_TRACK_MARK]);
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK_BREAK]),
            &_color[COLORABLE_TRACK_BREAK]);

    /* Initialize Route. */
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE]),
            &_color[COLORABLE_ROUTE]);
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE_WAY]),
            &_color[COLORABLE_ROUTE_WAY]);
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE_BREAK]),
            &_color[COLORABLE_ROUTE_BREAK]);

    /* Initialize POI. */
    hildon_color_button_set_color(
            HILDON_COLOR_BUTTON(cdi.col[COLORABLE_POI]),
            &_color[COLORABLE_POI]);

    gtk_widget_show_all(dialog);

    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
#ifndef LEGACY
        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK]), 
                &_color[COLORABLE_MARK]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK_VELOCITY]), 
                &_color[COLORABLE_MARK_VELOCITY]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK_OLD]),
                &_color[COLORABLE_MARK_OLD]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK]),
                &_color[COLORABLE_TRACK]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK_MARK]),
                &_color[COLORABLE_TRACK_MARK]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK_BREAK]),
                &_color[COLORABLE_TRACK_BREAK]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE]),
                &_color[COLORABLE_ROUTE]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE_WAY]),
                &_color[COLORABLE_ROUTE_WAY]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE_BREAK]),
                &_color[COLORABLE_ROUTE_BREAK]);

        hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_POI]),
                &_color[COLORABLE_POI]);
#else
        GdkColor *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK]));
        _color[COLORABLE_MARK] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK_VELOCITY]));
        _color[COLORABLE_MARK_VELOCITY] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_MARK_OLD]));
        _color[COLORABLE_MARK_OLD] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK]));
        _color[COLORABLE_TRACK] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK_MARK]));
        _color[COLORABLE_TRACK_MARK] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_TRACK_BREAK]));
        _color[COLORABLE_TRACK_BREAK] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE]));
        _color[COLORABLE_ROUTE] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE_WAY]));
        _color[COLORABLE_ROUTE_WAY] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_ROUTE_BREAK]));
        _color[COLORABLE_ROUTE_BREAK] = *color;

        color = hildon_color_button_get_color(
                HILDON_COLOR_BUTTON(cdi.col[COLORABLE_POI]));
        _color[COLORABLE_POI] = *color;

#endif
        
        break;
    }

    map_force_redraw();

    gtk_widget_hide(dialog);

    return TRUE;
}

static inline void
settings_dialog_set_save(gpointer dialog, gboolean save)
{
    g_object_set_data(G_OBJECT(dialog), "save_settings",
                      GINT_TO_POINTER(save));
}

static inline gboolean
settings_dialog_get_save(gpointer dialog)
{
    return g_object_get_data(G_OBJECT(dialog), "save_settings") != NULL;
}

static void
run_auto_center_dialog(GtkWindow *parent)
{
    GtkWidget *dialog;
    HildonTouchSelector *selector;
    GtkWidget *table, *label;
    GtkAdjustment *adjustment;
    GtkWidget *lead_is_fixed;
    GtkWidget *num_center_ratio;
    GtkWidget *num_lead_ratio;
    GtkWidget *num_ac_min_speed;
    gint i;

    /* Auto-Center page. */
    dialog = gtk_dialog_new_with_buttons
        (_("Auto-Center"), parent, GTK_DIALOG_MODAL,
         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
         NULL);

    table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table,
                       TRUE, TRUE, 0);

    /* Lead Amount. */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(selector, _("Speed based"));
    hildon_touch_selector_append_text(selector, _("Fixed"));
    lead_is_fixed =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Lead Amount"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);

    gtk_table_attach(GTK_TABLE(table),
                     lead_is_fixed,
                     0, 1, 0, 1, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    gtk_table_attach(GTK_TABLE(table),
                     num_lead_ratio = hildon_gtk_hscale_new(),
                     1, 2, 0, 1, GTK_FILL, 0, 2, 4);
    adjustment = gtk_range_get_adjustment(GTK_RANGE(num_lead_ratio));
    g_object_set(adjustment,
                 "step-increment", 1.0,
                 "lower", 1.0,
                 "upper", 10.0,
                 NULL);

    /* Auto-Center Pan Sensitivity. */
    gtk_table_attach(GTK_TABLE(table),
                     label = gtk_label_new(_("Pan Sensitivity")),
                     0, 1, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5f);
    gtk_table_attach(GTK_TABLE(table),
                     num_center_ratio = hildon_gtk_hscale_new(),
                     1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    adjustment = gtk_range_get_adjustment(GTK_RANGE(num_center_ratio));
    g_object_set(adjustment,
                 "step-increment", 1.0,
                 "lower", 1.0,
                 "upper", 10.0,
                 NULL);

    /* Minimum speed */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < 20; i++)
    {
        gchar buffer[5];
        sprintf(buffer, "%d", i * 5);
        hildon_touch_selector_append_text(selector, buffer);
    }
    num_ac_min_speed =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Min. Speed"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_table_attach(GTK_TABLE(table), num_ac_min_speed,
                     0, 2, 2, 3, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Initialize widgets */
    gtk_range_set_value(GTK_RANGE(num_center_ratio), _center_ratio);
    gtk_range_set_value(GTK_RANGE(num_lead_ratio), _lead_ratio);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(lead_is_fixed), _lead_is_fixed);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(num_ac_min_speed), _ac_min_speed / 5);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        _center_ratio = gtk_range_get_value(GTK_RANGE(num_center_ratio));

        _lead_ratio = gtk_range_get_value(GTK_RANGE(num_lead_ratio));

        _lead_is_fixed = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(lead_is_fixed));

        _ac_min_speed = hildon_picker_button_get_active(
                HILDON_PICKER_BUTTON(num_ac_min_speed)) * 5;

        settings_dialog_set_save(parent, TRUE);
    }

    gtk_widget_destroy(dialog);
}

static void
run_announce_dialog(GtkWindow *parent)
{
    GtkWidget *dialog;
    GtkWidget *table, *label;
    GtkAdjustment *adjustment;
    GtkWidget *num_announce_notice;
    GtkWidget *enable_voice;
    GtkWidget *enable_announce;

    /* Auto-Center page. */
    dialog = gtk_dialog_new_with_buttons
        (_("Announce"), parent, GTK_DIALOG_MODAL,
         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
         NULL);

    table = gtk_table_new(3, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table,
                       TRUE, TRUE, 0);

    /* Enable Waypoint Announcements. */
    enable_announce =
        hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(enable_announce),
                         _("Enable Waypoint Announcements"));
    gtk_table_attach(GTK_TABLE(table), enable_announce,
                     0, 2, 0, 1, GTK_FILL, 0, 2, 4);

    /* Announcement Advance Notice. */
    gtk_table_attach(GTK_TABLE(table),
                     label = gtk_label_new(_("Advance Notice")),
                     0, 1, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table),
                     num_announce_notice = hildon_gtk_hscale_new(),
                     1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    adjustment =
        gtk_range_get_adjustment(GTK_RANGE(num_announce_notice));
    g_object_set(adjustment,
                 "step-increment", 1.0,
                 "lower", 1.0,
                 "upper", 20.0,
                 NULL);

    /* Enable Voice. */
    enable_voice = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(enable_voice),
                         _("Enable Voice Synthesis (requires flite)"));
    gtk_table_attach(GTK_TABLE(table), enable_voice,
                     0, 2, 2, 3, GTK_FILL, 0, 2, 4);

    /* Initialize widgets */
    hildon_check_button_set_active
        (HILDON_CHECK_BUTTON(enable_announce), _enable_announce);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(enable_voice),
                                   _enable_voice);
    gtk_range_set_value(GTK_RANGE(num_announce_notice),
                        _announce_notice_ratio);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        _announce_notice_ratio =
            gtk_range_get_value(GTK_RANGE(num_announce_notice));

        _enable_announce = hildon_check_button_get_active
            (HILDON_CHECK_BUTTON(enable_announce));

        _enable_voice = hildon_check_button_get_active
            (HILDON_CHECK_BUTTON(enable_voice));

        settings_dialog_set_save(parent, TRUE);
    }

    gtk_widget_destroy(dialog);
}

static void
run_gps_dialog(GtkWindow *parent)
{
    MapController *controller = map_controller_get_instance();
    GtkWidget *dialog;
    GtkWidget *w_power_save;
    HildonTouchSelector *selector;
    GtkWidget *w_interval;
    static const gint intervals[] = { 1, 2, 5, 10, 20, 30, 60, 120, 0 };
    gint i, selected, interval;

    dialog = map_dialog_new(_("GPS"), parent, TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);

    /* Polling interval */
    interval = map_controller_gps_get_interval(controller);
    selected = 0;
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(selector, _("1 second"));
    for (i = 1; intervals[i] != 0; i++)
    {
        gchar buffer[64];
        snprintf(buffer, sizeof(buffer), _("%d seconds"), intervals[i]);
        hildon_touch_selector_append_text(selector, buffer);
        if (interval >= intervals[i])
            selected = i;
    }
    w_interval =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("GPS polling interval"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    hildon_picker_button_set_active(HILDON_PICKER_BUTTON(w_interval),
                                    selected);
    map_dialog_add_widget(MAP_DIALOG(dialog), w_interval);

    /* GPS power saving */
    w_power_save = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(w_power_save),
                         _("Reduce GPS usage on screen off"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(w_power_save),
        map_controller_gps_get_power_save(controller));
    map_dialog_add_widget(MAP_DIALOG(dialog), w_power_save);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        selected =
            hildon_picker_button_get_active(HILDON_PICKER_BUTTON(w_interval));
        map_controller_gps_set_interval(controller, intervals[selected]);

        map_controller_gps_set_power_save(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(w_power_save)));

        settings_dialog_set_save(parent, TRUE);
    }

    gtk_widget_destroy(dialog);
}

static void
run_misc_dialog(GtkWindow *parent)
{
    MapController *controller = map_controller_get_instance();
    GtkWidget *dialog;
    GtkWidget *pannable;
    HildonTouchSelector *selector;
    GtkWidget *vbox, *hbox;
    GtkWidget *draw_width;
    GtkWidget *orientation;
    GtkWidget *unblank_option;
    GtkWidget *info_font_size;
    GtkWidget *units;
    GtkWidget *degformat;
    GtkWidget *auto_download_precache;
    GtkWidget *speed_limit;
    GtkWidget *speed_location;
    gint i;

    dialog = gtk_dialog_new_with_buttons
        (_("Misc."), parent, GTK_DIALOG_MODAL,
         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
         NULL);

    vbox = gtk_vbox_new(FALSE, 0);

    pannable = hildon_pannable_area_new();
    gtk_widget_set_size_request(pannable, -1, 400);
    hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable),
                                           vbox);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), pannable,
                       TRUE, TRUE, 0);

    /* Line Width. */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 1; i <= 20; i++)
    {
        gchar buffer[5];
        sprintf(buffer, "%d", i);
        hildon_touch_selector_append_text(selector, buffer);
    }
    draw_width =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Line Width"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), draw_width, FALSE, TRUE, 0);

    /* Orientation */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(selector, _("Automatic"));
    hildon_touch_selector_append_text(selector, _("Always landscape"));
    hildon_touch_selector_append_text(selector, _("Always portrait"));
    orientation =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Orientation"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), orientation, FALSE, TRUE, 0);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(orientation),
         map_controller_get_orientation(controller));

    /* Unblank Screen */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < UNBLANK_ENUM_COUNT; i++)
        hildon_touch_selector_append_text(selector, UNBLANK_ENUM_TEXT[i]);
    unblank_option =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Unblank Screen"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), unblank_option, FALSE, TRUE, 0);

    /* Information Font Size. */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < INFO_FONT_ENUM_COUNT; i++)
        hildon_touch_selector_append_text(selector, INFO_FONT_ENUM_TEXT[i]);
    info_font_size =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Info Font Size"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), info_font_size, FALSE, TRUE, 0);

    /* Units. */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < UNITS_ENUM_COUNT; i++)
        hildon_touch_selector_append_text(selector, UNITS_ENUM_TEXT[i]);
    units =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Units"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), units, FALSE, TRUE, 0);


    /* Degrees format */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < DEG_FORMAT_ENUM_COUNT; i++)
        hildon_touch_selector_append_text(selector,
                                          DEG_FORMAT_ENUM_TEXT[i].name);
    degformat =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Degrees Format"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), degformat, FALSE, TRUE, 0);

    /* Pre-cache */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < 5; i++)
    {
        gchar buffer[5];
        sprintf(buffer, "%d", i);
        hildon_touch_selector_append_text(selector, buffer);
    }
    auto_download_precache =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Auto-Download Pre-cache"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), auto_download_precache,
                       FALSE, TRUE, 0);

    /* Speed warner. */
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(selector, _("Disabled"));
    for (i = 1; i <= 100; i++)
    {
        gchar buffer[5];
        sprintf(buffer, "%d", i * 5);
        hildon_touch_selector_append_text(selector, buffer);
    }
    speed_limit =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Speed Limit"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(hbox), speed_limit, TRUE, TRUE, 0);

    /* Speed location */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < SPEED_LOCATION_ENUM_COUNT; i++)
        hildon_touch_selector_append_text(selector,
                                          SPEED_LOCATION_ENUM_TEXT[i]);
    speed_location =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Location"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(hbox), speed_location, TRUE, TRUE, 0);

    /* Initialize widgets */
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(draw_width), _draw_width);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(units), _units);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(degformat), _degformat);
    if (_auto_download_precache < 1) _auto_download_precache = 2;
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(auto_download_precache),
         _auto_download_precache - 1);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(speed_limit),
         _speed_limit_on ? (_speed_limit / 5) : 0);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(speed_location), _speed_location);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(unblank_option), _unblank_option);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(info_font_size), _info_font_size);


    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        _auto_download_precache = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(auto_download_precache)) + 1;

        _draw_width = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(draw_width));

        _units = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(units));

        _degformat = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(degformat));

        _speed_limit = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(speed_limit)) * 5;

        _speed_limit_on = _speed_limit > 0;

        _speed_location = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(speed_location));

        _unblank_option = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(unblank_option));

        _info_font_size = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(info_font_size));

        map_controller_set_orientation(controller,
            hildon_picker_button_get_active(HILDON_PICKER_BUTTON(orientation)));

        settings_dialog_set_save(parent, TRUE);
    }

    gtk_widget_destroy(dialog);
}

static void
run_poi_dialog(GtkWindow *parent)
{
    GtkWidget *dialog;
    GtkBox *vbox;
    HildonTouchSelector *selector;
    GtkWidget *poi_db;
    GtkWidget *poi_zoom;
    gint i;

    /* POI page. */
    dialog = gtk_dialog_new_with_buttons
        (_("POI"), parent, GTK_DIALOG_MODAL,
         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
         NULL);

    vbox = GTK_BOX(GTK_DIALOG(dialog)->vbox);

    /* POI database. */
    poi_db = hildon_button_new_with_text
        (HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_VERTICAL,
         _("POI database"), "");
    gtk_button_set_alignment(GTK_BUTTON(poi_db), 0.0, 0.5);
    g_signal_connect(poi_db, "clicked",
                     G_CALLBACK(settings_dialog_browse_forfile), NULL);
    gtk_box_pack_start(vbox, poi_db, FALSE, TRUE, 0);

    /* Show POI below zoom. */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i <= MAX_ZOOM; i++)
    {
        gchar buffer[5];
        sprintf(buffer, "%d", i);
        hildon_touch_selector_append_text(selector, buffer);
    }
    poi_zoom =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Show POI below zoom"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), poi_zoom, FALSE, TRUE, 0);

    /* Initialize widgets */
    if(_poi_db_filename)
        hildon_button_set_value(HILDON_BUTTON(poi_db),
                                _poi_db_filename);
    hildon_picker_button_set_active
        (HILDON_PICKER_BUTTON(poi_zoom), _poi_zoom);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar *poi_db_path;

        /* Check if user specified a different POI database from before. */
        poi_db_path = hildon_button_get_value(HILDON_BUTTON(poi_db));
        if (g_strcmp0(_poi_db_filename, poi_db_path) != 0)
        {
            /* Clear old filename/dirname, if necessary. */
            if(_poi_db_filename)
            {
                g_free(_poi_db_filename);
                _poi_db_filename = NULL;
                g_free(_poi_db_dirname);
                _poi_db_dirname = NULL;
            }

            if(*poi_db_path)
            {
                _poi_db_filename = g_strdup(poi_db_path);
                _poi_db_dirname = g_path_get_dirname(_poi_db_filename);
            }

            poi_db_connect();
        }

        _poi_zoom = hildon_picker_button_get_active
            (HILDON_PICKER_BUTTON(poi_zoom));

        settings_dialog_set_save(parent, TRUE);
    }

    gtk_widget_destroy(dialog);
}

/**
 * Bring up the Settings dialog.  Return TRUE if and only if the recever
 * information has changed (MAC or channel).
 */
gboolean settings_dialog()
{
    static GtkWidget *dialog = NULL;
    GtkWindow *parent;
    gint response;
    gboolean quit;
    enum {
        SETTINGS_AUTO_CENTER,
        SETTINGS_ANNOUNCE,
        SETTINGS_GPS,
        SETTINGS_MISC,
        SETTINGS_POI,
        SETTINGS_KEYS,
        SETTINGS_COLORS,
    };

    if(dialog == NULL)
    {
        MapDialog *dlg;

        dialog = map_dialog_new(_("Settings"), GTK_WINDOW(_window), FALSE);
        dlg = (MapDialog *)dialog;

        /* Auto-Center page. */
        map_dialog_create_button(dlg, _("Auto-Center"), SETTINGS_AUTO_CENTER);
        map_dialog_create_button(dlg, _("Announce"), SETTINGS_ANNOUNCE);
        map_dialog_create_button(dlg, _("GPS"), SETTINGS_GPS);
        map_dialog_create_button(dlg, _("Misc."), SETTINGS_MISC);
        map_dialog_create_button(dlg, _("POI"), SETTINGS_POI);
        map_dialog_create_button(dlg, _("Hardware Keys..."), SETTINGS_KEYS);
        map_dialog_create_button(dlg, _("Colors..."), SETTINGS_COLORS);
    }

    gtk_widget_show_all(dialog);

    settings_dialog_set_save(dialog, FALSE);
    quit = FALSE;
    parent = GTK_WINDOW(dialog);
    do
    {
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        switch (response) {
            case SETTINGS_AUTO_CENTER:
                run_auto_center_dialog(parent); break;
            case SETTINGS_ANNOUNCE:
                run_announce_dialog(parent); break;
            case SETTINGS_GPS:
                run_gps_dialog(parent); break;
            case SETTINGS_MISC:
                run_misc_dialog(parent); break;
            case SETTINGS_POI:
                run_poi_dialog(parent); break;
            case SETTINGS_KEYS:
                settings_dialog_hardkeys(parent); break;
            case SETTINGS_COLORS:
                settings_dialog_colors(parent); break;
            default:
                quit = TRUE;
        }
    }
    while (!quit);

    gtk_widget_hide(dialog);

    if (settings_dialog_get_save(dialog))
    {
        settings_save();

        map_force_redraw();
        map_refresh_mark(TRUE);
    }

    return FALSE;
}


/**
 * Initialize all configuration from GCONF.  This should not be called more
 * than once during execution.
 */
void
settings_init(GConfClient *gconf_client)
{
    GConfValue *value;
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gchar *str;
    MapOrientation orientation;
    gboolean auto_download;
    gboolean gps_power_save;
    gint gps_interval;

    /* Initialize some constants. */
    CUSTOM_KEY_GCONF[CUSTOM_KEY_UP] = GCONF_KEY_PREFIX"/key_up";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_DOWN] = GCONF_KEY_PREFIX"/key_down";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_LEFT] = GCONF_KEY_PREFIX"/key_left";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_RIGHT] = GCONF_KEY_PREFIX"/key_right";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_SELECT] = GCONF_KEY_PREFIX"/key_select";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_INCREASE] = GCONF_KEY_PREFIX"/key_increase";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_DECREASE] = GCONF_KEY_PREFIX"/key_decrease";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_FULLSCREEN]= GCONF_KEY_PREFIX"/key_fullscreen";
    CUSTOM_KEY_GCONF[CUSTOM_KEY_ESC] = GCONF_KEY_PREFIX"/key_esc";

    COLORABLE_GCONF[COLORABLE_MARK] = GCONF_KEY_PREFIX"/color_mark";
    COLORABLE_GCONF[COLORABLE_MARK_VELOCITY]
        = GCONF_KEY_PREFIX"/color_mark_velocity";
    COLORABLE_GCONF[COLORABLE_MARK_OLD] = GCONF_KEY_PREFIX"/color_mark_old";
    COLORABLE_GCONF[COLORABLE_TRACK] = GCONF_KEY_PREFIX"/color_track";
    COLORABLE_GCONF[COLORABLE_TRACK_MARK] =GCONF_KEY_PREFIX"/color_track_mark";
    COLORABLE_GCONF[COLORABLE_TRACK_BREAK]
        = GCONF_KEY_PREFIX"/color_track_break";
    COLORABLE_GCONF[COLORABLE_ROUTE] = GCONF_KEY_PREFIX"/color_route";
    COLORABLE_GCONF[COLORABLE_ROUTE_WAY] = GCONF_KEY_PREFIX"/color_route_way";
    COLORABLE_GCONF[COLORABLE_ROUTE_BREAK]
        = GCONF_KEY_PREFIX"/color_route_break";
    COLORABLE_GCONF[COLORABLE_ROUTE_NEXT] = GCONF_KEY_PREFIX"/color_route_next";
    COLORABLE_GCONF[COLORABLE_POI] = GCONF_KEY_PREFIX"/color_poi";
    
    if(!gconf_client)
    {
        popup_error(_window, _("Failed to initialize GConf.  Quitting."));
        exit(1);
    }

    /* Get GPS Receiver Type.  Default is Bluetooth Receiver. */
    {
        gchar *gri_type_str = gconf_client_get_string(gconf_client,
                GCONF_KEY_GPS_RCVR_TYPE, NULL);
        gint i = 0;
        if(gri_type_str)
        {
            for(i = GPS_RCVR_ENUM_COUNT - 1; i > 0; i--)
                if(!strcmp(gri_type_str, GPS_RCVR_ENUM_TEXT[i]))
                    break;
            g_free(gri_type_str);
        }
        _gri.type = i;
    }

    /* Get Bluetooth Receiver MAC.  Default is NULL. */
    _gri.bt_mac = gconf_client_get_string(
            gconf_client, GCONF_KEY_GPS_BT_MAC, NULL);

    /* Get GPSD Host.  Default is localhost. */
    _gri.gpsd_host = gconf_client_get_string(
            gconf_client, GCONF_KEY_GPS_GPSD_HOST, NULL);
    if(!_gri.gpsd_host)
        _gri.gpsd_host = g_strdup("127.0.0.1");

    /* Get GPSD Port.  Default is GPSD_PORT_DEFAULT (2947). */
    if(!(_gri.gpsd_port = gconf_client_get_int(
            gconf_client, GCONF_KEY_GPS_GPSD_PORT, NULL)))
        _gri.gpsd_port = GPSD_PORT_DEFAULT;

    /* Get File Path.  Default is /dev/pgps. */
    _gri.file_path = gconf_client_get_string(
            gconf_client, GCONF_KEY_GPS_FILE_PATH, NULL);
    if(!_gri.file_path)
        _gri.file_path = g_strdup("/dev/pgps");

    /* Get GPS polling interval */
    value = gconf_client_get(gconf_client, GCONF_KEY_GPS_INTERVAL, NULL);
    if (value)
    {
        gps_interval = gconf_value_get_int(value);
        gconf_value_free(value);
    }
    else
        gps_interval = 1;
    map_controller_gps_set_interval(controller, gps_interval);

    /* Get GPS power save */
    value = gconf_client_get(gconf_client, GCONF_KEY_GPS_POWER_SAVE, NULL);
    if (value)
    {
        gps_power_save = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        gps_power_save = TRUE;
    map_controller_gps_set_power_save(controller, gps_power_save);

    /* Get Auto-Download.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_AUTO_DOWNLOAD, NULL);
    if (value)
    {
        auto_download = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        auto_download = TRUE;
    map_controller_set_auto_download(controller, auto_download);

    /* Get Auto-Download Pre-cache - Default is 0. */
    _auto_download_precache = gconf_client_get_int(gconf_client,
            GCONF_KEY_AUTO_DOWNLOAD_PRECACHE, NULL);
    if(!_auto_download_precache)
        _auto_download_precache = 0;

    /* Get Center Ratio - Default is 5. */
    _center_ratio = gconf_client_get_int(gconf_client,
            GCONF_KEY_CENTER_SENSITIVITY, NULL);
    if(!_center_ratio)
        _center_ratio = 5;

    /* Get Lead Ratio - Default is 5. */
    _lead_ratio = gconf_client_get_int(gconf_client,
            GCONF_KEY_LEAD_AMOUNT, NULL);
    if(!_lead_ratio)
        _lead_ratio = 5;

    /* Get Lead Is Fixed flag - Default is FALSE. */
    _lead_is_fixed = gconf_client_get_bool(gconf_client,
            GCONF_KEY_LEAD_IS_FIXED, NULL);

    /* Get Auto-Center/Rotate Minimum Speed - Default is 2. */
    value = gconf_client_get(gconf_client, GCONF_KEY_AC_MIN_SPEED, NULL);
    if(value)
    {
        _ac_min_speed = gconf_value_get_int(value);
        gconf_value_free(value);
    }
    else
        _ac_min_speed = 2;

    /* Get Draw Line Width- Default is 5. */
    _draw_width = gconf_client_get_int(gconf_client,
            GCONF_KEY_DRAW_WIDTH, NULL);
    if(!_draw_width)
        _draw_width = 5;

    /* Get Enable Announcements flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ENABLE_ANNOUNCE, NULL);
    if(value)
    {
        _enable_announce = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _enable_announce = TRUE;

    /* Get Announce Advance Notice - Default is 30. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ANNOUNCE_NOTICE, NULL);
    if(value)
    {
        _announce_notice_ratio = gconf_value_get_int(value);
        gconf_value_free(value);
    }
    else
        _announce_notice_ratio = 8;

    /* Get Enable Voice flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ENABLE_VOICE, NULL);
    if(value)
    {
        _enable_voice = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _enable_voice = TRUE;

#if 0
    if(_enable_voice)
    {
        /* Make sure we actually have voice capabilities. */
        GnomeVFSFileInfo file_info;
        _enable_voice = ((GNOME_VFS_OK == gnome_vfs_get_file_info(
                    _voice_synth_path, &file_info,
                    GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS))
            && (file_info.permissions & GNOME_VFS_PERM_ACCESS_EXECUTABLE));
    }
#endif

    /* Get Fullscreen flag. Default is FALSE. */
    _fullscreen = gconf_client_get_bool(gconf_client,
            GCONF_KEY_FULLSCREEN, NULL);

    /* Get Units.  Default is UNITS_KM. */
    {
        gchar *units_str = gconf_client_get_string(gconf_client,
                GCONF_KEY_UNITS, NULL);
        gint i = 0;
        if(units_str)
            for(i = UNITS_ENUM_COUNT - 1; i > 0; i--)
                if(!strcmp(units_str, UNITS_ENUM_TEXT[i]))
                    break;
        _units = i;
    }

    /* Get Custom Key Actions. */
    {
        gint i;
        for(i = 0; i < CUSTOM_KEY_ENUM_COUNT; i++)
        {
            gint j = CUSTOM_KEY_DEFAULT[i];
            gchar *str = gconf_client_get_string(gconf_client,
                    CUSTOM_KEY_GCONF[i], NULL);
            if(str)
                for(j = CUSTOM_ACTION_ENUM_COUNT - 1; j > 0; j--)
                    if(!strcmp(str, CUSTOM_ACTION_ENUM_TEXT[j]))
                        break;
            _action[i] = j;
        }
    }

    /* Get Deg format.  Default is DDPDDDDD. */
    {
        gchar *degformat_key_str = gconf_client_get_string(gconf_client,
                GCONF_KEY_DEG_FORMAT, NULL);
        gint i = 0;
        if(degformat_key_str)
            for(i = DEG_FORMAT_ENUM_COUNT - 1; i > 0; i--)
                if(!strcmp(degformat_key_str, DEG_FORMAT_ENUM_TEXT[i].name))
                    break;
        _degformat = i;
    }

    /* Get Speed Limit On flag.  Default is FALSE. */
    _speed_limit_on = gconf_client_get_bool(gconf_client,
            GCONF_KEY_SPEED_LIMIT_ON, NULL);

    /* Get Speed Limit */
    _speed_limit = gconf_client_get_int(gconf_client,
            GCONF_KEY_SPEED_LIMIT, NULL);
    if(_speed_limit <= 0)
        _speed_limit = 100;

    /* Get Speed Location.  Default is SPEED_LOCATION_TOP_LEFT. */
    {
        gchar *speed_location_str = gconf_client_get_string(gconf_client,
                GCONF_KEY_SPEED_LOCATION, NULL);
        gint i = 0;
        if(speed_location_str)
            for(i = SPEED_LOCATION_ENUM_COUNT - 1; i > 0; i--)
                if(!strcmp(speed_location_str, SPEED_LOCATION_ENUM_TEXT[i]))
                    break;
        _speed_location = i;
    }

    /* Get Unblank Option.  Default is UNBLANK_FULLSCREEN. */
    {
        gchar *unblank_option_str = gconf_client_get_string(gconf_client,
                GCONF_KEY_UNBLANK_SIZE, NULL);
        gint i = -1;
        if(unblank_option_str)
            for(i = UNBLANK_ENUM_COUNT - 1; i >= 0; i--)
                if(!strcmp(unblank_option_str, UNBLANK_ENUM_TEXT[i]))
                    break;
        if(i == -1)
            i = UNBLANK_FULLSCREEN;
        _unblank_option = i;
    }

    /* Get Info Font Size.  Default is INFO_FONT_MEDIUM. */
    {
        gchar *info_font_size_str = gconf_client_get_string(gconf_client,
                GCONF_KEY_INFO_FONT_SIZE, NULL);
        gint i = -1;
        if(info_font_size_str)
            for(i = INFO_FONT_ENUM_COUNT - 1; i >= 0; i--)
                if(!strcmp(info_font_size_str, INFO_FONT_ENUM_TEXT[i]))
                    break;
        if(i == -1)
            i = INFO_FONT_MEDIUM;
        _info_font_size = i;
    }

    /* Get last saved altitude.  Default is 0. */
    _pos.altitude = gconf_client_get_int(
            gconf_client, GCONF_KEY_LAST_ALT, NULL);

    /* Get last saved timestamp.  Default is 0. */
    _pos.time= gconf_client_get_float(gconf_client, GCONF_KEY_LAST_TIME, NULL);

    /* Get last center point. */
    {
        MapGeo center_lat, center_lon;

        /* Get last saved latitude.  Default is last saved latitude. */
        value = gconf_client_get(gconf_client, GCONF_KEY_CENTER_LAT, NULL);
        if(value)
        {
            center_lat = gconf_value_get_float(value);
            gconf_value_free(value);
        }
        else
        {
            _is_first_time = TRUE;
            center_lat = gps->lat;
        }

        /* Get last saved longitude.  Default is last saved longitude. */
        value = gconf_client_get(gconf_client, GCONF_KEY_CENTER_LON, NULL);
        if(value)
        {
            center_lon = gconf_value_get_float(value);
            gconf_value_free(value);
        }
        else
            center_lon = gps->lon;

        latlon2unit(center_lat, center_lon, _center.x, _center.y);
        _next_center = _center;
    }

    /* Get last viewing angle.  Default is 0. */
    _map_rotate_angle = _next_map_rotate_angle = gconf_client_get_int(
            gconf_client, GCONF_KEY_CENTER_ANGLE, NULL);

    /* Get last Zoom Level.  Default is 16. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ZOOM, NULL);
    if(value)
    {
        _zoom = gconf_value_get_int(value);
        gconf_value_free(value);
    }
    else
        _zoom = 16;
    _next_zoom = _zoom;

    /* Get Route Directory.  Default is NULL. */
    _route_dir_uri = gconf_client_get_string(gconf_client,
            GCONF_KEY_ROUTEDIR, NULL);

    /* Get Last Track File.  Default is NULL. */
    _track_file_uri = gconf_client_get_string(gconf_client,
            GCONF_KEY_TRACKFILE, NULL);

    /* Get Auto-Center Mode.  Default is CENTER_LATLON. */
    value = gconf_client_get(gconf_client, GCONF_KEY_AUTOCENTER_MODE, NULL);
    if(value)
    {
        _center_mode = gconf_value_get_int(value);
        gconf_value_free(value);
    }
    else
        _center_mode = CENTER_LATLON;

    /* Get Auto-Center Rotate Flag.  Default is FALSE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_AUTOCENTER_ROTATE, NULL);
    if(value)
    {
        _center_rotate = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _center_rotate = FALSE;

    /* Get Show Zoom Level flag.  Default is FALSE. */
    _show_zoomlevel = gconf_client_get_bool(gconf_client,
            GCONF_KEY_SHOWZOOMLEVEL, NULL);

    /* Get Show Scale flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_SHOWSCALE, NULL);
    if(value)
    {
        _show_scale = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _show_scale = TRUE;

    /* Get Show Compass Rose flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_SHOWCOMPROSE, NULL);
    if(value)
    {
        _show_comprose = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _show_comprose = TRUE;

    /* Get Show Tracks flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_SHOWTRACKS, NULL);
    if(value)
    {
        _show_paths |= (gconf_value_get_bool(value) ? TRACKS_MASK : 0);
        gconf_value_free(value);
    }
    else
        _show_paths |= TRACKS_MASK;

    /* Get Show Routes flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_SHOWROUTES, NULL);
    if(value)
    {
        _show_paths |= (gconf_value_get_bool(value) ? ROUTES_MASK : 0);
        gconf_value_free(value);
    }
    else
        _show_paths |= ROUTES_MASK;

    /* Get Show Velocity Vector flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_SHOWVELVEC, NULL);
    if(value)
    {
        _show_velvec = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _show_velvec = TRUE;

    /* Get Show Velocity Vector flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_SHOWPOIS, NULL);
    if(value)
    {
        _show_poi = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _show_poi = TRUE;

    /* Get Enable GPS flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ENABLE_GPS, NULL);
    if(value)
    {
        _enable_gps = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _enable_gps = TRUE;

    /* Get Enable Tracking flag.  Default is TRUE. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ENABLE_TRACKING, NULL);
    if(value)
    {
        _enable_tracking = gconf_value_get_bool(value);
        gconf_value_free(value);
    }
    else
        _enable_tracking = TRUE;

    /* Initialize _gps_state based on _enable_gps. */
    _gps_state = RCVR_OFF;

    /* Load the route locations. */
    {
        GSList *curr;
        _loc_list = gconf_client_get_list(gconf_client,
            GCONF_KEY_ROUTE_LOCATIONS, GCONF_VALUE_STRING, NULL);
        _loc_model = gtk_list_store_new(1, G_TYPE_STRING);
        for(curr = _loc_list; curr != NULL; curr = curr->next)
        {
            GtkTreeIter iter;
            gtk_list_store_insert_with_values(_loc_model, &iter, INT_MAX,
                    0, curr->data, -1);
        }
    }

    /* Get POI Database.  Default is in REPO_DEFAULT_CACHE_BASE */
    _poi_db_filename = gconf_client_get_string(gconf_client,
            GCONF_KEY_POI_DB, NULL);
    if(_poi_db_filename == NULL)
    {
        gchar *poi_base = gnome_vfs_expand_initial_tilde(
                CACHE_BASE_DIR);
        _poi_db_filename = gnome_vfs_uri_make_full_from_relative(
                poi_base, "poi.db");
        g_free(poi_base);
    }

    _poi_db_dirname = g_path_get_dirname(_poi_db_filename);

    _poi_zoom = gconf_client_get_int(gconf_client,
            GCONF_KEY_POI_ZOOM, NULL);
    if(!_poi_zoom)
        _poi_zoom = MAX_ZOOM - 10;


    /* Get GPS Info flag.  Default is FALSE. */
    _gps_info = gconf_client_get_bool(gconf_client, GCONF_KEY_GPS_INFO, NULL);

    /* Get default router. */
    str = gconf_client_get_string(gconf_client, GCONF_KEY_ROUTER_NAME, NULL);
    if (str)
    {
        const GSList *list;
        for (list = map_controller_list_plugins(controller); list != NULL;
             list = list->next)
        {
            MapRouter *router = list->data;

            if (!MAP_IS_ROUTER(router)) continue;

            if (strcmp(map_router_get_name(router), str) == 0)
            {
                map_controller_set_default_router(controller, router);
                break;
            }
        }
        g_free(str);
    }

    /* Get Route Download Radius.  Default is 4. */
    value = gconf_client_get(gconf_client, GCONF_KEY_ROUTE_DL_RADIUS, NULL);
    if(value)
    {
        _route_dl_radius = gconf_value_get_int(value);
        gconf_value_free(value);
    }
    else
        _route_dl_radius = 8;

    /* Get POI Download URL.  Default is:
     * "http://www.gnuite.com/cgi-bin/poi.cgi?saddr=%s&query=%s&page=%d" */
    _poi_dl_url = gconf_client_get_string(gconf_client,
            GCONF_KEY_POI_DL_URL, NULL);
    if(_poi_dl_url == NULL)
        _poi_dl_url = g_strdup(
            "http://www.gnuite.com/cgi-bin/poi.cgi?saddr=%s&query=%s&page=%d");

    /* Get Colors. */
    {
        gint i;
        for(i = 0; i < COLORABLE_ENUM_COUNT; i++)
        {
            str = gconf_client_get_string(gconf_client,
                    COLORABLE_GCONF[i], NULL);
            if(!str || !gdk_color_parse(str, &_color[i]))
                _color[i] = COLORABLE_DEFAULT[i];
        }
    }

    /* device orientation */
    orientation = gconf_client_get_int(gconf_client,
                                       GCONF_KEY_ORIENTATION, NULL);
    map_controller_set_orientation(controller, orientation);
}

