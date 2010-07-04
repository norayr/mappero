/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Max Lapan
 * Copyright (C) 2009-2010 Alberto Mardegan <mardy@users.sourceforge.net>
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
#define _GNU_SOURCE
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
#include "orientation.h"
#include "path.h"
#include "plugins/google.h"
#include "plugins/reittiopas.h"
#include "plugins/yandex.h"
#include "repository.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tile_source.h"
#include "util.h"

#include <hildon/hildon-banner.h>
#include <mappero/debug.h>
#include <mappero/loader.h>
#include <mappero/viewer.h>
#include <math.h>
#include <string.h>

#define VELVEC_SIZE_FACTOR (4)

#include "controller-priv.h"

static void map_controller_viewer_init(MapViewerIface *iface,
                                       gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE(MapController, map_controller, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MAP_TYPE_VIEWER,
                                              map_controller_viewer_init);
                       );

#define MAP_CONTROLLER_PRIV(controller) (MAP_CONTROLLER(controller)->priv)

static MapController *instance = NULL;

static void
map_controller_get_transformation(MapViewer *viewer,
                                  MapLatLonToUnit *latlon2unit,
                                  MapUnitToLatLon *unit2latlon)
{
    const TileSourceType *source_type;
    source_type = tile_source_get_primary_type();
    if (G_LIKELY(source_type)) {
        *latlon2unit = source_type->latlon_to_unit;
        *unit2latlon = source_type->unit_to_latlon;
    } else {
        g_warning("Tile source type is uninitialized");
        *latlon2unit = NULL;
        *unit2latlon = NULL;
    }
}

static void
map_controller_viewer_init(MapViewerIface *iface, gpointer iface_data)
{
    iface->get_transformation = map_controller_get_transformation;
}

static gboolean
download_precache_real(MapController *self)
{
    MapControllerPrivate *priv = self->priv;
    GtkAllocation *allocation;

    if (G_UNLIKELY(!priv->repository)) return FALSE;

    allocation = &(GTK_WIDGET(priv->screen)->allocation);

    map_download_precache(priv->repository->primary, priv->center, priv->zoom,
                          _auto_download_precache,
                          allocation->width, allocation->height);

    priv->source_download_precache = 0;
    return FALSE;
}

static void
map_controller_download_precache(MapController *self)
{
    MapControllerPrivate *priv = self->priv;

    if (!map_controller_get_auto_download(self) ||
        _auto_download_precache == 0)
        return;

    if (priv->source_download_precache == 0)
        priv->source_download_precache =
            g_idle_add_full(G_PRIORITY_LOW,
                            (GSourceFunc)download_precache_real, self, NULL);
}

static gboolean
activate_gps(MapController *self)
{
    /* Connect to receiver. */
    if (_enable_gps)
        map_controller_gps_connect(self);
    return FALSE;
}


static void
reset_tile_sources_countdown(MapController *self)
{
    GList *ts_list = self->priv->tile_sources_list;
    TileSource *ts;

    while (ts_list) {
        ts = (TileSource*)ts_list->data;
        if (ts->refresh && ts->countdown < 0)
            ts->countdown = ts->refresh-1;
        ts_list = ts_list->next;
    }
}

static gboolean
expired_tiles_housekeeper(MapController *self)
{
    MapControllerPrivate *priv = self->priv;

    /* If the diplay is off, do not download tiles and stop tileout routine */
    if (!map_controller_is_display_on(self))
        return FALSE;

    if (G_UNLIKELY(!priv->repository))
        return FALSE;

    if (repository_tile_sources_expired(priv->repository)) {
        refresh_expired_tiles();
        reset_tile_sources_countdown(self);
    }
    return TRUE;
}


static gboolean
set_center_real(MapController *self)
{
    MapControllerPrivate *priv = self->priv;

    map_screen_set_center(priv->screen,
                          priv->center.x, priv->center.y, priv->zoom);
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

    map_controller_gps_dispose(MAP_CONTROLLER(object));

    if (priv->source_map_center != 0)
    {
        g_source_remove(priv->source_map_center);
        priv->source_map_center = 0;
    }

    if (priv->source_download_precache != 0)
    {
        g_source_remove(priv->source_download_precache);
        priv->source_download_precache = 0;
    }

    while (priv->plugins)
    {
        g_object_unref(priv->plugins->data);
        priv->plugins = g_slist_delete_link(priv->plugins, priv->plugins);
    }

    if (priv->gconf_client)
    {
        g_object_unref(priv->gconf_client);
        priv->gconf_client = NULL;
    }

    if (priv->source_init_late != 0)
    {
        g_source_remove(priv->source_init_late);
        priv->source_init_late = 0;
    }

    G_OBJECT_CLASS(map_controller_parent_class)->dispose(object);
}

static gboolean
map_controller_init_late(MapController *controller)
{
    MapControllerPrivate *priv = controller->priv;
    GList *list;

    priv->source_init_late = 0;

    /* register plugins */
    map_loader_read_dir(MAP_PLUGIN_DIR);
    for (list = map_loader_get_objects(); list != NULL; list = list->next)
    {
        map_controller_register_plugin(controller, list->data);
        g_object_unref(list->data);
    }

    /* Load router settings */
    map_controller_load_routers_options(controller, priv->gconf_client);

    /* free the GConf client, we don't need it anymore */
    gconf_client_clear_cache(priv->gconf_client);
    g_object_unref(priv->gconf_client);
    priv->gconf_client = NULL;

    return FALSE;
}

static void
map_controller_init(MapController *controller)
{
    MapControllerPrivate *priv;

    TIME_START();

    priv = G_TYPE_INSTANCE_GET_PRIVATE(controller, MAP_TYPE_CONTROLLER,
                                       MapControllerPrivate);
    controller->priv = priv;

    g_assert(instance == NULL);
    instance = controller;

    priv->orientation = -1;
    /* until we know the display state, assume it's on */
    priv->display_on = TRUE;

    priv->gconf_client = gconf_client_get_default();
    gconf_client_preload(priv->gconf_client, GCONF_KEY_PREFIX,
                         GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

    /* Load repositories. It is important to load repositories first, because settings need
     * controller for transformations. */
    map_controller_load_repositories(controller, priv->gconf_client);

    /* init GPS */
    map_controller_gps_init(controller, priv->gconf_client);
    g_idle_add((GSourceFunc)activate_gps, controller);

    /* Load settings */
    settings_init(priv->gconf_client);

    priv->screen = g_object_new(MAP_TYPE_SCREEN, NULL);
    map_screen_show_compass(priv->screen, _show_comprose);
    map_screen_show_scale(priv->screen, _show_scale);
    map_screen_show_zoom_box(priv->screen, _show_zoomlevel);

    /* TODO: eliminate global _next_center, _next_zoom, _center, _zoom, etc values */
    map_controller_set_center(controller, _next_center, _next_zoom);

    priv->source_init_late =
        g_idle_add_full(G_PRIORITY_LOW,
                        (GSourceFunc)map_controller_init_late, controller,
                        NULL);

    TIME_STOP();
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
    if (self->priv->zoom <= MIN_ZOOM) return;
    map_screen_zoom_start(self->priv->screen, MAP_SCREEN_ZOOM_IN);
}

void
map_controller_action_zoom_out(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    if (self->priv->zoom >= MAX_ZOOM) return;
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
map_controller_activate_menu_point(MapController *self, const MapPoint *p)
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
        map_controller_gps_connect(self);
    else
        map_controller_gps_disconnect(self);

    map_move_mark();
    gps_show_info();
}

gboolean
map_controller_get_gps_enabled(MapController *self)
{
    return _enable_gps;
}

void
map_controller_set_gps_position(MapController *self, const MapPoint *p)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    _pos.unit = *p;

    memset(&priv->gps_data, 0, sizeof(MapGpsData));
    priv->gps_data.fields = MAP_GPS_LATLON;
    priv->gps_data.unit = *p;
    unit2latlon(_pos.unit.x, _pos.unit.y,
                priv->gps_data.lat, priv->gps_data.lon);
    priv->gps_data.time = time(0);

    /* Move mark to new location. */
    map_screen_update_mark(priv->screen);

    /* simulate a real move (especially useful for debugging navigation!) */
    map_path_route_step(&priv->gps_data, FALSE);

    /* TODO: update the GConf keys under /system/nokia/location/lastknown */
}

const MapGpsData *
map_controller_get_gps_data(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);
    return &self->priv->gps_data;
}

void
map_controller_set_center_mode(MapController *self, CenterMode mode)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    _center_mode = mode;
    if (mode > 0)
        map_screen_set_best_center(self->priv->screen);
}

CenterMode
map_controller_get_center_mode(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), CENTER_MANUAL);

    return _center_mode;
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
map_controller_set_show_gps_info(MapController *self, gboolean show)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (_gps_info == show) return;

    _gps_info = show;
    gps_show_info();
}

gboolean
map_controller_get_show_gps_info(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return _gps_info;
}

void
map_controller_set_center_no_act(MapController *self, MapPoint center,
                                 gint zoom)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    DEBUG("(%d, %d, %d)", center.x, center.y, zoom);
    if (zoom >= 0)
        priv->zoom = zoom;
    priv->center = center;
}

void
map_controller_set_center(MapController *self, MapPoint center, gint zoom)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    DEBUG("(%d, %d, %d)", center.x, center.y, zoom);
    map_controller_set_center_no_act(self, center, zoom);

    if (priv->display_on)
    {
        map_controller_download_precache(self);
        if (priv->source_map_center == 0)
            priv->source_map_center =
                g_idle_add((GSourceFunc)set_center_real, self);
    }
}

void
map_controller_get_center(MapController *self, MapPoint *center)
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
    if (priv->display_on)
        map_screen_set_rotation(priv->screen, angle);
}

gint
map_controller_get_rotation(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), 0);
    return self->priv->rotation_angle;
}

void
map_controller_rotate(MapController *self, gint angle)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    map_controller_set_rotation(self, self->priv->rotation_angle + angle);
}

void
map_controller_set_zoom_no_act(MapController *self, gint zoom)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    DEBUG("called: %d", zoom);

    if (zoom >= 0)
        self->priv->zoom = zoom;
}

void
map_controller_set_zoom(MapController *self, gint zoom)
{
    MapControllerPrivate *priv;
    MapPoint center;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    /* Round zoom according step in repository */
    zoom = zoom / priv->repository->zoom_step * priv->repository->zoom_step;

    map_controller_set_zoom_no_act(self, zoom);

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
map_controller_calc_best_center(MapController *self, MapPoint *new_center)
{
    MapControllerPrivate *priv;
    MapGpsData *gps;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;
    gps = &priv->gps_data;

    /* if the error is huge and it covers the current center, then don't change
     * the center */
    if (G_UNLIKELY(gps->hdop > 5000))
    {
        gfloat distance;
        MapGeo lat, lon;

        unit2latlon(priv->center.x, priv->center.y, lat, lon);
        distance = calculate_distance(gps->lat, gps->lon, lat, lon);
        DEBUG("distance = %.1f, hdop = %d, (%.5f, %.5f) - (%.5f, %.5f)",
              distance, gps->hdop, lat, lon, gps->lat, gps->lon);
        if (distance <= gps->hdop)
        {
            *new_center = priv->center;
            return;
        }
    }

    switch (_center_mode)
    {
    case CENTER_LEAD:
        {
            gfloat heading, speed;
            heading = gps->heading;
            speed = gps->speed;

            gfloat tmp = deg2rad(heading);
            gfloat screen_pixels = _view_width_pixels
                + (((gint)_view_height_pixels
                            - (gint)_view_width_pixels)
                        * fabsf(cosf(deg2rad(
                                (_center_rotate ? 0
                             : (_next_map_rotate_angle
                                 - (gint)(heading)))))));
            gfloat lead_pixels = 0.0025f
                * pixel2zunit((gint)screen_pixels, priv->zoom)
                * _lead_ratio
                * VELVEC_SIZE_FACTOR
                * (_lead_is_fixed ? 7 : sqrtf(speed));

            new_center->x = _pos.unit.x + (gint)(lead_pixels * sinf(tmp));
            new_center->y = _pos.unit.y - (gint)(lead_pixels * cosf(tmp));
            break;
        }
    case CENTER_LATLON:
        *new_center = _pos.unit;
        break;
    default:
        *new_center = priv->center;
    }
}

void
map_controller_refresh_paths(MapController *self)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    if (self->priv->display_on)
    {
        map_screen_redraw_overlays(self->priv->screen);
        map_screen_refresh_panel(self->priv->screen);
    }
}

/*
 * map_controller_update_gps:
 * @self: the MapController
 *
 * Call this method when the GPS position changes.
 */
void
map_controller_update_gps(MapController *self)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;
    if (priv->display_on)
    {
        map_screen_update_mark(priv->screen);
        map_screen_set_best_center(priv->screen);
    }
}

/*
 * Load all repositories and associated layers
 */
void
map_controller_load_repositories(MapController *self, GConfClient *gconf_client)
{
    MapControllerPrivate *priv;
    const TileSourceType *source_type;
    GConfValue *value;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    /* tile sources */
    value = gconf_client_get(gconf_client, GCONF_KEY_TILE_SOURCES, NULL);
    if (value) {
        priv->tile_sources_list = tile_source_xml_to_list(gconf_value_get_string(value));
        gconf_value_free(value);
    }

    /* repositories must be loaded after tile sources, because repository load routine performs
     * lookup in tile sources list */
    value = gconf_client_get(gconf_client, GCONF_KEY_REPOSITORIES, NULL);
    if (value) {
        priv->repositories_list = repository_xml_to_list(gconf_value_get_string(value));
        gconf_value_free(value);
    }

    /* if some data failed to load, switch to defaults */
    if (!priv->tile_sources_list || !priv->repositories_list)
        priv->repository = repository_create_default_lists(&priv->tile_sources_list,
                                                           &priv->repositories_list);
    else {
        /* current repository */
        value = gconf_client_get(gconf_client, GCONF_KEY_ACTIVE_REPOSITORY, NULL);
        if (value) {
            char *val = (char*)gconf_value_get_string(value);
            if (val)
                priv->repository = map_controller_lookup_repository(self, val);
            gconf_value_free(value);
        }

        if (!priv->repository)
            priv->repository = priv->repositories_list->data;
    }

    reset_tile_sources_countdown(self);

    /* Emit the signal on the MapViewer interface */
    source_type = priv->repository->primary->type;

    map_viewer_emit_transformation_changed(MAP_VIEWER(self),
                                           source_type->latlon_to_unit,
                                           source_type->unit_to_latlon);
}

/*
 * Save all repositories and associated layers
 */
void
map_controller_save_repositories(MapController *self, GConfClient *gconf_client)
{
    MapControllerPrivate *priv;
    gchar *xml;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    /* Repositories */
    xml = repository_list_to_xml(priv->repositories_list);
    if (xml) {
        gconf_client_set_string(gconf_client, GCONF_KEY_REPOSITORIES, xml, NULL);
        g_free(xml);
    }
    else
        gconf_client_unset(gconf_client, GCONF_KEY_REPOSITORIES, NULL);

    /* Tile sources */
    xml = tile_source_list_to_xml(priv->tile_sources_list);
    if (xml) {
        gconf_client_set_string(gconf_client, GCONF_KEY_TILE_SOURCES, xml, NULL);
        g_free(xml);
    }
    else
        gconf_client_unset(gconf_client, GCONF_KEY_TILE_SOURCES, NULL);

    if (priv->repository)
        gconf_client_set_string(gconf_client, GCONF_KEY_ACTIVE_REPOSITORY,
                                priv->repository->name, NULL);
    else
        gconf_client_unset(gconf_client, GCONF_KEY_ACTIVE_REPOSITORY, NULL);
}

Repository *
map_controller_get_repository(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);
    return self->priv->repository;
}


static void
update_path_coords(Repository *from, Repository *to, MapPath *path)
{
    MapPathPoint *curr;
    MapGeo lat, lon;

    for (curr = map_path_first(path);
         curr < map_path_end(path);
         curr = map_path_next(path, curr))
    {
        from->primary->type->unit_to_latlon(curr->unit.x, curr->unit.y,
                                            &lat, &lon);
        to->primary->type->latlon_to_unit(lat, lon,
                                          &curr->unit.x, &curr->unit.y);
    }
}


void
map_controller_set_repository(MapController *self, Repository *repo)
{
    MapControllerPrivate *priv;
    Repository *curr_repo;
    MapPoint center;
    const TileSourceType *curr_type, *new_type;

    g_return_if_fail(MAP_IS_CONTROLLER(self));

    priv = self->priv;
    center = priv->center;
    curr_repo = priv->repository;

    curr_type = curr_repo->primary->type;
    new_type = repo->primary->type;

    /* if new repo coordinate system differs from current one,
       recalculate map center, current track and route (if needed) */
    if (curr_repo && curr_type->latlon_to_unit != new_type->latlon_to_unit) {
        MapGeo lat, lon;
        MapPath *route = map_route_get_path();

        curr_type->unit_to_latlon(center.x, center.y, &lat, &lon);
        new_type->latlon_to_unit(lat, lon, &center.x, &center.y);

        curr_type->unit_to_latlon(_pos.unit.x, _pos.unit.y, &lat, &lon);
        new_type->latlon_to_unit(lat, lon, &_pos.unit.x, &_pos.unit.y);

        if ((_show_paths & ROUTES_MASK) && map_path_len(route) > 0)
            update_path_coords(curr_repo, repo, route);
        if ((_show_paths & TRACKS_MASK) && map_path_len(&_track) > 0)
            update_path_coords(curr_repo, repo, &_track);

        /* Emit the signal on the MapViewer interface */
        map_viewer_emit_transformation_changed(MAP_VIEWER(self),
                                               new_type->latlon_to_unit,
                                               new_type->unit_to_latlon);
    }

    priv->repository = repo;
    map_controller_set_center(self, center, priv->zoom);
    map_screen_update_mark(priv->screen);
}

GList *
map_controller_get_repo_list(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);
    return self->priv->repositories_list;
}

GList *
map_controller_get_tile_sources_list(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);
    return self->priv->tile_sources_list;
}


/* Return TileSource reference with this ID. If not found, return NULL */
TileSource *
map_controller_lookup_tile_source(MapController *self, const gchar *id)
{
    GList *ts_list;
    TileSource *ts;

    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);

    if (!id)
        return NULL;

    ts_list = self->priv->tile_sources_list;
    while (ts_list)
    {
        ts = (TileSource*)ts_list->data;
        if (ts && ts->id && strcmp(ts->id, id) == 0)
            return ts;
        ts_list = g_list_next(ts_list);
    }

    return NULL;
}


/* Return TileSource reference with this name. If not found, return NULL */
TileSource *
map_controller_lookup_tile_source_by_name(MapController *self, const gchar *name)
{
    GList *ts_list;
    TileSource *ts;

    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);

    if (!name)
        return NULL;

    ts_list = self->priv->tile_sources_list;
    while (ts_list)
    {
        ts = (TileSource*)ts_list->data;
        if (ts && ts->name && strcmp(ts->name, name) == 0)
            return ts;
        ts_list = g_list_next(ts_list);
    }

    return NULL;
}


Repository *
map_controller_lookup_repository(MapController *self, const gchar *name)
{
    GList *repo_list;
    Repository *repo;

    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);

    if (!name)
        return NULL;

    repo_list = self->priv->repositories_list;
    while (repo_list)
    {
        repo = (Repository*)repo_list->data;
        if (repo && repo->name && strcmp(repo->name, name) == 0)
            return repo;
        repo_list = g_list_next(repo_list);
    }

    return NULL;
}


void
map_controller_delete_repository(MapController *self, Repository *repo)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    /* Do not allow to delete active repository */
    if (repo == priv->repository)
        return;

    priv->repositories_list = g_list_remove(priv->repositories_list, repo);
    repository_free(repo);
}


void
map_controller_delete_tile_source(MapController *self, TileSource *ts)
{
    MapControllerPrivate *priv;
    GList *repo_list;
    Repository *repo;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    priv->tile_sources_list = g_list_remove(priv->tile_sources_list, ts);

    repo_list = priv->repositories_list;
    while (repo_list) {
        repo = (Repository*)repo_list->data;
        if (repo->primary == ts)
            repo->primary = NULL;
        if (repo->layers)
            while (g_ptr_array_remove(repo->layers, ts));
        repo_list = repo_list->next;
    }

    tile_source_free(ts);
}


void
map_controller_append_tile_source(MapController *self, TileSource *ts)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    self->priv->tile_sources_list = g_list_append(self->priv->tile_sources_list, ts);
}


void
map_controller_append_repository(MapController *self, Repository *repo)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    self->priv->repositories_list = g_list_append(self->priv->repositories_list, repo);
}


/*
 * Routine toggles visibility of given layer.
 */
void
map_controller_toggle_layer_visibility(MapController *self, RepositoryLayer *repo_layer)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    g_return_if_fail(repo_layer != NULL);

    repo_layer->visible = !repo_layer->visible;
    map_screen_refresh_map(self->priv->screen);
}

void
map_controller_set_auto_download(MapController *self,
                                 gboolean auto_download)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    priv->auto_download = auto_download;
    if (auto_download && priv->screen)
        map_screen_refresh_tiles(priv->screen);
}

gboolean
map_controller_get_auto_download(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);
    return self->priv->auto_download;
}

/* Obtain device activity state */
gboolean
map_controller_get_device_active(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), TRUE);

    return self->priv->device_active;
}


/*
 * Routine saves new state of device (active or not).
 */
void
map_controller_set_device_active(MapController *self, gboolean active)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    priv->device_active = active;
}

/**
 * map_controller_register_plugin:
 * @self: the #MapController
 * @plugin: a #GObject
 *
 * This mehod registers a plugin with Mapper. The @plugin should implement some
 * of the interfaces defined by Mapper (e.g., MapRouter). The #MapController
 * keeps a reference to these objects.
 */
void
map_controller_register_plugin(MapController *self, GObject *plugin)
{
    MapControllerPrivate *priv;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    g_return_if_fail(G_IS_OBJECT(plugin));
    priv = self->priv;

    priv->plugins = g_slist_prepend(priv->plugins, g_object_ref(plugin));
}

/**
 * map_controller_list_plugins:
 * @self: the #MapController
 *
 * Returns: a #GSList of the registered plugins.
 */
const GSList *
map_controller_list_plugins(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);

    return self->priv->plugins;
}

/**
 * map_controller_set_default_router:
 * @self: the #MapController
 * @router: a #MapRouter
 *
 * Sets @router as the default router.
 */
void
map_controller_set_default_router(MapController *self, MapRouter *router)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));

    self->priv->default_router = router;
}

MapRouter *
map_controller_get_default_router(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), NULL);

    return self->priv->default_router;
}


void
map_controller_load_routers_options(MapController *self, GConfClient *gconf_client)
{
    gchar *router_name;
    GSList *p;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    p = self->priv->plugins;

    /* Get default router. */
    router_name = gconf_client_get_string(gconf_client,
                                          GCONF_KEY_ROUTER_NAME, NULL);
    if (!router_name) router_name = g_strdup("Google");
    while (p) {
        MapRouter *router = p->data;

        if (!MAP_IS_ROUTER(router)) continue;
        map_router_load_options(router, gconf_client);

        if (router_name &&
            strcmp(map_router_get_name(router), router_name) == 0)
        {
            map_controller_set_default_router(self, router);
        }

        p = p->next;
    }

    g_free(router_name);
}


void
map_controller_save_routers_options(MapController *self, GConfClient *gconf_client)
{
    GSList *p;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    p = self->priv->plugins;

    while (p) {
        MapRouter *router = p->data;
        map_router_save_options(router, gconf_client);
        p = p->next;
    }
}

void
map_controller_display_status_changed(MapController *self, const gchar *status)
{
    MapControllerPrivate *priv;
    gboolean display_on;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    display_on = (g_strcmp0(status, "off") != 0);
    DEBUG("Display is %s", display_on ? "on" : "off");

    if (priv->display_on == display_on) return;

    priv->display_on = display_on;

    /* If new device state is active, we recharge timer. If device becomes
     * inactive, we do nothing here, but in timeout routine we'll return FALSE,
     * which will disable it. */
    if (display_on) {
        if (repository_tile_sources_can_expire(priv->repository))
        {
            /* Run routine explicitly, to force tiles to refresh immediately */
            expired_tiles_housekeeper(self);
            /* create periodical timer which wipes expired tiles from cache */
            g_timeout_add_seconds(60, (GSourceFunc)expired_tiles_housekeeper,
                                  self);
        }

        /* refresh the screen */
        set_center_real(self);
        map_screen_set_rotation(priv->screen, priv->rotation_angle);
        map_screen_update_mark(priv->screen);
        map_screen_refresh_panel(priv->screen);
    }

    map_controller_gps_display_changed(self, display_on);
}

gboolean
map_controller_is_display_on(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), FALSE);

    return self->priv->display_on;
}

