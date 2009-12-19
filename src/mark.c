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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "mark.h"

#include "controller.h"
#include "data.h"
#include "defines.h"
#include "math.h"

#include <clutter-gtk/clutter-gtk.h>
#include <stdlib.h>

#define VELVEC_SIZE_FACTOR 4
#define MARK_WIDTH      (_draw_width * 4)
#define MARK_HEIGHT     100

struct _MapMarkPrivate
{
    /* The mark itself */
    ClutterActor *dot;

    guint is_disposed : 1;
};

G_DEFINE_TYPE(MapMark, map_mark, CLUTTER_TYPE_GROUP);

#define MAP_MARK_PRIV(mark) (MAP_MARK(mark)->priv)

static inline void
set_source_color(cairo_t *cr, GdkColor *color)
{
    cairo_set_source_rgb(cr,
                         color->red / 65535.0,
                         color->green / 65535.0,
                         color->blue / 65535.0);
}

static void
map_mark_dispose(GObject *object)
{
    MapMarkPrivate *priv = MAP_MARK_PRIV(object);

    if (priv->is_disposed)
        return;

    priv->is_disposed = TRUE;

    G_OBJECT_CLASS(map_mark_parent_class)->dispose(object);
}

static void
map_mark_init(MapMark *mark)
{
    MapMarkPrivate *priv;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(mark, MAP_TYPE_MARK, MapMarkPrivate);
    mark->priv = priv;

    priv->dot = clutter_cairo_texture_new(MARK_WIDTH, MARK_HEIGHT);
    clutter_actor_set_anchor_point(priv->dot,
                                   MARK_WIDTH / 2,
                                   MARK_HEIGHT - MARK_WIDTH / 2);

    clutter_container_add_actor(CLUTTER_CONTAINER(mark), priv->dot);
}

static void
map_mark_class_init(MapMarkClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(object_class, sizeof(MapMarkPrivate));

    object_class->dispose = map_mark_dispose;
}

void
map_mark_update(MapMark *self)
{
    MapMarkPrivate *priv = self->priv;
    MapController *controller = map_controller_get_instance();
    cairo_t *cr;
    Colorable color;
    gfloat x, y, sqrt_speed;
    gint zoom;

    clutter_actor_get_anchor_point(priv->dot, &x, &y);
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->dot));

    cairo_new_sub_path(cr);
    cairo_arc(cr, x, y, _draw_width, 0, 2 * M_PI);
    cairo_set_line_width(cr, _draw_width);
    color = (_gps_state == RCVR_FIXED) ? COLORABLE_MARK : COLORABLE_MARK_OLD;
    set_source_color(cr, &_color[color]);
    cairo_stroke(cr);

    /* draw the speed vector */
    sqrt_speed = VELVEC_SIZE_FACTOR * sqrtf(10 + _gps.speed);
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x, y - sqrt_speed);
    cairo_set_line_width(cr, _draw_width);
    cairo_set_line_cap(cr, CAIRO_LINE_JOIN_ROUND);
    color = (_gps_state == RCVR_FIXED) ?
        (_show_velvec ? COLORABLE_MARK_VELOCITY : COLORABLE_MARK) :
        COLORABLE_MARK_OLD;
    set_source_color(cr, &_color[color]);
    cairo_stroke(cr);

    cairo_destroy(cr);

    zoom = map_controller_get_zoom(controller);
    /* set position and angle */
    clutter_actor_set_position(CLUTTER_ACTOR(self),
                               unit2zpixel(_pos.unitx, zoom),
                               unit2zpixel(_pos.unity, zoom));
    clutter_actor_set_rotation(priv->dot,
                               CLUTTER_Z_AXIS, _gps.heading, 0, 0, 0);
}

