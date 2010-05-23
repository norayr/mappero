/*
 * Copyright (C) 2006, 2007 John Costigan.
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

#ifndef MAP_PATH_H
#define MAP_PATH_H

#include <mappero/globals.h>

#define ARRAY_CHUNK_SIZE (1024)

/* Navigation directions */
typedef enum {
    MAP_DIRECTION_UNKNOWN = 0,
    MAP_DIRECTION_CS, /* continue straight */
    MAP_DIRECTION_TR, /* turn right */
    MAP_DIRECTION_TL,
    MAP_DIRECTION_STR, /* slight turn right */
    MAP_DIRECTION_STL,
    MAP_DIRECTION_EX1, /* first exit */
    MAP_DIRECTION_EX2,
    MAP_DIRECTION_EX3,
    MAP_DIRECTION_LAST,
} MapDirection;

/* definition of a track/route point */
typedef struct {
    MapPoint unit;
    time_t time;
    gchar zoom; /* zoom level at which this point becomes visible */
    gint16 altitude;
    gfloat distance; /* distance from previous point */
} MapPathPoint;

/** A MapPathWayPoint, which is a MapPathPoint with a description. */
typedef struct {
    MapPathPoint *point;
    MapDirection dir;
    gchar *desc;
} MapPathWayPoint;

/** A MapPath is a set of PathPoints and WayPoints. */
typedef struct {
    MapPathPoint *_head; /* points to first element in array; NULL if empty. */
    MapPathPoint *_tail; /* points to last element in array. */
    MapPathPoint *_cap; /* points after last slot in array. */
    MapPathWayPoint *whead; /* first element in array; NULL if empty. */
    MapPathWayPoint *wtail; /* points to last element in array. */
    MapPathWayPoint *wcap; /* points after last slot in array. */
    GList *_lines; /* MapLine elements */
    gfloat length; /* length of the path, in metres */
    gfloat last_lat; /* coordinates of the last point */
    gfloat last_lon;
    gint points_with_distance; /* number of points with distance computed */
    gint points_optimized;
    gint first_unsaved;
} MapPath;

typedef struct {
    const MapPath *path;
    GList *line;
} MapLineIter;

void path_resize(MapPath *path, gint size);
void path_wresize(MapPath *path, gint wsize);

guint map_path_get_duration(const MapPath *path);

void map_path_init(MapPath *path);
void map_path_unset(MapPath *path);

#define map_path_first(path) ((path)->_head)
#define map_path_next(path, point) ((point) + 1)
#define map_path_prev(path, point) ((point) - 1)
#define map_path_end(path) ((path)->_tail)
#define map_path_last(path) ((path)->_tail - 1)
#define map_path_len(path) ((path)->_tail - (path)->_head)

void map_path_line_iter_first(const MapPath *path, MapLineIter *iter);
void map_path_line_iter_last(const MapPath *path, MapLineIter *iter);
void map_path_line_iter_from_point(const MapPath *path,
                                   const MapPathPoint *point,
                                   MapLineIter *iter);
gboolean map_path_line_iter_next(MapLineIter *iter);
MapPathPoint *map_path_line_first(const MapLineIter *iter);
gint map_path_line_len(const MapLineIter *iter);

/* must be terminated with map_path_append_point_end() */
static inline MapPathPoint *
map_path_append_point_fast(MapPath *path, const MapPathPoint *p)
{
    if (path->_tail == path->_cap)
        path_resize(path, path->_cap - path->_head + ARRAY_CHUNK_SIZE);
    *path->_tail = *p;
    return path->_tail++;
}

void map_path_append_point_end(MapPath *path);

static inline MapPathPoint *
map_path_append_point(MapPath *path, const MapPathPoint *p)
{
    MapPathPoint *p_in_path;
    p_in_path = map_path_append_point_fast(path, p);
    map_path_append_point_end(path);
    return p_in_path;
}

static inline MapPathWayPoint *
map_path_make_waypoint_full(MapPath *path, const MapPathPoint *p,
                            MapDirection dir, gchar *desc)
{
    if (++(path->wtail) == path->wcap)
        path_wresize(path, path->wcap - path->whead + ARRAY_CHUNK_SIZE);
    path->wtail->point = (MapPathPoint *)p;
    path->wtail->dir = dir;
    path->wtail->desc = desc;
    return path->wtail;
}

#define map_path_make_waypoint(path, p, desc) \
    map_path_make_waypoint_full(path, p, MAP_DIRECTION_UNKNOWN, desc)

/* Appends a point to @path, and creates a MapPathWayPoint if @desc is not
 * %NULL */
static inline MapPathPoint *
map_path_append_point_with_desc(MapPath *path, const MapPathPoint *p,
                                const gchar *desc, MapDirection dir)
{
    MapPathPoint *p_in_path;
    p_in_path = map_path_append_point_fast(path, p);
    if (desc)
        map_path_make_waypoint_full(path, p_in_path, dir, g_strdup(desc));
    return p_in_path;
}

void map_path_optimize(MapPath *path);
void map_path_calculate_distances(MapPath *path);

typedef enum {
    MAP_PATH_MERGE_POLICY_REPLACE = 0,
    MAP_PATH_MERGE_POLICY_APPEND,
    MAP_PATH_MERGE_POLICY_PREPEND,
} MapPathMergePolicy;

void map_path_merge(MapPath *src_path, MapPath *dest_path,
                    MapPathMergePolicy policy);
void map_path_remove_range(MapPath *path,
                           MapPathPoint *start, MapPathPoint *end);

MapPathPoint *map_path_append_unit(MapPath *path, const MapPoint *p);
gboolean map_path_append_break(MapPath *path);
gboolean map_path_end_is_break(const MapPath *path);

typedef struct {
    gint p_near; /* index of the closest point */
    gint wp_next; /* index of the next waypoint */
    /* internal: */
    gint64 dist_squared_near; /* "distance" to p_near */
    gint64 dist_squared_after_near; /* "distance" to the one after p_near */
} MapRouteNearInfo;

gboolean map_path_update_near_info(const MapPath *path, const MapPoint *p,
                                   MapRouteNearInfo *ni, gboolean local);

#endif /* MAP_PATH_H */
