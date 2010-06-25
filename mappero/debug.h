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

#ifndef MAP_DEBUG_H
#define MAP_DEBUG_H

#include <glib.h>
#include <time.h>

#ifdef ENABLE_DEBUG

#define DEBUG(format, ...) G_STMT_START {                   \
    if (map_debug_get_level() > 0)                         \
        g_debug("%s: " format, G_STRFUNC, ##__VA_ARGS__);   \
} G_STMT_END

/* Macros for profiling */
#define TIME_START() \
    struct timespec tm0, tm1; \
    struct timespec tt0, tt1; \
    long ms_mdiff; \
    long ms_tdiff; \
    clock_gettime(CLOCK_MONOTONIC, &tm0); \
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tt0)

#define TIME_STOP() \
    clock_gettime(CLOCK_MONOTONIC, &tm1); \
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tt1); \
    ms_mdiff = (tm1.tv_sec - tm0.tv_sec) * 1000 + \
               (tm1.tv_nsec - tm0.tv_nsec) / 1000000; \
    ms_tdiff = (tt1.tv_sec - tt0.tv_sec) * 1000 + \
               (tt1.tv_nsec - tt0.tv_nsec) / 1000000; \
    DEBUG("%s, total %ld ms, thread %ld ms", G_STRLOC, ms_mdiff, ms_tdiff)

#else /* !ENABLE_DEBUG */

#define DEBUG(format, ...)
#define TIME_START()
#define TIME_STOP()

#endif

G_BEGIN_DECLS

extern gint map_debug_level;

void map_debug_init(void);

static inline gint map_debug_get_level(void)
{
    return map_debug_level;
}

G_END_DECLS
#endif /* MAP_DEBUG_H */
