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
#include "router.h"

#include <string.h>

typedef struct {
    MapRouterGeocodeCb callback;
    gpointer user_data;
} MapGeocodeData;

GType
map_router_get_type(void)
{
    static gsize once = 0;
    static GType type = 0;

    if (g_once_init_enter(&once))
    {
        static const GTypeInfo info = {
            sizeof(MapRouterIface),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            0, /* instance_size */
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL /* value_table */
        };

        type = g_type_register_static(G_TYPE_INTERFACE, "MapRouter", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);

        g_once_init_leave(&once, 1);
    }

    return type;
}

static void
geocode_from_route_cb(MapRouter *router, Path *path, const GError *error,
                      MapGeocodeData *mgd)
{
    MapPoint point = { 0, 0 };

    if (!error)
    {
        if (map_path_len(path) > 0)
            point = map_path_first(path)->unit;

        map_path_unset(path);
    }

    mgd->callback(router, point, error, mgd->user_data);

    g_slice_free(MapGeocodeData, mgd);
}

static void
geocode_from_route(MapRouter *router, const gchar *address,
                   MapRouterGeocodeCb callback, gpointer user_data)
{
    MapGeocodeData *mgd;
    MapRouterQuery q;

    g_return_if_fail(address != NULL);

    memset(&q, 0, sizeof(q));
    q.from.address = q.to.address = (gchar *)address;

    mgd = g_slice_new0(MapGeocodeData);
    mgd->callback = callback;
    mgd->user_data = user_data;
    map_router_calculate_route(router, &q,
                               (MapRouterCalculateRouteCb)geocode_from_route_cb,
                               mgd);
}

const gchar *
map_router_get_name(MapRouter *router)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_val_if_fail(iface != NULL, FALSE);

    return iface->get_name(router);
}

gboolean
map_router_has_options(MapRouter *router)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_val_if_fail(iface != NULL, FALSE);

    return iface->run_options_dialog != NULL;
}

void
map_router_run_options_dialog(MapRouter *router, GtkWindow *parent)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_if_fail(iface != NULL);
    g_return_if_fail(iface->run_options_dialog != NULL);

    iface->run_options_dialog(router, parent);
}

/**
 * map_router_calculate_route:
 * @router: the #MapRouter
 * @query: a #MapQuery structure holding the query data
 * @callback: callback to be invoked when the operation completes
 * @user_data: user data to be passed to @callback.
 *
 * Computes the route requested by @query.
 */
void
map_router_calculate_route(MapRouter *router, const MapRouterQuery *query,
                           MapRouterCalculateRouteCb callback,
                           gpointer user_data)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_if_fail(iface != NULL);
    g_return_if_fail(iface->calculate_route != NULL);
    g_return_if_fail(query != NULL);
    g_return_if_fail(callback != NULL);

    iface->calculate_route(router, query, callback, user_data);
}

/**
 * map_router_geocode:
 * @router: the #MapRouter
 * @address: the address which needs to be geocoded
 * @callback: callback to be invoked when the operation completes
 * @user_data: user data to be passed to @callback.
 *
 * Gets latitude and longitude for @address. This is currently implemented by
 * calculating a route from @address to @address; in the future, this might be
 * a virtual method for the #MapRouter instances to implement.
 */
void
map_router_geocode(MapRouter *router, const gchar *address,
                   MapRouterGeocodeCb callback, gpointer user_data)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_if_fail(iface != NULL);
    g_return_if_fail(address != NULL);
    g_return_if_fail(callback != NULL);

    if (iface->geocode)
        iface->geocode(router, address, callback, user_data);
    else
        geocode_from_route(router, address, callback, user_data);
}


void
map_router_load_options(MapRouter *router, GConfClient *gconf_client)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_if_fail(iface != NULL);

    if (iface->load_options)
        iface->load_options(router, gconf_client);
}


void
map_router_save_options(MapRouter *router, GConfClient *gconf_client)
{
    MapRouterIface *iface = MAP_ROUTER_GET_IFACE(router);

    g_return_if_fail(iface != NULL);

    if (iface->save_options)
        iface->save_options(router, gconf_client);
}
