/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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
#ifndef CONTROLLER_PRIV_H
#define CONTROLLER_PRIV_H

struct _MapControllerPrivate
{
    GList *repositories_list;
    GList *tile_sources_list;
    Repository *repository;
    MapScreen *screen;
    MapPoint center;
    gint rotation_angle;
    gint zoom;

    GSList *plugins;

    MapRouter *default_router;

    const WayPoint *next_waypoint;

    guint source_map_center;
    guint is_disposed : 1;
    guint device_active : 1;
};

#endif
