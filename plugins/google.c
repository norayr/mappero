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
#include <hildon/hildon-picker-button.h>
#include <mappero/error.h>
#include <mappero/kml.h>
#include <mappero/path.h>
#include <mappero/viewer.h>
#include <mappero-extras/dialog.h>
#include <math.h>
#include <string.h>

#define GCONF_GOOGLE_KEY_PREFIX GCONF_ROUTER_KEY_PREFIX"/google"
#define GCONF_GOOGLE_KEY_ROUTE_TYPE GCONF_GOOGLE_KEY_PREFIX"/route_type"
#define GCONF_GOOGLE_KEY_AVOID_HIGHWAYS GCONF_GOOGLE_KEY_PREFIX"/avoid_highways"
#define GCONF_GOOGLE_KEY_AVOID_TOLLS GCONF_GOOGLE_KEY_PREFIX"/avoid_tolls"

#define GOOGLE_ROUTER_URL \
    "http://maps.google.com/maps?saddr=%s&daddr=%s&output=kml"

static void router_iface_init(MapRouterIface *iface);

G_DEFINE_TYPE_WITH_CODE(MapGoogle, map_google, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MAP_TYPE_ROUTER,
                                              router_iface_init));

typedef struct {
    GtkWidget *btn_highways;
    GtkWidget *btn_tolls;
} DialogData;

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
route_download_by_url(MapGoogle *self, MapPath *path, const gchar *url,
                      GtkWindow *parent, GError **error)
{
    GFile *file;
    GInputStream *stream;

    /* Attempt to download the route from the server. */
    file = g_file_new_for_uri(url);
    g_debug("uri: %s", url);
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

static void
route_download_and_setup(MapGoogle *self, MapPath *path,
                         const gchar *from, const gchar *to, GtkWindow *parent,
                         GError **error)
{
    gchar *from_escaped;
    gchar *to_escaped;
    gchar *query;
    GString *string;
    const gchar *dirflg = NULL;
    MapViewer *viewer = map_viewer_get_default();
    MapPoint center;
    MapGeo c_lat, c_lon;

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

    map_viewer_get_center(viewer, &center);
    map_unit2latlon(center.x, center.y, c_lat, c_lon);
    g_string_append_printf(string, "&sll=%.6f,%.6f", c_lat, c_lon);

    query = g_string_free(string, FALSE);

    /* Attempt to download the route from the server. */
    route_download_by_url(self, path, query, parent, error);
    g_free(query);
}

static const gchar *
map_google_get_name(MapRouter *router)
{
    return "Google";
}

static void
on_transport_changed(HildonTouchSelector *selector, gint column,
                     DialogData *dd)
{
    MapGoogleRouteType route_type;
    gboolean car_enabled;

    route_type = hildon_touch_selector_get_active(selector, 0);
    car_enabled = (route_type == MAP_GOOGLE_ROUTE_TYPE_CAR);
    gtk_widget_set_sensitive(dd->btn_highways, car_enabled);
    gtk_widget_set_sensitive(dd->btn_tolls, car_enabled);
}

static void
map_google_run_options_dialog(MapRouter *router, GtkWindow *parent)
{
    MapGoogle *google = MAP_GOOGLE(router);
    GtkWidget *dialog;
    GtkWidget *widget;
    HildonTouchSelector *transport;
    DialogData dd;

    dialog = map_dialog_new(_("Google router options"), parent, TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          H_("wdgt_bd_save"), GTK_RESPONSE_ACCEPT);

    /* route type */
    transport = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(transport, _("car"));
    hildon_touch_selector_append_text(transport, _("walking"));
    hildon_touch_selector_append_text(transport, _("bike"));
    hildon_touch_selector_set_active(transport, 0, google->route_type);
    g_signal_connect(transport, "changed",
                     G_CALLBACK(on_transport_changed), &dd);

    widget =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Route type"),
                     "touch-selector", transport,
                     "xalign", 0.0,
                     NULL);
    map_dialog_add_widget(MAP_DIALOG(dialog), widget);

    /* Avoid Highways. */
    dd.btn_highways = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(dd.btn_highways), _("Avoid Highways"));
    map_dialog_add_widget(MAP_DIALOG(dialog), dd.btn_highways);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(dd.btn_highways),
                                   google->avoid_highways);

    /* Avoid tolls */
    dd.btn_tolls = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(dd.btn_tolls), _("Avoid tolls"));
    map_dialog_add_widget(MAP_DIALOG(dialog), dd.btn_tolls);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(dd.btn_tolls),
                                   google->avoid_tolls);

    /* check sensitivity of car controls */
    on_transport_changed(transport, 0, &dd);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        google->route_type = hildon_touch_selector_get_active(transport, 0);

        google->avoid_highways =
            hildon_check_button_get_active(
                HILDON_CHECK_BUTTON(dd.btn_highways));

        google->avoid_tolls =
            hildon_check_button_get_active(
                HILDON_CHECK_BUTTON(dd.btn_tolls));
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
    route_download_and_setup(google, &path, from, to, query->parent, &error);
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

    google->route_type =
        gconf_client_get_int(gconf_client, GCONF_GOOGLE_KEY_ROUTE_TYPE, NULL);
    google->avoid_highways =
        gconf_client_get_bool(gconf_client, GCONF_GOOGLE_KEY_AVOID_HIGHWAYS,
                              NULL);
    google->avoid_tolls =
        gconf_client_get_bool(gconf_client, GCONF_GOOGLE_KEY_AVOID_TOLLS,
                              NULL);
}

static void
map_google_save_options(MapRouter *router, GConfClient *gconf_client)
{
    MapGoogle *google = MAP_GOOGLE(router);

    gconf_client_set_int(gconf_client, GCONF_GOOGLE_KEY_ROUTE_TYPE,
                         google->route_type, NULL);
    gconf_client_set_bool(gconf_client, GCONF_GOOGLE_KEY_AVOID_HIGHWAYS,
                          google->avoid_highways, NULL);
    gconf_client_set_bool(gconf_client, GCONF_GOOGLE_KEY_AVOID_TOLLS,
                          google->avoid_tolls, NULL);
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

