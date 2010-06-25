/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
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
#ifndef CONTROLLER_PRIV_H
#define CONTROLLER_PRIV_H

#include <gconf/gconf-client.h>
#include <location/location-gpsd-control.h>
#include <location/location-gps-device.h>

struct _MapControllerPrivate
{
    GList *repositories_list;
    GList *tile_sources_list;
    Repository *repository;
    GConfClient *gconf_client;
    GtkWindow *window;
    MapScreen *screen;
    MapOrientation orientation;
    MapPoint center;
    gint rotation_angle;
    gint zoom;

    GSList *plugins;

    MapRouter *default_router;

    LocationGPSDControl *gpsd_control;
    LocationGPSDevice *gps_device;
    gint gps_effective_interval;
    gint gps_normal_interval; /* interval used when power save is not active */
    gboolean gps_power_save;
    MapGpsData gps_data;

    gboolean auto_download;
    gboolean display_on;

    guint source_map_center;
    guint source_download_precache;
    guint source_init_late;

    guint is_disposed : 1;
    guint device_active : 1;
};

#endif
