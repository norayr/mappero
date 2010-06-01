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

#ifndef MAP_GLOBALS_H
#define MAP_GLOBALS_H

#include <glib.h>

G_BEGIN_DECLS

#define GCONF_KEY_PREFIX "/apps/maemo/maemo-mapper"

#ifdef USE_DOUBLES_FOR_LATLON
typedef gdouble MapGeo;
#else
typedef gfloat MapGeo;
#endif

/** A general definition of a point in the Mappero unit system. */
typedef struct {
    gint x;
    gint y;
} MapPoint;

typedef void (*MapLatLonToUnit)(MapGeo lat, MapGeo lon, gint *x, gint *y);
typedef void (*MapUnitToLatLon)(gint x, gint y, MapGeo *lat, MapGeo *lon);

#define map_latlon2unit(lat, lon, unitx, unity) \
    map_viewer_latlon2unit(lat, lon, &unitx, &unity)
#define map_unit2latlon(unitx, unity, lat, lon) \
    map_viewer_unit2latlon(unitx, unity, &lat, &lon)
/* These global variables are defined and updated inside viewwer.c */
extern MapLatLonToUnit map_viewer_latlon2unit;
extern MapUnitToLatLon map_viewer_unit2latlon;

/* Some more questionable macros */
#define DISTANCE_SQUARED(a, b) \
   ((guint64)((((gint64)(b).x)-(a).x)*(((gint64)(b).x)-(a).x))\
  + (guint64)((((gint64)(b).y)-(a).y)*(((gint64)(b).y)-(a).y)))


G_END_DECLS
#endif /* MAP_GLOBALS_H */

