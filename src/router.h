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

#ifndef MAP_ROUTER_H
#define MAP_ROUTER_H

#include "types.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_ROUTER         (map_router_get_type())
#define MAP_ROUTER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), MAP_TYPE_ROUTER, MapRouter))
#define MAP_IS_ROUTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), MAP_TYPE_ROUTER))
#define MAP_ROUTER_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), MAP_TYPE_ROUTER, MapRouterIface))

typedef struct _MapRouter MapRouter;
typedef struct _MapRouterIface MapRouterIface;

typedef struct {
    MapLocation from;
    MapLocation to;
} MapRouterQuery;

typedef void (*MapRouterCalculateRouteCb)(MapRouter *router, Path *path,
                                          const GError *error,
                                          gpointer user_data);
typedef void (*MapRouterGeocodeCb)(MapRouter *router, MapPoint point,
                                   const GError *error, gpointer user_data);

struct _MapRouterIface
{
    GTypeInterface parent;

    const gchar *(*get_name)(MapRouter *router);
    void (*run_options_dialog)(MapRouter *router, GtkWindow *parent);
    void (*calculate_route)(MapRouter *router, const MapRouterQuery *query,
                            MapRouterCalculateRouteCb callback,
                            gpointer user_data);
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

G_END_DECLS
#endif /* MAP_ROUTER_H */