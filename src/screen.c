/* vi: set et sw=4 ts=8 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 8 -*- */
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
#include "screen.h"

#include "data.h"
#include "debug.h"
#include "defines.h"
#include "display.h"
#include "maps.h"
#include "mark.h"
#include "math.h"
#include "osm.h"
#include "path.h"
#include "poi.h"
#include "route.h"
#include "sign.h"
#include "tile.h"
#include "util.h"

#include <cairo/cairo.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-defines.h>
#include <string.h>

#define SCALE_WIDTH     100
#define SCALE_HEIGHT    22
#define ZOOM_WIDTH      25
#define ZOOM_HEIGHT     SCALE_HEIGHT

#define PANEL_WIDTH     200
#define PANEL_BORDER    2

/* duration of one step of the zoom animation */
#define ZOOM_DURATION   500

/* duration of the rotate animation */
#define ROTATE_DURATION   500

struct _MapScreenPrivate
{
    ClutterActor *map;
    gint map_center_ux;
    gint map_center_uy;

    ClutterActor *tile_group;
    ClutterActor *layers_group;
    ClutterActor *poi_group;

    /* On-screen Menu */
    ClutterActor *osm;

    ClutterActor *compass;
    ClutterActor *compass_north;
    ClutterActor *scale;
    ClutterActor *zoom_box;

    ClutterActor *message_box;

    ClutterActor *panel;

    /* marker for the GPS position/speed */
    ClutterActor *mark;

    /* layer for drawing over the map (used for paths) */
    ClutterActor *overlay;

    /* Sign for navigation directions */
    ClutterActor *sign;

    /* center of the draw layer, in pixels */
    gint overlay_center_px; /* Y is the same */

    /* upper left of the draw layer, in units */
    gint overlay_start_px;
    gint overlay_start_py;

    /* coordinates of last button press; reset x to -1 when released */
    gint btn_press_screen_x;
    gint btn_press_screen_y;

    gint zoom;

    guint source_overlay_redraw;
    guint source_panel_redraw;

    ClutterTimeline *rotate_tl;
    gint rotate_angle_start;
    gint rotate_angle_end;

    ClutterTimeline *zoom_tl;
    gint num_zoom_tl_completed;

    /* Ths flag sould be set while the user is panning the map or anyway when
     * we don't want the map view to be altered (moved or rotated) */
    gboolean grabbed;
    guint source_release_grab;

    guint zoom_direction_out : 1;

    /* Set this flag to TRUE when there is an action ongoing which requires
     * touchscreen interaction: this prevents the OSM from popping up */
    guint action_ongoing : 1;
    /* Set this flag to TRUE if action_ongoing is TRUE, but you still want to
     * allow the map to be scrolled (i.e., you are interested only in tap
     * events, not motion ones */
    guint allow_scrolling : 1;

    guint is_dragging : 1;

    guint show_compass : 1;

    guint is_disposed : 1;
};

typedef struct {
    gint icon_width;
    gint icon_height;

    /* track layout */
    PangoLayout *track_layout;
    gint track_height;
    gint track_gap;

    /* route layout */
    PangoLayout *route_layout;
    gint route_height;
    gint route_gap;

    /* waypoint layout */
    PangoLayout *wp_layout;
    gint wp_height;
    gint wp_gap;
} MapPanelData;

G_DEFINE_TYPE(MapScreen, map_screen, GTK_CLUTTER_TYPE_EMBED);

#define MAP_SCREEN_PRIV(screen) (MAP_SCREEN(screen)->priv)

static void map_screen_set_center_real(MapScreen *screen,
                                       gint x, gint y, gint zoom);

static gboolean
release_grab_real(MapScreen *self)
{
    MapController *controller = map_controller_get_instance();
    MapPoint p;
    gint angle;

    self->priv->grabbed = FALSE;
    self->priv->source_release_grab = 0;

    if (map_controller_get_center_mode(controller) != CENTER_MANUAL &&
        map_controller_get_gps_enabled(controller))
    {
        map_controller_get_center(controller, &p);
        angle = map_controller_get_rotation(controller);
        map_screen_set_center_real(self, p.x, p.y, -1);
        map_screen_set_rotation(self, angle);
    }

    return FALSE;
}

static void
release_grab(MapScreen *self)
{
    MapScreenPrivate *priv = self->priv;

    if (priv->source_release_grab)
        g_source_remove(priv->source_release_grab);

    priv->source_release_grab =
        g_timeout_add_seconds(5, (GSourceFunc)release_grab_real, self);
}

static void
on_zoom_tl_completed(ClutterTimeline *zoom_tl, MapScreen *self)
{
    MapScreenPrivate *priv = self->priv;
    MapController *controller = map_controller_get_instance();
    gint zoom, zoom_diff;
    gboolean ended;

    priv->num_zoom_tl_completed++;

    /* calculate the actual zoom level, to make sure we don't zoom too far or
     * too near */

    zoom_diff = priv->num_zoom_tl_completed;
    if (priv->zoom_direction_out)
        zoom_diff = -zoom_diff;
    zoom = map_controller_get_zoom(controller) - zoom_diff;

    if ((zoom <= MIN_ZOOM && !priv->zoom_direction_out) ||
        (zoom >= MAX_ZOOM && priv->zoom_direction_out))
    {
        map_screen_zoom_stop(self);
        clutter_timeline_stop(zoom_tl);
    }

    ended = !clutter_timeline_get_loop(zoom_tl);
    if (ended)
    {
        map_controller_set_zoom_no_act(controller, zoom);
        map_screen_set_center_real(self,
                                   priv->map_center_ux,
                                   priv->map_center_uy,
                                   zoom);
        map_mark_update(MAP_MARK(priv->mark));
    }
}

static void
on_zoom_tl_frame(ClutterTimeline *zoom_tl, gint elapsed, MapScreen *self)
{
    MapScreenPrivate *priv = self->priv;
    gfloat scale, exponent;

    /* For some reason (clutter bug?) the elapsed time passed to the signal
     * handler is always 0. So, we need to get it from the timeline. */
    elapsed = clutter_timeline_get_elapsed_time(zoom_tl);

    exponent = elapsed / (float)ZOOM_DURATION + priv->num_zoom_tl_completed;
    if (priv->zoom_direction_out)
        exponent = -exponent;
    scale = exp2f(exponent);
    clutter_actor_set_scale(priv->tile_group, scale, scale);
    clutter_actor_set_scale(priv->layers_group, scale, scale);
}

static void
actor_set_rotation_cb(ClutterActor *actor, gfloat angle)
{
    clutter_actor_set_rotation(actor, CLUTTER_Z_AXIS, angle, 0, 0, 0);
}

static void
map_screen_set_rotation_now(MapScreen *self, gfloat angle)
{
    MapScreenPrivate *priv = self->priv;
    union float_cast {
        gpointer ptr;
        gfloat angle;
    } cast;
    clutter_actor_set_rotation(priv->map,
                               CLUTTER_Z_AXIS, -angle, 0, 0, 0);

    if (priv->compass)
    {
        clutter_actor_set_rotation(priv->compass,
                                   CLUTTER_Z_AXIS, -angle, 0, 0, 0);
    }

    if (priv->poi_group)
    {
        cast.angle = angle;
        clutter_container_foreach(CLUTTER_CONTAINER(priv->poi_group),
                                  (ClutterCallback)actor_set_rotation_cb,
                                  cast.ptr);
    }
}

static void
on_rotate_tl_frame(ClutterTimeline *rotate_tl, gint elapsed, MapScreen *self)
{
    MapScreenPrivate *priv = self->priv;
    gfloat t, angle;

    /* For some reason (clutter bug?) the elapsed time passed to the signal
     * handler is always 0. So, we need to get it from the timeline. */
    elapsed = clutter_timeline_get_elapsed_time(rotate_tl);

    t = elapsed / (float) ROTATE_DURATION;
    angle = priv->rotate_angle_start * (1 - t) + priv->rotate_angle_end * t;
    map_screen_set_rotation_now(self, angle);
}

static void
map_screen_pixel_to_screen_units(MapScreenPrivate *priv, gint px, gint py,
                                 gint *ux, gint *uy)
{
    gfloat x, y, angle, sin_angle, cos_angle;
    gint px2, py2;

    angle = clutter_actor_get_rotation(priv->map, CLUTTER_Z_AXIS,
                                       NULL, NULL, NULL);
    angle = angle * PI / 180;
    cos_angle = cos(angle);
    sin_angle = sin(angle);
    x = px, y = py;
    px2 = x * cos_angle + y * sin_angle;
    py2 = y * cos_angle - x * sin_angle;
    *ux = pixel2zunit(px2, priv->zoom);
    *uy = pixel2zunit(py2, priv->zoom);
}

static inline void
map_screen_pixel_to_units(MapScreen *screen, gint px, gint py,
                          MapPoint *p)
{
    MapScreenPrivate *priv = screen->priv;
    GtkAllocation *allocation = &(GTK_WIDGET(screen)->allocation);
    gint usx, usy;

    px -= allocation->width / 2;
    py -= allocation->height / 2;
    map_screen_pixel_to_screen_units(priv, px, py, &usx, &usy);
    p->x = usx + priv->map_center_ux;
    p->y = usy + priv->map_center_uy;
}

static void
map_screen_add_poi(const MapPoint *p, GdkPixbuf *pixbuf, MapScreen *self)
{
    MapScreenPrivate *priv;
    ClutterActor *poi;
    gint angle, x, y;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    if (pixbuf)
        poi = gtk_clutter_texture_new_from_pixbuf(pixbuf);
    else
    {
        ClutterColor color;

        color.red = _color[COLORABLE_POI].red >> 8;
        color.green = _color[COLORABLE_POI].green >> 8;
        color.blue = _color[COLORABLE_POI].blue >> 8;
        color.alpha = 255;
        poi = clutter_rectangle_new_with_color(&color);
        clutter_actor_set_size(poi, 3 * _draw_width, 3 * _draw_width);
    }

    angle = clutter_actor_get_rotation(priv->map, CLUTTER_Z_AXIS,
                                       NULL, NULL, NULL);
    clutter_actor_set_rotation(poi, CLUTTER_Z_AXIS, -angle, 0, 0, 0);
    clutter_actor_set_anchor_point_from_gravity(poi, CLUTTER_GRAVITY_CENTER);
    x = unit2zpixel(p->x, priv->zoom);
    y = unit2zpixel(p->y, priv->zoom);
    clutter_actor_set_position(poi, x, y);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->poi_group), poi);
}

static inline void
activate_point_menu(MapScreen *screen, ClutterButtonEvent *event)
{
    MapController *controller;
    MapPoint p;

    /* Get the coordinates of the point, in units */
    map_screen_pixel_to_units(screen, event->x, event->y, &p);

    controller = map_controller_get_instance();
    map_controller_activate_menu_point(controller, &p);
}

static gboolean
on_point_chosen(ClutterActor *actor, ClutterButtonEvent *event,
                MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;

    g_signal_handlers_disconnect_by_func(actor, on_point_chosen, screen);
    clutter_actor_hide(priv->message_box);
    priv->action_ongoing = FALSE;

    activate_point_menu(screen, event);
    return TRUE; /* Event handled */
}

static gboolean
on_stage_event(ClutterActor *actor, ClutterEvent *event, MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;
    gboolean handled = FALSE;
    gint dx, dy;

    if (clutter_event_get_source(event) != actor) return FALSE;

    if (event->type == CLUTTER_BUTTON_PRESS)
    {
        ClutterButtonEvent *be = (ClutterButtonEvent *)event;
        priv->btn_press_screen_x = be->x;
        priv->btn_press_screen_y = be->y;

        if (!priv->action_ongoing)
        {
            map_osm_set_reactive(MAP_OSM(priv->osm), FALSE);
            map_osm_show(MAP_OSM(priv->osm));
        }

        if (priv->source_release_grab != 0)
        {
            g_source_remove(priv->source_release_grab);
            priv->source_release_grab = 0;
        }
    }
    else if (event->type == CLUTTER_BUTTON_RELEASE)
    {
        ClutterButtonEvent *be = (ClutterButtonEvent *)event;

        map_osm_set_reactive(MAP_OSM(priv->osm), TRUE);
        if (priv->is_dragging)
        {
            MapController *controller = map_controller_get_instance();
            MapPoint p;

            map_screen_pixel_to_screen_units(priv,
                                             be->x - priv->btn_press_screen_x,
                                             be->y - priv->btn_press_screen_y,
                                             &dx, &dy);
            p.x = priv->map_center_ux - dx;
            p.y = priv->map_center_uy - dy;
            map_screen_set_center_real(screen, p.x, p.y, -1);
            if (map_controller_get_center_mode(controller) == CENTER_MANUAL ||
                !map_controller_get_gps_enabled(controller))
                map_controller_set_center_no_act(controller, p, -1);
            handled = TRUE;
        }

        priv->btn_press_screen_x = -1;
        priv->is_dragging = FALSE;

        map_osm_hide(MAP_OSM(priv->osm));

        if (priv->grabbed)
            release_grab(screen);

        if (be->click_count > 1)
        {
            /* activating the point menu seems a reasonable action for double
             * clicks */
            activate_point_menu(screen, be);
        }
    }
    else if (!priv->action_ongoing || priv->allow_scrolling) /* motion event */
    {
        ClutterMotionEvent *me = (ClutterMotionEvent *)event;

        if (!(me->modifier_state & CLUTTER_BUTTON1_MASK))
            return TRUE; /* ignore pure motion events */

        if (priv->btn_press_screen_x == -1)
            return TRUE; /* do nothing if we missed the button press */

        dx = me->x - priv->btn_press_screen_x;
        dy = me->y - priv->btn_press_screen_y;

        if (!priv->is_dragging &&
            (ABS(dx) > TOUCH_RADIUS || ABS(dy) > TOUCH_RADIUS))
        {
            priv->is_dragging = TRUE;
            priv->grabbed = TRUE;
        }

        if (priv->is_dragging)
        {
            GtkAllocation *allocation = &(GTK_WIDGET(screen)->allocation);

            clutter_actor_set_position(priv->map,
                                       allocation->width / 2 + dx,
                                       allocation->height / 2 + dy);
            handled = TRUE;
        }
    }

    return handled;
}

static inline void
point_to_pixels(MapScreenPrivate *priv, const MapPoint p, gint *x, gint *y)
{
    *x = unit2zpixel(p.x, priv->zoom) - priv->overlay_start_px;
    *y = unit2zpixel(p.y, priv->zoom) - priv->overlay_start_py;
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
    cairo_arc(cr, x, y, _draw_width * 3 / 2, 0, 2 * PI);
    set_source_color(cr, color);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void
draw_path(MapScreen *screen, cairo_t *cr, Path *path, Colorable base)
{
    MapScreenPrivate *priv = screen->priv;
    MapLineIter line;
    Point *curr;
    WayPoint *wcurr;
    gint x = 0, y = 0;
#ifdef ENABLE_DEBUG
    gint segment_count = 0;
    gint waypoint_count = 0;
#endif

    DEBUG("");

    set_source_color(cr, &_color[base]);
    cairo_set_line_width(cr, _draw_width);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
    map_path_line_iter_first(path, &line);
    do
    {
        Point *start, *end;
        gint len;

        start = map_path_line_first(&line);
        len = map_path_line_len(&line);
        end = start + len;

        if (len > 0)
        {
            point_to_pixels(priv, start->unit, &x, &y);
            draw_break(cr, &_color[base + 2], x, y);
            cairo_move_to(cr, x, y);
        }

        for (curr = start + 1; curr < end - 1; curr++)
        {
            if (curr->zoom <= priv->zoom) continue;
#ifdef ENABLE_DEBUG
            segment_count++;
#endif
            point_to_pixels(priv, curr->unit, &x, &y);
            cairo_line_to(cr, x, y);
        }

        /* the last segment must always be drawn */
        if (len > 1)
        {
            point_to_pixels(priv, curr->unit, &x, &y);
            cairo_line_to(cr, x, y);
        }

        cairo_stroke(cr);
        draw_break(cr, &_color[base + 2], x, y);
    }
    while (map_path_line_iter_next(&line));

    for (wcurr = path->whead; wcurr <= path->wtail; wcurr++)
    {
        gint x1, y1;

        point_to_pixels(priv, wcurr->point->unit, &x1, &y1);
        draw_break(cr, &_color[base + 1], x1, y1);
        cairo_move_to(cr, x1, y1);
#ifdef ENABLE_DEBUG
        waypoint_count++;
#endif
    }

#ifdef ENABLE_DEBUG
    DEBUG("Drawn %d segments, %d waypoints", segment_count, waypoint_count);
#endif
}

static void
draw_paths(MapScreen *screen, cairo_t *cr)
{
    if ((_show_paths & ROUTES_MASK) && map_route_exists())
    {
        WayPoint *next_way;

        draw_path(screen, cr, map_route_get_path(), COLORABLE_ROUTE);
        next_way = map_route_get_next_waypoint();

        /* Now, draw the next waypoint on top of all other waypoints. */
        if (next_way)
        {
            gint x1, y1;
            point_to_pixels(screen->priv, next_way->point->unit, &x1, &y1);
            draw_break(cr, &_color[COLORABLE_ROUTE_NEXT], x1, y1);
        }
    }

    if (_show_paths & TRACKS_MASK)
        draw_path(screen, cr, &_track, COLORABLE_TRACK);
}

static gboolean
overlay_redraw_real(MapScreen *screen)
{
    MapScreenPrivate *priv;
    GtkAllocation *allocation;
    gfloat center_x, center_y;
    cairo_t *cr;

    g_return_val_if_fail (MAP_IS_SCREEN (screen), FALSE);
    priv = screen->priv;

    clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->overlay));
    allocation = &(GTK_WIDGET(screen)->allocation);
    clutter_actor_get_anchor_point(priv->map, &center_x, &center_y);
    clutter_actor_set_position(priv->overlay, center_x, center_y);

    priv->overlay_start_px = center_x - priv->overlay_center_px;
    priv->overlay_start_py = center_y - priv->overlay_center_px;

    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->overlay));
    g_assert(cr != NULL);

    draw_paths(screen, cr);

    cairo_destroy(cr);
    clutter_actor_show(priv->overlay);

    priv->source_overlay_redraw = 0;
    return FALSE;
}

static void
overlay_redraw_idle(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;

    if (priv->source_overlay_redraw == 0 && GTK_WIDGET_REALIZED(screen))
    {
        priv->source_overlay_redraw =
            g_idle_add((GSourceFunc)overlay_redraw_real, screen);
    }
}

static void
load_tiles_into_map(MapScreen *screen, Repository *repo, gint zoom,
                    gint tx1, gint ty1, gint tx2, gint ty2)
{
    MapScreenPrivate *priv = screen->priv;
    ClutterContainer *tile_group, *layers_group;
    ClutterActor *tile;
    gfloat center_x, center_y;
    gint tx, ty, layer;
    RepositoryLayer *repo_layer;

    tile_group = CLUTTER_CONTAINER(screen->priv->tile_group);
    layers_group = CLUTTER_CONTAINER(screen->priv->layers_group);

    clutter_actor_set_scale(priv->tile_group, 1, 1);
    clutter_actor_set_scale(priv->layers_group, 1, 1);
    /* hide all the existing tiles */
    clutter_container_foreach(tile_group,
                              (ClutterCallback)clutter_actor_hide, NULL);
    clutter_container_foreach(layers_group,
                              (ClutterCallback)clutter_actor_hide, NULL);

    clutter_actor_get_anchor_point(priv->map, &center_x, &center_y);

    clutter_actor_set_position(priv->tile_group, center_x, center_y);
    clutter_actor_set_anchor_point(priv->tile_group,
                                   center_x - (tx1 * TILE_SIZE_PIXELS),
                                   center_y - (ty1 * TILE_SIZE_PIXELS));

    clutter_actor_set_position(priv->layers_group, center_x, center_y);
    clutter_actor_set_anchor_point(priv->layers_group,
                                   center_x - (tx1 * TILE_SIZE_PIXELS),
                                   center_y - (ty1 * TILE_SIZE_PIXELS));

    for (tx = tx1; tx <= tx2; tx++)
    {
        for (ty = ty1; ty <= ty2; ty++)
        {
            tile = map_tile_cached(repo->primary, zoom, tx, ty);
            if (!tile)
            {
                gboolean new_tile;
                tile = map_tile_load(repo->primary, zoom, tx, ty, &new_tile);
                if (new_tile)
                    clutter_container_add_actor(tile_group, tile);
            }

            clutter_actor_set_position(tile,
                                       (tx - tx1) * TILE_SIZE_PIXELS,
                                       (ty - ty1) * TILE_SIZE_PIXELS);
            clutter_actor_show(tile);

            /* Handle layers */
            if (repo->layers) {
                for (layer = 0; layer < repo->layers->len; layer++)
                {
                    repo_layer = g_ptr_array_index(repo->layers, layer);
                    if (!repo_layer || !repo_layer->visible)
                        continue;
                    tile = map_tile_cached(repo_layer->ts, zoom, tx, ty);
                    if (!tile)
                    {
                        gboolean new_tile;
                        tile = map_tile_load(repo_layer->ts, zoom, tx, ty, &new_tile);
                        if (new_tile)
                            clutter_container_add_actor(layers_group, tile);
                    }

                    clutter_actor_set_position(tile,
                                               (tx - tx1) * TILE_SIZE_PIXELS,
                                               (ty - ty1) * TILE_SIZE_PIXELS);
                    clutter_actor_show(tile);
                }
            }
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
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
    cairo_destroy(cr);

    clutter_actor_set_anchor_point(priv->compass, 20, 45);

    /* create the "N" letter */
    width = 16, height = 16;
    priv->compass_north = clutter_cairo_texture_new(width, height);
    cr = clutter_cairo_texture_create
        (CLUTTER_CAIRO_TEXTURE(priv->compass_north));
    cairo_set_source_rgb (cr, 0, 0, 0);
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
    MapGeo lat1, lon1, lat2, lon2;

    unit2latlon(_center.x - pixel2zunit(SCALE_WIDTH / 2 - 4, zoom),
                _center.y, lat1, lon1);
    unit2latlon(_center.x + pixel2zunit(SCALE_WIDTH / 2 - 4, zoom),
                _center.y, lat2, lon2);
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
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 4);
    cairo_stroke(cr);

    /* text in scale box */
    compute_scale_text(buffer, sizeof(buffer), priv->zoom);
    cairo_set_source_rgb (cr, 0, 0, 0);
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
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 4);
    cairo_stroke(cr);

    snprintf(buffer, sizeof(buffer), "%d", priv->zoom);
    cairo_set_source_rgb (cr, 0, 0, 0);
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
create_mark(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;

    priv->mark = g_object_new(MAP_TYPE_MARK, NULL);
}

static gboolean
panel_create_layouts(MapPanelData *pd, PangoContext *context)
{
    PangoLayout *layout;
    static PangoFontDescription *font = NULL;
    const WayPoint *waypoint;
    gint w, h;
    gboolean has_data = FALSE;

    if (!font)
        font = pango_font_description_from_string("Sans 14");

    if (map_path_len(&_track))
    {
        gchar *text, buffer[32];
        guint duration;
        gint n = 0;

        if (has_data) pd->track_gap = PANEL_BORDER;
        layout = pango_layout_new(context);
        pango_layout_set_width(layout,
                               (PANEL_WIDTH - pd->icon_width) * PANGO_SCALE);
        pango_layout_set_font_description(layout, font);

        n += distance_to_string(buffer + n, sizeof(buffer) - n, _track.length);
        duration = map_path_get_duration(&_track);
        if (duration != 0)
        {
            buffer[n++] = ' ';
            n += duration_to_string(buffer + n, sizeof(buffer) - n, duration);
        }
        text = g_strdup_printf("%s", buffer);
        pango_layout_set_markup(layout, text, -1);
        g_free(text);
        pango_layout_get_pixel_size(layout, &w, &h);
        pd->track_layout = layout;

        pd->track_height += MAX(pd->icon_height, h);
        has_data = TRUE;
    }

    if (map_route_exists())
    {
        gchar *text, buffer[32];
        Path *route = map_route_get_path();
        time_t time;
        gint n = 0;
        Point *p;

        if (has_data) pd->route_gap = PANEL_BORDER;
        layout = pango_layout_new(context);
        pango_layout_set_width(layout,
                               (PANEL_WIDTH - pd->icon_width) * PANGO_SCALE);
        pango_layout_set_font_description(layout, font);

        /* Find last point. */
        p = map_path_last(route);
        n += distance_to_string(buffer + n, sizeof(buffer) - n, route->length);

        time = p->time;
        if (time != 0)
            n += time_to_string(buffer + n, sizeof(buffer) - n, " %H:%M", time);

        text = g_strdup_printf("<b>%s</b>", buffer);
        pango_layout_set_markup(layout, text, -1);
        g_free(text);
        pango_layout_get_pixel_size(layout, &w, &h);
        pd->route_layout = layout;

        pd->route_height += MAX(pd->icon_height, h);
        has_data = TRUE;
    }

    waypoint = map_route_get_next_waypoint();
    if (waypoint)
    {
        gchar *text, buffer[32];
        gfloat distance;
        gint n = 0;

        if (has_data) pd->wp_gap = PANEL_BORDER;
        layout = pango_layout_new(context);
        pango_layout_set_width(layout,
                               (PANEL_WIDTH - pd->icon_width) * PANGO_SCALE);
        pango_layout_set_font_description(layout, font);

        if (waypoint->point->time != 0)
            n += time_to_string(buffer + n, sizeof(buffer) - n, "%H:%M ",
                                waypoint->point->time);
        distance = map_route_get_distance_to_next_waypoint();
        distance_to_string(buffer + n, sizeof(buffer) - n, distance);
        text = g_strdup_printf("<b>%s</b>\n%s", buffer,
                               waypoint->desc ? waypoint->desc : "");
        pango_layout_set_markup(layout, text, -1);
        g_free(text);
        pango_layout_get_pixel_size(layout, &w, &h);
        pd->wp_layout = layout;


        pd->wp_height += MAX(pd->icon_height, h);
        has_data = TRUE;
    }

    return has_data;
}

static void
draw_icon_flag(cairo_t *cr, GdkColor *color, gint x, gint y, gint size)
{
    const gint size4 = size / 4;
    cairo_save(cr);
    cairo_move_to(cr, x + size4, y + size);
    cairo_line_to(cr, x + size4, y);
    cairo_line_to(cr, x + size - size4, y + size4);
    cairo_line_to(cr, x + size4, y + size / 2);
    set_source_color(cr, color);
    cairo_set_line_width(cr, 2);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void
draw_icon_path(cairo_t *cr, GdkColor *color, gint x, gint y, gint size)
{
    cairo_save(cr);
    cairo_move_to(cr, x + 4, y);
    cairo_line_to(cr, x + 12, y + 8);
    cairo_line_to(cr, x + 2, y + 16);
    cairo_line_to(cr, x + 8, y + size);
    set_source_color(cr, color);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
    cairo_restore(cr);
}

static gboolean
panel_redraw_real(MapScreen *self)
{
    MapScreenPrivate *priv;
    PangoContext *context;
    gint panel_height, panel_x, panel_y, x, y;
    MapPanelData pd;
    cairo_t *cr;

    priv = self->priv;
    priv->source_panel_redraw = 0;

    /* before drawing, calculate the height of the panel */
    context = gtk_widget_get_pango_context(GTK_WIDGET(self));
    memset(&pd, 0, sizeof(pd));

    pd.icon_width = pd.icon_height = _draw_width * 4;
    if (!panel_create_layouts(&pd, context))
    {
        /* nothing to display, hide the panel */
        clutter_actor_hide(priv->panel);
        return FALSE;
    }

    panel_height = PANEL_BORDER * 2 +
        pd.track_height + pd.track_gap +
        pd.route_height + pd.route_gap +
        pd.wp_height + pd.wp_gap;
    clutter_cairo_texture_set_surface_size(CLUTTER_CAIRO_TEXTURE(priv->panel),
                                           PANEL_WIDTH, panel_height);
    clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->panel));

    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->panel));
    g_assert(cr != NULL);

    cairo_rectangle(cr, 0, 0, PANEL_WIDTH, panel_height);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.6);
    cairo_fill(cr);

    panel_x = panel_y = PANEL_BORDER;

    cairo_set_source_rgb(cr, 0, 0, 0);

    x = panel_x, y = panel_y;

    if (pd.track_layout)
    {
        x = panel_x;
        y += pd.track_gap;

        draw_icon_path(cr, &_color[COLORABLE_TRACK],
                       x, y, pd.icon_height);
        x += pd.icon_width;

        cairo_move_to(cr, x, y);
        pango_cairo_show_layout(cr, pd.track_layout);
        g_object_unref(pd.track_layout);

        y += pd.track_height;
    }

    if (pd.route_layout)
    {
        x = panel_x;
        y += pd.route_gap;

        draw_icon_flag(cr, &_color[COLORABLE_ROUTE],
                       x, y, pd.icon_height);
        x += pd.icon_width;

        cairo_move_to(cr, x, y);
        pango_cairo_show_layout(cr, pd.route_layout);
        g_object_unref(pd.route_layout);

        y += pd.route_height;
    }

    if (pd.wp_layout)
    {
        x = panel_x;
        y += pd.wp_gap;

        draw_break(cr, &_color[COLORABLE_ROUTE_NEXT],
                   x + _draw_width * 3 / 2, y + _draw_width * 3 / 2);
        x += pd.icon_width;

        cairo_move_to(cr, x, y);
        pango_cairo_show_layout(cr, pd.wp_layout);
        g_object_unref(pd.wp_layout);

        y += pd.wp_height;
    }

    cairo_destroy(cr);

    clutter_actor_show(priv->panel);
    return FALSE;
}

static void
create_panel(MapScreen *screen)
{
    MapScreenPrivate *priv = screen->priv;

    priv->panel = clutter_cairo_texture_new(PANEL_WIDTH, 1);
    clutter_actor_set_anchor_point(priv->panel, PANEL_WIDTH, 0);
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

    /* route info, upper right corner */
    clutter_actor_set_position(priv->panel,
                               allocation->width - HILDON_MARGIN_HALF,
                               HILDON_MARGIN_HALF);

    /* resize the OSM */
    map_osm_set_screen_size(MAP_OSM(priv->osm),
                            allocation->width, allocation->height);

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

    /* message box */
    if (priv->message_box)
        clutter_actor_set_size(priv->message_box,
                               allocation->width - 2 * HILDON_MARGIN_DOUBLE,
                               allocation->height - 2 * HILDON_MARGIN_DOUBLE);
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

    if (priv->source_panel_redraw != 0)
    {
        g_source_remove (priv->source_panel_redraw);
        priv->source_panel_redraw = 0;
    }

    if (priv->source_release_grab != 0)
    {
        g_source_remove (priv->source_release_grab);
        priv->source_release_grab = 0;
    }

    G_OBJECT_CLASS(map_screen_parent_class)->dispose(object);
}

static void
map_screen_init(MapScreen *screen)
{
    MapScreenPrivate *priv;
    ClutterActor *stage;
    ClutterBackend *backend;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(screen, MAP_TYPE_SCREEN,
                                       MapScreenPrivate);
    screen->priv = priv;

    backend = clutter_get_default_backend();
    clutter_backend_set_double_click_distance(backend, TOUCH_RADIUS);

    stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(screen));
    g_return_if_fail(stage != NULL);
    g_signal_connect(stage, "event",
                     G_CALLBACK(on_stage_event), screen);
    priv->btn_press_screen_x = -1;

    priv->map = clutter_group_new();
    g_return_if_fail(priv->map != NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->map);
    clutter_actor_show(priv->map);

    priv->tile_group = clutter_group_new();
    g_return_if_fail(priv->tile_group != NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->tile_group);
    clutter_actor_show(priv->tile_group);

    priv->layers_group = clutter_group_new();
    g_return_if_fail(priv->layers_group != NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->layers_group);
    clutter_actor_show(priv->layers_group);

    priv->poi_group = clutter_group_new();
    g_return_if_fail(priv->poi_group != NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->poi_group);
    clutter_actor_show(priv->poi_group);

    create_compass(screen);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->compass);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->compass_north);

    priv->overlay = clutter_cairo_texture_new(0, 0);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->overlay);
    clutter_actor_show(priv->overlay);

    create_scale_and_zoom(screen);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->scale);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->zoom_box);

    create_mark(screen);
    clutter_container_add_actor(CLUTTER_CONTAINER(priv->map), priv->mark);

    /* Route Info */
    create_panel(screen);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->panel);

    /* On-screen Menu */
    priv->osm = g_object_new(MAP_TYPE_OSM,
                             "widget", screen,
                             NULL);
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->osm);
    map_osm_hide(MAP_OSM(priv->osm));

    /* Zoom timeline */
    priv->zoom_tl = clutter_timeline_new(ZOOM_DURATION);
    g_signal_connect(priv->zoom_tl, "new-frame",
                     G_CALLBACK(on_zoom_tl_frame), screen);
    g_signal_connect(priv->zoom_tl, "completed",
                     G_CALLBACK(on_zoom_tl_completed), screen);

    /* Rotate timeline */
    priv->rotate_tl = clutter_timeline_new(ROTATE_DURATION);
    g_signal_connect(priv->rotate_tl, "new-frame",
                     G_CALLBACK(on_rotate_tl_frame), screen);
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

static void
map_screen_set_center_real(MapScreen *screen, gint x, gint y, gint zoom)
{
    MapController *controller = map_controller_get_instance();
    Repository *repository;
    MapScreenPrivate *priv;
    GtkAllocation *allocation;
    gint halflength_units;
    gint start_tilex, start_tiley, stop_tilex, stop_tiley;
    MapArea area;
    gint px, py;
    gint new_zoom;

    priv = screen->priv;

    DEBUG("(%u, %u) zoom %d", x, y, zoom);
    allocation = &(GTK_WIDGET(screen)->allocation);
    clutter_actor_set_position(priv->map,
                               allocation->width / 2,
                               allocation->height / 2);

    new_zoom = (zoom > 0) ? zoom : priv->zoom;

    /* Move the anchor point to the new center */
    px = unit2zpixel(x, new_zoom);
    py = unit2zpixel(y, new_zoom);
    clutter_actor_set_anchor_point(priv->map, px, py);

    halflength_units =
        pixel2zunit(MAX(allocation->width, allocation->height) / 2,
                    new_zoom);

    area.x1 = x - halflength_units;
    area.y1 = y - halflength_units;
    area.x2 = x + halflength_units + TILE_SIZE_PIXELS - 1;
    area.y2 = y + halflength_units + TILE_SIZE_PIXELS - 1;

    repository = map_controller_get_repository(controller);

    start_tilex = unit2ztile(area.x1, new_zoom);
    start_tilex = MAX(start_tilex, 0);
    start_tiley = unit2ztile(area.y1, new_zoom);
    start_tiley = MAX(start_tiley, 0);
    stop_tilex = unit2ztile(area.x2, new_zoom);
    stop_tilex = MIN(stop_tilex,
                     unit2ztile(WORLD_SIZE_UNITS, new_zoom));
    stop_tiley = unit2ztile(area.y2, new_zoom);
    stop_tiley = MIN(stop_tiley,
                     unit2ztile(WORLD_SIZE_UNITS, new_zoom));

    /* create the tiles */
    load_tiles_into_map(screen, repository, new_zoom,
                        start_tilex, start_tiley, stop_tilex, stop_tiley);

    /* if the zoom changed, update scale, mark and zoom box */
    if (new_zoom != priv->zoom)
    {
        priv->zoom = new_zoom;
        update_scale_and_zoom(screen);
        map_mark_update(MAP_MARK(priv->mark));
        clutter_actor_show(priv->mark);
    }

    /* Update map data */
    priv->map_center_ux = x;
    priv->map_center_uy = y;

    /* render the POIs */
    map_screen_refresh_pois(screen, &area);

    /* draw the paths */
    overlay_redraw_idle(screen);
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

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    if (priv->grabbed)
        return;

    map_screen_set_center_real(screen, x, y, zoom);
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
    gint diff;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    if (priv->grabbed)
        return;

    priv->rotate_angle_start =
        -clutter_actor_get_rotation(priv->map, CLUTTER_Z_AXIS,
                                    NULL, NULL, NULL);
    priv->rotate_angle_start %= 360;
    priv->rotate_angle_end = angle % 360;

    if (clutter_timeline_is_playing(priv->rotate_tl))
        clutter_timeline_stop(priv->rotate_tl);

    diff = priv->rotate_angle_end - priv->rotate_angle_start;
    if (diff != 0)
    {
        if (diff < 0) diff = -diff;

        /* always rotate via the shortest angle */
        if (diff > 180)
            priv->rotate_angle_start += 360;
        clutter_timeline_start(priv->rotate_tl);
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
 * map_screen_show_message:
 * @screen: the #MapScreen.
 * @text: the message to be shown.
 *
 * Shows a message to the user.
 */
void
map_screen_show_message(MapScreen *screen, const gchar *text)
{
    MapScreenPrivate *priv = screen->priv;

    if (!priv->message_box)
    {
        ClutterActor *stage;
        GtkAllocation *allocation;

        priv->message_box = clutter_text_new();
        clutter_actor_set_position(priv->message_box,
                                   HILDON_MARGIN_DOUBLE,
                                   HILDON_MARGIN_DOUBLE);
        allocation = &(GTK_WIDGET(screen)->allocation);
        clutter_actor_set_size(priv->message_box,
                               allocation->width - 2 * HILDON_MARGIN_DOUBLE,
                               allocation->height - 2 * HILDON_MARGIN_DOUBLE);
        clutter_text_set_font_name(CLUTTER_TEXT(priv->message_box),
                                   "Sans Serif 20");
        clutter_text_set_line_wrap(CLUTTER_TEXT(priv->message_box), TRUE);
        clutter_text_set_line_alignment(CLUTTER_TEXT(priv->message_box),
                                        PANGO_ALIGN_CENTER);
        stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(screen));
        clutter_container_add_actor(CLUTTER_CONTAINER(stage),
                                    priv->message_box);
    }
    clutter_text_set_text(CLUTTER_TEXT(priv->message_box), text);
    clutter_actor_show(priv->message_box);
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

/**
 * map_screen_update_mark:
 * @screen: the #MapScreen.
 */
void
map_screen_update_mark(MapScreen *screen)
{
    g_return_if_fail(MAP_IS_SCREEN(screen));
    map_mark_update(MAP_MARK(screen->priv->mark));
}

void
map_screen_zoom_start(MapScreen *self, MapScreenZoomDirection dir)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    if (clutter_timeline_is_playing(priv->zoom_tl))
    {
        /* A zooming operation is still running; don't start a second one */
        return;
    }

    /* temporarily hide the overlays */
    clutter_actor_hide(priv->poi_group);
    clutter_actor_hide(priv->mark);
    clutter_actor_hide(priv->overlay);

    priv->zoom_direction_out = (dir == MAP_SCREEN_ZOOM_OUT);
    priv->num_zoom_tl_completed = 0;
    clutter_timeline_set_loop(priv->zoom_tl, TRUE);
    clutter_timeline_start(priv->zoom_tl);
}

void
map_screen_zoom_stop(MapScreen *self)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    if (!clutter_timeline_is_playing(priv->zoom_tl))
        return;

    clutter_timeline_set_loop(priv->zoom_tl, FALSE);
}

void
map_screen_action_point(MapScreen *screen)
{
    MapScreenPrivate *priv;
    ClutterActor *stage;

    g_return_if_fail(MAP_IS_SCREEN(screen));
    priv = screen->priv;

    clutter_actor_hide(priv->osm);
    priv->action_ongoing = TRUE;
    priv->allow_scrolling = TRUE;
    stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(screen));
    g_signal_connect(stage, "button-release-event",
                     G_CALLBACK(on_point_chosen), screen);
    map_screen_show_message(screen, _("Tap a point on the map"));
}

/**
 * map_screen_show_pois:
 * @self: the #MapScreen.
 * @show: %TRUE if the POIs should be shown.
 */
void
map_screen_show_pois(MapScreen *self, gboolean show)
{
    g_return_if_fail(MAP_IS_SCREEN(self));

    if (show)
        clutter_actor_show(self->priv->poi_group);
    else
        clutter_actor_hide(self->priv->poi_group);
}

/**
 * map_screen_clear_pois:
 * @self: the #MapScreen.
 *
 * Removes all POIs from the map.
 */
void
map_screen_clear_pois(MapScreen *self)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    clutter_group_remove_all(CLUTTER_GROUP(priv->poi_group));
}

void
map_screen_get_tap_area_from_units(MapScreen *self, const MapPoint *p,
                                   MapArea *area)
{
    gint radius;

    g_return_if_fail(MAP_IS_SCREEN(self));

    radius = pixel2zunit(TOUCH_RADIUS, self->priv->zoom);
    area->x1 = p->x - radius;
    area->y1 = p->y - radius;
    area->y2 = p->y + radius;
    area->x2 = p->x + radius;
}

void
map_screen_redraw_overlays(MapScreen *self)
{
    g_return_if_fail(MAP_IS_SCREEN(self));
    overlay_redraw_idle(self);
}

void
map_screen_refresh_tiles(MapScreen *self)
{
    g_return_if_fail(MAP_IS_SCREEN(self));

    /* hide all the existing tiles */
    clutter_container_foreach(CLUTTER_CONTAINER(self->priv->tile_group),
                              (ClutterCallback)map_tile_refresh, NULL);
    clutter_container_foreach(CLUTTER_CONTAINER(self->priv->layers_group),
                              (ClutterCallback)map_tile_refresh, NULL);
}


void
map_screen_refresh_map(MapScreen *self)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(self));

    priv = self->priv;

    map_screen_set_center_real(self, priv->map_center_ux, priv->map_center_uy, priv->zoom);
}



void
map_screen_track_append(MapScreen *self, const Point *p)
{
    MapScreenPrivate *priv;
    gint x, y;
    cairo_t *cr;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    /* if no track is to be shown, do nothing */
    if (!(_show_paths & TRACKS_MASK))
        return;

    /* if a redraw is queued, do nothing */
    if (priv->source_overlay_redraw != 0)
        return;

    cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->overlay));
    g_assert(cr != NULL);

    set_source_color(cr, &_color[COLORABLE_TRACK]);
    cairo_set_line_width(cr, _draw_width);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
    if (!map_path_end_is_break(&_track))
    {
        Point *last = map_path_last(&_track);
        point_to_pixels(priv, last->unit, &x, &y);
        cairo_move_to(cr, x, y);
        point_to_pixels(priv, p->unit, &x, &y);
        cairo_line_to(cr, x, y);
    }
    else
    {
        point_to_pixels(priv, p->unit, &x, &y);
        draw_break(cr, &_color[COLORABLE_TRACK_BREAK], x, y);
    }

    cairo_stroke(cr);

    cairo_destroy(cr);
}

void
map_screen_refresh_pois(MapScreen *self, MapArea *poi_area)
{
    MapController *controller = map_controller_get_instance();
    MapScreenPrivate *priv;
    MapArea area;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    map_screen_clear_pois(self);
    if (map_controller_get_show_poi(controller) && _poi_zoom > priv->zoom)
    {
        if (!poi_area)
        {
            GtkAllocation *allocation = &(GTK_WIDGET(self)->allocation);
            gint x, y, diag_halflength_units;

            x = priv->map_center_ux;
            y = priv->map_center_uy;
            diag_halflength_units =
                pixel2zunit(TILE_HALFDIAG_PIXELS +
                            MAX(allocation->width, allocation->height) / 2,
                            priv->zoom);

            area.x1 = x - diag_halflength_units;
            area.y1 = y - diag_halflength_units;
            area.x2 = x + diag_halflength_units;
            area.y2 = y + diag_halflength_units;

            poi_area = &area;
        }
        map_poi_render(poi_area, (MapPoiRenderCb)map_screen_add_poi, self);
        clutter_actor_show(priv->poi_group);
    }
}

void
map_screen_refresh_panel(MapScreen *self)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    if (priv->source_panel_redraw == 0 && GTK_WIDGET_REALIZED(self))
        priv->source_panel_redraw =
            g_idle_add((GSourceFunc)panel_redraw_real, self);
}

void
map_screen_set_best_center(MapScreen *self)
{
    MapController *controller = map_controller_get_instance();
    GtkAllocation *allocation;
    MapScreenPrivate *priv;
    MapPoint new_center;
    gint max_distance, dx, dy;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    map_controller_calc_best_center(controller, &new_center);

    /* If the new center doesn't fall near the center of the screen, we move
     * the map.  In computing this, we don't care about being too precise. We
     * ignore the map rotation, too.
     */
    allocation = &(GTK_WIDGET(self)->allocation);
    max_distance = (allocation->width + allocation->height) / 16;

    dx = unit2zpixel(priv->map_center_ux - new_center.x, priv->zoom);
    dy = unit2zpixel(priv->map_center_uy - new_center.y, priv->zoom);

    if (ABS(dx) > max_distance || ABS(dy) > max_distance)
    {
        map_controller_set_center(controller, new_center, -1);
    }
}

void
map_screen_toggle_layers_visibility(MapScreen *self)
{
    g_return_if_fail(MAP_IS_SCREEN(self));

    if (CLUTTER_ACTOR_IS_VISIBLE(self->priv->layers_group))
        clutter_actor_hide(self->priv->layers_group);
    else
        clutter_actor_show(self->priv->layers_group);
}

void
map_screen_show_sign(MapScreen *self, MapDirection dir, const gchar *text,
                     gfloat distance)
{
    MapScreenPrivate *priv;

    g_return_if_fail(MAP_IS_SCREEN(self));
    priv = self->priv;

    if (!priv->sign)
    {
        ClutterActor *stage;
        PangoContext *context;

        stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self));
        context = gtk_widget_get_pango_context(GTK_WIDGET(self));

        priv->sign = g_object_new(MAP_TYPE_SIGN, NULL);
        map_sign_set_pango_context(MAP_SIGN(priv->sign), context);
        clutter_container_add_actor(CLUTTER_CONTAINER(stage), priv->sign);
        /* move it below the OSM */
        clutter_actor_lower(priv->sign, priv->osm);
    }

    map_sign_set_info(MAP_SIGN(priv->sign), dir, text, distance);
    clutter_actor_show(priv->sign);
}

void
map_screen_update_sign(MapScreen *self, gfloat distance)
{
    g_return_if_fail(MAP_IS_SCREEN(self));

    if (self->priv->sign)
        map_sign_set_distance(MAP_SIGN(self->priv->sign), distance);
}

void
map_screen_hide_sign(MapScreen *self)
{
    g_return_if_fail(MAP_IS_SCREEN(self));

    if (self->priv->sign)
        clutter_actor_hide(self->priv->sign);
}

