/*
 * Copyright (C) 2006, 2007 John Costigan.
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

#ifndef MAEMO_MAPPER_MAPS_H
#define MAEMO_MAPPER_MAPS_H

gboolean mapdb_exists(TileSource *source, gint zoom, gint tilex, gint tiley);
GdkPixbuf* mapdb_get(TileSource *source, gint zoom, gint tilex, gint tiley);

gboolean mapdb_initiate_update(TileSource *repo, gint zoom, gint tilex,
        gint tiley, gint update_type, gint batch_id, gint priority,
        ThreadLatch *refresh_latch);

guint mut_exists_hashfunc(gconstpointer);
gboolean mut_exists_equalfunc(gconstpointer a, gconstpointer b);
gint mut_priority_comparefunc(gconstpointer a, gconstpointer b);
gboolean thread_proc_mut(void);

gboolean mapman_dialog(void);

gboolean map_layer_refresh_cb (gpointer data);

void maps_toggle_visible_layers ();

/* returns amount of seconds since tile downloaded */
gint get_tile_age (GdkPixbuf* pixbuf);

typedef struct {
    TileSource *source;
    gint tilex;
    gint tiley;
    gint8 zoom;
} MapTileSpec;

typedef void (*MapUpdateCb)(MapTileSpec *tile,
                            GdkPixbuf *pixbuf, const GError *error,
                            gpointer user_data);

void map_download_tile(MapTileSpec *tile, gint priority,
                       MapUpdateCb callback, gpointer user_data);

void map_download_precache(TileSource *tile_source, MapPoint center, gint zoom,
                           gint cache_amount,
                           gint screen_width, gint screen_height);

#endif /* ifndef MAEMO_MAPPER_MAPS_H */
