/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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
#   include "config.h"
#endif

#include "google.h"

#define H_(String) dgettext("hildon-libs", String)

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <hildon/hildon-check-button.h>
#include <mappero/error.h>
#include <mappero/kml.h>
#include <mappero/path.h>
#include <mappero-extras/dialog.h>
#include <math.h>
#include <string.h>

#define GCONF_GOOGLE_KEY_PREFIX GCONF_ROUTER_KEY_PREFIX"/google"
#define GCONF_GOOGLE_KEY_AVOID_HIGHWAYS GCONF_GOOGLE_KEY_PREFIX"/avoid_highways"

#define GOOGLE_ROUTER_URL \
    "http://maps.google.com/maps?saddr=%s&daddr=%s&output=kml"

static void router_iface_init(MapRouterIface *iface);

G_DEFINE_TYPE_WITH_CODE(MapGoogle, map_google, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MAP_TYPE_ROUTER,
                                              router_iface_init));

static inline const gchar *
get_address(const MapLocation *loc, gchar *buffer, gsize len)
{
    if (loc->address) 
        return loc->address;
    else
    {
        MapGeo lat, lon;
        map_unit2latlon(loc->point.x, loc->point.y, lat, lon);
        snprintf(buffer, len, "%.06f, %0.6f", lat, lon);
        return buffer;
    }
}

static void
route_download_and_setup(MapGoogle *self, MapPath *path,
                         const gchar *from, const gchar *to,
                         GError **error)
{
    gchar *from_escaped;
    gchar *to_escaped;
    gchar *query;
    GFile *file;
    GInputStream *stream;
    GString *string;
    const gchar *dirflg = NULL;

    string = g_string_sized_new(256);
    from_escaped = g_uri_escape_string(from, NULL, FALSE);
    to_escaped = g_uri_escape_string(to, NULL, FALSE);
    g_string_append_printf(string, GOOGLE_ROUTER_URL, from_escaped, to_escaped);
    g_free(from_escaped);
    g_free(to_escaped);

    if (self->route_type == MAP_GOOGLE_ROUTE_TYPE_WALK)
        dirflg = "w";
    else if (self->route_type == MAP_GOOGLE_ROUTE_TYPE_BIKE)
        dirflg = "b";
    else
    {
        static gchar dirflg_buf[3];
        gint i = 0;

        if (self->avoid_highways)
            dirflg_buf[i++] = 'h';
        if (self->avoid_tolls)
            dirflg_buf[i++] = 't';
        dirflg_buf[i] = '\0';
        if (i > 0)
            dirflg = dirflg_buf;
    }
    if (dirflg != NULL)
    {
        g_string_append(string, "&dirflg=");
        g_string_append(string, dirflg);
    }

    query = g_string_free(string, FALSE);

    /* Attempt to download the route from the server. */
    file = g_file_new_for_uri(query);
    g_debug("uri: %s", query);
    g_free (query);
    stream = (GInputStream *)g_file_read(file, NULL, error);

    if (G_UNLIKELY(*error != NULL))
        goto finish;

    if (!map_kml_path_parse(stream, path))
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

    map_path_infer_directions(path);

finish:
    if (stream)
        g_object_unref(stream);
    g_object_unref(file);
}

static const gchar *
map_google_get_name(MapRouter *router)
{
    return "Google";
}

static void
map_google_run_options_dialog(MapRouter *router, GtkWindow *parent)
{
    MapGoogle *google = MAP_GOOGLE(router);
    GtkWidget *dialog;
    GtkWidget *btn_highways;

    dialog = map_dialog_new(_("Google router options"), parent, TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          H_("wdgt_bd_save"), GTK_RESPONSE_ACCEPT);

    /* Avoid Highways. */
    btn_highways = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(btn_highways), _("Avoid Highways"));
    map_dialog_add_widget(MAP_DIALOG(dialog), btn_highways);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(btn_highways),
                                   google->avoid_highways);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        google->avoid_highways =
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(btn_highways));
    }

    gtk_widget_destroy(dialog);
}

static void
map_google_calculate_route(MapRouter *router, const MapRouterQuery *query,
                           MapRouterCalculateRouteCb callback,
                           gpointer user_data)
{
    MapGoogle *google = MAP_GOOGLE(router);
    gchar buf_from[64], buf_to[64];
    const gchar *from, *to;
    GError *error = NULL;
    MapPath path;

    from = get_address(&query->from, buf_from, sizeof(buf_from));
    to = get_address(&query->to, buf_to, sizeof(buf_to));

    map_path_init(&path);
    route_download_and_setup(google, &path, from, to, &error);
    if (!error)
    {
        callback(router, &path, NULL, user_data);
    }
    else
    {
        callback(router, NULL, error, user_data);
        g_error_free(error);
    }
    map_path_unset(&path);
}

static void
map_google_load_options(MapRouter *router, GConfClient *gconf_client)
{
    MapGoogle *google = MAP_GOOGLE(router);

    google->avoid_highways = gconf_client_get_bool(gconf_client, GCONF_GOOGLE_KEY_AVOID_HIGHWAYS, NULL);
}

static void
map_google_save_options(MapRouter *router, GConfClient *gconf_client)
{
    MapGoogle *google = MAP_GOOGLE(router);

    gconf_client_set_bool(gconf_client, GCONF_GOOGLE_KEY_AVOID_HIGHWAYS, google->avoid_highways, NULL);
}

static void
router_iface_init(MapRouterIface *iface)
{
    iface->get_name = map_google_get_name;
    iface->run_options_dialog = map_google_run_options_dialog;
    iface->calculate_route = map_google_calculate_route;
    iface->load_options = map_google_load_options;
    iface->save_options = map_google_save_options;
}

static void
map_google_init(MapGoogle *google)
{
}

static void
map_google_class_init(MapGoogleClass *klass)
{
}

const gchar * const *
map_plugin_list_objects()
{
    static const gchar *ids[] = { "router", NULL };
    return ids;
}

GObject *
map_plugin_get_object(const gchar *id)
{
    g_return_val_if_fail(strcmp(id, "router") == 0, NULL);
    return g_object_new(MAP_TYPE_GOOGLE, NULL);
}

