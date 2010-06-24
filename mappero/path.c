/*
 * Copyright (C) 2006, 2007 John Costigan.
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

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#define _GNU_SOURCE

#include "debug.h"
#include "path.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
    gint idx_start;
} MapLine;

/* since the MapLine fits in a pointer, we directly store it in the GList
 * data field */
#define MAP_LINE(list)  ((MapLine *)(&(list->data)))
#define MAP_LINE_NEW(index) GINT_TO_POINTER(index)

/**
 * infer_direction:
 * @text: textual description of a waypoint.
 *
 * Tries to guess a #MapDirection from a text string.
 */
static MapDirection
infer_direction(const gchar *text)
{
    MapDirection dir = MAP_DIRECTION_UNKNOWN;

    if (!text) return dir;

    if (strncmp(text, "Turn ", 5) == 0)
    {
        text += 5;
        if (strncmp(text, "left", 4) == 0)
            dir = MAP_DIRECTION_TL;
        else
            dir = MAP_DIRECTION_TR;
    }
    else if (strncmp(text, "Slight ", 7) == 0)
    {
        text += 7;
        if (strncmp(text, "left", 4) == 0)
            dir = MAP_DIRECTION_STL;
        else
            dir = MAP_DIRECTION_STR;
    }
    else if (strncmp(text, "Continue ", 9) == 0)
        dir = MAP_DIRECTION_CS;
    else
    {
        static const gchar *ordinals[] = { "1st ", "2nd ", "3rd ", NULL };
        gchar *ptr;
        gint i;

        for (i = 0; ordinals[i] != NULL; i++)
        {
            ptr = strstr(text, ordinals[i]);
            if (ptr != NULL) break;
        }

        if (ptr != NULL)
        {
            ptr += strlen(ordinals[i]);
            if (strncmp(ptr, "exit", 4) == 0)
                dir = MAP_DIRECTION_EX1 + i;
            else if (strncmp(ptr, "left", 4) == 0)
                dir = MAP_DIRECTION_TL;
            else
                dir = MAP_DIRECTION_TR;
        }
        else
        {
            /* all heuristics failed, add more here */
        }
    }
    return dir;
}

void
map_path_resize(MapPath *path, gint size)
{
    if(path->_head + size != path->_cap)
    {
        MapPathPoint *old_head = path->_head;
        MapPathWayPoint *curr;
        path->_head = g_renew(MapPathPoint, old_head, size);
        path->_cap = path->_head + size;
        if(path->_head != old_head)
        {
            path->_tail = path->_head + (path->_tail - old_head);

            /* Adjust all of the waypoints. */
            for(curr = path->whead - 1; curr++ != path->wtail; )
                curr->point = path->_head + (curr->point - old_head);
        }
    }
}

void
map_path_wresize(MapPath *path, gint wsize)
{
    if(path->whead + wsize != path->wcap)
    {
        MapPathWayPoint *old_whead = path->whead;
        path->whead = g_renew(MapPathWayPoint, old_whead, wsize);
        path->wtail = path->whead + (path->wtail - old_whead);
        path->wcap = path->whead + wsize;
    }
}

void
map_path_calculate_distances(MapPath *path)
{
    MapPathPoint *curr, *first;
    gfloat total = 0;
    MapGeo lat, lon, last_lat, last_lon;
    MapLineIter line;
    gint n_points;

    n_points = map_path_len(path) - path->points_with_distance;
    if (n_points <= 0) return;

#ifdef ENABLE_DEBUG
    struct timespec ts0, ts1;
    long ms_diff;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts0);
#endif

    first = map_path_first(path) + path->points_with_distance;
    map_path_line_iter_from_point(path, first, &line);
    do
    {
        MapPathPoint *start, *end;
        start = map_path_line_first(&line);
        end = start + map_path_line_len(&line);
        if (start < first)
        {
            /* we are in the middle of a line with already some distances
             * computed */
            start = first;
            last_lat = path->last_lat, last_lon = path->last_lon;
        }
        else if (start < end)
        {
            /* the first point of the line is treated specially */
            start->distance = 0;
            map_unit2latlon(start->unit.x, start->unit.y, last_lat, last_lon);
            start++;
        }
        else /* this line is empty */
        {
            last_lat = last_lon = 0;
        }

        for (curr = start; curr < end; curr++)
        {
            map_unit2latlon(curr->unit.x, curr->unit.y, lat, lon);
            curr->distance =
                map_calculate_distance(last_lat, last_lon, lat, lon);
            total += curr->distance;
            last_lat = lat;
            last_lon = lon;
        }
    }
    while (map_path_line_iter_next(&line));

    path->last_lat = last_lat;
    path->last_lon = last_lon;
    path->length += total;
    path->points_with_distance += n_points;

#ifdef ENABLE_DEBUG
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts1);
    ms_diff = (ts1.tv_sec - ts0.tv_sec) * 1000 +
        (ts1.tv_nsec - ts0.tv_nsec) / 1000000;
    DEBUG("%ld ms for %d points", ms_diff, n_points);
#endif
}

/**
 * map_path_find_closest:
 * @path: a #MapPath.
 * @p: the reference #MapPoint.
 * @start: the index of starting #MapPathPoint in @path.
 * @local: whether the search should be local only.
 *
 * Starting from @start, scans @path and returns the point which is closest to
 * @p. If @local is %TRUE, the search stops as soon as the distance is
 * increasing.
 *
 * Returns: the index of the closest #MapPathPoint.
 */
static gint
map_path_find_closest(const MapPath *path, const MapPoint *p, gint start,
                      gboolean local)
{
    MapPathPoint *curr, *near;
    gint64 near_dist_squared;

    if (start >= map_path_len(path)) return 0;

    /* First, set near_dist_squared with the new distance from
     * _near_point. */
    near = map_path_first(path) + start;
    near_dist_squared = DISTANCE_SQUARED(*p, near->unit);

    /* Now, search _route for a closer point.  If quick is TRUE, then we'll
     * only search forward, only as long as we keep finding closer points.
     */
    for (curr = near + 1; curr < map_path_end(path); curr++)
    {
        gint64 dist_squared = DISTANCE_SQUARED(*p, curr->unit);
        if (dist_squared <= near_dist_squared)
        {
            near = curr;
            near_dist_squared = dist_squared;
        }
        else if (local)
            break;
    }

    return near - map_path_first(path);
}

static inline void
compute_distances_to_near_and_following(const MapPath *path,
                                        const MapPoint *p,
                                        const MapPathPoint *near,
                                        gint64 *dist_squared_near,
                                        gint64 *dist_squared_after_near)
{
    *dist_squared_near = DISTANCE_SQUARED(*p, near->unit);
    if (near < map_path_end(path))
    {
        const MapPathPoint *after_near = near + 1;
        *dist_squared_after_near = DISTANCE_SQUARED(*p, after_near->unit);
    }
    else
        *dist_squared_after_near = -1;
}

/**
 * map_path_update_near_info:
 * @path: a #MapPath.
 * @p: the reference #MapPoint.
 * @ni: a #MapRouteNearInfo used as both input and output.
 * @local: whether the search should be local.
 *
 * Updates the near points information: finds the point in @path closest to @p,
 * and the next waypoint. If @local is %TRUE, only a local search is performed
 * (this should be set when the information in @ni was correct).
 *
 * Returns: %TRUE if the @ni has changed.
 */
gboolean
map_path_update_near_info(const MapPath *path, const MapPoint *p,
                          MapRouteNearInfo *ni, gboolean local)
{
    MapPathWayPoint *wcurr, *wnext;
    MapPathPoint *near;
    gint p_near;
    gint64 dist_squared_near = -1, dist_squared_after_near;
    gboolean p_near_is_set = FALSE;
    gboolean changed = FALSE;

    if (map_path_len(path) == 0)
    {
        if (ni->p_near != 0 || ni->wp_next != 0)
        {
            ni->p_near = ni->wp_next = 0;
            ni->dist_squared_near = ni->dist_squared_after_near = -1;
            changed = TRUE;
        }
        return changed;
    }

    if (ni->p_near >= map_path_len(path))
        return FALSE;

    /* If we are doing a local search, try to follow the path without skipping
     * points: */
    if (local)
    {
        near = map_path_first(path) + ni->p_near;
        compute_distances_to_near_and_following(path, p, near,
                                                &dist_squared_near,
                                                &dist_squared_after_near);
        if (ni->dist_squared_near > 0)
        {
            /* if we are still approaching the near point, don't change it */
            if (dist_squared_near <= ni->dist_squared_near)
            {
                p_near = ni->p_near;
                p_near_is_set = TRUE;
            }
            else /* we are getting farther from the near point */
            {
                /* see if we are approaching the next one */
                if (ni->p_near + 1 < map_path_len(path) &&
                    dist_squared_after_near <= ni->dist_squared_after_near)
                {
                    p_near = ni->p_near + 1;
                    dist_squared_near = dist_squared_after_near;
                    dist_squared_after_near = -1;
                    p_near_is_set = TRUE;
                }
            }
        }

        /* update the internal data */
        ni->dist_squared_near = dist_squared_near;
        ni->dist_squared_after_near = dist_squared_after_near;
    }
    else
    {
        ni->dist_squared_near = -1;
        ni->dist_squared_after_near = -1;
    }

    if (!p_near_is_set)
        p_near = map_path_find_closest(path, p, ni->p_near, local);

    near = map_path_first(path) + p_near;

    if (p_near != ni->p_near)
    {
        ni->p_near = p_near;
        changed = TRUE;

        /* Determine the "next waypoint" */
        for (wcurr = path->whead + ni->wp_next;
             wcurr <= path->wtail && wcurr->point < near;
             wcurr++);

        wnext = wcurr;
        ni->wp_next = wnext - path->whead;
    }

    return changed;
}

void
map_path_optimize(MapPath *path)
{
    MapPathPoint *curr, *prev;
    gint tolerance = 8; /* TODO: make it configurable */

    /* for every point, set the zoom level at which the point must be rendered
     */

    if (map_path_len(path) == 0) return;

    curr = map_path_first(path) + path->points_optimized;
    if (path->points_optimized == 0)
    {
        curr->zoom = SCHAR_MAX;
        path->points_optimized = 1;
        curr++;
    }

    for (; curr < map_path_end(path); curr++)
    {
        gint dx, dy, dmax, zoom;

        prev = curr - 1;

        dx = curr->unit.x - prev->unit.x;
        dy = curr->unit.y - prev->unit.y;
        dmax = MAX(ABS(dx), ABS(dy));

        for (zoom = 0; dmax > tolerance << zoom; zoom++);

        /* We got the zoom level for this point, supposing that the previous
         * one is always drawn.
         * But we need go back in the path to find the last point which is
         * _surely_ drawn when this point is; that is, we look for the last
         * point having a zoom value bigger than that of the current point. */

        while (zoom >= prev->zoom)
        {
            MapPathPoint *prev_before;
            /* going back is safe (we don't risk going past the head) because
             * the first point will always have zoom set to 127 */
            for (prev_before = prev; prev->zoom <= zoom; prev--);

            if (prev == prev_before) break;

            /* now, find the distance between these two points */
            dx = curr->unit.x - prev->unit.x;
            dy = curr->unit.y - prev->unit.y;
            dmax = MAX(ABS(dx), ABS(dy));

            for (; dmax > tolerance << zoom; zoom++);
        }

        curr->zoom = zoom;
    }

    path->points_optimized = map_path_len(path);
}

/**
 * map_path_merge:
 * @src_path: source path.
 * @dest_path: destination path.
 * @policy: merge algorithm.
 *
 * This function prepends, sets or appends @src_path into @dest_path.
 * Note that @src_path might be altered too.
 */
void
map_path_merge(MapPath *src_path, MapPath *dest_path, MapPathMergePolicy policy)
{
    map_path_calculate_distances(src_path);
    map_path_optimize(src_path);

    DEBUG("src length %.2f, dest %.2f", src_path->length, dest_path->length);

    if (map_path_len(src_path) == 0)
    {
        if (policy == MAP_PATH_MERGE_POLICY_REPLACE)
        {
            map_path_unset(dest_path);
            map_path_init(dest_path);
        }
        return;
    }

    if (policy != MAP_PATH_MERGE_POLICY_REPLACE
        && map_path_len(dest_path) > 0)
    {
        MapPathPoint *src_first;
        MapPath *src, *dest;
        gint num_dest_points;
        gint num_src_points;
        MapPathWayPoint *curr;
        GList *list;

        if (policy == MAP_PATH_MERGE_POLICY_APPEND)
        {
            /* Append to current path. */
            src = src_path;
            dest = dest_path;
        }
        else
        {
            /* Prepend to current route. */
            src = dest_path;
            dest = src_path;
        }

        src_first = map_path_first(src);

        /* Append route points from src to dest. */
        num_dest_points = map_path_len(dest);
        num_src_points = map_path_len(src);

        /* Adjust dest->tail to be able to fit src route data
         * plus room for more route data. */
        map_path_resize(dest,
                        num_dest_points + num_src_points + ARRAY_CHUNK_SIZE);

        memcpy(map_path_end(dest), src_first,
               num_src_points * sizeof(MapPathPoint));

        dest->_tail += num_src_points;

        /* Append waypoints from src to dest->. */
        map_path_wresize(dest, (dest->wtail - dest->whead)
                         + (src->wtail - src->whead) + 2 + ARRAY_CHUNK_SIZE);
        for (curr = src->whead; curr != src->wtail; curr++)
        {
            map_path_make_waypoint_full(dest,
                map_path_first(dest) + num_dest_points
                + (curr->point - src_first),
                curr->dir, curr->desc);
        }

        /* Adjust the indexes of the lines and concatenate the lists */
        /* First, remove the first line of the path, because it will be the
         * same as the last line of the destination path (if it was set,
         * otherwise the merge will unify the segments) */
        src->_lines = g_list_delete_link(src->_lines, src->_lines);
        for (list = src->_lines; list != NULL; list = list->next)
        {
            MapLine *line = MAP_LINE(list);
            line->idx_start += num_dest_points;
        }
        dest->_lines = g_list_concat(dest->_lines, src->_lines);

        dest->length += src->length;
        dest->last_lat = src->last_lat;
        dest->last_lon = src->last_lon;
        dest->points_with_distance = map_path_len(dest);

        /* Kill old route - don't use map_path_unset(), because that
         * would free the string desc's that we just moved to data.route. */
        g_free(src->_head);
        g_free(src->whead);
        memset(src_path, 0, sizeof(MapPath));
        if (policy == MAP_PATH_MERGE_POLICY_PREPEND)
            (*dest_path) = *dest;
    }
    else
    {
        map_path_unset(dest_path);
        /* Overwrite with data.route. */
        (*dest_path) = *src_path;
        map_path_resize(dest_path, map_path_len(dest_path) + ARRAY_CHUNK_SIZE);
        map_path_wresize(dest_path, map_path_len(dest_path) + ARRAY_CHUNK_SIZE);
        memset(src_path, 0, sizeof(MapPath));
    }
    DEBUG("total length: %.2f", dest_path->length);
}

void
map_path_remove_range(MapPath *path, MapPathPoint *start, MapPathPoint *end)
{
    g_warning("%s not implemented", G_STRFUNC);
}

guint
map_path_get_duration(const MapPath *path)
{
    MapLineIter line;
    guint duration = 0;

    map_path_line_iter_first(path, &line);
    do
    {
        MapPathPoint *first, *last;
        first = map_path_line_first(&line);
        last = first + map_path_line_len(&line) - 1;
        if (last > first && first->time != 0 && last->time != 0)
            duration += last->time - first->time;
    }
    while (map_path_line_iter_next(&line));

    return duration;
}

void
map_path_append_point_end(MapPath *path)
{
    map_path_calculate_distances(path);
    map_path_optimize(path);
}

MapPathPoint *
map_path_append_unit(MapPath *path, const MapPoint *p)
{
    MapPathPoint pt, *p_in_path;

    pt.unit = *p;
    pt.altitude = 0;
    pt.time = 0;
    pt.zoom = SCHAR_MAX;
    p_in_path = map_path_append_point(path, &pt);
    return p_in_path;
}

/**
 * map_path_append_break:
 * @path: the #MapPath.
 *
 * Breaks the line. Returns %FALSE if the line was already broken.
 */
gboolean
map_path_append_break(MapPath *path)
{
    GList *list;
    MapLine *line;
    gint path_len;

    path_len = map_path_len(path);
    if (G_UNLIKELY(path_len == 0)) return FALSE;

    list = g_list_last(path->_lines);
    g_assert(list != NULL);

    line = MAP_LINE(list);
    if (line->idx_start == path_len) return FALSE;

    path->_lines = g_list_append(path->_lines, MAP_LINE_NEW(path_len));
    path->last_lon = path->last_lat = 0;
    return TRUE;
}

/**
 * map_path_end_is_break:
 * @path: the #MapPath.
 *
 * Returns %TRUE if the path ends with a break point.
 */
gboolean
map_path_end_is_break(const MapPath *path)
{
    GList *list;
    MapLine *line;
    gint path_len;

    path_len = map_path_len(path);
    if (path_len == 0) return TRUE;

    list = g_list_last(path->_lines);
    g_assert(list != NULL);

    line = MAP_LINE(list);
    return line->idx_start == path_len;
}

void
map_path_init(MapPath *path)
{
    path->_head = path->_tail = g_new(MapPathPoint, ARRAY_CHUNK_SIZE);
    path->_cap = path->_head + ARRAY_CHUNK_SIZE;
    path->whead = g_new(MapPathWayPoint, ARRAY_CHUNK_SIZE);
    path->wtail = path->whead - 1;
    path->wcap = path->whead + ARRAY_CHUNK_SIZE;
    path->length = 0;
    path->last_lat = 0;
    path->last_lon = 0;
    path->points_with_distance = 0;
    path->points_optimized = 0;
    path->_lines = g_list_prepend(NULL, MAP_LINE_NEW(0));
    path->first_unsaved = 0;
}

void
map_path_unset(MapPath *path)
{
    if (path->_head) {
        MapPathWayPoint *curr;
        g_free(path->_head);
        path->_head = path->_tail = path->_cap = NULL;
        for (curr = path->whead - 1; curr++ != path->wtail; )
            g_free(curr->desc);
        g_free(path->whead);
        path->whead = path->wtail = path->wcap = NULL;
        g_list_free(path->_lines);
        path->_lines = NULL;
    }
}

void
map_path_line_iter_first(const MapPath *path, MapLineIter *iter)
{
    iter->path = path;
    iter->line = path->_lines;
    g_assert(iter->line != NULL);
}

void
map_path_line_iter_last(const MapPath *path, MapLineIter *iter)
{
    iter->path = path;
    iter->line = g_list_last(path->_lines);
    g_assert(iter->line != NULL);
}

void
map_path_line_iter_from_point(const MapPath *path, const MapPathPoint *point,
                              MapLineIter *iter)
{
    MapLineIter line;

    g_assert(point >= path->_head);
    g_assert(point <= path->_tail);

    map_path_line_iter_first(path, &line);
    *iter = line;
    while (map_path_line_iter_next(&line) &&
           point >= map_path_line_first(&line))
        *iter = line;
}

gboolean
map_path_line_iter_next(MapLineIter *iter)
{
    g_assert(iter->path != NULL);
    iter->line = g_list_next(iter->line);
    return iter->line != NULL;
}

MapPathPoint *
map_path_line_first(const MapLineIter *iter)
{
    return map_path_first(iter->path) + MAP_LINE(iter->line)->idx_start;
}

gint
map_path_line_len(const MapLineIter *iter)
{
    gint start, end;

    start = MAP_LINE(iter->line)->idx_start;
    if (iter->line->next)
        end = MAP_LINE(iter->line->next)->idx_start;
    else
        end = map_path_len(iter->path);
    g_assert(start <= end);
    return end - start;
}

void
map_path_infer_directions(MapPath *path)
{
    MapPathWayPoint *curr;
    for (curr = path->whead; curr < path->wtail; curr++)
    {
        if (curr->dir == MAP_DIRECTION_UNKNOWN)
            curr->dir = infer_direction(curr->desc);
    }
}

