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
#include <mappero/gpx.h>
#include <mappero/path.h>
#include <mappero-extras/dialog.h>
#include <math.h>
#include <string.h>

#define GCONF_GOOGLE_KEY_PREFIX GCONF_ROUTER_KEY_PREFIX"/google"
#define GCONF_GOOGLE_KEY_AVOID_HIGHWAYS GCONF_GOOGLE_KEY_PREFIX"/avoid_highways"

#define GOOGLE_ROUTER_URL \
    "http://www.gnuite.com/cgi-bin/gpx.cgi?saddr=%s&daddr=%s"

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

/**
 * infer_direction:
 * @text: textual description of a waypoint.
 *
 * Tries to guess a #MapDirection from a text string.
 */
static MapDirection
infer_direction(const gchar *text)
{
    MapDirection dir = MAP_DIRECTION_UNKNOWN;

    if (!text) return dir;

    if (strncmp(text, "Turn ", 5) == 0)
    {
        text += 5;
        if (strncmp(text, "left", 4) == 0)
            dir = MAP_DIRECTION_TL;
        else
            dir = MAP_DIRECTION_TR;
    }
    else if (strncmp(text, "Slight ", 7) == 0)
    {
        text += 7;
        if (strncmp(text, "left", 4) == 0)
            dir = MAP_DIRECTION_STL;
        else
            dir = MAP_DIRECTION_STR;
    }
    else if (strncmp(text, "Continue ", 9) == 0)
        dir = MAP_DIRECTION_CS;
    else
    {
        static const gchar *ordinals[] = { "1st ", "2nd ", "3rd ", NULL };
        gchar *ptr;
        gint i;

        for (i = 0; ordinals[i] != NULL; i++)
        {
            ptr = strstr(text, ordinals[i]);
            if (ptr != NULL) break;
        }

        if (ptr != NULL)
        {
            ptr += strlen(ordinals[i]);
            if (strncmp(ptr, "exit", 4) == 0)
                dir = MAP_DIRECTION_EX1 + i;
            else if (strncmp(ptr, "left", 4) == 0)
                dir = MAP_DIRECTION_TL;
            else
                dir = MAP_DIRECTION_TR;
        }
        else
        {
            /* all heuristics failed, add more here */
        }
    }
    return dir;
}

static void
add_directions(MapPath *path)
{
    MapPathWayPoint *curr;
    for (curr = path->whead; curr != path->wtail; curr++)
    {
        if (curr->dir == MAP_DIRECTION_UNKNOWN)
            curr->dir = infer_direction(curr->desc);
    }
}

static void
route_download_and_setup(MapPath *path, const gchar *source_url,
                         const gchar *from, const gchar *to,
                         gboolean avoid_highways, GError **error)
{
    gchar *from_escaped;
    gchar *to_escaped;
    gchar *buffer;
    GFile *file;
    GInputStream *stream;

    from_escaped = g_uri_escape_string(from, NULL, FALSE);
    to_escaped = g_uri_escape_string(to, NULL, FALSE);
    buffer = g_strdup_printf(source_url, from_escaped, to_escaped);
    g_free(from_escaped);
    g_free(to_escaped);

    if (avoid_highways)
    {
        gchar *old = buffer;
        buffer = g_strconcat(old, "&avoid_highways=on", NULL);
        g_free(old);
    }

    /* Attempt to download the route from the server. */
    file = g_file_new_for_uri(buffer);
    g_debug("uri: %s", buffer);
    g_free (buffer);
    stream = (GInputStream *)g_file_read(file, NULL, error);

    if (G_UNLIKELY(*error != NULL))
        goto finish;

    if (!map_gpx_path_parse(stream, path))
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

    add_directions(path);

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
    route_download_and_setup(&path, GOOGLE_ROUTER_URL, from, to,
                             google->avoid_highways, &error);
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

