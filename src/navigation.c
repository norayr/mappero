/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "navigation.h"

#include "data.h"
#include "debug.h"
#include "path.h"

#include <canberra.h>
#include <hildon/hildon-sound.h>
#include <stdlib.h>
#include <string.h>

#define SOUND_DIR "/usr/share/sounds/mapper/"

/* these need to be aligned with the MapDirection enum values */
static const gchar *dir_names[] = {
    "unknown",
    "straight",
    "turn-right",
    "turn-left",
    "slight-right",
    "slight-left",
    "first-exit",
    "second-exit",
    "third-exit",
    NULL,
};

/**
 * map_navigation_infer_direction:
 * @text: textual description of a waypoint.
 *
 * Tries to guess a #MapDirection from a text string. Ideally this function
 * should not exists, if all routers provided properly formatted directions.
 * This is just a hack to recognize google router descriptions, and should
 * probably belong there; it's here so that we can recognize directions in
 * previously saved routes.
 */
MapDirection
map_navigation_infer_direction(const gchar *text)
{
    MapDirection dir = MAP_DIRECTION_UNKNOWN;

    if (!text) return dir;

    if (strncmp(text, "Turn ", 5) == 0)
    {
        text += 5;
        if (strncmp(text, "left", 4) == 0)
            dir = MAP_DIRECTION_TL;
        else
            dir = MAP_DIRECTION_TR;
    }
    else if (strncmp(text, "Slight ", 6) == 0)
    {
        text += 6;
        if (strncmp(text, "left", 4) == 0)
            dir = MAP_DIRECTION_STL;
        else
            dir = MAP_DIRECTION_STR;
    }
    else if (strncmp(text, "Continue ", 9) == 0)
        dir = MAP_DIRECTION_CS;
    else
    {
        static const gchar *ordinals[] = { "1st ", "2nd ", "3rd ", NULL };
        gchar *ptr;
        gint i;

        for (i = 0; ordinals[i] != NULL; i++)
        {
            ptr = strstr(text, ordinals[i]);
            if (ptr != NULL) break;
        }

        if (ptr != NULL)
        {
            ptr += strlen(ordinals[i]);
            if (strncmp(text, "exit", 4) == 0)
                dir = MAP_DIRECTION_EX1 + i;
            else if (strncmp(text, "left", 4) == 0)
                dir = MAP_DIRECTION_TL;
            else
                dir = MAP_DIRECTION_TR;
        }
        else
        {
            /* all heuristics failed, add more here */
        }
    }
    return dir;
}

static ca_context *
map_ca_context_get (void)
{
    static GStaticPrivate context_private = G_STATIC_PRIVATE_INIT;
    ca_context *c = NULL;
    const gchar *name = NULL;
    gint ret;

    if ((c = g_static_private_get(&context_private)))
        return c;

    if ((ret = ca_context_create(&c)) != CA_SUCCESS) {
        g_warning("ca_context_create: %s\n", ca_strerror(ret));
        return NULL;
    }
    if ((ret = ca_context_open(c)) != CA_SUCCESS) {
        g_warning("ca_context_open: %s\n", ca_strerror(ret));
        ca_context_destroy(c);
        return NULL;
    }

    if ((name = g_get_application_name()))
        ca_context_change_props(c, CA_PROP_APPLICATION_NAME, name, NULL);

    g_static_private_set(&context_private, c, (GDestroyNotify) ca_context_destroy);

    return c;
}

static void
play_sound(const gchar *sample)
{
    ca_context *ca_con = NULL;
    ca_proplist *pl = NULL;

    ca_con = map_ca_context_get ();

    ca_proplist_create(&pl);
    ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, sample);
    ca_proplist_sets(pl, CA_PROP_MEDIA_ROLE, "x-maemo");

    ca_context_play_full(ca_con, 0, pl, NULL, NULL);

    ca_proplist_destroy(pl);
}

static gboolean
play_sound_idle(gpointer userdata)
{
    gchar *sample = userdata;

    play_sound(sample);
    g_free(sample);
    return FALSE;
}

static void
alert_played(ca_context *c, uint32_t id, int error_code, void *userdata)
{
    DEBUG("alert played");
    g_idle_add(play_sound_idle, userdata);
}

static void
play_sound_with_alert(const gchar *sample)
{
    ca_context *ca_con = NULL;
    ca_proplist *pl = NULL;

    ca_con = map_ca_context_get ();

    ca_proplist_create(&pl);
    /* first, play the alert sound */
    ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME,
                     "/usr/share/sounds/ui-information_note.wav");
    ca_proplist_sets(pl, CA_PROP_MEDIA_ROLE, "x-maemo");

    DEBUG("playing alert");
    ca_context_play_full(ca_con, 0, pl, alert_played, g_strdup(sample));

    ca_proplist_destroy(pl);
}

static void
play_direction(MapDirection dir)
{
    const gchar *direction_name, *lang;
    gchar path[256] = "";
    gint i;

    if (dir < 0 || dir >= MAP_DIRECTION_LAST) return;

    direction_name = dir_names[dir];
    if (!direction_name) return;

    lang = g_getenv("LC_MESSAGES");
    if (!lang) lang = "";

    i = g_snprintf(path, sizeof(path), SOUND_DIR "%s/", lang);
    if (!g_file_test(path, G_FILE_TEST_IS_DIR))
        i = g_snprintf(path, sizeof(path), SOUND_DIR);

    g_snprintf(path + i, sizeof(path) - i, "%s.wav", direction_name);

    DEBUG("Playing %s", path);
    play_sound_with_alert(path);
}

void
map_navigation_announce_voice(const WayPoint *wp)
{
    static const WayPoint *last_wp = NULL;
    static struct timespec last_wp_time;
    struct timespec now;

    /* check if this is the same waypoint we announced last; if so, re-announce
     * it only after a certain time has passed */
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (wp == last_wp && now.tv_sec - last_wp_time.tv_sec < 30)
        return;

    last_wp = wp;
    last_wp_time = now;

#ifdef USE_SYNTH
    if (!fork())
    {
        /* We are the fork child.  Synthesize the voice. */
        hildon_play_system_sound(
            "/usr/share/sounds/ui-information_note.wav");
        sleep(1);
        DEBUG("%s %s", _voice_synth_path, wp->desc);
        execl(_voice_synth_path, basename(_voice_synth_path),
              "-t", wp->desc, (char *)NULL);
        /* No good?  Try to launch it with /bin/sh */
        execl("/bin/sh", "sh", _voice_synth_path,
              "-t", wp->desc, (char *)NULL);
        /* Still no good? Oh well... */
        exit(0);
    }
#else
    play_direction(wp->dir);
#endif
}
