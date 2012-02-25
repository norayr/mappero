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

#ifndef MAP_TYPES_H
#define MAP_TYPES_H

#include <QMetaType>
#include <QPoint>
#include <math.h>

namespace Mappero {

#define TILE_SIZE_PIXELS (256)
#define TILE_HALFDIAG_PIXELS (181)
#define TILE_SIZE_P2 (8)

class TiledLayer;

#ifdef USE_DOUBLES_FOR_LATLON
typedef double Geo;
#else
typedef float Geo;
#endif

typedef int Unit;

struct Point: public QPoint {
    Point(): QPoint() {}
    Point(Unit x, Unit y): QPoint(x, y) {}
    Point(const QPoint &p): QPoint(p) {}

    inline QPoint toTile(int zoom) {
        return QPoint(x() >> (TILE_SIZE_P2 + zoom),
                      y() >> (TILE_SIZE_P2 + zoom));
    }

    inline QPoint toPixel(int zoom) {
        return QPoint(x() >> zoom, y() >> zoom);
    }

    inline QPoint toPixel(qreal zoom) {
        return QPoint(x() / exp2(zoom), y() / exp2(zoom));
    }

    inline Point translated(const QPoint &p) {
        return Point(x() + p.x(), y() + p.y());
    }
};

struct GeoPoint {
    GeoPoint() {}
    GeoPoint(const QPointF &p): lat(p.x()), lon(p.y()) {}
    GeoPoint(Geo lat, Geo lon): lat(lat), lon(lon) {}
    Geo lat;
    Geo lon;

    QPointF toPointF() const { return QPointF(lat, lon); }
    friend inline bool operator==(const GeoPoint &, const GeoPoint &);
    friend inline bool operator!=(const GeoPoint &, const GeoPoint &);
};

inline bool operator==(const GeoPoint &p1, const GeoPoint &p2)
{ return p1.lat == p2.lat && p1.lon == p2.lon; }

inline bool operator!=(const GeoPoint &p1, const GeoPoint &p2)
{ return p1.lat != p2.lat || p1.lon != p2.lon; }

struct TileSpec
{
    TileSpec(int x, int y, int zoom, TiledLayer *layer):
        x(x), y(y), zoom(zoom), layer(layer) {}
    int x;
    int y;
    int zoom;
    TiledLayer *layer;
};

Unit metre2unit(qreal metres, Geo latitude);

} // namespace

inline uint qHash(const Mappero::TileSpec &tile)
{
    return (tile.x & 0xff) |
        ((tile.y & 0xff) << 8) |
        ((tile.zoom & 0xff) << 16);
}

Q_DECLARE_METATYPE(Mappero::GeoPoint)

#include <QDebug>

QDebug operator<<(QDebug dbg, const Mappero::GeoPoint &p);
QDebug operator<<(QDebug dbg, const Mappero::Point &p);
QDebug operator<<(QDebug dbg, const Mappero::TileSpec &t);

inline bool operator==(const Mappero::TileSpec &t1, const Mappero::TileSpec &t2)
{
    return t1.x == t2.x && t1.y == t2.y && t1.zoom == t2.zoom &&
        t1.layer == t2.layer;
}


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
#endif

#define PI   ((Mappero::Geo)3.14159265358979323846)

/** MAX_ZOOM defines the largest map zoom level we will download.
 * (MAX_ZOOM - 1) is the largest map zoom level that the user can zoom to.
 */
#define MIN_ZOOM (0)
#define MAX_ZOOM (20)

#define WORLD_SIZE_UNITS (2 << (MAX_ZOOM + TILE_SIZE_P2))

#define deg2rad(deg) ((deg) * (PI / 180.0))
#define rad2deg(rad) ((rad) * (180.0 / PI))

#endif /* MAP_TYPES_H */

