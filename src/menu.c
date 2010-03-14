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

#include <math.h>
#include <string.h>

#ifndef LEGACY
#    include <hildon/hildon-program.h>
#    include <hildon/hildon-banner.h>
#else
#    include <osso-helplib.h>
#    include <hildon-widgets/hildon-program.h>
#    include <hildon-widgets/hildon-banner.h>
#    include <hildon-widgets/hildon-input-mode-hint.h>
#endif

#include "types.h"
#include "data.h"
#include "debug.h"
#include "defines.h"
#include "dialog.h"
#include "display.h"
#include "error.h"
#include "gpx.h"
#include "he-about-dialog.h"
#include "maps.h"
#include "menu.h"
#include "path.h"
#include "poi.h"
#include "repository.h"
#include "tile_source.h"
#include "screen.h"
#include "settings.h"
#include "util.h"

#include <hildon/hildon-check-button.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>

/****************************************************************************
 * BELOW: ROUTE MENU ********************************************************
 ****************************************************************************/

static gboolean
menu_cb_route_open(GtkMenuItem *item)
{
    gchar *buffer;
    gint size;

    if(display_open_file(GTK_WINDOW(_window), &buffer, NULL, &size,
                &_route_dir_uri, NULL, GTK_FILE_CHOOSER_ACTION_OPEN))
    {
        /* If auto is enabled, append the route, otherwise replace it. */
        if(gpx_path_parse(&_route, buffer, size,
                          autoroute_enabled() ? 0 : 1))
        {
            MapController *controller = map_controller_get_instance();

            path_save_route_to_db();

            cancel_autoroute();

            /* Find the nearest route point, if we're connected. */
            route_find_nearest_point();

            map_controller_refresh_paths(controller);
            MACRO_BANNER_SHOW_INFO(_window, _("Route Opened"));
        }
        else
            popup_error(_window, _("Error parsing GPX file."));
        g_free(buffer);
    }

    return TRUE;
}

static gboolean
menu_cb_route_download(GtkMenuItem *item)
{
    route_download(NULL);
    return TRUE;
}

static gboolean
menu_cb_route_save(GtkMenuItem *item)
{
    GnomeVFSHandle *handle;

    if(display_open_file(GTK_WINDOW(_window), NULL, &handle, NULL,
                &_route_dir_uri, NULL, GTK_FILE_CHOOSER_ACTION_SAVE))
    {
        if(gpx_path_write(&_route, handle))
        {
            MACRO_BANNER_SHOW_INFO(_window, _("Route Saved"));
        }
        else
            popup_error(_window, _("Error writing GPX file."));
        gnome_vfs_close(handle);
    }

    return TRUE;
}

gboolean
menu_cb_route_reset(GtkMenuItem *item)
{
    path_reset_route();

    return TRUE;
}

static gboolean
menu_cb_route_clear(GtkMenuItem *item)
{
    MapController *controller = map_controller_get_instance();

    cancel_autoroute();
    MACRO_PATH_FREE(_route);
    MACRO_PATH_INIT(_route);
    path_save_route_to_db();
    route_find_nearest_point();
    map_controller_refresh_paths(controller);

    return TRUE;
}

/****************************************************************************
 * ABOVE: ROUTE MENU ********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: TRACK MENU ********************************************************
 ****************************************************************************/

static gboolean
menu_cb_track_open(GtkMenuItem *item)
{
    gchar *buffer;
    gint size;

    if(display_open_file(GTK_WINDOW(_window), &buffer, NULL, &size,
                NULL, &_track_file_uri, GTK_FILE_CHOOSER_ACTION_OPEN))
    {
        if(gpx_path_parse(&_track, buffer, size, -1))
        {
            MapController *controller = map_controller_get_instance();
            map_controller_refresh_paths(controller);
            MACRO_BANNER_SHOW_INFO(_window, _("Track Opened"));
        }
        else
            popup_error(_window, _("Error parsing GPX file."));
        g_free(buffer);
    }

    return TRUE;
}

static gboolean
menu_cb_track_save(GtkMenuItem *item)
{
    GnomeVFSHandle *handle;

    if(display_open_file(GTK_WINDOW(_window), NULL, &handle, NULL,
                NULL, &_track_file_uri, GTK_FILE_CHOOSER_ACTION_SAVE))
    {
        if(gpx_path_write(&_track, handle))
        {
            MACRO_BANNER_SHOW_INFO(_window, _("Track Saved"));
        }
        else
            popup_error(_window, _("Error writing GPX file."));
        gnome_vfs_close(handle);
    }

    return TRUE;
}

static gboolean
menu_cb_track_insert_break(GtkMenuItem *item)
{
    track_insert_break(TRUE);

    return TRUE;
}

static gboolean
menu_cb_track_insert_mark(GtkMenuItem *item)
{
    MapGeo lat, lon;
    gchar tmp1[LL_FMT_LEN], tmp2[LL_FMT_LEN], *p_latlon;
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *lbl_latlon = NULL;
    static GtkWidget *txt_scroll = NULL;
    static GtkWidget *txt_desc = NULL;

    if(dialog == NULL)
    {
        dialog = gtk_dialog_new_with_buttons(_("Insert Mark"),
                GTK_WINDOW(_window), GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                NULL);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                table = gtk_table_new(2, 2, FALSE), TRUE, TRUE, 0);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Lat, Lon:")),
                0, 1, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                lbl_latlon = gtk_label_new(""),
                1, 2, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(lbl_latlon), 0.0f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Description")),
                0, 1, 1, 2, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

        txt_scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(txt_scroll),
                                       GTK_SHADOW_IN);
        gtk_table_attach(GTK_TABLE(table),
                txt_scroll,
                1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 2, 4);

        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(txt_scroll),
                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

        txt_desc = gtk_text_view_new ();
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txt_desc), GTK_WRAP_WORD);

        gtk_container_add(GTK_CONTAINER(txt_scroll), txt_desc);
        gtk_widget_set_size_request(GTK_WIDGET(txt_scroll), 400, 60);
    }

    unit2latlon(_pos.unit.x, _pos.unit.y, lat, lon);
    
    format_lat_lon(lat, lon, tmp1, tmp2);
    //lat_format(lat, tmp1);
    //lon_format(lon, tmp2);
    p_latlon = g_strdup_printf("%s, %s", tmp1, tmp2);
    gtk_label_set_text(GTK_LABEL(lbl_latlon), p_latlon);
    g_free(p_latlon);

    gtk_text_buffer_set_text(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_desc)), "", 0);

    gtk_widget_show_all(dialog);

    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        GtkTextBuffer *tbuf;
        GtkTextIter ti1, ti2;
        gchar *desc;

        tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_desc));
        gtk_text_buffer_get_iter_at_offset(tbuf, &ti1, 0);
        gtk_text_buffer_get_end_iter(tbuf, &ti2);
        desc = gtk_text_buffer_get_text(tbuf, &ti1, &ti2, TRUE);

        if(*desc)
        {
            MACRO_PATH_INCREMENT_WTAIL(_track);
            _track.wtail->point = _track.tail;
            _track.wtail->desc
                = gtk_text_buffer_get_text(tbuf, &ti1, &ti2, TRUE);
        }
        else
        {
            popup_error(dialog,
                    _("Please provide a description for the mark."));
            g_free(desc);
            continue;
        }
        break;
    }
    gtk_widget_hide(dialog);

    return TRUE;
}

static gboolean
menu_cb_track_clear(GtkMenuItem *item)
{
    track_clear();

    return TRUE;
}

static void
on_enable_tracking_toggled(GtkToggleButton *button)
{
    MapController *controller;
    GtkWidget *dialog;

    controller = map_controller_get_instance();
    map_controller_set_tracking(controller,
                                gtk_toggle_button_get_active(button));

    /* close the dialog */
    dialog = gtk_widget_get_toplevel((GtkWidget *)button);
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
}

/****************************************************************************
 * ABOVE: TRACK MENU ********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: POI MENU **********************************************************
 ****************************************************************************/

static gboolean
menu_cb_poi_import(GtkMenuItem *item)
{
    if (poi_import_dialog(&_center))
        map_force_redraw();

    return TRUE;
}

static gboolean
menu_cb_poi_download(GtkMenuItem *item)
{
    MapPoint p = { 0, 0 }; /* 0, 0 means no default origin */
    if (poi_download_dialog(&p))
        map_force_redraw();

    return TRUE;
}

static gboolean
menu_cb_poi_browse(GtkMenuItem *item)
{
    MapPoint p = { 0, 0 }; /* 0, 0 means no default origin */
    if (poi_browse_dialog(&p))
        map_force_redraw();

    return TRUE;
}

static gboolean
menu_cb_poi_categories(GtkMenuItem *item)
{
    if(category_list_dialog(_window))
        map_force_redraw();

    return TRUE;
}

/****************************************************************************
 * ABOVE: POI MENU **********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: MAPS MENU *********************************************************
 ****************************************************************************/
static gboolean
menu_cb_maps_repoman(GtkMenuItem *item)
{
    repository_list_edit_dialog();
    return TRUE;
}


static gboolean
menu_cb_maps_tile_sources(GtkMenuItem *item)
{
    tile_source_list_edit_dialog();
    return TRUE;
}


static gboolean
menu_cb_maps_select(GtkMenuItem *item, gpointer new_repo)
{
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
    {
        map_controller_set_repository(map_controller_get_instance(), new_repo);
        map_refresh_mark(TRUE);
    }
    return TRUE;
}

static gboolean
menu_cb_maps_mapman(GtkMenuItem *item)
{
    mapman_dialog();

    return TRUE;
}

static gboolean
menu_cb_maps_auto_download(GtkMenuItem *item)
{
    if((_auto_download = gtk_check_menu_item_get_active(
            GTK_CHECK_MENU_ITEM(_menu_maps_auto_download_item))))
    {
        map_refresh_mark(TRUE);
    }

    return TRUE;
}

/****************************************************************************
 * ABOVE: MAPS MENU *********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: LAYERS MENU *******************************************************
 ****************************************************************************/

static gboolean
menu_cb_layers_toggle(GtkCheckMenuItem *item, RepositoryLayer *repo_layer)
{
    map_controller_toggle_layer_visibility(map_controller_get_instance(), repo_layer);

    return TRUE;
}

/****************************************************************************
 * ABOVE: LAYERS MENU *******************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: VIEW/GOTO MENU ****************************************************
 ****************************************************************************/

static gboolean
menu_cb_view_goto_latlon(GtkMenuItem *item)
{
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *txt_lat = NULL;
    static GtkWidget *txt_lon = NULL;

    if(dialog == NULL)
    {
        dialog = gtk_dialog_new_with_buttons(_("Go to Lat/Lon"),
                GTK_WINDOW(_window), GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                NULL);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                table = gtk_table_new(2, 3, FALSE), TRUE, TRUE, 0);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Latitude")),
                0, 1, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                txt_lat = gtk_entry_new(),
                1, 2, 0, 1, GTK_FILL, 0, 2, 4);
        gtk_entry_set_width_chars(GTK_ENTRY(txt_lat), 16);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
#ifdef MAEMO_CHANGES
#ifndef LEGACY
        g_object_set(G_OBJECT(txt_lat), "hildon-input-mode",
                HILDON_GTK_INPUT_MODE_FULL, NULL);
#else
        g_object_set(G_OBJECT(txt_lat), HILDON_AUTOCAP, FALSE, NULL);
        g_object_set(G_OBJECT(txt_lat), HILDON_INPUT_MODE_HINT,
                HILDON_INPUT_MODE_HINT_ALPHANUMERICSPECIAL, NULL);
#endif
#endif

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Longitude")),
                0, 1, 1, 2, GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                txt_lon = gtk_entry_new(),
                1, 2, 1, 2, GTK_FILL, 0, 2, 4);
        gtk_entry_set_width_chars(GTK_ENTRY(txt_lon), 16);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
#ifdef MAEMO_CHANGES
#ifndef LEGACY
        g_object_set(G_OBJECT(txt_lon), "hildon-input-mode",
                HILDON_GTK_INPUT_MODE_FULL, NULL);
#else
        g_object_set(G_OBJECT(txt_lon), HILDON_AUTOCAP, FALSE, NULL);
        g_object_set(G_OBJECT(txt_lon), HILDON_INPUT_MODE_HINT,
                HILDON_INPUT_MODE_HINT_ALPHANUMERICSPECIAL, NULL);
#endif
#endif
    }

    /* Initialize with the current center position. */
    {
        gchar buffer1[32];
        gchar buffer2[32];
        MapGeo lat, lon;
        unit2latlon(_center.x, _center.y, lat, lon);
        //lat_format(lat, buffer1);
        //lon_format(lon, buffer2);
        format_lat_lon(lat, lon, buffer1, buffer2);
        gtk_entry_set_text(GTK_ENTRY(txt_lat), buffer1);
        gtk_entry_set_text(GTK_ENTRY(txt_lon), buffer2);
    }

    gtk_widget_show_all(dialog);

    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        const gchar *text;
        gchar *error_check;
        MapGeo lat, lon;
        MapPoint sel_unit;
        MapController *controller = map_controller_get_instance();

        text = gtk_entry_get_text(GTK_ENTRY(txt_lat));
        lat = strdmstod(text, &error_check);
        if(text == error_check || lat < -90. || lat > 90.) {
            popup_error(dialog, _("Invalid Latitude"));
            continue;
        }

        text = gtk_entry_get_text(GTK_ENTRY(txt_lon));
        lon = strdmstod(text, &error_check);
        if(text == error_check || lon < -180. || lon > 180.) {
            popup_error(dialog, _("Invalid Longitude"));
            continue;
        }

        latlon2unit(lat, lon, sel_unit.x, sel_unit.y);
        map_controller_disable_auto_center(controller);
        map_controller_set_center(controller, sel_unit, -1);
        break;
    }
    gtk_widget_hide(dialog);
    return TRUE;
}

static void
geocode_cb(MapRouter *router, MapPoint p, const GError *error,
           GtkWidget **p_dialog)
{
    GtkWidget *dialog = *p_dialog;
    MapController *controller = map_controller_get_instance();

    DEBUG("called (error = %p)", error);
    g_slice_free(GtkWidget *, p_dialog);
    if (!dialog) return;

    g_object_remove_weak_pointer(G_OBJECT(dialog), (gpointer)p_dialog);

    hildon_gtk_window_set_progress_indicator(GTK_WINDOW(dialog), FALSE);
    gtk_widget_set_sensitive(dialog, TRUE);

    if (G_UNLIKELY(error))
    {
        map_error_show(GTK_WINDOW(dialog), error);
        return;
    }

    map_controller_disable_auto_center(controller);
    map_controller_set_center(controller, p, -1);

    gtk_widget_destroy(dialog);
}

static void
on_goto_address_response(GtkWidget *dialog, gint response, GtkEntry *txt_addr)
{
    MapController *controller = map_controller_get_instance();
    MapRouter *router;
    GtkWidget **p_dialog;

    if (response != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    gtk_widget_set_sensitive(dialog, FALSE);
    hildon_gtk_window_set_progress_indicator(GTK_WINDOW(dialog), TRUE);

    /* weak pointer trick to prevent crashes if the callback is invoked
     * after the dialog is destroyed. */
    p_dialog = g_slice_new(GtkWidget *);
    *p_dialog = dialog;
    g_object_add_weak_pointer(G_OBJECT(dialog), (gpointer)p_dialog);

    router = map_controller_get_default_router(controller);
    map_router_geocode(router, gtk_entry_get_text(txt_addr),
                       (MapRouterGeocodeCb)geocode_cb, p_dialog);
}

static gboolean
menu_cb_view_goto_address(GtkMenuItem *item)
{
    GtkWidget *dialog;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *txt_addr;

    GtkEntryCompletion *comp;
    dialog = gtk_dialog_new_with_buttons(_("Go to Address"),
            GTK_WINDOW(_window), GTK_DIALOG_MODAL,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
            GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
            NULL);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
            table = gtk_table_new(2, 3, FALSE), TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(table),
            label = gtk_label_new(_("Address")),
            0, 1, 0, 1, GTK_FILL, 0, 2, 4);
    gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);

    gtk_table_attach(GTK_TABLE(table),
            txt_addr = gtk_entry_new(),
            1, 2, 0, 1, GTK_FILL, 0, 2, 4);
    gtk_entry_set_width_chars(GTK_ENTRY(txt_addr), 25);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);

    /* Set up auto-completion. */
    comp = gtk_entry_completion_new();
    gtk_entry_completion_set_model(comp, GTK_TREE_MODEL(_loc_model));
    gtk_entry_completion_set_text_column(comp, 0);
    gtk_entry_set_completion(GTK_ENTRY(txt_addr), comp);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_goto_address_response), txt_addr);

    gtk_widget_show_all(dialog);

    return TRUE;
}

static gboolean
menu_cb_view_goto_gps(GtkMenuItem *item)
{
    MapController *controller = map_controller_get_instance();

    map_controller_disable_auto_center(controller);
    map_controller_set_center(controller, _pos.unit, -1);

    return TRUE;
}

gboolean
menu_cb_view_goto_nextway(GtkMenuItem *item)
{
    WayPoint *next_way;
    MapController *controller = map_controller_get_instance();

    next_way = path_get_next_way();

    if(next_way && next_way->point->unit.y)
    {
        map_controller_disable_auto_center(controller);
        map_controller_set_center(controller, next_way->point->unit, -1);
    }
    else
    {
        MACRO_BANNER_SHOW_INFO(_window, _("There is no next waypoint."));
    }

    return TRUE;
}

gboolean
menu_cb_view_goto_nearpoi(GtkMenuItem *item)
{
    if(_poi_enabled)
    {
        PoiInfo poi;
        gchar *banner;

        if ((_center_mode > 0 ? get_nearest_poi(&_pos.unit, &poi)
                    : get_nearest_poi(&_center, &poi) ))
        {
            /* Auto-Center is enabled - use the GPS position. */
            MapPoint unit;
            MapController *controller = map_controller_get_instance();
            latlon2unit(poi.lat, poi.lon, unit.x, unit.y);
            banner = g_strdup_printf("%s (%s)", poi.label, poi.clabel);
            MACRO_BANNER_SHOW_INFO(_window, banner);
            g_free(banner);
            g_free(poi.label);
            g_free(poi.desc);
            g_free(poi.clabel);

            map_controller_disable_auto_center(controller);
            map_controller_set_center(controller, unit, -1);
        }
        else
        {
            MACRO_BANNER_SHOW_INFO(_window, _("No POIs found."));
            /* Auto-Center is disabled - use the view center. */
        }
    }

    return TRUE;
}

/****************************************************************************
 * BELOW: GPS MENU **********************************************************
 ****************************************************************************/

static gboolean
menu_cb_gps_details(GtkMenuItem *item)
{
    gps_details();

    return TRUE;
}

/****************************************************************************
 * ABOVE: GPS MENU **********************************************************
 ****************************************************************************/

static gboolean
menu_cb_settings(GtkMenuItem *item)
{
    settings_dialog();

    return TRUE;
}

static gboolean
menu_cb_about(GtkMenuItem *item)
{
    he_about_dialog_present(GTK_WINDOW(_window),
                            "Mapper", "maemo-mapper", VERSION,
                            _("Geographical mapping and driving directions"),
                            _("This application is free software.\n"
                              "Support its development by contributing code,\n"
                              "accurate bug reporting or donating money.\n"
                              "2009-2010 Alberto Mardegan\n"
                              "2009-2010 Max Lapan\n"
                              "2006-2009 John Costigan"),
                            NULL,
                            "https://garage.maemo.org/tracker/?group_id=29",
                            "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=DLKNAWDRW4WVG");

    return TRUE;
}



void
menu_maps_remove_repos()
{
    GList *child;

    /* Delete one menu item for each repo. */
    while ((child = gtk_container_get_children(GTK_CONTAINER(_menu_maps_submenu))))
        gtk_widget_destroy(child->data);

    menu_layers_remove_repos ();
}


void
menu_layers_remove_repos()
{
    GList *child;

    /* Delete one menu item for each repo. */
    while ((child = gtk_container_get_children(GTK_CONTAINER(_menu_layers_submenu))))
        gtk_widget_destroy (child->data);
}


void
menu_maps_add_repos()
{
    GList *curr, *repo_list = map_controller_get_repo_list(map_controller_get_instance());
    GtkWidget *last_repo = NULL;
    DEBUG("");
    Repository *cur_repo = map_controller_get_repository(map_controller_get_instance());

    for(curr = g_list_last(repo_list); curr; curr = curr->prev)
    {
        Repository *rd = (Repository*)curr->data;
        GtkWidget *menu_item;
        if(last_repo)
            gtk_menu_prepend(_menu_maps_submenu, menu_item
                    = gtk_radio_menu_item_new_with_label_from_widget(
                        GTK_RADIO_MENU_ITEM(last_repo), rd->name));
        else
        {
            gtk_menu_prepend(_menu_maps_submenu, menu_item
                    = gtk_radio_menu_item_new_with_label(NULL, rd->name));
            last_repo = menu_item;
        }
        gtk_check_menu_item_set_active(
                GTK_CHECK_MENU_ITEM(menu_item), rd == cur_repo);
        if (rd->menu_item)
            gtk_widget_destroy(rd->menu_item);
        rd->menu_item = menu_item;
    }

    /* Add signals (must be after entire menu is built). */
    {
        GList *currmi = gtk_container_get_children(
                GTK_CONTAINER(_menu_maps_submenu));
        for(curr = repo_list; curr; curr = curr->next, currmi = currmi->next)
        {
            g_signal_connect(G_OBJECT(currmi->data), "activate",
                             G_CALLBACK(menu_cb_maps_select), curr->data);
        }
    }

    gtk_widget_show_all(_menu_maps_submenu);
    menu_layers_add_repos ();
}


void
menu_layers_add_repos()
{
    GList *curr;

    DEBUG("");

    menu_layers_remove_repos();

    for(curr = map_controller_get_repo_list(map_controller_get_instance()); curr; curr = curr->next)
    {
        Repository* rd = (Repository*)curr->data;
        GtkWidget *item, *submenu = NULL, *layer_item;
        gchar *title;
        gint i;
        RepositoryLayer *repo_layer;

        if (rd->layers) {
            switch (rd->layers->len) {
            case 0:
                break;

            case 1:
                repo_layer = g_ptr_array_index(rd->layers, 0);
                title = g_malloc(strlen(rd->name) + strlen(repo_layer->ts->name) + 3);
                sprintf (title, "%s[%s]", rd->name, repo_layer->ts->name);
                gtk_menu_append(_menu_layers_submenu, layer_item = gtk_check_menu_item_new_with_label(title));
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (layer_item), repo_layer->visible);
                g_signal_connect(G_OBJECT(layer_item), "toggled", G_CALLBACK(menu_cb_layers_toggle), repo_layer);
                break;

            default:
                gtk_menu_append(_menu_layers_submenu, item = gtk_menu_item_new_with_label(rd->name));
                for (i = 0; i < rd->layers->len; i++) {
                    if (!submenu)
                        submenu = gtk_menu_new();
                    repo_layer = g_ptr_array_index(rd->layers, i);
                    gtk_menu_append(submenu, layer_item = gtk_check_menu_item_new_with_label(repo_layer->ts->name));
                    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(layer_item), repo_layer->visible);
                    g_signal_connect(G_OBJECT(layer_item), "toggled", G_CALLBACK(menu_cb_layers_toggle), repo_layer);
                }

                if (submenu)
                    gtk_menu_item_set_submenu(GTK_MENU_ITEM (item), submenu);
            }
        }
    }

    gtk_widget_show_all(_menu_layers_submenu);
}


/**
 * Create the menu items needed for the drop down menu.
 */
void
menu_init()
{
    /* Create needed handles. */
    GtkMenu *menu;
    GtkWidget *submenu;
    GtkWidget *menu_item;

    /* Get the menu of our view. */
    menu = GTK_MENU(gtk_menu_new());

    /* Create the menu items. */

    /* The "POI" submenu. */
    gtk_menu_append(menu, menu_item = _menu_poi_item
            = gtk_menu_item_new_with_label(_("POI")));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
            submenu = gtk_menu_new());
    gtk_menu_append(submenu, _menu_poi_import_item
            = gtk_menu_item_new_with_label(_("Import...")));
    gtk_menu_append(submenu, _menu_poi_download_item
            = gtk_menu_item_new_with_label(_("Download...")));
    gtk_menu_append(submenu, _menu_poi_browse_item
            = gtk_menu_item_new_with_label(_("Browse...")));
    gtk_menu_append(submenu, _menu_poi_categories_item
            = gtk_menu_item_new_with_label(_("Categories...")));

    _menu_layers_submenu = gtk_menu_new();

    /* The "Maps" submenu. */
    gtk_menu_append(menu, menu_item
            = gtk_menu_item_new_with_label(_("Maps")));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
            _menu_maps_submenu = gtk_menu_new());
    gtk_menu_append(_menu_maps_submenu, gtk_separator_menu_item_new());
    gtk_menu_append(_menu_maps_submenu, _menu_maps_mapman_item
            = gtk_menu_item_new_with_label(_("Manage Maps...")));
    gtk_menu_append(_menu_maps_submenu, _menu_maps_auto_download_item
            = gtk_check_menu_item_new_with_label(_("Auto-Download")));
    gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(_menu_maps_auto_download_item),_auto_download);
    gtk_menu_append(_menu_maps_submenu, gtk_separator_menu_item_new());
    gtk_menu_append(_menu_maps_submenu, _menu_maps_repoman_item
            = gtk_menu_item_new_with_label(_("Manage Repositories...")));
    gtk_menu_append(_menu_maps_submenu, _menu_maps_tile_sources_item
            = gtk_menu_item_new_with_label(_("Manage Tiles/Layers...")));
    menu_maps_add_repos();

    gtk_menu_append(menu, gtk_separator_menu_item_new());

    /* The "Show" submenu. */
    gtk_menu_append(menu, menu_item
            = gtk_menu_item_new_with_label(_("Show")));
    g_signal_connect(menu_item, "activate",
                      G_CALLBACK(map_menu_show), NULL);

    /* GPS details */
    gtk_menu_append(menu, menu_item
        = gtk_menu_item_new_with_label(_("GPS Details...")));
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(menu_cb_gps_details), NULL);

    gtk_menu_append(menu, gtk_separator_menu_item_new());

    /* The other menu items. */
    gtk_menu_append(menu, _menu_settings_item
        = gtk_menu_item_new_with_label(_("Settings...")));
    gtk_menu_append(menu, gtk_separator_menu_item_new());
    gtk_menu_append(menu, _menu_about_item
        = gtk_menu_item_new_with_label(_("About...")));

    /* We need to show menu items. */
    gtk_widget_show_all(GTK_WIDGET(menu));

    hildon_window_set_menu(HILDON_WINDOW(_window), menu);

    /* Connect the "POI" signals. */
    g_signal_connect(G_OBJECT(_menu_poi_import_item), "activate",
                      G_CALLBACK(menu_cb_poi_import), NULL);
    g_signal_connect(G_OBJECT(_menu_poi_download_item), "activate",
                      G_CALLBACK(menu_cb_poi_download), NULL);
    g_signal_connect(G_OBJECT(_menu_poi_browse_item), "activate",
                      G_CALLBACK(menu_cb_poi_browse), NULL);
    g_signal_connect(G_OBJECT(_menu_poi_categories_item), "activate",
                      G_CALLBACK(menu_cb_poi_categories), NULL);

    /* Connect the "Maps" signals. */
    g_signal_connect(G_OBJECT(_menu_maps_repoman_item), "activate",
                      G_CALLBACK(menu_cb_maps_repoman), NULL);
    g_signal_connect(G_OBJECT(_menu_maps_tile_sources_item), "activate",
                      G_CALLBACK(menu_cb_maps_tile_sources), NULL);
    g_signal_connect(G_OBJECT(_menu_maps_mapman_item), "activate",
                      G_CALLBACK(menu_cb_maps_mapman), NULL);
    g_signal_connect(G_OBJECT(_menu_maps_auto_download_item), "toggled",
                      G_CALLBACK(menu_cb_maps_auto_download), NULL);

    /* Connect the other menu item signals. */
    g_signal_connect(G_OBJECT(_menu_settings_item), "activate",
                      G_CALLBACK(menu_cb_settings), NULL);
    g_signal_connect(G_OBJECT(_menu_about_item), "activate",
                      G_CALLBACK(menu_cb_about), NULL);
}

void
map_menu_route()
{
    GtkWidget *dialog;
    MapController *controller;
    MapDialog *dlg;
    gint response;
    enum {
        ROUTE_OPEN,
        ROUTE_DOWNLOAD,
        ROUTE_SAVE,
        ROUTE_RESET,
        ROUTE_CLEAR,
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Route"),
                            map_controller_get_main_window(controller),
                            TRUE);
    dlg = (MapDialog *)dialog;

    map_dialog_create_button(dlg, _("Open..."), ROUTE_OPEN);
    map_dialog_create_button(dlg, _("Download..."), ROUTE_DOWNLOAD);
    map_dialog_create_button(dlg, _("Save..."), ROUTE_SAVE);
    map_dialog_create_button(dlg, _("Reset"), ROUTE_RESET);
    map_dialog_create_button(dlg, _("Clear"), ROUTE_CLEAR);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (response) {
    case ROUTE_OPEN:
        menu_cb_route_open(NULL); break;
    case ROUTE_DOWNLOAD:
        menu_cb_route_download(NULL); break;
    case ROUTE_SAVE:
        menu_cb_route_save(NULL); break;
    case ROUTE_RESET:
        menu_cb_route_reset(NULL); break;
    case ROUTE_CLEAR:
        menu_cb_route_clear(NULL); break;
    }
}

void
map_menu_track()
{
    GtkWidget *dialog, *button;
    MapController *controller;
    MapDialog *dlg;
    gint response;
    enum {
        TRACK_OPEN,
        TRACK_SAVE,
        TRACK_INSERT_BREAK,
        TRACK_INSERT_MARK,
        TRACK_CLEAR,
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Track"),
                            map_controller_get_main_window(controller),
                            TRUE);
    dlg = (MapDialog *)dialog;

    map_dialog_create_button(dlg, _("Open..."), TRACK_OPEN);
    map_dialog_create_button(dlg, _("Save..."), TRACK_SAVE);
    map_dialog_create_button(dlg, _("Insert Break"), TRACK_INSERT_BREAK);
    map_dialog_create_button(dlg, _("Insert Mark..."), TRACK_INSERT_MARK);
    map_dialog_create_button(dlg, _("Clear"), TRACK_CLEAR);

    button = gtk_toggle_button_new_with_label(_("Enable Tracking"));
    hildon_gtk_widget_set_theme_size(button, HILDON_SIZE_FINGER_HEIGHT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), _enable_tracking);
    gtk_widget_show(button);
    map_dialog_add_widget(dlg, button);
    g_signal_connect(button, "toggled",
                     G_CALLBACK(on_enable_tracking_toggled), NULL);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (response) {
    case TRACK_OPEN:
        menu_cb_track_open(NULL); break;
    case TRACK_SAVE:
        menu_cb_track_save(NULL); break;
    case TRACK_INSERT_BREAK:
        menu_cb_track_insert_break(NULL); break;
    case TRACK_INSERT_MARK:
        menu_cb_track_insert_mark(NULL); break;
    case TRACK_CLEAR:
        menu_cb_track_clear(NULL); break;
    }
}

void
map_menu_go_to()
{
    GtkWidget *dialog;
    MapController *controller;
    MapDialog *dlg;
    gint response;
    enum {
        GO_TO_LATLON,
        GO_TO_ADDRESS,
        GO_TO_GPS,
        GO_TO_WAYPOINT,
        GO_TO_POI,
    };

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Go to"),
                            map_controller_get_main_window(controller),
                            TRUE);
    dlg = (MapDialog *)dialog;

    map_dialog_create_button(dlg, _("Lat/Lon..."), GO_TO_LATLON);
    map_dialog_create_button(dlg, _("Address..."), GO_TO_ADDRESS);
    map_dialog_create_button(dlg, _("GPS Location"), GO_TO_GPS);
    map_dialog_create_button(dlg, _("Next Waypoint"), GO_TO_WAYPOINT);
    map_dialog_create_button(dlg, _("Nearest POI"), GO_TO_POI);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch (response) {
    case GO_TO_LATLON:
        menu_cb_view_goto_latlon(NULL); break;
    case GO_TO_ADDRESS:
        menu_cb_view_goto_address(NULL); break;
    case GO_TO_GPS:
        menu_cb_view_goto_gps(NULL); break;
    case GO_TO_WAYPOINT:
        menu_cb_view_goto_nextway(NULL); break;
    case GO_TO_POI:
        menu_cb_view_goto_nearpoi(NULL); break;
    }
}

void
map_menu_view()
{
    GtkWidget *dialog;
    GtkWindow *parent;
    MapController *controller;
    GtkBox *vbox;
    HildonTouchSelector *selector;
    GtkWidget *auto_rotate;
    GtkWidget *auto_center;
    gint auto_center_value;
    CenterMode center_mode;

    controller = map_controller_get_instance();
    parent = map_controller_get_main_window(controller);
    dialog = gtk_dialog_new_with_buttons(_("View"), parent, GTK_DIALOG_MODAL,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         NULL);

    vbox = GTK_BOX(GTK_DIALOG(dialog)->vbox);

    /* Auto center */
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(selector, _("Lat/Lon"));
    hildon_touch_selector_append_text(selector, _("Lead"));
    hildon_touch_selector_append_text(selector, _("None"));
    auto_center =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Auto-Center"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    gtk_box_pack_start(GTK_BOX(vbox), auto_center, FALSE, TRUE, 0);

    center_mode = map_controller_get_center_mode(controller);
    if (center_mode == CENTER_LATLON)
        auto_center_value = 0;
    else if (center_mode == CENTER_LEAD)
        auto_center_value = 1;
    else
        auto_center_value = 2;

    hildon_picker_button_set_active(HILDON_PICKER_BUTTON(auto_center),
                                    auto_center_value);

    /* Auto rotation */
    auto_rotate = gtk_toggle_button_new_with_label(_("Auto-Rotate"));
    hildon_gtk_widget_set_theme_size(auto_rotate, HILDON_SIZE_FINGER_HEIGHT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_rotate),
                                 _center_rotate);
    gtk_box_pack_start(GTK_BOX(vbox), auto_rotate, FALSE, TRUE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        auto_center_value =
            hildon_picker_button_get_active(HILDON_PICKER_BUTTON(auto_center));
        if (auto_center_value == 0)
            center_mode = CENTER_LATLON;
        else if (auto_center_value == 1)
            center_mode = CENTER_LEAD;
        else
            center_mode = CENTER_MANUAL;
        map_controller_set_center_mode(controller, center_mode);

        map_controller_set_auto_rotate(controller,
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_rotate)));
    }
    gtk_widget_destroy(dialog);
}

void
map_menu_show()
{
    GtkWidget *dialog;
    GtkWidget *zoom, *scale, *compass, *routes, *tracks, *velocity, *poi;
    GtkWidget *gps_info;
    MapController *controller;
    MapDialog *dlg;

    controller = map_controller_get_instance();
    dialog = map_dialog_new(_("Show"),
                            map_controller_get_main_window(controller),
                            TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
    dlg = (MapDialog *)dialog;

    zoom = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(zoom), _("Zoom Level"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(zoom),
                                   map_controller_get_show_zoom(controller));
    map_dialog_add_widget(dlg, zoom);

    scale = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(scale), _("Scale"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(scale),
                                   map_controller_get_show_scale(controller));
    map_dialog_add_widget(dlg, scale);

    compass = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(compass), _("Compass Rose"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(compass),
                                   map_controller_get_show_compass(controller));
    map_dialog_add_widget(dlg, compass);

    routes = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(routes), _("Route"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(routes),
                                   map_controller_get_show_routes(controller));
    map_dialog_add_widget(dlg, routes);

    tracks = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(tracks), _("Track"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(tracks),
                                   map_controller_get_show_tracks(controller));
    map_dialog_add_widget(dlg, tracks);

    velocity = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(velocity), _("Velocity Vector"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(velocity),
        map_controller_get_show_velocity(controller));
    map_dialog_add_widget(dlg, velocity);

    poi = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(poi), _("POI"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(poi),
                                   map_controller_get_show_poi(controller));
    map_dialog_add_widget(dlg, poi);

    gps_info = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(gps_info), _("GPS status"));
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(gps_info),
        map_controller_get_show_gps_info(controller));
    map_dialog_add_widget(dlg, gps_info);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        map_controller_set_show_zoom(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(zoom)));

        map_controller_set_show_scale(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(scale)));

        map_controller_set_show_compass(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(compass)));

        map_controller_set_show_routes(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(routes)));

        map_controller_set_show_tracks(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(tracks)));

        map_controller_set_show_velocity(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(velocity)));

        map_controller_set_show_poi(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(poi)));

        map_controller_set_show_gps_info(controller,
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(gps_info)));
    }
    gtk_widget_destroy(dialog);
}

