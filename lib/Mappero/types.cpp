/*
 * Copyright (C) 2011 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "types.h"

#include <QDataStream>
#include <math.h>

using namespace Mappero;

#define EQUATORIAL_CIRCUMFERENCE 40075017
#define EARTH_RADIUS 6378137.01

Geo GeoPoint::distanceTo(const GeoPoint &other) const
{
    Geo lat1, lon1, lat2, lon2;
    Geo dlat, dlon, slat, slon, a;

    /* Convert to radians. */
    lat1 = deg2rad(lat);
    lon1 = deg2rad(lon);
    lat2 = deg2rad(other.lat);
    lon2 = deg2rad(other.lon);

    dlat = lat2 - lat1;
    dlon = lon2 - lon1;

    slat = GSIN(dlat / 2.0);
    slon = GSIN(dlon / 2.0);
    a = (slat * slat) + (GCOS(lat1) * GCOS(lat2) * slon * slon);

    return ((2.0 * GATAN2(GSQTR(a), GSQTR(1.0 - a))) * EARTH_RADIUS);
}

QDataStream &operator<<(QDataStream &out, const Mappero::GeoPoint &geoPoint)
{
    return out << geoPoint.lat << geoPoint.lon;
}

QDataStream &operator>>(QDataStream &in, Mappero::GeoPoint &geoPoint)
{
    in >> geoPoint.lat;
    in >> geoPoint.lon;
    return in;
}

QDebug operator<<(QDebug dbg, const GeoPoint &p)
{
    dbg.nospace() << "(" << p.lat << ", " << p.lon << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Point &p)
{
    dbg.nospace() << "(" << p.x() << ", " << p.y() << ")";
    return dbg.space();
}

Unit Mappero::metre2unit(qreal metres, Geo latitude)
{
    return metres * WORLD_SIZE_UNITS /
        (EQUATORIAL_CIRCUMFERENCE * GCOS(deg2rad(latitude)));
}
