/*
 * Copyright (C) 2006, 2007 John Costigan.
 *
 * POI and GPS-Info code originally written by Cezary Jackiewicz.
 *
 * Default map data provided by http://www.openstreetmap.org/
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
#    include "config.h"
#endif

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <locale.h>
#include <time.h>

#ifndef LEGACY
#    include <hildon/hildon-note.h>
#    include <hildon/hildon-file-chooser-dialog.h>
#    include <hildon/hildon-number-editor.h>
#    include <hildon/hildon-banner.h>
#else
#    include <osso-helplib.h>
#    include <hildon-widgets/hildon-note.h>
#    include <hildon-widgets/hildon-file-chooser-dialog.h>
#    include <hildon-widgets/hildon-number-editor.h>
#    include <hildon-widgets/hildon-banner.h>
#    include <hildon-widgets/hildon-input-mode-hint.h>
#endif


#include "types.h"
#include "data.h"
#include "debug.h"
#include "defines.h"

#include "controller.h"
#include "display.h"
#include "main.h"
#include "maps.h"
#include "menu.h"
#include "repository.h"
#include "screen.h"
#include "settings.h"
#include "tile_source.h"
#include "util.h"

typedef struct
{
    MapUpdateCb callback;
    gpointer user_data;
} MapUpdateCbData;

typedef struct
{
    MapTileSpec tile;
    gint priority;
    gint8 update_type;
    guint downloading : 1;
    /* list of MapUpdateCbData */
    GSList *callbacks;

    /* Output values */
    GdkPixbuf *pixbuf;
    GError *error;
} MapUpdateTask;



typedef struct _MapmanInfo MapmanInfo;
struct _MapmanInfo {
    GtkWidget *dialog;
    GtkWidget *notebook;
    GtkWidget *tbl_area;

    /* The "Setup" tab. */
    GtkWidget *rad_download;
    GtkWidget *rad_delete;
    GtkWidget *chk_overwrite;
    GtkWidget *rad_by_area;
    GtkWidget *rad_by_route;
    GtkWidget *num_route_radius;

    /* The "Area" tab. */
    GtkWidget *txt_topleft_lat;
    GtkWidget *txt_topleft_lon;
    GtkWidget *txt_botright_lat;
    GtkWidget *txt_botright_lon;

    /* The "Zoom" tab. */
    GtkWidget *chk_zoom_levels[MAX_ZOOM + 1];
};


typedef struct _CompactInfo CompactInfo;
struct _CompactInfo {
    GtkWidget *dialog;
    GtkWidget *txt;
    GtkWidget *banner;
    const gchar *db_filename;
    gboolean is_sqlite;
    gchar *status_msg;
};

typedef struct _MapCacheKey MapCacheKey;
struct _MapCacheKey {
    TileSource     *source;
    gint           zoom;
    gint           tilex;
    gint           tiley;
};

typedef struct _MapCacheEntry MapCacheEntry;
struct _MapCacheEntry {
    MapCacheKey    key;
    int            list;
    guint          size;
    guint          data_sz;
    gchar         *data;
    GdkPixbuf     *pixbuf;
    MapCacheEntry *next;
    MapCacheEntry *prev;
};

typedef struct _MapCacheList MapCacheList;
struct _MapCacheList {
    MapCacheEntry *head;
    MapCacheEntry *tail;
    size_t         size;
    size_t         data_sz;
};

typedef struct _MapCache MapCache;
struct _MapCache {
    MapCacheList  lists[4];
    size_t        cache_size;
    size_t        p;
    size_t        thits;
    size_t        bhits;
    size_t        misses;
    GHashTable   *entries;
};

static void
build_tile_path(gchar *buffer, gsize size,
                TileSource *source, gint zoom, gint tilex, gint tiley)
{
    g_snprintf(buffer, size,
               "/home/user/MyDocs/.maps/%s/%d/%d/",
               source->cache_dir, 21 - zoom, tilex);
}

static void
build_tile_filename(gchar *buffer, gsize size,
                    TileSource *source, gint zoom, gint tilex, gint tiley)
{
    g_snprintf(buffer, size,
               "/home/user/MyDocs/.maps/%s/%d/%d/%d.%s",
               source->cache_dir, 21 - zoom, tilex, tiley,
               tile_source_format_extention(source->format));
}

static gboolean
is_tile_expired(const gchar *tile_path, gint mins_to_expire)
{
    struct stat st;

    if (!mins_to_expire)
        return FALSE;

    if (!g_stat (tile_path, &st))
        return st.st_mtime < (time(NULL) - mins_to_expire * 60);
    else
        return TRUE;
}


gboolean
mapdb_exists(TileSource *source, gint zoom, gint tilex, gint tiley)
{
    gchar filename[200];
    gboolean exists = FALSE;
    DEBUG("%s, %d, %d, %d", source->name, zoom, tilex, tiley);

    build_tile_filename(filename, sizeof(filename), source, zoom, tilex, tiley);
    if (!is_tile_expired(filename, source->refresh))
        exists = g_file_test(filename, G_FILE_TEST_EXISTS);

    return exists;
}

GdkPixbuf*
mapdb_get(TileSource *source, gint zoom, gint tilex, gint tiley)
{
    gchar filename[200];
    GdkPixbuf *pixbuf = NULL;
    DEBUG("%s, %d, %d, %d", source->name, zoom, tilex, tiley);

    build_tile_filename(filename, sizeof(filename), source, zoom, tilex, tiley);
    if (!is_tile_expired(filename, source->refresh))
        pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

    return pixbuf;
}

static gboolean
mapdb_update(TileSource *source, gint zoom, gint tilex, gint tiley,
        void *bytes, gint size)
{
    gint success = TRUE;
    gchar filename[200], path[200];
    DEBUG("%s, %d, %d, %d", source->name, zoom, tilex, tiley);

    build_tile_path(path, sizeof(path), source, zoom, tilex, tiley);
    g_mkdir_with_parents(path, 0766);
    build_tile_filename(filename, sizeof(filename), source, zoom, tilex, tiley);
    success = g_file_set_contents(filename, bytes, size, NULL);

    return success;
}

static gboolean
mapdb_delete(TileSource *source, gint zoom, gint tilex, gint tiley)
{
    gchar filename[200];
    gint success = FALSE;
    DEBUG("%s, %d, %d, %d", source->name, zoom, tilex, tiley);

    build_tile_filename(filename, sizeof(filename), source, zoom, tilex, tiley);
    g_remove(filename);

    return success;
}


/**
 * Construct the URL that we should fetch, based on the current URI format.
 * This method works differently depending on if a "%s" string is present in
 * the URI format, since that would indicate a quadtree-based map coordinate
 * system.
 */
static gint
map_construct_url(gchar *buffer, gint len, TileSource *source, gint zoom, gint tilex, gint tiley)
{
    g_return_val_if_fail(source->type != NULL, -1);

    return source->type->get_url(source, buffer, len, zoom, tilex, tiley);
}

static void
mapdb_initiate_update_banner()
{
    if (!_download_banner)
    {
        _download_banner = hildon_banner_show_progress(
                _window, NULL, _("Processing Maps"));
        /* If we're not connected, then hide the banner immediately.  It will
         * be unhidden if/when we're connected. */
        if(!_conic_is_connected)
            gtk_widget_hide(_download_banner);
    }
}

static void
map_update_tile_int(MapTileSpec *tile, gint priority, MapUpdateType update_type,
                    MapUpdateCb callback, gpointer user_data)
{
    MapUpdateTask *mut;
    MapUpdateTask *old_mut;
    MapUpdateCbData *cb_data = NULL;

    printf("%s(%s, %d, %d, %d, %d)\n", G_STRFUNC,
            tile->source->name, tile->zoom, tile->tilex, tile->tiley, update_type);

    mut = g_slice_new0(MapUpdateTask);
    if (!mut)
    {
        /* Could not allocate memory. */
        g_error("Out of memory in allocation of update task");
        return;
    }
    memcpy(&mut->tile, tile, sizeof(MapTileSpec));
    mut->priority = priority;
    mut->update_type = update_type;

    if (callback)
    {
        cb_data = g_slice_new(MapUpdateCbData);
        cb_data->callback = callback;
        cb_data->user_data = user_data;
    }

    g_mutex_lock(_mut_priority_mutex);
    old_mut = g_hash_table_lookup(_mut_exists_table, mut);
    if (old_mut)
    {
        printf("Task already queued, adding listener\n");

        if (cb_data)
            old_mut->callbacks = g_slist_prepend(old_mut->callbacks, cb_data);

        /* Check if the priority of the new task is higher */
        if (old_mut->priority > mut->priority && !old_mut->downloading)
        {
            printf("re-insert, old priority = %d\n", old_mut->priority);
            /* It is, so remove the task from the tree, update its priority and
             * re-insert it with the new one */
            g_tree_remove(_mut_priority_tree, old_mut);
            old_mut->priority = mut->priority;
            g_tree_insert(_mut_priority_tree, old_mut, old_mut);
        }

        /* free the newly allocated task */
        g_slice_free(MapUpdateTask, mut);
        g_mutex_unlock(_mut_priority_mutex);
        return;
    }

    if (cb_data)
        mut->callbacks = g_slist_prepend(mut->callbacks, cb_data);

    g_hash_table_insert(_mut_exists_table, mut, mut);
    g_tree_insert(_mut_priority_tree, mut, mut);

    g_mutex_unlock(_mut_priority_mutex);

    /* Increment download count and (possibly) display banner. */
    if (g_hash_table_size(_mut_exists_table) >= 20 && !_download_banner)
        mapdb_initiate_update_banner();

    /* This doesn't need to be thread-safe.  Extras in the pool don't
     * really make a difference. */
    if (g_thread_pool_get_num_threads(_mut_thread_pool)
        < g_thread_pool_get_max_threads(_mut_thread_pool))
        g_thread_pool_push(_mut_thread_pool, (gpointer)1, NULL);
}

void
map_download_tile(MapTileSpec *tile, gint priority,
                  MapUpdateCb callback, gpointer user_data)
{
    map_update_tile_int(tile, priority, MAP_UPDATE_AUTO, callback, user_data);
}

/**
 * Initiate a download of the given xyz coordinates using the given buffer
 * as the URL.  If the map already exists on disk, or if we are already
 * downloading the map, then this method does nothing.
 */
gboolean
mapdb_initiate_update(TileSource *source, gint zoom, gint tilex, gint tiley,
        gint update_type, gint batch_id, gint priority,
        ThreadLatch *refresh_latch)
{
    MapTileSpec tile;

    tile.source = source;
    tile.zoom = zoom;
    tile.tilex = tilex;
    tile.tiley = tiley;
    map_update_tile_int(&tile, priority, update_type,
                        map_download_refresh_idle,
                        GINT_TO_POINTER(update_type));
    return FALSE;
}

static gboolean
get_next_mut(gpointer key, gpointer value, MapUpdateTask **data)
{
    *data = key;
    return TRUE;
}

static gboolean
map_handle_error(gchar *error)
{
    MACRO_BANNER_SHOW_INFO(_window, error);
    return FALSE;
}

static gboolean
map_update_task_completed(MapUpdateTask *mut)
{
    MapTileSpec *tile = &mut->tile;

    printf("%s(%s, %d, %d, %d)\n", G_STRFUNC,
            tile->source->name, tile->zoom, tile->tilex, tile->tiley);

    g_mutex_lock(_mut_priority_mutex);
    g_hash_table_remove(_mut_exists_table, mut);

    /* notify all listeners */
    mut->callbacks = g_slist_reverse(mut->callbacks);
    while (mut->callbacks != NULL)
    {
        MapUpdateCbData *cb_data = mut->callbacks->data;

        (cb_data->callback)(tile, mut->pixbuf, mut->error, cb_data->user_data);

        g_slice_free(MapUpdateCbData, cb_data);
        mut->callbacks = g_slist_delete_link(mut->callbacks, mut->callbacks);
    }
    g_mutex_unlock(_mut_priority_mutex);

    if (g_hash_table_size(_mut_exists_table) == 0)
    {
        g_thread_pool_stop_unused_threads();

        if (_download_banner)
        {
            gtk_widget_destroy(_download_banner);
            _download_banner = NULL;
        }
    }

    /* clean up the task memory */
    if (mut->pixbuf)
        g_object_unref(mut->pixbuf);
    if (mut->error)
        g_error_free(mut->error);
    g_slice_free(MapUpdateTask, mut);

    /* don't call again */
    return FALSE;
}

static void
download_tile(MapTileSpec *tile, gchar **bytes, gint *size,
              GdkPixbuf **pixbuf, GError **error)
{
    gchar src_url[256];         /* 256 may become too little, somtetimes, I guess */
    GnomeVFSResult vfs_result;
    GdkPixbufLoader *loader = NULL;
    gint ret;

    printf("%s (%s, %d, %d, %d)\n", G_STRFUNC,
            tile->source->name, tile->zoom, tile->tilex, tile->tiley);

    /* First, construct the URL from which we will get the data. */
    ret = map_construct_url(src_url, sizeof(src_url), tile->source,
                            tile->zoom, tile->tilex, tile->tiley);
    if (ret < 0) src_url[0] = 0; /* download will fail */

    /* Now, attempt to read the entire contents of the URL. */
    vfs_result = gnome_vfs_read_entire_file(src_url, size, bytes);

    if (vfs_result != GNOME_VFS_OK || *bytes == NULL)
    {
        /* it might not be very proper, but let's use GFile error codes */
        g_set_error(error, g_file_error_quark(), G_FILE_ERROR_FAULT,
                    "%s", gnome_vfs_result_to_string(vfs_result));
        goto l_error;
    }

    /* Attempt to parse the bytes into a pixbuf. */
    loader = gdk_pixbuf_loader_new();
    gdk_pixbuf_loader_write(loader, (guchar *)(*bytes), *size, NULL);
    gdk_pixbuf_loader_close(loader, error);
    if (*error) goto l_error;

    *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    if (!*pixbuf)
    {
        g_set_error(error, g_file_error_quark(), G_FILE_ERROR_INVAL,
                    "Creation of pixbuf failed");
        goto l_error;
    }
    g_object_ref(*pixbuf);
    g_object_unref(loader);
    return;

l_error:
    if (loader)
        g_object_unref(loader);
    g_free(*bytes);
    *bytes = NULL;
}

static void
map_update_task_remove_all(const GError *error)
{
    printf("%s\n", G_STRFUNC);

    while (1)
    {
        MapUpdateTask *mut = NULL;

        g_mutex_lock(_mut_priority_mutex);
        g_tree_foreach(_mut_priority_tree, (GTraverseFunc)get_next_mut, &mut);
        if (!mut)
        {
            g_mutex_unlock(_mut_priority_mutex);
            break;
        }

        /* Mark this MUT as "in-progress". */
        mut->downloading = TRUE;
        g_tree_remove(_mut_priority_tree, mut);
        g_mutex_unlock(_mut_priority_mutex);

        mut->pixbuf = NULL;
        mut->error = g_error_copy(error);
        g_idle_add((GSourceFunc)map_update_task_completed, mut);
    }
}

gboolean
thread_proc_mut()
{
    DEBUG("");

    /* Make sure things are inititalized. */
    gnome_vfs_init();

    while(conic_ensure_connected())
    {
        gint retries;
        gboolean notification_sent = FALSE;
        MapUpdateTask *mut = NULL;
        MapTileSpec *tile;

        /* Get the next MUT from the mut tree. */
        g_mutex_lock(_mut_priority_mutex);
        g_tree_foreach(_mut_priority_tree, (GTraverseFunc)get_next_mut, &mut);
        if (!mut)
        {
            /* No more MUTs to process.  Return. */
            g_mutex_unlock(_mut_priority_mutex);
            return FALSE;
        }

        tile = &mut->tile;
        /* Mark this MUT as "in-progress". */
        mut->downloading = TRUE;
        g_tree_remove(_mut_priority_tree, mut);
        g_mutex_unlock(_mut_priority_mutex);

        printf("%s %p (%s, %d, %d, %d)\n", G_STRFUNC, mut,
                tile->source->name, tile->zoom, tile->tilex, tile->tiley);

        mut->pixbuf = NULL;
        mut->error = NULL;

        if (mut->update_type == MAP_UPDATE_DELETE)
        {
            /* Easy - just delete the entry from the database.  We don't care
             * about failures (sorry). */
            mapdb_delete(tile->source, tile->zoom, tile->tilex, tile->tiley);
        }
        else
        {
            gboolean download_needed = TRUE;

            /* First check for existence. */
            if (mut->update_type == MAP_UPDATE_ADD)
            {
                /* We don't want to overwrite, so check for existence. */
                /* Map already exists, and we're not going to overwrite. */
                if (mapdb_exists(tile->source, tile->zoom, tile->tilex, tile->tiley))
                {
                    download_needed = FALSE;
                }
            }

            if (download_needed)
            {
                TileSource *source = tile->source;
                gint zoom, tilex, tiley;
                gchar *bytes = NULL;
                gint size;

                for (retries = source->transparent ? 1 : INITIAL_DOWNLOAD_RETRIES; retries > 0; --retries)
                {
                    g_clear_error(&mut->error);
                    download_tile(tile, &bytes, &size,
                                  &mut->pixbuf, &mut->error);
                    if (mut->pixbuf) break;

                    printf("Download failed, retrying\n");
                }

                /* Copy database-relevant mut data before we release it. */
                zoom = tile->zoom;
                tilex = tile->tilex;
                tiley = tile->tiley;

                printf("%s(%s, %d, %d, %d): %s\n", G_STRFUNC,
                        tile->source->name, tile->zoom, tile->tilex, tile->tiley,
                        mut->pixbuf ? "Success" : "Failed");
                g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                                (GSourceFunc)map_update_task_completed, mut,
                                NULL);
                notification_sent = TRUE;

                /* DO NOT USE mut FROM THIS POINT ON. */

                /* Also attempt to add to the database. */
                if (bytes &&
                    !mapdb_update(source, zoom, tilex, tiley, bytes, size)) {
                    g_idle_add((GSourceFunc)map_handle_error,
                               _("Error saving map to disk - disk full?"));
                }

                /* Success! */
                g_free(bytes);
            }
        }

        if (!notification_sent)
            g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                            (GSourceFunc)map_update_task_completed, mut, NULL);
    }

    if (!_conic_is_connected)
    {
        /* borrow error from glib fileutils */
        GError error = { G_FILE_ERROR, G_FILE_ERROR_FAULT, "Disconnected" };

        /* Abort all tasks */
        map_update_task_remove_all(&error);
        return FALSE;
    }

    return FALSE;
}

guint
mut_exists_hashfunc(gconstpointer a)
{
    const MapUpdateTask *t = a;
    const MapTileSpec *r = &t->tile;
    return r->zoom + r->tilex + r->tiley + t->update_type;
}

gboolean
mut_exists_equalfunc(gconstpointer a, gconstpointer b)
{
    const MapUpdateTask *t1 = a;
    const MapUpdateTask *t2 = b;
    return (t1->tile.tilex == t2->tile.tilex
            && t1->tile.tiley == t2->tile.tiley
            && t1->tile.zoom == t2->tile.zoom
            && t1->update_type == t2->update_type
            && t1->tile.source == t2->tile.source);
}

gint
mut_priority_comparefunc(gconstpointer _a, gconstpointer _b)
{
    const MapUpdateTask *a = _a;
    const MapUpdateTask *b = _b;
    /* The update_type enum is sorted in order of ascending priority. */
    gint diff = (b->update_type - a->update_type);
    if(diff)
        return diff;
    diff = (a->priority - b->priority); /* Lower priority numbers first. */
    if(diff)
        return diff;

    /* At this point, we don't care, so just pick arbitrarily. */
    diff = (a->tile.source - b->tile.source);
    if(diff)
        return diff;
    diff = (a->tile.tilex - b->tile.tilex);
    if(diff)
        return diff;
    diff = (a->tile.tiley - b->tile.tiley);
    if(diff)
        return diff;
    return (a->tile.zoom - b->tile.zoom);
}


static gboolean
mapman_by_area(gdouble start_lat, gdouble start_lon,
        gdouble end_lat, gdouble end_lon, MapmanInfo *mapman_info,
        MapUpdateType update_type,
        gint download_batch_id)
{
    gint start_unitx, start_unity, end_unitx, end_unity;
    gint num_maps = 0;
    gint z;
    gchar buffer[80];
    GtkWidget *confirm;
    Repository* rd = map_controller_get_repository(map_controller_get_instance());

    DEBUG("%f, %f, %f, %f", start_lat, start_lon, end_lat, end_lon);

    latlon2unit(start_lat, start_lon, start_unitx, start_unity);
    latlon2unit(end_lat, end_lon, end_unitx, end_unity);

    /* Swap if they specified flipped lats or lons. */
    if(start_unitx > end_unitx)
    {
        gint swap = start_unitx;
        start_unitx = end_unitx;
        end_unitx = swap;
    }
    if(start_unity > end_unity)
    {
        gint swap = start_unity;
        start_unity = end_unity;
        end_unity = swap;
    }

    /* First, get the number of maps to download. */
    for(z = 0; z <= MAX_ZOOM; ++z)
    {
        if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(mapman_info->chk_zoom_levels[z])))
        {
            gint start_tilex, start_tiley, end_tilex, end_tiley;
            start_tilex = unit2ztile(start_unitx, z);
            start_tiley = unit2ztile(start_unity, z);
            end_tilex = unit2ztile(end_unitx, z);
            end_tiley = unit2ztile(end_unity, z);
            num_maps += (end_tilex - start_tilex + 1)
                * (end_tiley - start_tiley + 1);
        }
    }

    if(update_type == MAP_UPDATE_DELETE)
    {
        snprintf(buffer, sizeof(buffer), "%s %d %s", _("Confirm DELETION of"),
                num_maps, _("maps "));
    }
    else
    {
        snprintf(buffer, sizeof(buffer),
                "%s %d %s\n", _("Confirm download of"),
                num_maps, _("maps"));
    }
    confirm = hildon_note_new_confirmation(
            GTK_WINDOW(mapman_info->dialog), buffer);

    if(GTK_RESPONSE_OK != gtk_dialog_run(GTK_DIALOG(confirm)))
    {
        gtk_widget_destroy(confirm);
        return FALSE;
    }

    for(z = 0; z <= MAX_ZOOM; ++z)
    {
        if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(mapman_info->chk_zoom_levels[z])))
        {
            gint start_tilex, start_tiley, end_tilex, end_tiley;
            gint tilex, tiley;
            start_tilex = unit2ztile(start_unitx, z);
            start_tiley = unit2ztile(start_unity, z);
            end_tilex = unit2ztile(end_unitx, z);
            end_tiley = unit2ztile(end_unity, z);
            for(tiley = start_tiley; tiley <= end_tiley; tiley++)
            {
                for(tilex = start_tilex; tilex <= end_tilex; tilex++)
                {
                    /* Make sure this tile is even possible. */
                    if((unsigned)tilex < unit2ztile(WORLD_SIZE_UNITS, z)
                      && (unsigned)tiley < unit2ztile(WORLD_SIZE_UNITS, z))
                    {
                        mapdb_initiate_update(rd->primary, z, tilex, tiley,
                                              update_type, download_batch_id,
                                              (abs(tilex - unit2tile(_next_center.x))
                                               +abs(tiley - unit2tile(_next_center.y))),
                                              NULL);
                    }
                }
            }
        }
    }

    gtk_widget_destroy(confirm);
    return TRUE;
}

static gboolean
mapman_by_route(MapmanInfo *mapman_info, MapUpdateType update_type,
        gint download_batch_id)
{
    GtkWidget *confirm;
    gint prev_tilex, prev_tiley, num_maps = 0, z;
    Point *curr;
    gchar buffer[80];
    Repository* rd = map_controller_get_repository(map_controller_get_instance());
    gint radius = hildon_number_editor_get_value(
            HILDON_NUMBER_EDITOR(mapman_info->num_route_radius));
    DEBUG("");

    /* First, get the number of maps to download. */
    for(z = 0; z <= MAX_ZOOM; ++z)
    {
        if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(mapman_info->chk_zoom_levels[z])))
        {
            prev_tilex = 0;
            prev_tiley = 0;
            for(curr = _route.head - 1; curr++ != _route.tail; )
            {
                if(curr->unit.y)
                {
                    gint tilex = unit2ztile(curr->unit.x, z);
                    gint tiley = unit2ztile(curr->unit.y, z);
                    if(tilex != prev_tilex || tiley != prev_tiley)
                    {
                        if(prev_tiley)
                            num_maps += (abs((gint)tilex - prev_tilex) + 1)
                                * (abs((gint)tiley - prev_tiley) + 1) - 1;
                        prev_tilex = tilex;
                        prev_tiley = tiley;
                    }
                }
            }
        }
    }
    num_maps *= 0.625 * pow(radius + 1, 1.85);

    if(update_type == MAP_UPDATE_DELETE)
    {
        snprintf(buffer, sizeof(buffer), "%s %s %d %s",
                _("Confirm DELETION of"), _("about"),
                num_maps, _("maps "));
    }
    else
    {
        snprintf(buffer, sizeof(buffer),
                "%s %s %d %s\n", _("Confirm download of"),
                _("about"),
                 num_maps, _("maps"));
    }
    confirm = hildon_note_new_confirmation(
            GTK_WINDOW(mapman_info->dialog), buffer);

    if(GTK_RESPONSE_OK != gtk_dialog_run(GTK_DIALOG(confirm)))
    {
        gtk_widget_destroy(confirm);
        return FALSE;
    }

    /* Now, do the actual download. */
    for(z = 0; z <= MAX_ZOOM; ++z)
    {
        if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(mapman_info->chk_zoom_levels[z])))
        {
            prev_tilex = 0;
            prev_tiley = 0;
            for(curr = _route.head - 1; curr++ != _route.tail; )
            {
                if(curr->unit.y)
                {
                    gint tilex = unit2ztile(curr->unit.x, z);
                    gint tiley = unit2ztile(curr->unit.y, z);
                    if(tilex != prev_tilex || tiley != prev_tiley)
                    {
                        gint minx, miny, maxx, maxy, x, y;
                        if(prev_tiley != 0)
                        {
                            minx = MIN(tilex, prev_tilex) - radius;
                            miny = MIN(tiley, prev_tiley) - radius;
                            maxx = MAX(tilex, prev_tilex) + radius;
                            maxy = MAX(tiley, prev_tiley) + radius;
                        }
                        else
                        {
                            minx = tilex - radius;
                            miny = tiley - radius;
                            maxx = tilex + radius;
                            maxy = tiley + radius;
                        }
                        for(x = minx; x <= maxx; x++)
                        {
                            for(y = miny; y <= maxy; y++)
                            {
                                /* Make sure this tile is even possible. */
                                if((unsigned)tilex
                                        < unit2ztile(WORLD_SIZE_UNITS, z)
                                  && (unsigned)tiley
                                        < unit2ztile(WORLD_SIZE_UNITS, z))
                                {
                                    mapdb_initiate_update(rd->primary, z, x, y,
                                        update_type, download_batch_id,
                                        (abs(tilex - unit2tile(
                                                 _next_center.x))
                                         + abs(tiley - unit2tile(
                                                 _next_center.y))),
                                        NULL);
                                }
                            }
                        }
                        prev_tilex = tilex;
                        prev_tiley = tiley;
                    }
                }
            }
        }
    }
    _route_dl_radius = radius;
    gtk_widget_destroy(confirm);
    return TRUE;
}

static void
mapman_clear(GtkWidget *widget, MapmanInfo *mapman_info)
{
    gint z;
    if(gtk_notebook_get_current_page(GTK_NOTEBOOK(mapman_info->notebook)))
        /* This is the second page (the "Zoom" page) - clear the checks. */
        for(z = 0; z <= MAX_ZOOM; ++z)
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(mapman_info->chk_zoom_levels[z]), FALSE);
    else
    {
        /* This is the first page (the "Area" page) - clear the text fields. */
        gtk_entry_set_text(GTK_ENTRY(mapman_info->txt_topleft_lat), "");
        gtk_entry_set_text(GTK_ENTRY(mapman_info->txt_topleft_lon), "");
        gtk_entry_set_text(GTK_ENTRY(mapman_info->txt_botright_lat), "");
        gtk_entry_set_text(GTK_ENTRY(mapman_info->txt_botright_lon), "");
    }
}

void mapman_update_state(GtkWidget *widget, MapmanInfo *mapman_info)
{
    gtk_widget_set_sensitive( mapman_info->chk_overwrite,
            gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(mapman_info->rad_download)));

    if(gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(mapman_info->rad_by_area)))
        gtk_widget_show(mapman_info->tbl_area);
    else if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(mapman_info->notebook)) == 3)
        gtk_widget_hide(mapman_info->tbl_area);

    gtk_widget_set_sensitive(mapman_info->num_route_radius,
            gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(mapman_info->rad_by_route)));
}

gboolean
mapman_dialog()
{
    static GtkWidget *dialog = NULL;
    static GtkWidget *vbox = NULL;
    static GtkWidget *hbox = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *button = NULL;
    static GtkWidget *lbl_gps_lat = NULL;
    static GtkWidget *lbl_gps_lon = NULL;
    static GtkWidget *lbl_center_lat = NULL;
    static GtkWidget *lbl_center_lon = NULL;
    static MapmanInfo mapman_info;
    static gint last_deg_format = 0;
    MapController *controller = map_controller_get_instance();
    GtkAllocation *allocation;
    MapPoint center;
    gint half_screen;
    
    gchar buffer[80];
    gdouble lat, lon;
    gint z;
    gint prev_degformat = _degformat;
    gint fallback_deg_format = _degformat;
    gdouble top_left_lat, top_left_lon, bottom_right_lat, bottom_right_lon;

    map_controller_get_center(controller, &center);
    allocation =
        &(GTK_WIDGET(map_controller_get_screen(controller))->allocation);
    half_screen = MAX(allocation->width, allocation->height) / 2;

    // - If the coord system has changed then we need to update certain values
    /* Initialize to the bounds of the screen. */
    unit2latlon(center.x - pixel2unit(half_screen),
                center.y - pixel2unit(half_screen),
                top_left_lat, top_left_lon);
    BOUND(top_left_lat, -90.f, 90.f);
    BOUND(top_left_lon, -180.f, 180.f);

        
    unit2latlon(center.x + pixel2unit(half_screen),
                center.y + pixel2unit(half_screen),
                bottom_right_lat, bottom_right_lon);
    BOUND(bottom_right_lat, -90.f, 90.f);
    BOUND(bottom_right_lon, -180.f, 180.f);

    if(!coord_system_check_lat_lon (top_left_lat, top_left_lon, &fallback_deg_format))
    {
    	_degformat = fallback_deg_format;
    }
    else
    {
    	// top left is valid, also check bottom right
        if(!coord_system_check_lat_lon (bottom_right_lat, bottom_right_lon, &fallback_deg_format))
        {
        	_degformat = fallback_deg_format;
        }
    }
    
    
    if(_degformat != last_deg_format)
    {
    	last_deg_format = _degformat;
    	
		if(dialog != NULL) gtk_widget_destroy(dialog);
    	dialog = NULL;
    }

    if(dialog == NULL)
    {
        mapman_info.dialog = dialog = gtk_dialog_new_with_buttons(
                _("Manage Maps"),
                GTK_WINDOW(_window), GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                NULL);

        /* Enable the help button. */
#ifndef LEGACY
#else
        ossohelp_dialog_help_enable(
                GTK_DIALOG(mapman_info.dialog), HELP_ID_MAPMAN, _osso);
#endif

        /* Clear button. */
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
                button = gtk_button_new_with_label(_("Clear")));
        g_signal_connect(G_OBJECT(button), "clicked",
                          G_CALLBACK(mapman_clear), &mapman_info);

        /* Cancel button. */
        gtk_dialog_add_button(GTK_DIALOG(dialog),
                GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                mapman_info.notebook = gtk_notebook_new(), TRUE, TRUE, 0);

        /* Setup page. */
        gtk_notebook_append_page(GTK_NOTEBOOK(mapman_info.notebook),
                vbox = gtk_vbox_new(FALSE, 2),
                label = gtk_label_new(_("Setup")));
        gtk_notebook_set_tab_label_packing(
                GTK_NOTEBOOK(mapman_info.notebook), vbox,
                FALSE, FALSE, GTK_PACK_START);

        gtk_box_pack_start(GTK_BOX(vbox),
                hbox = gtk_hbox_new(FALSE, 4),
                FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                mapman_info.rad_download = gtk_radio_button_new_with_label(
                    NULL,_("Download Maps")),
                FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                label = gtk_alignment_new(0.f, 0.5f, 0.f, 0.f),
                FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(label),
                mapman_info.chk_overwrite
                        = gtk_check_button_new_with_label(_("Overwrite"))),

        gtk_box_pack_start(GTK_BOX(vbox),
                mapman_info.rad_delete
                        = gtk_radio_button_new_with_label_from_widget(
                            GTK_RADIO_BUTTON(mapman_info.rad_download),
                            _("Delete Maps")),
                FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(vbox),
                gtk_hseparator_new(),
                FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(vbox),
                mapman_info.rad_by_area
                        = gtk_radio_button_new_with_label(NULL,
                            _("By Area (see tab)")),
                FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox),
                hbox = gtk_hbox_new(FALSE, 4),
                FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                mapman_info.rad_by_route
                        = gtk_radio_button_new_with_label_from_widget(
                            GTK_RADIO_BUTTON(mapman_info.rad_by_area),
                            _("Along Route - Radius (tiles):")),
                FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                mapman_info.num_route_radius = hildon_number_editor_new(0,100),
                FALSE, FALSE, 0);
        hildon_number_editor_set_value(
                HILDON_NUMBER_EDITOR(mapman_info.num_route_radius),
                _route_dl_radius);


        /* Zoom page. */
        gtk_notebook_append_page(GTK_NOTEBOOK(mapman_info.notebook),
                table = gtk_table_new(5, 5, FALSE),
                label = gtk_label_new(_("Zoom")));
        gtk_notebook_set_tab_label_packing(
                GTK_NOTEBOOK(mapman_info.notebook), table,
                FALSE, FALSE, GTK_PACK_START);
        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(
                    _("Zoom Levels to Download: (0 = most detail)")),
                0, 4, 0, 1, GTK_FILL, 0, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 0.f, 0.5f);
        snprintf(buffer, sizeof(buffer), "%d", 0);
        gtk_table_attach(GTK_TABLE(table),
                mapman_info.chk_zoom_levels[0]
                        = gtk_check_button_new_with_label(buffer),
                4, 5 , 0, 1, GTK_FILL, 0, 0, 0);
        for(z = 0; z < MAX_ZOOM; ++z)
        {
            snprintf(buffer, sizeof(buffer), "%d", z + 1);
            gtk_table_attach(GTK_TABLE(table),
                    mapman_info.chk_zoom_levels[z + 1]
                            = gtk_check_button_new_with_label(buffer),
                    z / 4, z / 4 + 1, z % 4 + 1, z % 4 + 2,
                    GTK_FILL, 0, 0, 0);
        }

        /* Area page. */
        gtk_notebook_append_page(GTK_NOTEBOOK(mapman_info.notebook),
            mapman_info.tbl_area = gtk_table_new(5, 3, FALSE),
            label = gtk_label_new(_("Area")));

        /* Label Columns. */
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
        		label = gtk_label_new(DEG_FORMAT_ENUM_TEXT[_degformat].long_field_1),
                1, 2, 0, 1, GTK_FILL, 0, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 0.5f, 0.5f);
        
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        {
	        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
	        		label = gtk_label_new(DEG_FORMAT_ENUM_TEXT[_degformat].long_field_2),
	                2, 3, 0, 1, GTK_FILL, 0, 4, 0);
	        gtk_misc_set_alignment(GTK_MISC(label), 0.5f, 0.5f);
        }
        
        /* GPS. */
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                label = gtk_label_new(_("GPS Location")),
                0, 1, 1, 2, GTK_FILL, 0, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                lbl_gps_lat = gtk_label_new(""),
                1, 2, 1, 2, GTK_FILL, 0, 4, 0);
        gtk_label_set_selectable(GTK_LABEL(lbl_gps_lat), TRUE);
        gtk_misc_set_alignment(GTK_MISC(lbl_gps_lat), 1.f, 0.5f);
        
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        {
	        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
	                lbl_gps_lon = gtk_label_new(""),
	                2, 3, 1, 2, GTK_FILL, 0, 4, 0);
	        gtk_label_set_selectable(GTK_LABEL(lbl_gps_lon), TRUE);
	        gtk_misc_set_alignment(GTK_MISC(lbl_gps_lon), 1.f, 0.5f);
        }
        
        /* Center. */
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                label = gtk_label_new(_("View Center")),
                0, 1, 2, 3, GTK_FILL, 0, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                lbl_center_lat = gtk_label_new(""),
                1, 2, 2, 3, GTK_FILL, 0, 4, 0);
        gtk_label_set_selectable(GTK_LABEL(lbl_center_lat), TRUE);
        gtk_misc_set_alignment(GTK_MISC(lbl_center_lat), 1.f, 0.5f);
        
    
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        {
	        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
	                lbl_center_lon = gtk_label_new(""),
	                2, 3, 2, 3, GTK_FILL, 0, 4, 0);
	        gtk_label_set_selectable(GTK_LABEL(lbl_center_lon), TRUE);
	        gtk_misc_set_alignment(GTK_MISC(lbl_center_lon), 1.f, 0.5f);
        }

        /* default values for Top Left and Bottom Right are defined by the
         * rectangle of the current and the previous Center */

        /* Top Left. */
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                label = gtk_label_new(_("Top-Left")),
                0, 1, 3, 4, GTK_FILL, 0, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                mapman_info.txt_topleft_lat = gtk_entry_new(),
                1, 2, 3, 4, GTK_FILL, 0, 4, 0);
        gtk_entry_set_width_chars(GTK_ENTRY(mapman_info.txt_topleft_lat), 12);
        gtk_entry_set_alignment(GTK_ENTRY(mapman_info.txt_topleft_lat), 1.f);
#ifdef MAEMO_CHANGES
        g_object_set(G_OBJECT(mapman_info.txt_topleft_lat),
#ifndef LEGACY
                "hildon-input-mode",
                HILDON_GTK_INPUT_MODE_FULL, NULL);
#else
                HILDON_INPUT_MODE_HINT,
                HILDON_INPUT_MODE_HINT_ALPHANUMERICSPECIAL, NULL);
        g_object_set(G_OBJECT(mapman_info.txt_topleft_lat),
                HILDON_AUTOCAP,
                FALSE, NULL);
#endif
#endif
        
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        {
	        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
	                mapman_info.txt_topleft_lon = gtk_entry_new(),
	                2, 3, 3, 4, GTK_FILL, 0, 4, 0);
	        gtk_entry_set_width_chars(GTK_ENTRY(mapman_info.txt_topleft_lon), 12);
	        gtk_entry_set_alignment(GTK_ENTRY(mapman_info.txt_topleft_lon), 1.f);
#ifdef MAEMO_CHANGES
		    g_object_set(G_OBJECT(mapman_info.txt_topleft_lon),
#ifndef LEGACY
	                "hildon-input-mode",
	                HILDON_GTK_INPUT_MODE_FULL, NULL);
#else
	                HILDON_INPUT_MODE_HINT,
	                HILDON_INPUT_MODE_HINT_ALPHANUMERICSPECIAL, NULL);
	        g_object_set(G_OBJECT(mapman_info.txt_topleft_lon),
	                HILDON_AUTOCAP,
	                FALSE, NULL);
#endif
#endif

        }
        
        /* Bottom Right. */
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                label = gtk_label_new(_("Bottom-Right")),
                0, 1, 4, 5, GTK_FILL, 0, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
                mapman_info.txt_botright_lat = gtk_entry_new(),
                1, 2, 4, 5, GTK_FILL, 0, 4, 0);
        gtk_entry_set_width_chars(GTK_ENTRY(mapman_info.txt_botright_lat), 12);
        gtk_entry_set_alignment(GTK_ENTRY(mapman_info.txt_botright_lat), 1.f);
#ifdef MAEMO_CHANGES
        g_object_set(G_OBJECT(mapman_info.txt_botright_lat),
#ifndef LEGACY
                "hildon-input-mode",
                HILDON_GTK_INPUT_MODE_FULL, NULL);
#else
                HILDON_INPUT_MODE_HINT,
                HILDON_INPUT_MODE_HINT_ALPHANUMERICSPECIAL, NULL);
        g_object_set(G_OBJECT(mapman_info.txt_botright_lat),
                HILDON_AUTOCAP,
                FALSE, NULL);
#endif
#endif
        
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        {
	        gtk_table_attach(GTK_TABLE(mapman_info.tbl_area),
	                mapman_info.txt_botright_lon = gtk_entry_new(),
	                2, 3, 4, 5, GTK_FILL, 0, 4, 0);
	        gtk_entry_set_width_chars(GTK_ENTRY(mapman_info.txt_botright_lon), 12);
	        gtk_entry_set_alignment(GTK_ENTRY(mapman_info.txt_botright_lon), 1.f);
#ifdef MAEMO_CHANGES
	        g_object_set(G_OBJECT(mapman_info.txt_botright_lon),
#ifndef LEGACY
	                "hildon-input-mode",
	                HILDON_GTK_INPUT_MODE_FULL, NULL);
#else
	            	HILDON_INPUT_MODE_HINT,
	                HILDON_INPUT_MODE_HINT_ALPHANUMERICSPECIAL, NULL);
	        g_object_set(G_OBJECT(mapman_info.txt_botright_lon),
	                HILDON_AUTOCAP,
	                FALSE, NULL);
#endif
#endif

        }
        
        /* Default action is to download by area. */
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(mapman_info.rad_by_area), TRUE);

        g_signal_connect(G_OBJECT(mapman_info.rad_download), "clicked",
                          G_CALLBACK(mapman_update_state), &mapman_info);
        g_signal_connect(G_OBJECT(mapman_info.rad_delete), "clicked",
                          G_CALLBACK(mapman_update_state), &mapman_info);
        g_signal_connect(G_OBJECT(mapman_info.rad_by_area), "clicked",
                          G_CALLBACK(mapman_update_state), &mapman_info);
        g_signal_connect(G_OBJECT(mapman_info.rad_by_route), "clicked",
                          G_CALLBACK(mapman_update_state), &mapman_info);
    }

    /* Initialize fields.  Do no use g_ascii_formatd; these strings will be
     * output (and parsed) as locale-dependent. */

    gtk_widget_set_sensitive(mapman_info.rad_by_route,
            _route.head != _route.tail);

    
    gchar buffer1[15];
    gchar buffer2[15];
    format_lat_lon(_gps.lat, _gps.lon, buffer1, buffer2);
    //lat_format(_gps.lat, buffer);
    gtk_label_set_text(GTK_LABEL(lbl_gps_lat), buffer1);
    //lon_format(_gps.lon, buffer);
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	gtk_label_set_text(GTK_LABEL(lbl_gps_lon), buffer2);
    
    unit2latlon(center.x, center.y, lat, lon);
    
    format_lat_lon(lat, lon, buffer1, buffer2);
    //lat_format(lat, buffer);
    gtk_label_set_text(GTK_LABEL(lbl_center_lat), buffer1);
    //lon_format(lon, buffer);
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	gtk_label_set_text(GTK_LABEL(lbl_center_lon), buffer2);

    format_lat_lon(top_left_lat, top_left_lon, buffer1, buffer2);
    
    gtk_entry_set_text(GTK_ENTRY(mapman_info.txt_topleft_lat), buffer1);
    
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	gtk_entry_set_text(GTK_ENTRY(mapman_info.txt_topleft_lon), buffer2);
    
    format_lat_lon(bottom_right_lat, bottom_right_lon, buffer1, buffer2);
    //lat_format(lat, buffer);
    gtk_entry_set_text(GTK_ENTRY(mapman_info.txt_botright_lat), buffer1);
    //lon_format(lon, buffer);
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	gtk_entry_set_text(GTK_ENTRY(mapman_info.txt_botright_lon), buffer2);

    /* Initialize zoom levels. */
    {
        gint i;
        for(i = 0; i <= MAX_ZOOM; i++)
        {
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(mapman_info.chk_zoom_levels[i]), FALSE);
        }
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mapman_info.chk_zoom_levels[_zoom]), TRUE);

    gtk_widget_show_all(dialog);

    mapman_update_state(NULL, &mapman_info);
    gtk_widget_set_sensitive(mapman_info.rad_download, TRUE);

    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        MapUpdateType update_type;
        static gint8 download_batch_id = INT8_MIN;

        if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(mapman_info.rad_delete)))
            update_type = MAP_UPDATE_DELETE;
        else if(gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(mapman_info.chk_overwrite)))
            update_type = MAP_UPDATE_OVERWRITE;
        else
            update_type = MAP_UPDATE_ADD;

        ++download_batch_id;
        if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(mapman_info.rad_by_route)))
        {
            if(mapman_by_route(&mapman_info, update_type, download_batch_id))
                break;
        }
        else
        {
            const gchar *text_lon, *text_lat;
            //gchar *error_check;
            gdouble start_lat, start_lon, end_lat, end_lon;

            text_lat = gtk_entry_get_text(GTK_ENTRY(mapman_info.txt_topleft_lat));
            text_lon = gtk_entry_get_text(GTK_ENTRY(mapman_info.txt_topleft_lon));
            
            if(!parse_coords(text_lat, text_lon, &start_lat, &start_lon))
            {
            	popup_error(dialog, _("Invalid Top-Left coordinate specified"));
            	continue;
            }
            
            text_lat = gtk_entry_get_text(GTK_ENTRY(mapman_info.txt_botright_lat));
            text_lon = gtk_entry_get_text(GTK_ENTRY(mapman_info.txt_botright_lon));

            if(!parse_coords(text_lat, text_lon, &end_lat, &end_lon))
            {
            	popup_error(dialog, _("Invalid Bottom-Right coordinate specified"));
            	continue;
            }

  

            if(mapman_by_area(start_lat, start_lon, end_lat, end_lon,
                        &mapman_info, update_type, download_batch_id))
                break;
        }
    }

    gtk_widget_hide(dialog);
    
    _degformat = prev_degformat;

    return TRUE;
}


void maps_toggle_visible_layers ()
{
    map_screen_toggle_layers_visibility(map_controller_get_screen(map_controller_get_instance()));
}
