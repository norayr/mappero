/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "screen.h"
#include "tile.h"

#include "controller.h"
#include "defines.h"

#include <clutter-gtk/clutter-gtk.h>
#include <mappero/debug.h>
#include <stdlib.h>

struct _MapTilePrivate
{
    guint has_pixbuf : 1;
    guint is_disposed : 1;
};

G_DEFINE_TYPE(MapTile, map_tile, CLUTTER_TYPE_TEXTURE);

#define MAP_TILE_PRIV(tile) (MAP_TILE(tile)->priv)

/* given the screen size, we usually load no more than 25 tiles per layer */
#define NUM_TILES_PER_LAYER 25
#define NUM_TILES_CACHED 15
#define MAX_CACHE_SIZE  (NUM_TILES_PER_LAYER * (num_active_layers() + 1) + \
                         NUM_TILES_CACHED)

static GList *tile_cache = NULL;
static GList *tile_cache_last = NULL;
static gint tile_cache_size = 0;

static gint
num_active_layers()
{
    MapController *controller = map_controller_get_instance();
    Repository *repository = map_controller_get_repository(controller);

    if (G_UNLIKELY(!repository)) return 0;

    return (repository->layers != NULL) ? repository->layers->len : 0;
}

static void
map_tile_clear(MapTile *tile)
{
    guchar data[3] = { 0xff, 0xff, 0xff };
    clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(tile),
                                      data, FALSE, 1, 1, 1, 3, 0, NULL);
}

static gboolean
compare_tile_spec(MapTile *tile, MapTileSpec *ts)
{
    return (tile->ts.zoom == ts->zoom &&
            tile->ts.tilex == ts->tilex &&
            tile->ts.tiley == ts->tiley &&
            tile->ts.source == ts->source) ? 0 : 1;
}

static void
download_tile_cb(MapTileSpec *ts, GdkPixbuf *pixbuf, const GError *error,
                 gpointer user_data)
{
    MapTile *tile, **p_tile;

    DEBUG("");
    p_tile = user_data;
    tile = *p_tile;
    g_slice_free(MapTile *, p_tile);
    if (!tile)
        return;

    g_object_remove_weak_pointer(G_OBJECT(tile), user_data);

    /* Are we still interested in this pixbuf? */
    if (ts->zoom != tile->ts.zoom ||
        ts->tilex != tile->ts.tilex ||
        ts->tiley != tile->ts.tiley)
    {
        return;
    }

    if (pixbuf)
    {
        gtk_clutter_texture_set_from_pixbuf(CLUTTER_TEXTURE(tile),
                                            pixbuf, NULL);
        tile->priv->has_pixbuf = TRUE;
    }
    else
        tile->priv->has_pixbuf = FALSE;
}

static void
map_tile_download(MapTile *tile)
{
    MapController *controller;
    MapTile **p_tile;
    gint priority, zoom;
    MapPoint center;

    /* The priority is lower (that is, higher number) when we walk away
     * from the center of the map */
    controller = map_controller_get_instance();
    map_controller_get_center(controller, &center);
    zoom = tile->ts.zoom;
    priority =
        abs(tile->ts.tilex - unit2ztile(center.x, zoom)) +
        abs(tile->ts.tiley - unit2ztile(center.y, zoom));

    /* weak pointer trick to prevent crashes if the callback is invoked
     * after the tile is destroyed. */
    p_tile = g_slice_new(MapTile *);
    *p_tile = tile;
    g_object_add_weak_pointer(G_OBJECT(tile), (gpointer)p_tile);
    map_download_tile(&tile->ts, priority, download_tile_cb, p_tile);
}

static void
map_tile_dispose(GObject *object)
{
    MapTilePrivate *priv = MAP_TILE_PRIV(object);

    if (priv->is_disposed)
	return;

    priv->is_disposed = TRUE;

    G_OBJECT_CLASS(map_tile_parent_class)->dispose(object);
}

static void
map_tile_init(MapTile *tile)
{
    MapTilePrivate *priv;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(tile, MAP_TYPE_TILE, MapTilePrivate);
    tile->priv = priv;
}

static void
map_tile_class_init(MapTileClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MapTilePrivate));

    object_class->dispose = map_tile_dispose;
}

MapTile *
map_tile_get_instance(gboolean *new_tile)
{
    MapTile *tile;
    GList *list = NULL;

    if (tile_cache_size >= MAX_CACHE_SIZE)
    {
        /* get the least-recently used tile from the cache and reuse it */
        g_return_val_if_fail(tile_cache_last != NULL, NULL);

        list = tile_cache_last;
        tile = MAP_TILE(list->data);

        /* move the tile to the beginning of the cache */
        tile_cache_last = list->prev;
        tile_cache = g_list_remove_link(tile_cache, list);
        tile_cache = g_list_concat(list, tile_cache);
        *new_tile = FALSE;
    }
    else
    {
        /* We can create a new tile object */
        tile = g_object_new (MAP_TYPE_TILE, NULL);
        tile_cache = g_list_prepend(tile_cache, tile);
        if (tile_cache_size == 0)
            tile_cache_last = tile_cache;

        tile_cache_size++;
        *new_tile = TRUE;
    }
    return tile;
}

/**
 * map_tile_load:
 * @zoom: the zoom level.
 * @x: x coordinate of the tile, in units.
 * @y: y coordinate of the tile, in units.
 *
 * Returns: a #ClutterActor representing the tile.
 */
ClutterActor *
map_tile_load(TileSource *source, gint zoom, gint x, gint y, gboolean *new_tile)
{
    MapTile *tile;
    GdkPixbuf *pixbuf, *area;
    gboolean must_clear = TRUE;
    gint zoff;

    tile = map_tile_get_instance(new_tile);

    tile->ts.source = source;
    tile->ts.zoom = zoom;
    tile->ts.tilex = x;
    tile->ts.tiley = y;

    for (zoff = 0; zoff + zoom <= MAX_ZOOM && zoff < 4; zoff++)
    {
        pixbuf = mapdb_get(source, zoom + zoff, x >> zoff, y >> zoff);
        if (pixbuf)
        {
            if (zoff != 0)
            {
                gint area_size, modulo, area_x, area_y;

                area_size = TILE_SIZE_PIXELS >> zoff;
                modulo = 1 << zoff;
                area_x = (x % modulo) * area_size;
                area_y = (y % modulo) * area_size;
                area = gdk_pixbuf_new_subpixbuf (pixbuf, area_x, area_y,
                                                 area_size, area_size);
                g_object_unref (pixbuf);
                pixbuf = gdk_pixbuf_scale_simple (area,
                                                  TILE_SIZE_PIXELS,
                                                  TILE_SIZE_PIXELS,
                                                  GDK_INTERP_NEAREST);
                g_object_unref (area);
                must_clear = FALSE;
            }
            gtk_clutter_texture_set_from_pixbuf(CLUTTER_TEXTURE(tile),
                                                pixbuf, NULL);
            g_object_unref(pixbuf);
            break;
        }

        /* For faster load, we don't scale transparent layers */
        if (source->transparent) {
            zoff = 1;
            break;
        }
    }

    if (zoff != 0)
    {
        MapController *controller = map_controller_get_instance();
        if (map_controller_get_auto_download(controller))
            map_tile_download(tile);

        /* if this is not a new tile, it contains dirty data: clean it */
        if (must_clear)
            map_tile_clear(tile);
    }

    tile->priv->has_pixbuf = (zoff == 0);

    return CLUTTER_ACTOR(tile);
}

ClutterActor *
map_tile_cached(TileSource *source, gint zoom, gint x, gint y)
{
    MapTileSpec ts;
    GList *list;
    ClutterActor *tile = NULL;

    ts.source = source;
    ts.tilex = x;
    ts.tiley = y;
    ts.zoom = zoom;

    /* first, check whether the tile is in the cache */
    list = g_list_find_custom(tile_cache, &ts,
                              (GCompareFunc)compare_tile_spec);
    if (list)
    {
        tile = CLUTTER_ACTOR(list->data);

        /* if found, move it to the beginning of the list */
        if (list != tile_cache)
        {
            if (list == tile_cache_last)
                tile_cache_last = list->prev;

            tile_cache = g_list_remove_link(tile_cache, list);
            tile_cache = g_list_concat(list, tile_cache);
        }
    }

    return tile;
}

void
map_tile_refresh(MapTile *tile)
{
    if (tile->priv->has_pixbuf)
        return;

    map_tile_download(tile);
}

/*
 * Iterate over cache and request redownload of expired tiles.
 */
void
refresh_expired_tiles()
{
    GList *iter = tile_cache;
    MapTile *mt;

    while (iter) {
        mt = MAP_TILE(iter->data);
        if (mt->ts.source->countdown < 0)
            map_tile_download(mt);
        iter = iter->next;
    }
}