/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * POI and GPS-Info code originally written by Cezary Jackiewicz.
 *
 * Default map data provided by http://www.openstreetmap.org/
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
#    include "config.h"
#endif

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include <hildon/hildon-note.h>

#include "types.h"
#include "data.h"
#include "defines.h"
#include "display.h"
#include "path.h"
#include "poi.h"
#include "route.h"
#include "util.h"
#include "screen.h"

#include <mappero/debug.h>

/* uncertainty below which a GPS position is considered to be precise enough 
 * to be added to the track (in metres) */
#define MAX_UNCERTAINTY 200

/* minimum distance the GPS position must have from the previous track point in
 * order to be registered in the track (in metres) */
#define MIN_TRACK_DISTANCE 10

typedef union {
    gint field;
    struct {
        gchar zoom;
        gint16 altitude;
    } s;
} FieldMix;

static sqlite3 *_path_db = NULL;
static sqlite3_stmt *_track_stmt_select = NULL;
static sqlite3_stmt *_track_stmt_delete_path = NULL;
static sqlite3_stmt *_track_stmt_delete_way = NULL;
static sqlite3_stmt *_track_stmt_insert_path = NULL;
static sqlite3_stmt *_track_stmt_insert_way = NULL;
static sqlite3_stmt *_route_stmt_select = NULL;
static sqlite3_stmt *_route_stmt_delete_path = NULL;
static sqlite3_stmt *_route_stmt_delete_way = NULL;
static sqlite3_stmt *_route_stmt_insert_path = NULL;
static sqlite3_stmt *_route_stmt_insert_way = NULL;
static sqlite3_stmt *_path_stmt_trans_begin = NULL;
static sqlite3_stmt *_path_stmt_trans_commit = NULL;
static sqlite3_stmt *_path_stmt_trans_rollback = NULL;

static void
read_path_from_db(MapPath *path, sqlite3_stmt *select_stmt)
{
    map_path_init(path);
    while(SQLITE_ROW == sqlite3_step(select_stmt))
    {
        const gchar *desc;
        MapDirection dir;
        FieldMix mix;
        MapPathPoint pt;

        pt.unit.x = sqlite3_column_int(select_stmt, 0);
        pt.unit.y = sqlite3_column_int(select_stmt, 1);
        pt.time = sqlite3_column_int(select_stmt, 2);
        mix.field = sqlite3_column_int(select_stmt, 3);
        pt.zoom = mix.s.zoom;
        pt.altitude = mix.s.altitude;
        pt.distance = 0;

        desc = (const gchar *)sqlite3_column_text(select_stmt, 4);
        dir = sqlite3_column_int(select_stmt, 5);
        if (pt.unit.y != 0)
            map_path_append_point_with_desc(path, &pt, desc, dir);
        else
            map_path_append_break(path);
    }
    sqlite3_reset(select_stmt);

    map_path_append_break(path);

    map_path_append_point_end(path);

    path->first_unsaved = map_path_len(path);
}

static gboolean
write_point_to_db(const MapPathPoint *p, sqlite3_stmt *insert_path_stmt)
{
    FieldMix mix;
    gboolean success = TRUE;

    mix.field = 0;
    mix.s.altitude = p->altitude;
    mix.s.zoom = p->zoom;

    /* Insert the path point. */
    if (SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 1, p->unit.x)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 2, p->unit.y)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 3, p->time)
        || SQLITE_OK != sqlite3_bind_int(insert_path_stmt, 4, mix.field)
        || SQLITE_DONE != sqlite3_step(insert_path_stmt))
    {
        gchar buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "%s\n%s",
                _("Failed to write to path database. "
                    "Tracks and routes may not be saved."),
                sqlite3_errmsg(_path_db));
        popup_error(_window, buffer);
        success = FALSE;
    }
    sqlite3_reset(insert_path_stmt);
    return success;
}

/* Returns the new next_update_index. */
static gint
write_path_to_db(MapPath *path,
        sqlite3_stmt *delete_path_stmt,
        sqlite3_stmt *delete_way_stmt,
        sqlite3_stmt *insert_path_stmt,
        sqlite3_stmt *insert_way_stmt)
{
    MapPathPoint *curr, *first;
    MapPathWayPoint *wcurr;
    gint saved = 0;
    gboolean success = TRUE;
    MapLineIter line;
    DEBUG("%d", path->first_unsaved);

    /* Start transaction. */
    sqlite3_step(_path_stmt_trans_begin);
    sqlite3_reset(_path_stmt_trans_begin);

    if (path->first_unsaved == 0)
    {
        /* Replace the whole thing, so delete the table first. */
        if(SQLITE_DONE != sqlite3_step(delete_way_stmt)
                || SQLITE_DONE != sqlite3_step(delete_path_stmt))
        {
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%s\n%s",
                    _("Failed to write to path database. "
                        "Tracks and routes may not be saved."),
                    sqlite3_errmsg(_path_db));
            popup_error(_window, buffer);
            success = FALSE;
        }
        sqlite3_reset(delete_way_stmt);
        sqlite3_reset(delete_path_stmt);
    }

    first = map_path_first(path) + path->first_unsaved;
    wcurr = path->whead;
    map_path_line_iter_from_point(path, first, &line);
    do
    {
        MapPathPoint *start, *end;
        start = map_path_line_first(&line);
        end = start + map_path_line_len(&line);
        if (start >= first)
        {
            /* new line: write a break point */
            success = write_point_to_db(&_point_null, insert_path_stmt);
        }
        else
            start = first;

        for (curr = start; success && curr < end; curr++)
        {
            success = write_point_to_db(curr, insert_path_stmt);

            /* Now, check if curr is a waypoint. */
            if(success && wcurr <= path->wtail && wcurr->point == curr)
            {
                gint num = sqlite3_last_insert_rowid(_path_db);
                if(SQLITE_OK != sqlite3_bind_int(insert_way_stmt, 1, num)
                || SQLITE_OK != sqlite3_bind_text(insert_way_stmt, 2, wcurr->desc,
                    -1, SQLITE_STATIC)
                || SQLITE_OK != sqlite3_bind_int(insert_way_stmt, 3, wcurr->dir)
                || SQLITE_DONE != sqlite3_step(insert_way_stmt))
                {
                    gchar buffer[BUFFER_SIZE];
                    snprintf(buffer, sizeof(buffer), "%s\n%s",
                            _("Failed to write to path database. "
                                "Tracks and routes may not be saved."),
                            sqlite3_errmsg(_path_db));
                    popup_error(_window, buffer);
                    success = FALSE;
                }
                sqlite3_reset(insert_way_stmt);
                wcurr++;
            }

            if (success) saved++;
        }
    }
    while (map_path_line_iter_next(&line));

    if(success)
    {
        sqlite3_step(_path_stmt_trans_commit);
        sqlite3_reset(_path_stmt_trans_commit);
        path->first_unsaved += saved;
        g_assert(path->first_unsaved <= map_path_len(path));
        return saved;
    }
    else
    {
        sqlite3_step(_path_stmt_trans_rollback);
        sqlite3_reset(_path_stmt_trans_rollback);
        return 0;
    }
}

void
map_path_save_route(MapPath *path)
{
    if(_path_db)
    {
        write_path_to_db(path,
                          _route_stmt_delete_path,
                          _route_stmt_delete_way,
                          _route_stmt_insert_path,
                          _route_stmt_insert_way);
    }
}

static void
path_save_track_to_db()
{
    if(_path_db)
    {
        write_path_to_db(&_track,
                          _track_stmt_delete_path,
                          _track_stmt_delete_way,
                          _track_stmt_insert_path,
                          _track_stmt_insert_way);
    }
}

/**
 * Returns TRUE if the point needs to be added to the path.
 */
static gboolean
point_is_significant(const MapGpsData *gps, const MapPath *path)
{
    gint xdiff, ydiff, min_distance_units;
    MapLineIter line;
    MapPathPoint *prev;

    /* check if the track is empty */
    if (map_path_len(path) == 0)
        return TRUE;

    /* check if the track was in a break */
    map_path_line_iter_last(path, &line);
    if (map_path_line_len(&line) == 0)
        return TRUE;

    /* check how much time has passed since last update */
    prev = map_path_last(path);
    if (gps->time - prev->time > 60)
        return TRUE;

    /* Have we moved enough? */
    xdiff = abs(gps->unit.x - prev->unit.x);
    ydiff = abs(gps->unit.y - prev->unit.y);

    /* check if the distances are obviously big enough */
    if (xdiff >= METRES_TO_UNITS(300) ||
        ydiff >= METRES_TO_UNITS(300))
        return TRUE;

    /* Euristics to compute the minimum distance: we use MIN_TRACK_DISTANCE +
     * a fraction of the error. */
    min_distance_units = METRES_TO_UNITS(MIN_TRACK_DISTANCE + gps->hdop / 3);
    if (SQUARE(xdiff) + SQUARE(ydiff) >= SQUARE(min_distance_units))
        return TRUE;

    return FALSE;
}

/**
 * Add a point to the _track list.
 */
void
map_path_track_update(const MapGpsData *gps)
{
    MapController *controller = map_controller_get_instance();
    MapPathPoint pos = _point_null;
    gboolean must_add = FALSE;

    if (gps->fields & MAP_GPS_LATLON)
    {
        /* Add the point to the track only if it's not too inaccurate: if the
         * uncertainty is greater than 200m, don't add it (TODO: this should be
         * a configuration option). */
        if (gps->hdop < MAX_UNCERTAINTY && point_is_significant(gps, &_track))
        {
            must_add = TRUE;

            pos.unit = gps->unit;
            if (gps->fields & MAP_GPS_ALTITUDE)
                pos.altitude = gps->altitude;

            /* Draw the line immediately */
            if (map_controller_is_display_on(controller))
            {
                MapScreen *screen = map_controller_get_screen(controller);

                map_screen_track_append(screen, &pos);
                map_screen_refresh_panel(screen);
            }

            if (_track.last_lat != 0 && _track.last_lon != 0)
            {
                pos.distance =
                    calculate_distance(_track.last_lat, _track.last_lon,
                                       gps->lat, gps->lon);
            }
            pos.time = gps->time;
            map_path_append_point_fast(&_track, &pos);
            _track.last_lat = gps->lat;
            _track.last_lon = gps->lon;
            _track.length += pos.distance;
            _track.points_with_distance++;
            map_path_optimize(&_track);
        }
    }
    else
    {
        /* insert a break */
        if (map_path_len(&_track) > 0)
            must_add = map_path_append_break(&_track);
    }

    /* Maybe update the track database. */
    {
        static time_t last_track_db_update = 0;
        if (!gps->time || (gps->time - last_track_db_update > 60
                           && map_path_len(&_track) > _track.first_unsaved))
        {
            path_save_track_to_db();
            last_track_db_update = gps->time;
        }
    }

    UNBLANK_SCREEN(must_add, FALSE);
}

void
track_clear()
{
    GtkWidget *confirm;

    confirm = hildon_note_new_confirmation(GTK_WINDOW(_window),
                            _("Really clear the track?"));

    if(GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(confirm))) {
        MapController *controller = map_controller_get_instance();
        map_path_unset(&_track);
        map_path_init(&_track);
        path_save_track_to_db();
        map_controller_refresh_paths(controller);
    }

    gtk_widget_destroy(confirm);
}

void
track_insert_break(gboolean temporary)
{
    if (!map_path_append_break(&_track))
        return;

    /* To mark a "waypoint" in a track, we'll add a break point and then
     * another instance of the most recent track point. */
    if (temporary)
    {
        map_path_append_point(&_track, map_path_last(&_track));
    }

    /* Update the track database. */
    path_save_track_to_db();
}

static gboolean
update_db_to_v1(sqlite3 *db)
{
    gint ret;

    DEBUG("called");
    ret = sqlite3_exec(db,
                       "ALTER TABLE route_way ADD COLUMN direction INTEGER "
                       "DEFAULT 0"
                       ";"
                       "ALTER TABLE track_way ADD COLUMN direction INTEGER "
                       "DEFAULT 0"
                       ";"
                       "PRAGMA user_version = 1",
                       NULL, NULL, NULL);
    DEBUG("last error: %s", sqlite3_errmsg(db));
    return ret == SQLITE_OK;
}

static sqlite3 *
open_db(const gchar *filename)
{
    sqlite3 *db;
    gboolean exists;
    gint ret;

    if (!filename) return NULL;

    exists = g_file_test(filename, G_FILE_TEST_EXISTS);
    if (SQLITE_OK != sqlite3_open(filename, &db))
        return NULL;

    if (!exists)
    {
        sqlite3_exec(db,
                     "PRAGMA user_version = 1"
                     ";"
                     "CREATE TABLE route_path ("
                     "num INTEGER PRIMARY KEY, "
                     "unitx INTEGER, "
                     "unity INTEGER, "
                     "time INTEGER, "
                     "altitude INTEGER)"
                     ";"
                     "CREATE TABLE route_way ("
                     "route_point PRIMARY KEY, "
                     "direction INTEGER DEFAULT 0, "
                     "description TEXT)"
                     ";"
                     "CREATE TABLE track_path ("
                     "num INTEGER PRIMARY KEY, "
                     "unitx INTEGER, "
                     "unity INTEGER, "
                     "time INTEGER, "
                     "altitude INTEGER)"
                     ";"
                     "CREATE TABLE track_way ("
                     "track_point PRIMARY KEY, "
                     "direction INTEGER DEFAULT 0, "
                     "description TEXT)",
                     NULL, NULL, NULL);
    }
    else
    {
        sqlite3_stmt *stmt;
        gint version;

        ret = sqlite3_prepare(db, "PRAGMA user_version", -1, &stmt, NULL);
        if (G_UNLIKELY(ret != SQLITE_OK)) return NULL;

        ret = sqlite3_step(stmt);
        if (G_UNLIKELY(ret != SQLITE_ROW)) return NULL;

        version = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        if (G_UNLIKELY(version < 1))
        {
            if (!update_db_to_v1(db)) return NULL;
        }
    }

    return db;
}

void
path_init_late()
{
    MapController *controller = map_controller_get_instance();
    gchar *settings_dir;

    /* Initialize settings_dir. */
    settings_dir = gnome_vfs_expand_initial_tilde(CONFIG_DIR_NAME);
    g_mkdir_with_parents(settings_dir, 0700);

    /* Open path database. */
    {   
        gchar *path_db_file;

        path_db_file = gnome_vfs_uri_make_full_from_relative(
                settings_dir, CONFIG_PATH_DB_FILE);

        _path_db = open_db(path_db_file);
        g_free(path_db_file);

        if (!_path_db
            /* Create prepared statements - failure here is bad! */
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "select unitx, unity, time, altitude, description, "
                    "direction "
                    "from route_path left join route_way on "
                    "route_path.num = route_way.route_point "
                    "order by route_path.num",
                    -1, &_route_stmt_select, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "select unitx, unity, time, altitude, description, "
                    "direction "
                    "from track_path left join track_way on "
                    "track_path.num = track_way.track_point "
                    "order by track_path.num",
                    -1, &_track_stmt_select, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from route_path",
                    -1, &_route_stmt_delete_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from route_way",
                    -1, &_route_stmt_delete_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into route_path "
                    "(num, unitx, unity, time, altitude) "
                    "values (NULL, ?, ?, ?, ?)",
                    -1, &_route_stmt_insert_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into route_way "
                    "(route_point, description, direction) "
                    "values (?, ?, ?)",
                    -1, &_route_stmt_insert_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from track_path",
                    -1, &_track_stmt_delete_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "delete from track_way",
                    -1, &_track_stmt_delete_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into track_path "
                    "(num, unitx, unity, time, altitude) "
                    "values (NULL, ?, ?, ?, ?)",
                    -1, &_track_stmt_insert_path, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db,
                    "insert into track_way "
                    "(track_point, description, direction) "
                    "values (?, ?, ?)",
                    -1, &_track_stmt_insert_way, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db, "begin transaction",
                    -1, &_path_stmt_trans_begin, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db, "commit transaction",
                    -1, &_path_stmt_trans_commit, NULL)
            || SQLITE_OK != sqlite3_prepare(_path_db, "rollback transaction",
                    -1, &_path_stmt_trans_rollback, NULL))
        {   
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%s\n%s",
                    _("Failed to open path database. "
                        "Tracks and routes will not be saved."),
                    sqlite3_errmsg(_path_db));
            sqlite3_close(_path_db);
            _path_db = NULL;

            popup_error(_window, buffer);
        }
        else
        {   
            read_path_from_db(map_route_get_path(), _route_stmt_select);
            map_route_path_changed();
            read_path_from_db(&_track, _track_stmt_select);
        }
    }

    g_free(settings_dir);

    map_controller_refresh_paths(controller);
}

void
path_init()
{
    map_path_init(&_track);
    map_path_init(map_route_get_path());
    map_route_path_changed();
}

void
path_destroy()
{
    /* Save paths. */
    map_path_append_break(&_track);
    path_save_track_to_db();
    path_save_route_to_db();

    if(_path_db)
    {   
        sqlite3_close(_path_db);
        _path_db = NULL;
    }

    map_path_unset(&_track);
    map_route_destroy();
}

