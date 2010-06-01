/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef MAP_ERROR_H
#define MAP_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

#define MAP_ERROR (map_error_quark())

typedef enum {
    MAP_ERROR_GENERIC = 0,
    MAP_ERROR_NETWORK,
    MAP_ERROR_INVALID_ADDRESS,
    MAP_ERROR_USER_CANCELED,
    MAP_ERROR_LAST,
} MapError;

GQuark map_error_quark(void);

G_END_DECLS
#endif /* MAP_ERROR_H */
