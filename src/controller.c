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
#include "controller.h"

#include "cmenu.h"
#include "data.h"
#include "defines.h"
#include "display.h"
#include "gps.h"
#include "menu.h"
#include "path.h"
#include "screen.h"

#include <hildon/hildon-banner.h>
#include <math.h>

#define VELVEC_SIZE_FACTOR (4)

struct _MapControllerPrivate
{
    MapScreen *screen;
    Point center;
    gint rotation_angle;
    gint zoom;

    guint source_map_center;
    guint is_disposed : 1;
};

G_DEFINE_TYPE(MapController, map_controller, G_TYPE_OBJECT);

#define MAP_CONTROLLER_PRIV(controller) (MAP_CONTROLLER(controller)->priv)

static MapController *instance = NULL;

static gboolean
activate_gps()
{
    /* Connect to receiver. */
    if (_enable_gps)
        rcvr_connect();
    return FALSE;
}

static gboolean
set_center_real(MapController *self)
{
    MapControllerPrivate *priv = self->priv;

    map_screen_set_center(priv->screen,
                          priv->center.unitx, priv->center.unity, priv->zoom);
    priv->source_map_center = 0;
    return FALSE;
}

static void
map_controller_dispose(GObject *object)
{
    MapControllerPrivate *priv = MAP_CONTROLLER_PRIV(object);

    if (priv->is_disposed)
        return;

    priv->is_disposed = TRUE;

    if (priv->source_map_center != 0)
    {
        g_source_remove(priv->source_map_center);
        priv->source_map_center = 0;
    }

    G_OBJECT_CLASS(map_controller_parent_class)->dispose(object);
}

static void
map_controller_init(MapController *controller)
{
    MapControllerPrivate *priv;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(controller, MAP_TYPE_CONTROLLER,
                                       MapControllerPrivate);
    controller->priv = priv;

    g_assert(instance == NULL);
    instance = controller;

    /* TODO: load the settings from inside here */

    priv->screen = g_object_new(MAP_TYPE_SCREEN, NULL);
    map_screen_show_compass(priv->screen, _show_comprose);
    map_screen_show_scale(priv->screen, _show_scale);
    map_screen_show_zoom_box(priv->screen, _show_zoomlevel);

    map_controller_set_center(controller, _next_center, _next_zoom);

    g_idle_add(activate_gps, NULL);
}

static void
map_controller_class_init(MapControllerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(object_class, sizeof(MapControllerPrivate));

    object_class->dispose = map_controller_dispose;
}

MapController *
map_controller_get_instance()
{
    return instance;
}

MapScreen *
map_controller_get_screen(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);
    return self->priv->screen;
}

GtkWindow *
map_controller_get_main_window(MapController *self)
{
    return GTK_WINDOW(gtk_widget_get_toplevel((GtkWidget *)self->priv->screen));
}

void
map_controller_action_zoom_in(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_screen_zoom_start(self->priv->screen, MAP_SCREEN_ZOOM_IN);
}

void
map_controller_action_zoom_out(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_screen_zoom_start(self->priv->screen, MAP_SCREEN_ZOOM_OUT);
}

void
map_controller_action_zoom_stop(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_screen_zoom_stop(self->priv->screen);
}

void
map_controller_action_switch_fullscreen(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    _fullscreen = !_fullscreen;

    if(_fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(_window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(_window));
}

void
map_controller_activate_menu_settings(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    gtk_menu_item_activate(GTK_MENU_ITEM(_menu_settings_item));
}

void
map_controller_action_point(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_screen_action_point(self->priv->screen);
}

void
map_controller_action_route(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_menu_route();
}

void
map_controller_action_track(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_menu_track();
}

void
map_controller_action_view(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_menu_view();
}

void
map_controller_action_go_to(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_menu_go_to();
}

void
map_controller_activate_menu_point(MapController *self, const Point *p)
{
    MapArea area;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    map_screen_get_tap_area_from_units(self->priv->screen, p, &area);
    map_menu_point(p, &area);
}

void
map_controller_set_gps_enabled(MapController *self, gboolean enabled)
{
    _enable_gps = enabled;

    if (enabled)
        rcvr_connect();
    else
        rcvr_disconnect();

    map_move_mark();
    gps_show_info();
}

gboolean
map_controller_get_gps_enabled(MapController *self)
{
    return _enable_gps;
}

void
map_controller_set_center_mode(MapController *self, CenterMode mode)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    _center_mode = mode;
    if (mode > 0)
        map_refresh_mark(TRUE);
}

void
map_controller_disable_auto_center(MapController *self)
{
    if (_center_mode > 0)
    {
        map_controller_set_center_mode(self, -_center_mode);
        MACRO_BANNER_SHOW_INFO(_window, _("Auto-Center Off"));
    }
}

void
map_controller_set_auto_rotate(MapController *self, gboolean enable)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (enable == _center_rotate) return;

    if (!enable)
        map_controller_set_rotation(self, 0);
    _center_rotate = enable;
}

gboolean
map_controller_get_auto_rotate(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);
    return _center_rotate;
}

void
map_controller_set_tracking(MapController *self, gboolean enable)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (enable == _enable_tracking) return;

    if (enable)
    {
        MACRO_BANNER_SHOW_INFO(_window, _("Tracking Enabled"));
    }
    else
    {
        track_insert_break(FALSE); /* FALSE = not temporary */
        MACRO_BANNER_SHOW_INFO(_window, _("Tracking Disabled"));
    }

    _enable_tracking = enable;
}

gboolean
map_controller_get_tracking(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);
    return _center_rotate;
}

void
map_controller_set_show_compass(MapController *self, gboolean show)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (_show_comprose == show) return;

    _show_comprose = show;
    map_screen_show_compass(self->priv->screen, _show_comprose);
}

gboolean
map_controller_get_show_compass(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_comprose;
}

void
map_controller_set_show_routes(MapController *self, gboolean show)
{
    gboolean current;

    g_return_if_fail(MAP_IS_CONTROLLER(self));

    current = _show_paths & ROUTES_MASK;
    if (show == !!current) return;

    if (show)
    {
        _show_paths |= ROUTES_MASK;
        MACRO_BANNER_SHOW_INFO(_window, _("Routes are now shown"));
    }
    else
    {
        _show_paths &= ~ROUTES_MASK;
        MACRO_BANNER_SHOW_INFO(_window, _("Routes are now hidden"));
    }

    map_screen_redraw_overlays(self->priv->screen);
}

gboolean
map_controller_get_show_routes(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_paths & ROUTES_MASK;
}

void
map_controller_set_show_tracks(MapController *self, gboolean show)
{
    gboolean current;

    g_return_if_fail(MAP_IS_CONTROLLER(self));

    current = _show_paths & TRACKS_MASK;
    if (show == !!current) return;

    if (show)
    {
        _show_paths |= TRACKS_MASK;
        MACRO_BANNER_SHOW_INFO(_window, _("Tracks are now shown"));
    }
    else
    {
        _show_paths &= ~TRACKS_MASK;
        MACRO_BANNER_SHOW_INFO(_window, _("Tracks are now hidden"));
    }

    map_screen_redraw_overlays(self->priv->screen);
}

gboolean
map_controller_get_show_tracks(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_paths & TRACKS_MASK;
}

void
map_controller_set_show_scale(MapController *self, gboolean show)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (_show_scale == show) return;

    _show_scale = show;
    map_screen_show_scale(self->priv->screen, _show_scale);
}

gboolean
map_controller_get_show_scale(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_scale;
}

void
map_controller_set_show_poi(MapController *self, gboolean show)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (_show_poi == show) return;

    _show_poi = show;
    map_screen_show_pois(self->priv->screen, show);
}

gboolean
map_controller_get_show_poi(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_poi;
}

void
map_controller_set_show_velocity(MapController *self, gboolean show)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (_show_velvec == show) return;

    _show_velvec = show;
    map_move_mark();
}

gboolean
map_controller_get_show_velocity(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_velvec;
}

void
map_controller_set_show_zoom(MapController *self, gboolean show)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (_show_zoomlevel == show) return;

    _show_zoomlevel = show;
    map_screen_show_zoom_box(MAP_SCREEN(_w_map), _show_zoomlevel);
}

gboolean
map_controller_get_show_zoom(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _show_zoomlevel;
}

void
map_controller_set_center(MapController *self, Point center, gint zoom)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    if (zoom < 0)
        zoom = priv->zoom;
    else
        priv->zoom = zoom;
    priv->center = center;

    if (priv->source_map_center == 0)
        priv->source_map_center =
            g_idle_add((GSourceFunc)set_center_real, self);
}

void
map_controller_get_center(MapController *self, Point *center)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    g_return_if_fail(center != NULL);
    *center = priv->center;
}

void
map_controller_set_rotation(MapController *self, gint angle)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    angle = angle % 360;
    priv->rotation_angle = angle;
    map_screen_set_rotation(priv->screen, angle);
}

void
map_controller_rotate(MapController *self, gint angle)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    map_controller_set_rotation(self, self->priv->rotation_angle + angle);
}

void
map_controller_set_zoom(MapController *self, gint zoom)
{
    MapControllerPrivate *priv;
    Point center;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    if (zoom == priv->zoom) return;

    g_debug("%s %d", G_STRFUNC, zoom);
    priv->zoom = zoom;
    map_controller_calc_best_center(self, &center);
    map_controller_set_center(self, center, zoom);
}

gint
map_controller_get_zoom(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), 0);
    return self->priv->zoom;
}

void
map_controller_calc_best_center(MapController *self, Point *new_center)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    switch (_center_mode)
    {
    case CENTER_LEAD:
        {
            gfloat tmp = deg2rad(_gps.heading);
            gfloat screen_pixels = _view_width_pixels
                + (((gint)_view_height_pixels
                            - (gint)_view_width_pixels)
                        * fabsf(cosf(deg2rad(
                                (_center_rotate ? 0
                             : (_next_map_rotate_angle
                                 - (gint)(_gps.heading)))))));
            gfloat lead_pixels = 0.0025f
                * pixel2zunit((gint)screen_pixels, priv->zoom)
                * _lead_ratio
                * VELVEC_SIZE_FACTOR
                * (_lead_is_fixed ? 7 : sqrtf(_gps.speed));

            new_center->unitx = _pos.unitx + (gint)(lead_pixels * sinf(tmp));
            new_center->unity = _pos.unity - (gint)(lead_pixels * cosf(tmp));
            break;
        }
    case CENTER_LATLON:
        *new_center = _pos;
        break;
    default:
        *new_center = priv->center;
    }
}

void
map_controller_refresh_paths(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    map_screen_redraw_overlays(self->priv->screen);
}
