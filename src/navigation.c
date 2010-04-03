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

#include <hildon/hildon-sound.h>
#include <stdlib.h>
#include <string.h>

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

void
map_navigation_announce_voice(WayPoint *wp)
{
    static gchar *_last_spoken_phrase = NULL;

    /* And that we haven't already announced it. */
    if (strcmp(wp->desc, _last_spoken_phrase))
    {
        g_free(_last_spoken_phrase);
        _last_spoken_phrase = g_strdup(wp->desc);
        if(!fork())
        {
            /* We are the fork child.  Synthesize the voice. */
            hildon_play_system_sound(
                "/usr/share/sounds/ui-information_note.wav");
            sleep(1);
            DEBUG("%s %s", _voice_synth_path, _last_spoken_phrase);
            execl(_voice_synth_path, basename(_voice_synth_path),
                  "-t", _last_spoken_phrase, (char *)NULL);
            /* No good?  Try to launch it with /bin/sh */
            execl("/bin/sh", "sh", _voice_synth_path,
                  "-t", _last_spoken_phrase, (char *)NULL);
            /* Still no good? Oh well... */
            exit(0);
        }
    }
}

