/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Max Lapan
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

#ifndef MAP_CONTROLLER_H
#define MAP_CONTROLLER_H

#include "types.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_CONTROLLER         (map_controller_get_type ())
#define MAP_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MAP_TYPE_CONTROLLER, MapController))
#define MAP_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_CONTROLLER, MapControllerClass))
#define MAP_IS_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MAP_TYPE_CONTROLLER))
#define MAP_IS_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MAP_TYPE_CONTROLLER))
#define MAP_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MAP_TYPE_CONTROLLER, MapControllerClass))

typedef struct _MapController MapController;
typedef struct _MapControllerPrivate MapControllerPrivate;
typedef struct _MapControllerClass MapControllerClass;

#include "orientation.h"
#include "router.h"
#include "screen.h"

struct _MapController
{
    GObject parent;

    MapControllerPrivate *priv;
};

struct _MapControllerClass
{
    GObjectClass parent_class;
};

GType map_controller_get_type(void);

MapController *map_controller_get_instance();

MapScreen *map_controller_get_screen(MapController *self);
GtkWindow *map_controller_get_main_window(MapController *self);

/* Repository */
Repository *map_controller_get_repository(MapController *self);
TileSource *map_controller_lookup_tile_source(MapController *self, const gchar *id);
TileSource *map_controller_lookup_tile_source_by_name(MapController *self, const gchar *name);
Repository *map_controller_lookup_repository(MapController *self, const gchar *name);
void map_controller_set_repository(MapController *self, Repository *repo);
GList *map_controller_get_repo_list(MapController *self);
GList *map_controller_get_tile_sources_list(MapController *self);
void map_controller_delete_repository(MapController *self, Repository *repo);
void map_controller_delete_tile_source(MapController *self, TileSource *ts);
void map_controller_append_tile_source(MapController *self, TileSource *ts);
void map_controller_append_repository(MapController *self, Repository *repo);
void map_controller_toggle_layer_visibility(MapController *self, RepositoryLayer *repo_layer);
void map_controller_set_auto_download(MapController *self,
                                      gboolean auto_download);
gboolean map_controller_get_auto_download(MapController *self);

void map_controller_action_zoom_in(MapController *self);
void map_controller_action_zoom_out(MapController *self);
void map_controller_action_zoom_stop(MapController *self);
void map_controller_action_switch_fullscreen(MapController *self);
void map_controller_action_point(MapController *self);
void map_controller_action_route(MapController *self);
void map_controller_action_track(MapController *self);
void map_controller_action_view(MapController *self);
void map_controller_action_go_to(MapController *self);

void map_controller_activate_menu_point(MapController *self, const MapPoint *p);

void map_controller_set_gps_enabled(MapController *self, gboolean enabled);
gboolean map_controller_get_gps_enabled(MapController *self);
void map_controller_set_gps_position(MapController *self, const MapPoint *p);
const MapGpsData *map_controller_get_gps_data(MapController *self);

void map_controller_set_center_mode(MapController *self, CenterMode mode);
CenterMode map_controller_get_center_mode(MapController *self);
void map_controller_disable_auto_center(MapController *self);

void map_controller_set_auto_rotate(MapController *self, gboolean enable);
gboolean map_controller_get_auto_rotate(MapController *self);

void map_controller_set_tracking(MapController *self, gboolean enable);
gboolean map_controller_get_tracking(MapController *self);

void map_controller_set_show_compass(MapController *self, gboolean show);
gboolean map_controller_get_show_compass(MapController *self);

void map_controller_set_show_routes(MapController *self, gboolean show);
gboolean map_controller_get_show_routes(MapController *self);

void map_controller_set_show_tracks(MapController *self, gboolean show);
gboolean map_controller_get_show_tracks(MapController *self);

void map_controller_set_show_scale(MapController *self, gboolean show);
gboolean map_controller_get_show_scale(MapController *self);

void map_controller_set_show_poi(MapController *self, gboolean show);
gboolean map_controller_get_show_poi(MapController *self);

void map_controller_set_show_velocity(MapController *self, gboolean show);
gboolean map_controller_get_show_velocity(MapController *self);

void map_controller_set_show_zoom(MapController *self, gboolean show);
gboolean map_controller_get_show_zoom(MapController *self);

void map_controller_set_show_gps_info(MapController *self, gboolean show);
gboolean map_controller_get_show_gps_info(MapController *self);

void map_controller_set_center(MapController *self, MapPoint center, gint zoom);
void map_controller_get_center(MapController *self, MapPoint *center);
void map_controller_set_rotation(MapController *self, gint angle);
gint map_controller_get_rotation(MapController *self);
void map_controller_rotate(MapController *self, gint angle);
void map_controller_set_zoom(MapController *self, gint zoom);
gint map_controller_get_zoom(MapController *self);
void map_controller_calc_best_center(MapController *self, MapPoint *new_center);

void map_controller_refresh_paths(MapController *controller);
void map_controller_update_gps(MapController *self);

/* Settings */
void map_controller_load_repositories(MapController *self, GConfClient *gconf_client);
void map_controller_save_repositories(MapController *self, GConfClient *gconf_client);

/* Device activity state */
gboolean map_controller_get_device_active(MapController *self);
void map_controller_set_device_active(MapController *self, gboolean active);
void map_controller_display_status_changed(MapController *self,
                                           const gchar *status);
gboolean map_controller_is_display_on(MapController *self);

/* Plugins */
void map_controller_register_plugin(MapController *self, GObject *plugin);
const GSList *map_controller_list_plugins(MapController *self);

void map_controller_set_default_router(MapController *self, MapRouter *router);
MapRouter *map_controller_get_default_router(MapController *self);

void map_controller_load_routers_options(MapController *self, GConfClient *gconf_client);
void map_controller_save_routers_options(MapController *self, GConfClient *gconf_client);

G_END_DECLS
#endif /* MAP_CONTROLLER_H */
