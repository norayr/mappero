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

#ifndef MAP_DEBUG_H
#define MAP_DEBUG_H

#include <glib.h>

#ifdef ENABLE_DEBUG

#define DEBUG(format, ...) G_STMT_START {                   \
    if (_map_debug_get_level() > 0)                         \
        g_debug("%s: " format, G_STRFUNC, ##__VA_ARGS__);   \
} G_STMT_END

#else /* !ENABLE_DEBUG */

#define DEBUG(format, ...)

#endif

G_BEGIN_DECLS

extern gint _map_debug_level;

void map_debug_init(void);

static inline gint _map_debug_get_level(void)
{
    return _map_debug_level;
}

G_END_DECLS
#endif /* MAP_DEBUG_H */
