/*
 * Copyright (C) 2006, 2007 John Costigan.
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

#ifndef MAEMO_MAPPER_PATH_H
#define MAEMO_MAPPER_PATH_H

void path_resize(Path *path, gint size);
void path_wresize(Path *path, gint wsize);

void path_save_route_to_db(void);

void route_find_nearest_point(void);
gboolean route_calc_distance_to(const Point *point, gfloat *distance);
gboolean route_show_distance_to(Point *point);
void route_show_distance_to_next(void);
void route_show_distance_to_last(void);

void track_show_distance_from_last(void);
void track_show_distance_from_first(void);

gboolean track_add(const MapGpsData *gps, gboolean newly_fixed);
void track_clear(void);
void track_insert_break(gboolean temporary);

void map_path_route_step(const MapGpsData *gps, gboolean newly_fixed);
void path_reset_route(void);

void cancel_autoroute(void);
gboolean autoroute_enabled(void);

WayPoint * find_nearest_waypoint(gint unitx, gint unity);

gboolean route_download(gchar *to);
void route_add_way_dialog(gint unitx, gint unity);

WayPoint* path_get_next_way(void);

void path_init(void);
void path_destroy(void);

void map_path_optimize(Path *path);

typedef enum {
    MAP_PATH_MERGE_POLICY_REPLACE = 0,
    MAP_PATH_MERGE_POLICY_APPEND,
    MAP_PATH_MERGE_POLICY_PREPEND,
} MapPathMergePolicy;

void map_path_merge(Path *src_path, Path *dest_path, MapPathMergePolicy policy);

#endif /* ifndef MAEMO_MAPPER_PATH_H */
