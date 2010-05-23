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
 * along with Maemo Mapper.  If not, see <http://www.gnu.org/licenses/>.
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
typedef struct _Point Point;
struct _Point {
    MapPoint unit;
    time_t time;
    gchar zoom; /* zoom level at which this point becomes visible */
    gint16 altitude;
    gfloat distance; /* distance from previous point */
};

/** A WayPoint, which is a Point with a description. */
typedef struct _WayPoint WayPoint;
struct _WayPoint {
    Point *point;
    MapDirection dir;
    gchar *desc;
};

/** A Path is a set of PathPoints and WayPoints. */
typedef struct _Path Path;
struct _Path {
    Point *_head; /* points to first element in array; NULL if empty. */
    Point *_tail; /* points to last element in array. */
    Point *_cap; /* points after last slot in array. */
    WayPoint *whead; /* points to first element in array; NULL if empty. */
    WayPoint *wtail; /* points to last element in array. */
    WayPoint *wcap; /* points after last slot in array. */
    GList *_lines; /* MapLine elements */
    gfloat length; /* length of the path, in metres */
    gfloat last_lat; /* coordinates of the last point */
    gfloat last_lon;
    gint points_with_distance; /* number of points with distance computed */
    gint points_optimized;
    gint first_unsaved;
};

typedef struct {
    const Path *path;
    GList *line;
} MapLineIter;

void path_resize(Path *path, gint size);
void path_wresize(Path *path, gint wsize);

void map_path_track_update(const MapGpsData *gps);
void track_clear(void);
void track_insert_break(gboolean temporary);

guint map_path_get_duration(const Path *path);

void path_init(void);
void path_init_late(void);
void path_destroy(void);

void map_path_save_route(Path *path);

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
    return path->_tail++;
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
map_path_append_point_with_desc(Path *path, const Point *p, const gchar *desc,
                                MapDirection dir)
{
    Point *p_in_path;
    p_in_path = map_path_append_point_fast(path, p);
    if (desc)
        map_path_make_waypoint_full(path, p_in_path, dir, g_strdup(desc));
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

typedef struct {
    gint p_near; /* index of the closest point */
    gint wp_next; /* index of the next waypoint */
    /* internal: */
    gint64 dist_squared_near; /* "distance" to p_near */
    gint64 dist_squared_after_near; /* "distance" to the one after p_near */
} MapRouteNearInfo;

gboolean map_path_update_near_info(const Path *path, const MapPoint *p,
                                   MapRouteNearInfo *ni, gboolean local);

#endif /* MAP_PATH_H */
