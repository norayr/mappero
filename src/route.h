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

#ifndef MAP_ROUTE_H
#define MAP_ROUTE_H

#include "defines.h"
#include "types.h"
#include "path.h"

typedef struct _MapRoute MapRoute;

extern MapRoute *_p_route;

#define map_route_get() _p_route
#define map_route_get_path() ((MapPath *)_p_route)

void map_route_clear(void);
void map_route_destroy(void);

#define map_route_exists() \
    (map_path_len(map_route_get_path()) > 0)

void map_route_path_changed(void);

MapPathWayPoint *map_route_get_next_waypoint();
gfloat map_route_get_distance_to_next_waypoint();

/* TODO: rename and review these functions */
void path_save_route_to_db(void);
void route_find_nearest_point(void);
gboolean route_calc_distance_to(const MapPathPoint *point, gfloat *distance);
gboolean route_show_distance_to(MapPathPoint *point);

void map_path_route_step(const MapGpsData *gps, gboolean newly_fixed);
void path_reset_route(void);

void cancel_autoroute(void);
gboolean autoroute_enabled(void);

MapPathWayPoint * find_nearest_waypoint(const MapPoint *p);

gboolean route_download(gchar *to);
void route_add_way_dialog(const MapPoint *p);

void map_route_take_path(MapPath *path, MapPathMergePolicy policy);

#endif /* MAP_ROUTE_H */
