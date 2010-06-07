/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of libMappero.
 *
 * libMappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libMappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libMappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAP_VIEWER_H
#define MAP_VIEWER_H

#include <mappero/globals.h>

#include <gconf/gconf-client.h>
#include <glib.h>
#include <glib-object.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_VIEWER         (map_viewer_get_type())
#define MAP_VIEWER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), MAP_TYPE_VIEWER, MapViewer))
#define MAP_IS_VIEWER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), MAP_TYPE_VIEWER))
#define MAP_VIEWER_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), MAP_TYPE_VIEWER, MapViewerIface))

typedef struct _MapViewer MapViewer;
typedef struct _MapViewerIface MapViewerIface;

struct _MapViewerIface
{
    GTypeInterface parent;

    void (*get_transformation)(MapViewer *viewer,
                               MapLatLonToUnit *latlon2unit,
                               MapUnitToLatLon *unit2latlon);

    /* signals */
    void (*transformation_changed)(MapViewer *viewer,
                                   MapLatLonToUnit latlon2unit,
                                   MapUnitToLatLon unit2latlon);
};

GType map_viewer_get_type (void) G_GNUC_CONST;

void map_viewer_get_transformation(MapViewer *viewer,
                                   MapLatLonToUnit *latlon2unit,
                                   MapUnitToLatLon *unit2latlon);

void map_viewer_emit_transformation_changed(MapViewer *viewer,
                                            MapLatLonToUnit latlon2unit,
                                            MapUnitToLatLon unit2latlon);

G_END_DECLS
#endif /* MAP_VIEWER_H */
