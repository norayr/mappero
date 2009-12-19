/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2009 Alberto Mardegan.
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

#ifndef MAP_MARK_H
#define MAP_MARK_H

#include <glib.h>
#include <glib-object.h>
#include <clutter/clutter.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_MARK         (map_mark_get_type ())
#define MAP_MARK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MAP_TYPE_MARK, MapMark))
#define MAP_MARK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_MARK, MapMarkClass))
#define MAP_IS_MARK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MAP_TYPE_MARK))
#define MAP_IS_MARK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MAP_TYPE_MARK))
#define MAP_MARK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MAP_TYPE_MARK, MapMarkClass))

typedef struct _MapMark MapMark;
typedef struct _MapMarkPrivate MapMarkPrivate;
typedef struct _MapMarkClass MapMarkClass;


struct _MapMark
{
    ClutterCairoTexture parent;

    MapMarkPrivate *priv;
};

struct _MapMarkClass
{
    ClutterCairoTextureClass parent_class;
};

GType map_mark_get_type (void);

void map_mark_update(MapMark *mark);

G_END_DECLS
#endif /* MAP_MARK_H */
