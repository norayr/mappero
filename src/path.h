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

typedef struct {
    const Path *path;
    GList *line;
} MapLineIter;

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

#define map_path_first(path) ((path)->_head)
#define map_path_next(path, point) ((point) + 1)
#define map_path_prev(path, point) ((point) - 1)
#define map_path_end(path) ((path)->_tail)
#define map_path_last(path) ((path)->_tail - 1)
#define map_path_len(path) ((path)->_tail - (path)->_head)

void map_path_line_iter_first(const Path *path, MapLineIter *iter);
void map_path_line_iter_last(const Path *path, MapLineIter *iter);
void map_path_line_iter_from_point(const Path *path, const Point *point,
                                   MapLineIter *iter);
gboolean map_path_line_iter_next(MapLineIter *iter);
Point *map_path_line_first(const MapLineIter *iter);
gint map_path_line_len(const MapLineIter *iter);

/* must be terminated with map_path_append_point_end() */
static inline Point *
map_path_append_point_fast(Path *path, const Point *p)
{
    if (path->_tail == path->_cap)
        path_resize(path, path->_cap - path->_head + ARRAY_CHUNK_SIZE);
    *path->_tail = *p;
    return ++path->_tail;
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
map_path_make_waypoint_full(Path *path, const Point *p, MapDirection dir,
                            gchar *desc)
{
    if (++(path->wtail) == path->wcap)
        path_wresize(path, path->wcap - path->whead + ARRAY_CHUNK_SIZE);
    path->wtail->point = (Point *)p;
    path->wtail->dir = dir;
    path->wtail->desc = desc;
    return path->wtail;
}

#define map_path_make_waypoint(path, p, desc) \
    map_path_make_waypoint_full(path, p, MAP_DIRECTION_UNKNOWN, desc)

/* Appends a point to @path, and creates a WayPoint if @desc is not %NULL */
static inline Point *
map_path_append_point_with_desc(Path *path, const Point *p, const gchar *desc)
{
    Point *p_in_path;
    p_in_path = map_path_append_point_fast(path, p);
    if (desc)
        map_path_make_waypoint(path, path->_tail, g_strdup(desc));
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
void map_path_remove_range(Path *path, Point *start, Point *end);

Point *map_path_append_unit(Path *path, const MapPoint *p);
gboolean map_path_append_break(Path *path);
gboolean map_path_end_is_break(const Path *path);

#endif /* ifndef MAEMO_MAPPER_PATH_H */
