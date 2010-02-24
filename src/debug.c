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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "debug.h"

#include <stdlib.h>

/* we assume that if the package is built with debugging enabled, we want
 * to see all the debug messages */
#ifdef ENABLE_DEBUG
#define DEFAULT_DEBUG_LEVEL 1
#else
#define DEFAULT_DEBUG_LEVEL 0
#endif

gint _map_debug_level = DEFAULT_DEBUG_LEVEL;

void
map_debug_init(void)
{
    const gchar *env;

    env = getenv("MAP_DEBUG");
    if (env)
        _map_debug_level = atoi(env);

    if (_map_debug_level > 0)
        g_debug("%s version %s", PACKAGE, VERSION);
}

