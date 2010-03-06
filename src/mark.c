/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2009-2010 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include <string.h>

#define VELVEC_SIZE_FACTOR 4
#define MARK_WIDTH      (_draw_width * 4)
#define MARK_HEIGHT     100

struct _MapMarkPrivate
{
    /* The mark itself */
    ClutterActor *dot;

    /* The elliptical actor representing the GPS uncertainty */
    ClutterActor *uncertainty;

    guint is_disposed : 1;
};

G_DEFINE_TYPE(MapMark, map_mark, CLUTTER_TYPE_GROUP);

#define MAP_MARK_PRIV(mark) (MAP_MARK(mark)->priv)

/* Define the uncertainty actor */
#define MAP_TYPE_UNCERTAINTY (map_uncertainty_get_type())
#define MAP_UNCERTAINTY(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), MAP_TYPE_UNCERTAINTY, MapUncertainty))
#define MAP_IS_UNCERTAINTY(obj) \
(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAP_TYPE_UNCERTAINTY))
#define MAP_UNCERTAINTY_CLASS(klass) \
(G_TYPE_CHECK_CLASS_CAST ((klass), MAP_TYPE_UNCERTAINTY, MapUncertaintyClass))
#define MAP_IS_UNCERTAINTY_CLASS(klass) \
(G_TYPE_CHECK_CLASS_TYPE ((klass), MAP_TYPE_UNCERTAINTY))
#define MAP_UNCERTAINTY_GET_CLASS(obj) \
(G_TYPE_INSTANCE_GET_CLASS ((obj), MAP_TYPE_UNCERTAINTY, MapUncertaintyClass))

typedef struct _MapUncertainty
{
      ClutterActor parent_instance;
} MapUncertainty;

typedef struct _MapUncertaintyClass
{
      ClutterActorClass parent_class;
} MapUncertaintyClass;

G_DEFINE_TYPE(MapUncertainty, map_uncertainty, CLUTTER_TYPE_ACTOR);

typedef struct {
    gfloat sin;
    gfloat cos;
} SineCosine;

static const SineCosine sine_cosine[] = {
    { 0.0, 1.0 },
    { 0.195090322016, 0.980785280403 },
    { 0.382683432365, 0.923879532511 },
    { 0.55557023302, 0.831469612303 },
    { 0.707106781187, 0.707106781187 },
    { 0.831469612303, 0.55557023302 },
    { 0.923879532511, 0.382683432365 },
    { 0.980785280403, 0.195090322016 },
};
#define POINTS_PER_QUADRANT G_N_ELEMENTS(sine_cosine)

static void
map_uncertainty_paint(ClutterActor *actor)
{
    ClutterActorBox allocation = { 0, };
    GdkColor *color;
    gfloat width, height;
    CoglTextureVertex vertices[POINTS_PER_QUADRANT * 4];
    gint i;

    color = &_color[COLORABLE_MARK];

    clutter_actor_get_allocation_box(actor, &allocation);
    clutter_actor_box_get_size(&allocation, &width, &height);

    cogl_push_matrix();
    cogl_set_source_color4ub(color->red >> 8,
                             color->green >> 8,
                             color->blue >> 8,
                             clutter_actor_get_paint_opacity(actor) / 8);

    cogl_scale(width, height, 1);
    memset(vertices, 0, sizeof(vertices));
    for (i = 0; i < POINTS_PER_QUADRANT; i++)
    {
        gint v = i;
        vertices[v].x = sine_cosine[i].cos;
        vertices[v].y = sine_cosine[i].sin;
    }
    for (i = 0; i < POINTS_PER_QUADRANT; i++)
    {
        gint v = i + POINTS_PER_QUADRANT;
        vertices[v].x = -sine_cosine[i].sin;
        vertices[v].y = sine_cosine[i].cos;
    }
    for (i = 0; i < POINTS_PER_QUADRANT; i++)
    {
        gint v = i + 2 * POINTS_PER_QUADRANT;
        vertices[v].x = -sine_cosine[i].cos;
        vertices[v].y = -sine_cosine[i].sin;
    }
    for (i = 0; i < POINTS_PER_QUADRANT; i++)
    {
        gint v = i + 3 * POINTS_PER_QUADRANT;
        vertices[v].x = sine_cosine[i].sin;
        vertices[v].y = -sine_cosine[i].cos;
    }
    cogl_polygon(vertices, POINTS_PER_QUADRANT * 4, FALSE);
#if 0
    /* See http://bugzilla.openedhand.com/show_bug.cgi?id=1916 */
    cogl_path_ellipse(0, 0, width, height);
    cogl_path_fill();
#endif
    cogl_pop_matrix();
}

static void
map_uncertainty_class_init(MapUncertaintyClass *klass)
{
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS(klass);

    actor_class->paint = map_uncertainty_paint;
}

static void
map_uncertainty_init(MapUncertainty *uncertainty)
{
}


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

    priv->uncertainty = g_object_new(MAP_TYPE_UNCERTAINTY, NULL);
    clutter_actor_set_anchor_point(priv->uncertainty, 0, 0);
    clutter_actor_set_position(priv->uncertainty, 0, 0);

    clutter_actor_set_size(priv->uncertainty, 0, 0);

    clutter_container_add_actor(CLUTTER_CONTAINER(mark), priv->uncertainty);
    clutter_actor_hide(priv->uncertainty);

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
    cairo_arc(cr, x, y, _draw_width, 0, 2 * PI);
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
                               unit2zpixel(_pos.unit.x, zoom),
                               unit2zpixel(_pos.unit.y, zoom));
    clutter_actor_set_rotation(priv->dot,
                               CLUTTER_Z_AXIS, _gps.heading, 0, 0, 0);

    /* set the uncertainty ellipse */
    if (_gps_state == RCVR_FIXED)
    {
        gint units_per_metre_y;
        gint x, y;

        units_per_metre_y = WORLD_SIZE_UNITS / (EARTH_CIRCUMFERENCE / 2);

        /* TODO: make it an ellipse, considering that near the poles the the
         * density of x units is higher */

        /* _gps.hdop is in m */
        y = _gps.hdop * units_per_metre_y;

        x = y;

        y = unit2zpixel(y, zoom);
        x = unit2zpixel(x, zoom);
        clutter_actor_set_size(priv->uncertainty, x, y);
        clutter_actor_show(priv->uncertainty);
    }
    else
        clutter_actor_hide(priv->uncertainty);
}

