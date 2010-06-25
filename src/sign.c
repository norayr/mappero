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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "sign.h"

#include "navigation.h"
#include "util.h"
#include <clutter-gtk/clutter-gtk.h>
#include <mappero/debug.h>
#include <math.h>

#define ICON_SIZE   128
#define TEXT_BORDER 4
#define DISTANCE_HEIGHT ICON_SIZE
#define DISTANCE_WIDTH ICON_SIZE

struct _MapSignPrivate
{
    ClutterActor *c_direction;
    ClutterActor *c_distance;
    ClutterActor *c_text;

    PangoContext *context;
    PangoFontDescription *font_distance;
    PangoFontDescription *font_text;
    gint width;
    gint height;
    gint text_height;

    gchar *text;
    gfloat distance;

    guint is_disposed : 1;
};

G_DEFINE_TYPE(MapSign, map_sign, CLUTTER_TYPE_GROUP);

#define MAP_SIGN_PRIV(sign) (MAP_SIGN(sign)->priv)

static void
map_sign_relayout(MapSign *self)
{
    MapSignPrivate *priv = self->priv;

    if (priv->width > priv->height)
    {
        clutter_actor_set_position(priv->c_direction,
            0,
            priv->height - ICON_SIZE);
        clutter_actor_set_position(priv->c_distance,
            ICON_SIZE,
            priv->height - DISTANCE_HEIGHT);
        clutter_actor_set_position(priv->c_text,
            ICON_SIZE + DISTANCE_WIDTH,
            priv->height - priv->text_height);
    }
    else
    {
        clutter_actor_set_position(priv->c_text,
            0,
            priv->height - priv->text_height);
        clutter_actor_set_position(priv->c_direction,
            0,
            priv->height - priv->text_height - ICON_SIZE);
        clutter_actor_set_position(priv->c_distance,
            ICON_SIZE,
            priv->height - priv->text_height - DISTANCE_HEIGHT);
    }
}

static void
map_sign_draw_distance(MapSign *self)
{
    MapSignPrivate *priv = self->priv;
    PangoLayout *layout;
    gchar text[64];
    const gchar *units;
    gint decimals, len, h, start_y;
    gfloat distance;
    cairo_t *cr;

    distance = priv->distance;
    map_util_format_distance(&distance, &units, &decimals);
    /* round the distance to the tens */
    if (decimals == 0)
        distance = roundf(distance / 10) * 10;
    len = g_snprintf(text, sizeof(text),
                     "%.*f\n<span size=\"xx-small\">%s</span>",
                     decimals, distance, units);


    layout = pango_layout_new(priv->context);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_width(layout, DISTANCE_WIDTH * PANGO_SCALE);
    pango_layout_set_font_description(layout, priv->font_distance);

    pango_layout_set_markup(layout, text, len);

    /* vertically align the layout */
    pango_layout_get_pixel_size(layout, NULL, &h);
    start_y = (DISTANCE_HEIGHT - h) / 2;


    clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->c_distance));
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->c_distance));
    g_assert(cr != NULL);

    cairo_rectangle(cr, 0, 0, DISTANCE_WIDTH, DISTANCE_HEIGHT);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, TEXT_BORDER, start_y);
    pango_cairo_show_layout(cr, layout);

    cairo_destroy(cr);
}

static void
map_sign_draw_text(MapSign *self)
{
    MapSignPrivate *priv = self->priv;
    PangoLayout *layout;
    cairo_t *cr;
    gint w, h, lh;

    if (!priv->text) return;

    if (priv->width > priv->height)
        w = priv->width - ICON_SIZE - DISTANCE_WIDTH;
    else
        w = priv->width;

    layout = pango_layout_new(priv->context);
    pango_layout_set_width(layout, (w - 2 * TEXT_BORDER) * PANGO_SCALE);
    pango_layout_set_font_description(layout, priv->font_text);

    pango_layout_set_text(layout, priv->text, -1);
    pango_layout_get_pixel_size(layout, NULL, &lh);
    h = lh + 2 * TEXT_BORDER;

    clutter_cairo_texture_set_surface_size(CLUTTER_CAIRO_TEXTURE(priv->c_text),
                                           w, h);

    clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->c_text));
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->c_text));
    g_assert(cr != NULL);

    cairo_rectangle(cr, 0, 0, w, h);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, TEXT_BORDER, TEXT_BORDER);
    pango_cairo_show_layout(cr, layout);

    cairo_destroy(cr);

    priv->text_height = h;
}

static void
map_sign_redraw(MapSign *self)
{
    map_sign_draw_distance(self);
    map_sign_draw_text(self);
    map_sign_relayout(self);
}

static GdkPixbuf *
get_icon_for_direction(MapDirection dir)
{
    GtkIconTheme *icon_theme;
    const gchar *dir_name;
    gchar icon_name[128];

    /* TODO: consider implementing caching */

    dir_name = map_direction_get_name(dir);
    DEBUG("name = %s", dir_name);
    if (!dir_name) return NULL;

    g_snprintf(icon_name, sizeof(icon_name), PACKAGE "-nav-%s", dir_name);
    DEBUG("icon name = %s", icon_name);
    icon_theme = gtk_icon_theme_get_default();
    return gtk_icon_theme_load_icon(icon_theme, icon_name, ICON_SIZE, 0, NULL);
}

static void
map_sign_dispose(GObject *object)
{
    MapSignPrivate *priv = MAP_SIGN_PRIV(object);

    if (priv->is_disposed)
        return;

    priv->is_disposed = TRUE;

    if (priv->context)
    {
        g_object_unref(priv->context);
        priv->context = NULL;
    }

    G_OBJECT_CLASS(map_sign_parent_class)->dispose(object);
}

static void
map_sign_finalize(GObject *object)
{
    MapSignPrivate *priv = MAP_SIGN_PRIV(object);

    g_free(priv->text);
    pango_font_description_free(priv->font_distance);
    pango_font_description_free(priv->font_text);

    G_OBJECT_CLASS(map_sign_parent_class)->finalize(object);
}

static void
on_parent_allocation_changed(ClutterActor *parent, GParamSpec *pspec,
                             MapSign *self)
{
    MapSignPrivate *priv = self->priv;
    gfloat w, h;

    clutter_actor_get_size(parent, &w, &h);
    DEBUG("Parent size: %.0f, %.0f", w, h);

    priv->width = w;
    priv->height = h;
    map_sign_redraw(self);
}

static void
map_sign_parent_set(ClutterActor *actor, ClutterActor *old_parent)
{
    MapSignPrivate *priv = MAP_SIGN_PRIV(actor);
    ClutterActor *parent;
    gfloat w, h;

    if (old_parent)
        g_signal_handlers_disconnect_by_func(old_parent,
                                             on_parent_allocation_changed,
                                             actor);

    parent = clutter_actor_get_parent(actor); /* this is the clutter stage */
    if (parent)
    {
        g_signal_connect(parent, "notify::allocation",
                         G_CALLBACK(on_parent_allocation_changed), actor);
        clutter_actor_get_size(parent, &w, &h);
        priv->width = w;
        priv->height = h;
    }
}

static void
map_sign_init(MapSign *self)
{
    MapSignPrivate *priv;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(self, MAP_TYPE_SIGN, MapSignPrivate);
    self->priv = priv;

    priv->c_direction = clutter_texture_new();
    clutter_container_add_actor(CLUTTER_CONTAINER(self), priv->c_direction);

    priv->c_distance = clutter_cairo_texture_new(DISTANCE_WIDTH,
                                                 DISTANCE_HEIGHT);
    clutter_container_add_actor(CLUTTER_CONTAINER(self), priv->c_distance);

    priv->c_text = clutter_cairo_texture_new(0, 0);
    clutter_container_add_actor(CLUTTER_CONTAINER(self), priv->c_text);
}

static void
map_sign_class_init(MapSignClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS(klass);

    g_type_class_add_private(object_class, sizeof(MapSignPrivate));

    object_class->dispose = map_sign_dispose;
    object_class->finalize = map_sign_finalize;
    actor_class->parent_set = map_sign_parent_set;
}

void
map_sign_set_pango_context(MapSign *self, PangoContext *context)
{
    MapSignPrivate *priv;

    g_return_if_fail(MAP_IS_SIGN(self));
    priv = self->priv;

    DEBUG("Context = %p", context);
    if (context)
    {
        priv->context = g_object_ref(context);
        priv->font_distance = pango_font_description_from_string("Sans 32");
        priv->font_text = pango_font_description_from_string("Sans 24");
    }
}

void
map_sign_set_info(MapSign *self, MapDirection dir, const gchar *text,
                  gfloat distance)
{
    MapSignPrivate *priv;
    GdkPixbuf *pixbuf;

    g_return_if_fail(MAP_IS_SIGN(self));
    priv = self->priv;

    pixbuf = get_icon_for_direction(dir);
    if (pixbuf)
    {
        gtk_clutter_texture_set_from_pixbuf(CLUTTER_TEXTURE(priv->c_direction),
                                            pixbuf, NULL);
        g_object_unref(pixbuf);
    }

    g_free(priv->text);
    priv->text = g_strdup(text);

    priv->distance = distance;

    map_sign_redraw(self);
}

void
map_sign_set_distance(MapSign *self, gfloat distance)
{
    MapSignPrivate *priv;

    g_return_if_fail(MAP_IS_SIGN(self));
    priv = self->priv;

    if (distance != priv->distance)
    {
        priv->distance = distance;
        map_sign_draw_distance(self);
    }
}

