/* vi: set et sw=4 ts=8 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 8 -*- */
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
#include "screen.h"

#include "data.h"
#include "defines.h"
#include "maps.h"
#include "math.h"
#include "tile.h"
#include "types.h"
#include "util.h"

#include <cairo/cairo.h>

#define SCALE_WIDTH     100
#define SCALE_HEIGHT    22
#define ZOOM_WIDTH      25
#define ZOOM_HEIGHT     SCALE_HEIGHT

struct _MapScreenPrivate
{
    ClutterActor *map;
    gint map_center_ux;
    gint map_center_uy;

    ClutterActor *tile_group;

    ClutterActor *compass;
    ClutterActor *compass_north;
    ClutterActor *scale;
    ClutterActor *zoom_box;

    /* layer for drawing over the map (used for paths) */
    ClutterActor *overlay;

    /* center of the draw layer, in pixels */
    gint overlay_center_px; /* Y is the same */

    /* upper left of the draw layer, in units */
    gint overlay_start_px;
    gint overlay_start_py;

    gint zoom;

    guint source_overlay_redraw;

    guint show_compass : 1;

    guint is_disposed : 1;
};

G_DEFINE_TYPE(MapScreen, map_screen, GTK_CLUTTER_TYPE_EMBED);

#define MAP_SCREEN_PRIV(screen) (MAP_SCREEN(screen)->priv)

static inline void
point_to_pixels(MapScreenPrivate *priv, Point *p, gint *x, gint *y)
{
    *x = unit2zpixel(p->unitx, priv->zoom) - priv->overlay_start_px;
    *y = unit2zpixel(p->unity, priv->zoom) - priv->overlay_start_py;
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
draw_break(cairo_t *cr, GdkColor *color, gint x, gint y)
{
    cairo_save(cr);
    cairo_new_sub_path(cr);
    cairo_arc(cr, x, y, _draw_width, 0, 2 * M_PI);
    cairo_set_line_width(cr, _draw_width);
    set_source_color(cr, color);
    cairo_stroke(cr);
    cairo_restore(cr);
}

static void
draw_path(MapScreen *screen, Path *path, Colorable base)
{
    MapScreenPrivate *priv = screen->priv;
    Point *curr;
    WayPoint *wcurr;
    cairo_t *cr;
    gint x = 0, y = 0;
    gboolean segment_open = FALSE;

    g_debug ("%s", G_STRFUNC);
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->overlay));
    g_assert(cr != NULL);

    set_source_color(cr, &_color[base]);
    cairo_set_line_width(cr, _draw_width);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
    for (curr = path->head, wcurr = path->whead; curr <= path->tail; curr++)
    {
        if (G_UNLIKELY(curr->unity == 0))
        {
            if (segment_open)
            {
                /* use x and y from the previous iteration */
                cairo_stroke(cr);
                draw_break(cr, &_color[base + 2], x, y);
                segment_open = FALSE;
            }
        }
        else
        {
            point_to_pixels(priv, curr, &x, &y);
            if (G_UNLIKELY(!segment_open))
            {
                draw_break(cr, &_color[base + 2], x, y);
                cairo_move_to(cr, x, y);
                segment_open = TRUE;
            }
            else
                cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    cairo_destroy(cr);
}

static void
draw_paths(MapScreen *screen)
{
    if ((_show_paths & ROUTES_MASK) && _route.head != _route.tail)
        draw_path(screen, &_route, COLORABLE_ROUTE);

    if (_show_paths & TRACKS_MASK)
        draw_path(screen, &_track, COLORABLE_TRACK);
}

static gboolean
overlay_redraw_real(MapScreen *screen)
{
    MapScreenPrivate *priv;
    GtkAllocation *allocation;
    gfloat center_x, center_y;

    g_return_val_if_fail (MAP_IS_SCREEN (screen), FALSE);
    priv = screen->priv;

    clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->overlay));
    allocation = &(GTK_WIDGET(screen)->allocation);
    clutter_actor_get_anchor_point(priv->map, &center_x, &center_y);
    clutter_actor_set_position(priv->overlay, center_x, center_y);

    priv->overlay_start_px = center_x - priv->overlay_center_px;
    priv->overlay_start_py = center_y - priv->overlay_center_px;

    draw_paths(screen);

    priv->source_overlay_redraw = 0;
    return FALSE;
}

static void
overlay_redraw_idle(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;

    if (priv->source_overlay_redraw == 0)
    {
        priv->source_overlay_redraw =
            g_idle_add((GSourceFunc)overlay_redraw_real, screen);
    }
}

static void
load_tiles_into_map(MapScreen *screen, RepoData *repo, gint zoom,
                    gint tx1, gint ty1, gint tx2, gint ty2)
{
    ClutterContainer *tile_group;
    ClutterActor *tile;
    gint tx, ty;

    tile_group = CLUTTER_CONTAINER(screen->priv->tile_group);

    for (tx = tx1; tx <= tx2; tx++)
    {
        for (ty = ty1; ty <= ty2; ty++)
        {
            tile = map_tile_load(repo, zoom, tx, ty);
            clutter_container_add_actor(tile_group, tile);
        }
    }
}

static void
create_compass(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;
    gint height, width;
    cairo_text_extents_t te;
    cairo_t *cr;

    width = 40, height = 75;
    priv->compass = clutter_cairo_texture_new(width, height);
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->compass));

    cairo_move_to(cr, 20, 57);
    cairo_line_to(cr, 40, 75);
    cairo_line_to(cr, 20, 0);
    cairo_line_to(cr, 0, 75);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
    cairo_destroy(cr);

    clutter_actor_set_anchor_point(priv->compass, 20, 45);

    /* create the "N" letter */
    width = 16, height = 16;
    priv->compass_north = clutter_cairo_texture_new(width, height);
    cr = clutter_cairo_texture_create
        (CLUTTER_CAIRO_TEXTURE(priv->compass_north));
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_select_font_face (cr, "Sans Serif",
                            CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 20);
    cairo_text_extents (cr, "N", &te);
    cairo_move_to (cr, 8 - te.width / 2 - te.x_bearing,
                       8 - te.height / 2 - te.y_bearing);
    cairo_show_text (cr, "N");
    cairo_destroy(cr);

    clutter_actor_set_anchor_point(priv->compass_north, 8, 8);
}

static void
compute_scale_text(gchar *buffer, gsize len, gint zoom)
{
    gfloat distance;
    gdouble lat1, lon1, lat2, lon2;

    unit2latlon(_center.unitx - pixel2zunit(SCALE_WIDTH / 2 - 4, zoom),
                _center.unity, lat1, lon1);
    unit2latlon(_center.unitx + pixel2zunit(SCALE_WIDTH / 2 - 4, zoom),
                _center.unity, lat2, lon2);
    distance = calculate_distance(lat1, lon1, lat2, lon2) *
        UNITS_CONVERT[_units];

    if(distance < 1.f)
        snprintf(buffer, len, "%0.2f %s", distance, UNITS_ENUM_TEXT[_units]);
    else if(distance < 10.f)
        snprintf(buffer, len, "%0.1f %s", distance, UNITS_ENUM_TEXT[_units]);
    else
        snprintf(buffer, len, "%0.f %s", distance, UNITS_ENUM_TEXT[_units]);
}

static void
update_scale_and_zoom(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;
    cairo_text_extents_t te;
    gchar buffer[16];
    cairo_t *cr;

    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->scale));

    cairo_rectangle(cr, 0, 0, SCALE_WIDTH, SCALE_HEIGHT);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 4);
    cairo_stroke(cr);

    /* text in scale box */
    compute_scale_text(buffer, sizeof(buffer), priv->zoom);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_select_font_face (cr, "Sans Serif",
                            CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 16);
    cairo_text_extents (cr, buffer, &te);
    cairo_move_to (cr,
                   SCALE_WIDTH / 2  - te.width / 2 - te.x_bearing,
                   SCALE_HEIGHT / 2 - te.height / 2 - te.y_bearing);
    cairo_show_text (cr, buffer);

    /* arrows */
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 4, SCALE_HEIGHT / 2 - 4);
    cairo_line_to(cr, 4, SCALE_HEIGHT / 2 + 4);
    cairo_move_to(cr, 4, SCALE_HEIGHT / 2);
    cairo_line_to(cr, (SCALE_WIDTH - te.width) / 2 - 4, SCALE_HEIGHT / 2);
    cairo_stroke(cr);
    cairo_move_to(cr, SCALE_WIDTH - 4, SCALE_HEIGHT / 2 - 4);
    cairo_line_to(cr, SCALE_WIDTH - 4, SCALE_HEIGHT / 2 + 4);
    cairo_move_to(cr, SCALE_WIDTH - 4, SCALE_HEIGHT / 2);
    cairo_line_to(cr, (SCALE_WIDTH + te.width) / 2 + 4, SCALE_HEIGHT / 2);
    cairo_stroke(cr);

    cairo_destroy(cr);

    /* zoom box */
    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->zoom_box));

    cairo_rectangle(cr, 0, 0, ZOOM_WIDTH, ZOOM_HEIGHT);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 4);
    cairo_stroke(cr);

    snprintf(buffer, sizeof(buffer), "%d", priv->zoom);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_select_font_face (cr, "Sans Serif",
                            CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 16);
    cairo_text_extents (cr, buffer, &te);
    cairo_move_to (cr,
                   ZOOM_WIDTH / 2  - te.width / 2 - te.x_bearing,
                   ZOOM_HEIGHT / 2 - te.height / 2 - te.y_bearing);
    cairo_show_text (cr, buffer);
    cairo_destroy(cr);
}

static void
create_scale_and_zoom(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;

    priv->scale = clutter_cairo_texture_new(SCALE_WIDTH, SCALE_HEIGHT);
    clutter_actor_set_anchor_point(priv->scale,
                                   SCALE_WIDTH / 2 + 1,
                                   SCALE_HEIGHT);

    priv->zoom_box = clutter_cairo_texture_new(ZOOM_WIDTH, ZOOM_HEIGHT);
    clutter_actor_set_anchor_point(priv->zoom_box,
                                   ZOOM_WIDTH - 1 + SCALE_WIDTH / 2,
                                   ZOOM_HEIGHT);
}

static void
map_screen_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    MapScreenPrivate *priv = MAP_SCREEN_PRIV(widget);
    gint diagonal;

    GTK_WIDGET_CLASS(map_screen_parent_class)->size_allocate
        (widget, allocation);

    /* Position the map at the center of the widget */
    clutter_actor_set_position(priv->map,
                               allocation->width / 2,
                               allocation->height / 2);
    if (priv->compass)
    {
        gint x, y;

        x = allocation->width - 42;
        y = allocation->height - 42;
        clutter_actor_set_position(priv->compass, x, y);
        clutter_actor_set_position(priv->compass_north, x, y);
    }

    /* scale and zoom box */
    clutter_actor_set_position(priv->scale,
                               allocation->width / 2,
                               allocation->height);
    clutter_actor_set_position(priv->zoom_box,
                               allocation->width / 2,
                               allocation->height);

    /* Resize the map overlay */

    /* The overlayed texture used for drawing must be big enough to cover
     * the screen when the map rotates: so, it will be a square whose side
     * is as big as the diagonal of the widget */
    diagonal = gint_sqrt(allocation->width * allocation->width +
                         allocation->height * allocation->height);
    clutter_cairo_texture_set_surface_size
        (CLUTTER_CAIRO_TEXTURE(priv->overlay), diagonal, diagonal);
    priv->overlay_center_px = diagonal / 2;
    clutter_actor_set_anchor_point(priv->overlay,
                                   priv->overlay_center_px,
                                   priv->overlay_center_px);

    overlay_redraw_idle((MapScreen *)widget);
}

static void
map_screen_dispose(GObject *object)
{
    MapScreenPrivate *priv = MAP_SCREEN_PRIV(object);

    if (priv->is_disposed)
	return;

    priv->is_disposed = TRUE;

    if (priv->source_overlay_redraw != 0)
    {
        g_source_remove (priv->source_overlay_redraw);
        priv->source_overlay_redraw = 0;
    }

    G_OBJECT_CLASS(map_screen_parent_class)->dispose(object);
}

static void
map_screen_init(MapScreen *screen)
{
    MapScreenPrivate *priv;
    ClutterActor *stage;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(screen, MAP_TYPE_SCREEN,
                                       MapScreenPrivate);
    screen->priv = priv;

    stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(screen));
    g_return_if_fail(stage != NULL);

    priv->map = clutter_group_new();
    g_return_if_fail(priv->map != NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->map);
    clutter_actor_show(priv->map);

    priv->tile_group = clutter_group_new();
    g_return_if_fail(priv->tile_group != NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->tile_group);
    clutter_actor_show(priv->tile_group);

    create_compass(screen);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->compass);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->compass_north);

    priv->overlay = clutter_cairo_texture_new(0, 0);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->overlay);
    clutter_actor_show(priv->overlay);

    create_scale_and_zoom(screen);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->scale);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->zoom_box);
}

static void
map_screen_class_init(MapScreenClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    g_type_class_add_private (object_class, sizeof (MapScreenPrivate));

    object_class->dispose = map_screen_dispose;

    widget_class->size_allocate = map_screen_size_allocate;
}

/**
 * map_screen_set_center:
 * @screen: the #MapScreen.
 * @x: the x coordinate of the new center.
 * @y: the y coordinate of the new center.
 * @zoom: the new zoom level, or -1 to keep the current one.
 */
void
map_screen_set_center(MapScreen *screen, gint x, gint y, gint zoom)
{
    MapScreenPrivate *priv;
    GtkAllocation *allocation;
    gint diag_halflength_units;
    gint start_tilex, start_tiley, stop_tilex, stop_tiley;
    gint px, py;
    gint cache_amount;
    gint new_zoom;
    RepoData *repo = _curr_repo;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    new_zoom = (zoom > 0) ? zoom : priv->zoom;

    /* Destroying all the existing tiles.
     * TODO: implement some caching, reuse tiles when possible. */
    clutter_group_remove_all(CLUTTER_GROUP(priv->tile_group));

    /* Calculate cache amount */
    if(repo->type != REPOTYPE_NONE && MAPDB_EXISTS(repo))
        cache_amount = _auto_download_precache;
    else
        cache_amount = 1; /* No cache. */

    allocation = &(GTK_WIDGET(screen)->allocation);
    diag_halflength_units =
        pixel2zunit(TILE_HALFDIAG_PIXELS +
                    MAX(allocation->width, allocation->height) / 2,
                    new_zoom);

    start_tilex = unit2ztile(x - diag_halflength_units + _map_correction_unitx,
                             new_zoom);
    start_tilex = MAX(start_tilex - (cache_amount - 1), 0);
    start_tiley = unit2ztile(y - diag_halflength_units + _map_correction_unity,
                             new_zoom);
    start_tiley = MAX(start_tiley - (cache_amount - 1), 0);
    stop_tilex = unit2ztile(x + diag_halflength_units + _map_correction_unitx,
                            new_zoom);
    stop_tilex = MIN(stop_tilex + (cache_amount - 1),
                     unit2ztile(WORLD_SIZE_UNITS, new_zoom));
    stop_tiley = unit2ztile(y + diag_halflength_units + _map_correction_unity,
                            new_zoom);
    stop_tiley = MIN(stop_tiley + (cache_amount - 1),
                     unit2ztile(WORLD_SIZE_UNITS, new_zoom));

    /* TODO check what tiles are already on the map */

    /* create the tiles */
    load_tiles_into_map(screen, repo, new_zoom,
                        start_tilex, start_tiley, stop_tilex, stop_tiley);

    /* Move the anchor point to the new center */
    px = unit2zpixel(x, new_zoom);
    py = unit2zpixel(y, new_zoom);
    clutter_actor_set_anchor_point(priv->map, px, py);

    /* if the zoom changed, update scale and zoom box */
    if (new_zoom != priv->zoom)
    {
        priv->zoom = new_zoom;
        update_scale_and_zoom(screen);
    }

    /* Update map data */
    priv->map_center_ux = x;
    priv->map_center_uy = y;

    /* draw the paths */
    overlay_redraw_idle(screen);
}

/**
 * map_screen_set_rotation:
 * @screen: the #MapScreen.
 * @angle: the new rotation angle.
 */
void
map_screen_set_rotation(MapScreen *screen, gint angle)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;
    clutter_actor_set_rotation(priv->map,
                               CLUTTER_Z_AXIS, -angle, 0, 0, 0);

    if (priv->compass)
    {
        clutter_actor_set_rotation(priv->compass,
                                   CLUTTER_Z_AXIS, -angle, 0, 0, 0);
    }
}

/**
 * map_screen_show_compass:
 * @screen: the #MapScreen.
 * @show: %TRUE if the compass should be shown.
 */
void
map_screen_show_compass(MapScreen *screen, gboolean show)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    priv->show_compass = show;
    if (show)
    {
        clutter_actor_show(priv->compass);
        clutter_actor_show(priv->compass_north);
    }
    else
    {
        clutter_actor_hide(priv->compass);
        clutter_actor_hide(priv->compass_north);
    }
}

/**
 * map_screen_show_scale:
 * @screen: the #MapScreen.
 * @show: %TRUE if the scale should be shown.
 */
void
map_screen_show_scale(MapScreen *screen, gboolean show)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    if (show)
        clutter_actor_show(priv->scale);
    else
        clutter_actor_hide(priv->scale);
}

/**
 * map_screen_show_zoom_box:
 * @screen: the #MapScreen.
 * @show: %TRUE if the zoom box should be shown.
 */
void
map_screen_show_zoom_box(MapScreen *screen, gboolean show)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    if (show)
        clutter_actor_show(priv->zoom_box);
    else
        clutter_actor_hide(priv->zoom_box);
}
