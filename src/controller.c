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
#include "repository.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tile_source.h"

#include <gconf/gconf-client.h>
#include <hildon/hildon-banner.h>
#include <math.h>
#include <string.h>

#define VELVEC_SIZE_FACTOR (4)

struct _MapControllerPrivate
{
    GList *repositories_list;
    GList *tile_sources_list;
    Repository *repository;
    MapScreen *screen;
    MapPoint center;
    gint rotation_angle;
    gint zoom;

    guint source_map_center;
    guint is_disposed : 1;
    guint device_active : 1;
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


static void
reset_tile_sources_countdown()
{
    GList *ts_list = map_controller_get_tile_sources_list(map_controller_get_instance());
    TileSource *ts;

    while (ts_list) {
        ts = (TileSource*)ts_list->data;
        if (ts->refresh && ts->countdown < 0)
            ts->countdown = ts->refresh-1;
        ts_list = ts_list->next;
    }
}


static gboolean
expired_tiles_housekeeper(gpointer data)
{
    GList *ts_list = map_controller_get_tile_sources_list(map_controller_get_instance());
    MapController *controller = map_controller_get_instance();
    TileSource *ts;
    gboolean expired = FALSE;

    /* If device is inactive, do not download tiles and stop tileout routine */
    if (!map_controller_get_device_active(controller))
        return FALSE;

    /* Iterate over all tile sources and if they have refresh turned on, decrement coundown */
    while (ts_list) {
        ts = (TileSource*)ts_list->data;
        if (ts->refresh) {
            ts->countdown--;
            if (ts->countdown < 0)
                expired = TRUE;
        }
        ts_list = ts_list->next;
    }

    if (expired) {
        refresh_expired_tiles();
        reset_tile_sources_countdown();
    }
    return map_controller_get_device_active(controller);
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
    GConfClient *gconf_client = gconf_client_get_default();

    priv = G_TYPE_INSTANCE_GET_PRIVATE(controller, MAP_TYPE_CONTROLLER,
                                       MapControllerPrivate);
    controller->priv = priv;

    g_assert(instance == NULL);
    instance = controller;

    /* Load settings */
    settings_init(gconf_client);

    /* Load repositories */
    map_controller_load_repositories(controller, gconf_client);

    priv->screen = g_object_new(MAP_TYPE_SCREEN, NULL);
    map_screen_show_compass(priv->screen, _show_comprose);
    map_screen_show_scale(priv->screen, _show_scale);
    map_screen_show_zoom_box(priv->screen, _show_zoomlevel);

    /* TODO: eliminate global _next_center, _next_zoom, _center, _zoom, etc values */
    map_controller_set_center(controller, _next_center, _next_zoom);

    g_idle_add(activate_gps, NULL);

    gconf_client_clear_cache(gconf_client);
    g_object_unref(gconf_client);
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
map_controller_set_center(MapController *self, MapPoint center, gint zoom)
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
    MapPoint center;

    g_return_if_fail(MAP_IS_CONTROLLER(self));
    priv = self->priv;

    /* Round zoom according step in repository */
    zoom = zoom / priv->repository->zoom_step * priv->repository->zoom_step;

    if (zoom == priv->zoom) return;

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
map_controller_calc_best_center(MapController *self, MapPoint *new_center)
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

    map_screen_redraw_overlays(self->priv->screen);
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
    map_screen_update_mark(priv->screen);
    map_screen_set_best_center(priv->screen);
}

/*
 * Load all repositories and associated layers
 */
void
map_controller_load_repositories(MapController *self, GConfClient *gconf_client)
{
    MapControllerPrivate *priv;
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

    reset_tile_sources_countdown();
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

void
map_controller_set_repository(MapController *self, Repository *repo)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    self->priv->repository = repo;
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
map_controller_toggle_layer_visibility(MapController *self, TileSource *ts)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    g_return_if_fail(ts != NULL);

    ts->visible = !ts->visible;
    map_screen_refresh_map(self->priv->screen);
}


/* Obtain device activity state */
gboolean
map_controller_get_device_active(MapController *self)
{
    g_return_val_if_fail(MAP_IS_CONTROLLER(self), TRUE);

    return self->priv->device_active;
}


/*
 * Routine saves new state of device (active or not). According to this,
 * we register or unregister timeout routine to save battery.
 */
void
map_controller_set_device_active(MapController *self, gboolean active)
{
    g_return_if_fail(MAP_IS_CONTROLLER(self));
    self->priv->device_active = active;

    /* If new device state is active, we recharge timer. If device becomes inactive, we do nothing
     * here, but in timeout routine well return FALSE, which will disable it. */
    if (active) {
        /* Run routine explicitly, to force tiles to refresh immediately */
        expired_tiles_housekeeper(NULL);
        /* create periodical timer which wipes expired tiles from cache */
        g_timeout_add_seconds(60, expired_tiles_housekeeper, NULL);
    }
}
