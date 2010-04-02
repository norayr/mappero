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

void map_path_track_update(const MapGpsData *gps);
void track_clear(void);
void track_insert_break(gboolean temporary);

void map_path_route_step(const MapGpsData *gps, gboolean newly_fixed);
void path_reset_route(void);

void cancel_autoroute(void);
gboolean autoroute_enabled(void);

WayPoint * find_nearest_waypoint(const MapPoint *p);

gboolean route_download(gchar *to);
void route_add_way_dialog(const MapPoint *p);

WayPoint* path_get_next_way(void);
guint map_path_get_duration(const Path *path);

void path_init(void);
void path_destroy(void);

void map_path_init(Path *path);
void map_path_unset(Path *path);

#define map_path_first(path) ((path)->head)
#define map_path_next(path, point) ((point) + 1)
#define map_path_last(path) ((path)->tail)
#define map_path_len(path) ((path)->tail - (path)->head + 1)

/* must be terminated with map_path_append_point_end() */
static inline Point *
map_path_append_point_fast(Path *path, const Point *p)
{
    if (++(path->tail) == path->cap)
        path_resize(path, path->cap - path->head + ARRAY_CHUNK_SIZE);
    *path->tail = *p;
    return path->tail;
}

void map_path_append_point_end(Path *path);

static inline Point *
map_path_append_point(Path *path, const Point *p)
{
    Point *p_in_path;
    p_in_path = map_path_append_point_fast(path, p);
    map_path_append_point_end(path);
    return p_in_path;
}

static inline WayPoint *
map_path_make_waypoint(Path *path, const Point *p, gchar *desc)
{
    if (++(path->wtail) == path->wcap)
        path_wresize(path, path->wcap - path->whead + ARRAY_CHUNK_SIZE);
    path->wtail->point = (Point *)p;
    path->wtail->desc = desc;
    return path->wtail;
}

/* Appends a point to @path, and creates a WayPoint if @desc is not %NULL */
static inline Point *
map_path_append_point_with_desc(Path *path, const Point *p, const gchar *desc)
{
    Point *p_in_path;
    p_in_path = map_path_append_point_fast(path, p);
    if (desc)
        map_path_make_waypoint(path, path->tail, g_strdup(desc));
    return p_in_path;
}

void map_path_optimize(Path *path);
void map_path_calculate_distances(Path *path);

typedef enum {
    MAP_PATH_MERGE_POLICY_REPLACE = 0,
    MAP_PATH_MERGE_POLICY_APPEND,
    MAP_PATH_MERGE_POLICY_PREPEND,
} MapPathMergePolicy;

void map_path_merge(Path *src_path, Path *dest_path, MapPathMergePolicy policy);

void map_path_append_unit(Path *path, const MapPoint *p);
void map_path_append_null(Path *path);

#endif /* ifndef MAEMO_MAPPER_PATH_H */
