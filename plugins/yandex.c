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

#include "yandex.h"

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


#define GCONF_YANDEX_KEY_PREFIX GCONF_ROUTER_KEY_PREFIX"/yandex"
#define GCONF_YANDEX_KEY_USE_TRAFFIC GCONF_YANDEX_KEY_PREFIX"/use_traffic"

#define YANDEX_ROUTER_URL \
    "http://mm-proxy.appspot.com/yaroute?from=%s&to=%s&traffic=%d"

static void router_iface_init(MapRouterIface *iface);

G_DEFINE_TYPE_WITH_CODE(MapYandex, map_yandex, G_TYPE_OBJECT,
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
route_download_and_setup(MapPath *path, const gchar *source_url,
                         const gchar *from, const gchar *to,
                         gboolean use_traffic, GError **error)
{
    gchar *from_escaped;
    gchar *to_escaped;
    gchar *buffer;
    GFile *file;
    GInputStream *stream;

    from_escaped = g_uri_escape_string(from, NULL, FALSE);
    to_escaped = g_uri_escape_string(to, NULL, FALSE);
    buffer = g_strdup_printf(source_url, from_escaped, to_escaped, use_traffic ? 1 : 0);
    g_free(from_escaped);
    g_free(to_escaped);

    /* Attempt to download the route from the server. */
    file = g_file_new_for_uri(buffer);
    g_free (buffer);
    stream = (GInputStream *)g_file_read(file, NULL, error);

    if (G_UNLIKELY(error != NULL))
        goto finish;

    /* TODO: remove last parameter, add error */
    if (!map_gpx_path_parse(path, stream, FALSE))
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

finish:
    if (stream)
        g_object_unref(stream);
    g_object_unref(file);
}

static const gchar *
map_yandex_get_name(MapRouter *router)
{
    return "Yandex";
}

static void
map_yandex_run_options_dialog(MapRouter *router, GtkWindow *parent)
{
    MapYandex *yandex = MAP_YANDEX(router);
    GtkWidget *dialog;
    GtkWidget *btn_traffic;

    dialog = map_dialog_new(_("Yandex router options"), parent, TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          H_("wdgt_bd_save"), GTK_RESPONSE_ACCEPT);

    /* Use traffic information. */
    btn_traffic = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(btn_traffic), _("Use traffic information"));
    map_dialog_add_widget(MAP_DIALOG(dialog), btn_traffic);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(btn_traffic),
                                   yandex->use_traffic);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        yandex->use_traffic =
            hildon_check_button_get_active(HILDON_CHECK_BUTTON(btn_traffic));
    }

    gtk_widget_destroy(dialog);
}

static void
map_yandex_calculate_route(MapRouter *router, const MapRouterQuery *query,
                           MapRouterCalculateRouteCb callback,
                           gpointer user_data)
{
    MapYandex *yandex = MAP_YANDEX(router);
    gchar buf_from[64], buf_to[64];
    const gchar *from, *to;
    GError *error = NULL;
    MapPath path;

    from = get_address(&query->from, buf_from, sizeof(buf_from));
    to = get_address(&query->to, buf_to, sizeof(buf_to));

    map_path_init(&path);
    route_download_and_setup(&path, YANDEX_ROUTER_URL, from, to, yandex->use_traffic, &error);
    if (!error)
    {
        callback(router, &path, NULL, user_data);
    }
    else
    {
        callback(router, NULL, error, user_data);
        map_path_unset(&path);
        g_error_free(error);
    }
}

static void
map_yandex_load_options(MapRouter *router, GConfClient *gconf_client)
{
    MapYandex *yandex = MAP_YANDEX(router);

    yandex->use_traffic = gconf_client_get_bool(gconf_client, GCONF_YANDEX_KEY_USE_TRAFFIC, NULL);
}

static void
map_yandex_save_options(MapRouter *router, GConfClient *gconf_client)
{
    MapYandex *yandex = MAP_YANDEX(router);

    gconf_client_set_bool(gconf_client, GCONF_YANDEX_KEY_USE_TRAFFIC, yandex->use_traffic, NULL);
}

static void
router_iface_init(MapRouterIface *iface)
{
    iface->get_name = map_yandex_get_name;
    iface->run_options_dialog = map_yandex_run_options_dialog;
    iface->calculate_route = map_yandex_calculate_route;
    iface->load_options = map_yandex_load_options;
    iface->save_options = map_yandex_save_options;
}

static void
map_yandex_init(MapYandex *yandex)
{
}

static void
map_yandex_class_init(MapYandexClass *klass)
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
    return g_object_new(MAP_TYPE_YANDEX, NULL);
}

