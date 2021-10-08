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

#ifndef MAP_REITTIOPAS_H
#define MAP_REITTIOPAS_H

#include <mappero/router.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_REITTIOPAS         (map_reittiopas_get_type())
#define MAP_REITTIOPAS(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), MAP_TYPE_REITTIOPAS, MapReittiopas))
#define MAP_REITTIOPAS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_REITTIOPAS, MapReittiopasClass))
#define MAP_IS_REITTIOPAS(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), MAP_TYPE_REITTIOPAS))
#define MAP_IS_REITTIOPAS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE((k), MAP_TYPE_REITTIOPAS))
#define MAP_REITTIOPAS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), MAP_TYPE_REITTIOPAS, MapReittiopasClass))

typedef struct _MapReittiopas MapReittiopas;
typedef struct _MapReittiopasClass MapReittiopasClass;

typedef enum {
    RO_ROUTE_TYPE_PUBLIC = 0,
    RO_ROUTE_TYPE_CYCLING,
} RoRouteType;

typedef enum {
    RO_TRANSPORT_TYPE_BUS = 0,
    RO_TRANSPORT_TYPE_TRAIN,
    RO_TRANSPORT_TYPE_FERRY,
    RO_TRANSPORT_TYPE_METRO,
    RO_TRANSPORT_TYPE_TRAM,
    RO_TRANSPORT_TYPE_LAST
} RoTransportType;

typedef enum {
    RO_OPTIMIZE_DEFAULT = 0,
    RO_OPTIMIZE_FASTEST,
    RO_OPTIMIZE_LEAST_TRANSFERS,
    RO_OPTIMIZE_LEAST_WALKING,
    RO_OPTIMIZE_LAST
} RoOptimizeGoal;

typedef enum {
    RO_WALKSPEED_SLOW = 1,
    RO_WALKSPEED_NORMAL,
    RO_WALKSPEED_FAST,
    RO_WALKSPEED_RUNNING,
    RO_WALKSPEED_CYCLING,
} RoWalkspeed;

typedef enum {
    RO_CW_OPTIMIZE_CYCLE = 0,
    RO_CW_OPTIMIZE_ASPHALT,
    RO_CW_OPTIMIZE_GRAVEL,
    RO_CW_OPTIMIZE_SHORTEST,
    RO_CW_OPTIMIZE_LAST
} RoCWOptimize;

struct _MapReittiopas
{
    GObject parent;
    RoRouteType route_type;

    /* Public transport options */
    gboolean transport_allowed[RO_TRANSPORT_TYPE_LAST];
    RoOptimizeGoal optimize;
    RoWalkspeed walkspeed;
    gint margin;

    /* cycling/walking options */
    RoCWOptimize cw_optimize;
};

struct _MapReittiopasClass
{
    GObjectClass parent;
};

GType map_reittiopas_get_type(void) G_GNUC_CONST;

G_END_DECLS
#endif /* MAP_REITTIOPAS_H */