/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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
#   include "config.h"
#endif

#include "yandex.h"

#include "data.h"
#include "defines.h"
#include "dialog.h"
#include "error.h"
#include "gpx.h"
#include "path.h"

#include <gconf/gconf-client.h>
#include <hildon/hildon-check-button.h>
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
        unit2latlon(loc->point.x, loc->point.y, lat, lon);
        snprintf(buffer, len, "%.06f, %0.6f", lat, lon);
        return buffer;
    }
}

static void
route_download_and_setup(Path *path, const gchar *source_url,
                         const gchar *from, const gchar *to,
                         gboolean use_traffic, GError **error)
{
    gchar *from_escaped;
    gchar *to_escaped;
    gchar *buffer;
    gchar *bytes = NULL;
    gint size;
    GnomeVFSResult vfs_result;

    from_escaped = gnome_vfs_escape_string(from);
    to_escaped = gnome_vfs_escape_string(to);
    buffer = g_strdup_printf(source_url, from_escaped, to_escaped, use_traffic ? 1 : 0);
    g_free(from_escaped);
    g_free(to_escaped);

    /* Attempt to download the route from the server. */
    vfs_result = gnome_vfs_read_entire_file(buffer, &size, &bytes);
    g_free (buffer);

    if (vfs_result != GNOME_VFS_OK)
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_NETWORK,
                    gnome_vfs_result_to_string(vfs_result));
        goto finish;
    }

    if (strncmp(bytes, "<?xml", 5))
    {
        /* Not an XML document - must be bad locations. */
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

    /* TODO: remove last parameter, add error */
    gpx_path_parse(path, bytes, size, FALSE);

finish:
    g_free(bytes);
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
    Path path;

    from = get_address(&query->from, buf_from, sizeof(buf_from));
    to = get_address(&query->to, buf_to, sizeof(buf_to));

    MACRO_PATH_INIT(path);
    route_download_and_setup(&path, YANDEX_ROUTER_URL, from, to, yandex->use_traffic, &error);
    if (!error)
    {
        callback(router, &path, NULL, user_data);
    }
    else
    {
        callback(router, NULL, error, user_data);
        MACRO_PATH_FREE(path);
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

