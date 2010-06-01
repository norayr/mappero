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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "viewer.h"

#include "map-signals-marshal.h"

enum
{
    TRANSFORMATION_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

MapLatLonToUnit map_viewer_latlon2unit;
MapUnitToLatLon map_viewer_unit2latlon;

static void
map_viewer_transformation_changed(MapViewer *viewer,
                                  MapLatLonToUnit latlon2unit,
                                  MapUnitToLatLon unit2latlon)
{
    /* update the global variables, for faster access to transformations */
    map_viewer_latlon2unit = latlon2unit;
    map_viewer_unit2latlon = unit2latlon;
}

static void
class_init (gpointer klass, gpointer data)
{
    GType type = G_TYPE_FROM_CLASS(klass);
    MapViewerIface *viewer_iface = klass;

    viewer_iface->transformation_changed = map_viewer_transformation_changed;

    /**
     * MapViewer::transformation-changed
     * @viewer: the #MapViewer.
     * @latlon2unit: the new #MapLatLonToUnit function.
     * @unit2latlon: the new #MapUnitToLatLon function.
     *
     * Emitted when the view transformation changes.
     */
    signals[TRANSFORMATION_CHANGED] =
        g_signal_new("transformation-changed",
                     type, G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(MapViewerIface, transformation_changed),
                     NULL, NULL,
                     _map_marshal_VOID__POINTER_POINTER, G_TYPE_NONE,
                     2, G_TYPE_POINTER, G_TYPE_POINTER);
}

GType
map_viewer_get_type(void)
{
    static gsize once = 0;
    static GType type = 0;

    if (g_once_init_enter(&once))
    {
        static const GTypeInfo info = {
            sizeof(MapViewerIface),
            NULL, /* base_init */
            NULL, /* base_finalize */
            class_init, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            0, /* instance_size */
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL /* value_table */
        };

        type = g_type_register_static(G_TYPE_INTERFACE, "MapViewer", &info, 0);
        g_type_interface_add_prerequisite(type, G_TYPE_OBJECT);

        g_once_init_leave(&once, 1);
    }

    return type;
}

/**
 * map_viewer_get_transformation:
 * @viewer: the #MapViewer
 * @latlon2unit: the function to transform from lat/lon to Mappero's units
 * @unit2latlon: the function to transform from Mappero's units to lat/lon
 *
 * Returns the currently active coordinate transformation.
 */
void
map_viewer_get_transformation(MapViewer *viewer,
                              MapLatLonToUnit *latlon2unit,
                              MapUnitToLatLon *unit2latlon)
{
    MapViewerIface *iface = MAP_VIEWER_GET_IFACE(viewer);

    g_return_if_fail(iface != NULL);
    g_return_if_fail(iface->get_transformation != NULL);
    g_return_if_fail(latlon2unit != NULL);
    g_return_if_fail(unit2latlon != NULL);

    iface->get_transformation(viewer, latlon2unit, unit2latlon);
}

/**
 * map_viewer_emit_transformation_changed:
 * @viewer: the #MapViewer
 * @latlon2unit: the function to transform from lat/lon to Mappero's units
 * @unit2latlon: the function to transform from Mappero's units to lat/lon
 *
 * Signals that the viewer coordinate transformation has changed.
 */
void
map_viewer_emit_transformation_changed(MapViewer *viewer,
                                       MapLatLonToUnit latlon2unit,
                                       MapUnitToLatLon unit2latlon)
{
    g_signal_emit(viewer, signals[TRANSFORMATION_CHANGED], 0,
                  latlon2unit, unit2latlon);
}

