/*
 * Copyright (C) 2011-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAPPERO_TYPES_H
#define MAPPERO_TYPES_H

#include "global.h"

#include <QMetaType>
#include <QPoint>
#include <limits.h>
#include <math.h>

namespace Mappero {

#define TILE_SIZE_PIXELS (256)
#define TILE_HALFDIAG_PIXELS (181)
#define TILE_SIZE_P2 (8)

#ifdef USE_DOUBLES_FOR_LATLON
typedef double Geo;
#else
typedef float Geo;
#endif

#ifdef USE_DOUBLES_FOR_LATLON
#define GSIN(x) sin(x)
#define GCOS(x) cos(x)
#define GASIN(x) asin(x)
#define GTAN(x) tan(x)
#define GATAN(x) atan(x)
#define GATAN2(x, y) atan2(x, y)
#define GEXP(x) exp(x)
#define GLOG(x) log(x)
#define GPOW(x, y) pow(x, y)
#define GSQTR(x) sqrt(x)
#define GFLOOR(x) floor(x)
#define GROUND(x) round(x)
#define GCEIL(x) ceil(x)
#define GTRUNC(x) trunc(x)
#define GMOD(x, y) fmod(x, y)
#else
#define GSIN(x) sinf(x)
#define GCOS(x) cosf(x)
#define GASIN(x) asinf(x)
#define GTAN(x) tanf(x)
#define GATAN(x) atanf(x)
#define GATAN2(x, y) atan2f(x, y)
#define GEXP(x) expf(x)
#define GLOG(x) logf(x)
#define GPOW(x, y) powf(x, y)
#define GSQTR(x) sqrtf(x)
#define GFLOOR(x) floorf(x)
#define GROUND(x) roundf(x)
#define GCEIL(x) ceilf(x)
#define GTRUNC(x) truncf(x)
#define GMOD(x, y) fmodf(x, y)
#endif

typedef int Unit;

struct Point: public QPoint {
    Point(): QPoint(0, INT_MIN) {}
    Point(Unit x, Unit y): QPoint(x, y) {}
    Point(const QPoint &p): QPoint(p) {}

    bool isValid() const { return y() != INT_MIN; }

    inline QPoint toTile(int zoom) const {
        return QPoint(x() / (1 << (TILE_SIZE_P2 + zoom)),
                      y() / (1 << (TILE_SIZE_P2 + zoom)));
    }

    inline QPoint toPixel(int zoom) const {
        return QPoint(x() / (1 << zoom), y() / (1 << zoom));
    }

    inline QPoint toPixel(qreal zoom) const {
        return QPoint(GFLOOR(x() / exp2(zoom)), GFLOOR(y() / exp2(zoom)));
    }

    inline QPointF toPixelF(qreal zoom) const {
        return QPointF(x() / exp2(zoom), y() / exp2(zoom));
    }

    inline Point translated(const QPoint &p) const {
        return Point(x() + p.x(), y() + p.y());
    }

    static Point fromPixel(const QPoint p, int zoom) {
        return Point(p.x() << zoom, p.y() << zoom);
    }

    static Point fromPixel(const QPoint p, qreal zoom) {
        return Point(p.x() * exp2(zoom), p.y() * exp2(zoom));
    }
};

struct MAPPERO_EXPORT GeoPoint {
    Q_GADGET
    Q_PROPERTY(bool valid READ isValid)
    Q_PROPERTY(qreal lat MEMBER lat)
    Q_PROPERTY(qreal lon MEMBER lon)

public:
    GeoPoint(): lat(NAN), lon(0) {}
    GeoPoint(const QPointF &p): lat(p.x()), lon(p.y()) {}
    GeoPoint(Geo lat, Geo lon): lat(lat), lon(lon) {}
    Geo lat;
    Geo lon;

    inline GeoPoint normalized() const;
    QPointF toPointF() const { return QPointF(lat, lon); }
    Geo distanceTo(const GeoPoint &other) const;
    bool isValid() const { return lat == lat; /* NaN != NaN */ }
    friend inline bool operator==(const GeoPoint &, const GeoPoint &);
    friend inline bool operator!=(const GeoPoint &, const GeoPoint &);
};

GeoPoint GeoPoint::normalized() const
{
    GeoPoint ret(GMOD(lat, 180), GMOD(lon, 360));
    /* TODO: normalize latitude */
    if (ret.lon > 180) {
        ret.lon -= 360;
    } else if (ret.lon < -180) {
        ret.lon += 360;
    }
    return ret;
}

inline bool operator==(const GeoPoint &p1, const GeoPoint &p2)
{ return p1.lon == p2.lon && (p1.isValid() == p2.isValid()) &&
    (!p1.isValid() || p1.lat == p2.lat); }

inline bool operator!=(const GeoPoint &p1, const GeoPoint &p2)
{ return p1.lon != p2.lon || (p1.isValid() != p2.isValid()) ||
    (p1.isValid() && p1.lat != p2.lat); }

MAPPERO_EXPORT Unit metre2unit(qreal metres, Geo latitude);

MAPPERO_EXPORT void registerTypes();

} // namespace

Q_DECLARE_METATYPE(Mappero::GeoPoint)

MAPPERO_EXPORT QDataStream &operator<<(QDataStream &out, const Mappero::GeoPoint &geoPoint);
MAPPERO_EXPORT QDataStream &operator>>(QDataStream &in, Mappero::GeoPoint &geoPoint);

#include <QDebug>

MAPPERO_EXPORT QDebug operator<<(QDebug dbg, const Mappero::GeoPoint &p);
MAPPERO_EXPORT QDebug operator<<(QDebug dbg, const Mappero::Point &p);


#define PI   ((Mappero::Geo)3.14159265358979323846)

/** MAX_ZOOM defines the largest map zoom level we will download.
 * (MAX_ZOOM - 1) is the largest map zoom level that the user can zoom to.
 */
#define MIN_ZOOM (0)
#define MAX_ZOOM (20)

#define WORLD_SIZE_UNITS (2 << (MAX_ZOOM + TILE_SIZE_P2))

#define deg2rad(deg) ((deg) * (PI / 180.0))
#define rad2deg(rad) ((rad) * (180.0 / PI))

#endif /* MAPPERO_TYPES_H */

