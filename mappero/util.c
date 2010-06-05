/*
 * Copyright (C) 2006, 2007 John Costigan.
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
#    include "config.h"
#endif
#include "util.h"

#include <math.h>


/**
 * Calculate the distance between two lat/lon pairs.  The distance is returned
 * in kilometers and should be converted using UNITS_CONVERT[_units].
 */
MapGeo
map_calculate_distance(MapGeo lat1, MapGeo lon1,
                       MapGeo lat2, MapGeo lon2)
{
    MapGeo dlat, dlon, slat, slon, a;

    /* Convert to radians. */
    lat1 = deg2rad(lat1);
    lon1 = deg2rad(lon1);
    lat2 = deg2rad(lat2);
    lon2 = deg2rad(lon2);

    dlat = lat2 - lat1;
    dlon = lon2 - lon1;

    slat = GSIN(dlat / 2.0);
    slon = GSIN(dlon / 2.0);
    a = (slat * slat) + (GCOS(lat1) * GCOS(lat2) * slon * slon);

    return ((2.0 * GATAN2(GSQTR(a), GSQTR(1.0 - a))) * EARTH_RADIUS);
}

/**
 * Calculate the bearing between two lat/lon pairs.  The bearing is returned
 * as the angle from lat1/lon1 to lat2/lon2.
 */
MapGeo
map_calculate_bearing(MapGeo lat1, MapGeo lon1,
                      MapGeo lat2, MapGeo lon2)
{
    MapGeo x, y;
    MapGeo dlon = deg2rad(lon2 - lon1);
    lat1 = deg2rad(lat1);
    lat2 = deg2rad(lat2);

    y = GSIN(dlon) * GCOS(lat2);
    x = (GCOS(lat1) * GSIN(lat2)) - (GSIN(lat1) * GCOS(lat2) * GCOS(dlon));

    dlon = rad2deg(GATAN2(y, x));
    if(dlon < 0.0)
        dlon += 360.0;
    return dlon;
}

