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

#ifndef MAP_SIGN_H
#define MAP_SIGN_H

#include <glib.h>
#include <glib-object.h>
#include <clutter/clutter.h>
 
#include "types.h"

G_BEGIN_DECLS

#define MAP_TYPE_SIGN         (map_sign_get_type ())
#define MAP_SIGN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MAP_TYPE_SIGN, MapSign))
#define MAP_SIGN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_SIGN, MapSignClass))
#define MAP_IS_SIGN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MAP_TYPE_SIGN))
#define MAP_IS_SIGN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MAP_TYPE_SIGN))
#define MAP_SIGN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MAP_TYPE_SIGN, MapSignClass))

typedef struct _MapSign MapSign;
typedef struct _MapSignPrivate MapSignPrivate;
typedef struct _MapSignClass MapSignClass;


struct _MapSign
{
    ClutterGroup parent;

    MapSignPrivate *priv;
};

struct _MapSignClass
{
    ClutterGroupClass parent_class;
};

GType map_sign_get_type(void);

void map_sign_set_pango_context(MapSign *self, PangoContext *context);

void map_sign_set_info(MapSign *self, MapDirection dir, const gchar *text,
                       gfloat distance);
void map_sign_set_distance(MapSign *self, gfloat distance);

G_END_DECLS
#endif /* MAP_SIGN_H */
