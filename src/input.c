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

#include <math.h>
#include <gdk/gdkkeysyms.h>

#ifndef LEGACY
#    include <hildon/hildon-defines.h>
#    include <hildon/hildon-banner.h>
#else
#    include <hildon-widgets/hildon-defines.h>
#    include <hildon-widgets/hildon-banner.h>
#endif

#include "types.h"
#include "data.h"
#include "debug.h"
#include "defines.h"

#include "display.h"
#include "gps.h"
#include "input.h"
#include "maps.h"
#include "path.h"
#include "poi.h"
#include "util.h"

static CustomKey
get_custom_key_from_keyval(gint keyval)
{
    CustomKey custom_key;
    DEBUG("%d", keyval);

    switch(keyval)
    {
        case HILDON_HARDKEY_UP:
            custom_key = CUSTOM_KEY_UP;
            break;
        case HILDON_HARDKEY_DOWN:
            custom_key = CUSTOM_KEY_DOWN;
            break;
        case HILDON_HARDKEY_LEFT:
            custom_key = CUSTOM_KEY_LEFT;
            break;
        case HILDON_HARDKEY_RIGHT:
            custom_key = CUSTOM_KEY_RIGHT;
            break;
        case HILDON_HARDKEY_SELECT:
            custom_key = CUSTOM_KEY_SELECT;
            break;
        case HILDON_HARDKEY_INCREASE:
            custom_key = CUSTOM_KEY_INCREASE;
            break;
        case HILDON_HARDKEY_DECREASE:
            custom_key = CUSTOM_KEY_DECREASE;
            break;
        case HILDON_HARDKEY_FULLSCREEN:
            custom_key = CUSTOM_KEY_FULLSCREEN;
            break;
        case HILDON_HARDKEY_ESC:
            custom_key = CUSTOM_KEY_ESC;
            break;
        default:
            custom_key = -1;
    }

    return custom_key;
}

gboolean
window_cb_key_press(GtkWidget* widget, GdkEventKey *event)
{
    MapController *controller;
    CustomKey custom_key;

    controller = map_controller_get_instance();
    custom_key = get_custom_key_from_keyval(event->keyval);
    if(custom_key == -1)
        return FALSE; /* Not our event. */

    switch(_action[custom_key])
    {
        case CUSTOM_ACTION_PAN_NORTH:
        case CUSTOM_ACTION_PAN_SOUTH:
        case CUSTOM_ACTION_PAN_EAST:
        case CUSTOM_ACTION_PAN_WEST:
        case CUSTOM_ACTION_PAN_UP:
        case CUSTOM_ACTION_PAN_DOWN:
        case CUSTOM_ACTION_PAN_LEFT:
        case CUSTOM_ACTION_PAN_RIGHT:
            g_warning("Panning action is not implemented");
            break;

        case CUSTOM_ACTION_RESET_VIEW_ANGLE:
            map_rotate(-_next_map_rotate_angle);
            break;

        case CUSTOM_ACTION_ROTATE_CLOCKWISE:
        {
            map_rotate(-ROTATE_DEGREES);
            break;
        }

        case CUSTOM_ACTION_ROTATE_COUNTERCLOCKWISE:
        {
            map_rotate(ROTATE_DEGREES);
            break;
        }

        case CUSTOM_ACTION_TOGGLE_AUTOROTATE:
            map_controller_set_auto_rotate(controller,
                !map_controller_get_auto_rotate(controller));
            break;

        case CUSTOM_ACTION_TOGGLE_AUTOCENTER:
            switch(_center_mode)
            {
                case CENTER_LATLON:
                case CENTER_WAS_LEAD:
                    map_controller_set_center_mode(controller, CENTER_LEAD);
                    break;
                case CENTER_LEAD:
                case CENTER_WAS_LATLON:
                    map_controller_set_center_mode(controller, CENTER_LATLON);
                    break;
            }
            break;

        case CUSTOM_ACTION_ZOOM_IN:
            map_controller_action_zoom_in(controller);
            break;

        case CUSTOM_ACTION_ZOOM_OUT:
            map_controller_action_zoom_out(controller);
            break;

        case CUSTOM_ACTION_TOGGLE_FULLSCREEN:
            map_controller_action_switch_fullscreen(controller);
            break;

        case CUSTOM_ACTION_TOGGLE_TRACKING:
            map_controller_set_tracking(controller,
                !map_controller_get_tracking(controller));
            break;

        case CUSTOM_ACTION_TOGGLE_TRACKS:
            switch(_show_paths)
            {
                case 0:
                    /* Nothing shown, nothing saved; just set both. */
                    _show_paths = TRACKS_MASK | ROUTES_MASK;
                    break;
                case TRACKS_MASK << 16:
                case ROUTES_MASK << 16:
                case (ROUTES_MASK | TRACKS_MASK) << 16:
                    /* Something was saved and nothing changed since.
                     * Restore saved. */
                    _show_paths = _show_paths >> 16;
                    break;
                default:
                    /* There is no history, or they changed something
                     * since the last historical change. Save and
                     * clear. */
                    _show_paths = _show_paths << 16;
            }
            map_controller_set_show_routes(controller,
                                           _show_paths & ROUTES_MASK);

            map_controller_set_show_tracks(controller,
                                           _show_paths & TRACKS_MASK);

        case CUSTOM_ACTION_TOGGLE_SCALE:
            map_controller_set_show_scale(controller,
                !map_controller_get_show_scale(controller));
            break;

        case CUSTOM_ACTION_TOGGLE_POI:
            map_controller_set_show_poi(controller,
                !map_controller_get_show_poi(controller));
            break;
        case CUSTOM_ACTION_CHANGE_REPO:
            /* TODO: change to next next-able repository (do we really need this?) */
            break;

        case CUSTOM_ACTION_ROUTE_DISTNEXT:
            route_show_distance_to_next();
            break;

        case CUSTOM_ACTION_ROUTE_DISTLAST:
            route_show_distance_to_last();
            break;

        case CUSTOM_ACTION_TRACK_BREAK:
            track_insert_break(TRUE);
            break;

        case CUSTOM_ACTION_TRACK_CLEAR:
            track_clear();
            break;

        case CUSTOM_ACTION_TRACK_DISTLAST:
            track_show_distance_from_last();
            break;

        case CUSTOM_ACTION_TRACK_DISTFIRST:
            track_show_distance_from_first();
            break;

        case CUSTOM_ACTION_TOGGLE_GPS:
            gtk_check_menu_item_set_active(
                    GTK_CHECK_MENU_ITEM(_menu_gps_enable_item),
                    !_enable_gps);
            break;

        case CUSTOM_ACTION_TOGGLE_GPSINFO:
            gtk_check_menu_item_set_active(
                    GTK_CHECK_MENU_ITEM(_menu_gps_show_info_item),
                    !_gps_info);
            break;

        case CUSTOM_ACTION_TOGGLE_SPEEDLIMIT:
            _speed_limit_on ^= 1;
            break;

        case CUSTOM_ACTION_TOGGLE_LAYERS:
            maps_toggle_visible_layers ();
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

gboolean
window_cb_key_release(GtkWidget* widget, GdkEventKey *event)
{
    MapController *controller = map_controller_get_instance();
    CustomKey custom_key;

    custom_key = get_custom_key_from_keyval(event->keyval);
    if(custom_key == -1)
        return FALSE; /* Not our event. */

    switch(_action[custom_key])
    {
        case CUSTOM_ACTION_ZOOM_IN:
        case CUSTOM_ACTION_ZOOM_OUT:
            map_controller_action_zoom_stop(controller);
            return TRUE;

        default:
            return FALSE;
    }
}

void
input_init()
{
    g_signal_connect(G_OBJECT(_window), "key_press_event",
            G_CALLBACK(window_cb_key_press), NULL);

    g_signal_connect(G_OBJECT(_window), "key_release_event",
            G_CALLBACK(window_cb_key_release), NULL);
}
