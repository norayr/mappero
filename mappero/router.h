/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of libMappero.
 *
 * libMappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libMappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libMappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAP_ROUTER_H
#define MAP_ROUTER_H

#include <mappero/globals.h>
#include <mappero/path.h>

#include <gconf/gconf-client.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
 
G_BEGIN_DECLS

#define GCONF_ROUTER_KEY_PREFIX GCONF_KEY_PREFIX"/routers"

#define MAP_TYPE_ROUTER         (map_router_get_type())
#define MAP_ROUTER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), MAP_TYPE_ROUTER, MapRouter))
#define MAP_IS_ROUTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), MAP_TYPE_ROUTER))
#define MAP_ROUTER_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), MAP_TYPE_ROUTER, MapRouterIface))

typedef struct _MapRouter MapRouter;
typedef struct _MapRouterIface MapRouterIface;

/**
 * MapLocation:
 * Definiton of a location in either geographic coordinates or as an address.
 * If the @address field is not %NULL, it has priority over the coordinates.
 */
typedef struct {
    MapPoint point;
    gchar *address;
} MapLocation;

/**
 * MapRouterQuery:
 * @from: starting location
 * @to: destination
 * @parent: a #GtkWindow to be set as parent for any dialogs the router
 * implementation needs to show. If this is %NULL, then no UI elements must be
 * shown.
 */
typedef struct {
    MapLocation from;
    MapLocation to;
    GtkWindow *parent;
} MapRouterQuery;

typedef void (*MapRouterCalculateRouteCb)(MapRouter *router, MapPath *path,
                                          const GError *error,
                                          gpointer user_data);
typedef void (*MapRouterGeocodeCb)(MapRouter *router, MapPoint point,
                                   const GError *error, gpointer user_data);

struct _MapRouterIface
{
    GTypeInterface parent;

    const gchar *(*get_name)(MapRouter *router);
    void (*save_options)(MapRouter *route, GConfClient *gconf_client);
    void (*load_options)(MapRouter *route, GConfClient *gconf_client);
    void (*run_options_dialog)(MapRouter *router, GtkWindow *parent);
    void (*calculate_route)(MapRouter *router, const MapRouterQuery *query,
                            MapRouterCalculateRouteCb callback,
                            gpointer user_data);
    void (*geocode)(MapRouter *router, const gchar *address,
                    MapRouterGeocodeCb callback, gpointer user_data);
};

GType map_router_get_type (void) G_GNUC_CONST;

const gchar *map_router_get_name(MapRouter *router);

gboolean map_router_has_options(MapRouter *router);
void map_router_run_options_dialog(MapRouter *router, GtkWindow *parent);

void map_router_calculate_route(MapRouter *router,
                                const MapRouterQuery *query,
                                MapRouterCalculateRouteCb callback,
                                gpointer user_data);
void map_router_geocode(MapRouter *router, const gchar *address,
                        MapRouterGeocodeCb callback, gpointer user_data);

void map_router_load_options(MapRouter *router, GConfClient *gconf_client);
void map_router_save_options(MapRouter *router, GConfClient *gconf_client);

G_END_DECLS
#endif /* MAP_ROUTER_H */
