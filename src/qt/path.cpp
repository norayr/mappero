/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "debug.h"
#include "gpx.h"
#include "kml.h"
#include "path.h"
#include "projection.h"

#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Mappero;

static const Projection *global_projection = 0;

static inline void ensure_projection()
{
    if (global_projection == 0) {
        global_projection = Projection::get(Projection::GOOGLE);
    }
}

inline GeoPoint operator-(const GeoPoint &p1, const GeoPoint &p2)
{ return GeoPoint(p1.lat - p2.lat, p1.lon - p2.lon); }

inline GeoPoint operator+(const GeoPoint &p1, const GeoPoint &p2)
{ return GeoPoint(p1.lat + p2.lat, p1.lon + p2.lon); }

inline GeoPoint operator*(const GeoPoint &p, double x)
{ return GeoPoint(p.lat * x, p.lon * x); }

struct PathVector
{
    PathVector(GeoPoint geo, qint16 altitude):
        geo(geo), altitude(altitude) {}

    PathVector operator*(double x) const {
        return PathVector(geo * x, altitude * x);
    }

    GeoPoint geo;
    qint16 altitude;
};

inline PathVector operator-(const PathPoint &a, const PathPoint &b)
{
    return PathVector(a.geo - b.geo, a.altitude - b.altitude);
}

inline PathPoint operator+(const PathPoint &p, const PathVector &v)
{
    return PathPoint(p.geo + v.geo, p.altitude + v.altitude);
}

PathPoint::PathPoint():
    geo(),
    unit(0, 0),
    time(0),
    zoom(SCHAR_MAX),
    altitude(0),
    distance(0.0)
{
}

PathPoint::PathPoint(const GeoPoint &p, int altitude):
    geo(p),
    time(0),
    zoom(SCHAR_MAX),
    altitude(altitude),
    distance(0.0)
{
    ensure_projection();
    unit = global_projection->geoToUnit(p);
}

PathWayPoint::PathWayPoint():
    pointIndex(-1),
    dir(DIRECTION_UNKNOWN)
{
}

PathWayPoint::PathWayPoint(const QString &desc, int pointIndex):
    pointIndex(pointIndex),
    dir(DIRECTION_UNKNOWN),
    desc(desc)
{
}

Path::Path():
    d(new PathData)
{
}

Path::~Path()
{
}

bool Path::load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    return load(&file);
}

bool Path::load(QIODevice *device)
{
    QXmlStreamReader xml(device);

    while (xml.readNextStartElement()) {
        if (xml.name() == "gpx") {
            Gpx gpx;
            return d->load(xml, &gpx);
        } else if (xml.name() == "kml") {
            Kml kml;
            return d->load(xml, &kml);
        }
    }
    return false;
}

bool Path::save(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return save(&file);
}

bool Path::save(QIODevice *device) const
{
    QXmlStreamWriter xml(device);

    /* Only GPX for the time being */
    Gpx gpx;
    return gpx.write(xml, *d);
}

bool Path::isEmpty() const
{
    return d->points.isEmpty();
}

const PathPoint &Path::firstPoint() const
{
    return d->points.first();
}

const PathPoint &Path::lastPoint() const
{
    return d->points.last();
}

PathPoint Path::positionAt(time_t time) const
{
    return d->positionAt(time);
}

QRectF Path::boundingRect() const
{
    return d->boundingRect();
}

Geo Path::length() const
{
    return d->length();
}

void Path::clear()
{
    d = new PathData;
}

void Path::addPoint(const GeoPoint &geo, int altitude, time_t time,
                    Geo distance)
{
    DEBUG() << geo;
    PathPoint p(geo);
    p.altitude = altitude;
    p.time = time;
    p.distance = distance;
    d->addPoint(p);
}

void Path::appendBreak()
{
    d->appendBreak();
}

QPainterPath Path::toPainterPath(int zoomLevel) const
{
    if (d->points.isEmpty()) return QPainterPath();

    QPainterPath pp;
    QList<PathSegment>::const_iterator curr, next;
    for (curr = d->segments.constBegin();
         curr != d->segments.constEnd();
         curr++) {
        next = curr + 1;
        int lastIndex = (next == d->segments.constEnd()) ?
            d->points.count() : next->startIndex;

        pp.moveTo(d->points[curr->startIndex].unit.toPixel(zoomLevel));
        for (int i = curr->startIndex + 1; i < lastIndex; i++) {
            const PathPoint &point = d->points.at(i);
            // skip points based on their zoom value
            if (point.zoom <= zoomLevel) continue;

            pp.lineTo(point.unit.toPixel(zoomLevel));
        }
    }
    return pp;
}

void Path::setProjection(const Projection *projection)
{
    global_projection = projection;
    // maybe TODO: keep track of the existing paths, and update all of them
}

PathStream::PathStream()
{
}

PathStream::~PathStream()
{
}

PathData::PathData():
    pointsOptimized(0),
    latMin(100), latMax(-100),
    lonMin(200), lonMax(-200),
    m_length(0)
{
    segments.append(PathSegment());
}

bool PathData::load(QXmlStreamReader &xml, PathStream *stream)
{
    if (!stream->read(xml, *this)) return false;
    optimize();
    return true;
}

void PathData::addPoint(const PathPoint &p)
{
    int distance = p.distance;
    if (distance < 0 && !points.isEmpty()) {
        distance = p.geo.distanceTo(points.last().geo);
    }
    points.append(p);

    if (distance >= 0) {
        PathPoint &addedPoint = points.last();
        addedPoint.distance = distance;
        m_length += distance;
    }

    /* update bounding rect */
    if (p.geo.lat < latMin) {
        latMin = p.geo.lat;
    } else if (p.geo.lat > latMax) {
        latMax = p.geo.lat;
    }

    if (p.geo.lon < lonMin) {
        lonMin = p.geo.lon;
    } else if (p.geo.lon > lonMax) {
        lonMax = p.geo.lon;
    }

    optimize();
}

void PathData::makeWayPoint(const QString &desc, int pointIndex)
{
    wayPoints.append(PathWayPoint(desc, pointIndex));
}

bool PathData::appendBreak()
{
    if (points.isEmpty()) return false;

    Q_ASSERT (!segments.isEmpty());

    int length = points.count();
    const PathSegment &segment = segments.last();
    if (segment.startIndex == length) return false;
    segments.append(PathSegment(length));
    return true;
}

PathPoint PathData::positionAt(time_t time) const
{
    if (points.isEmpty() ||
        time < points.first().time ||
        time > points.last().time) {
        return PathPoint();
    }

    /* Find the two closest elements in the path. */
    QVector<PathPoint>::const_iterator a, b;
    for (b = points.begin(); b != points.end(); b++)
        if (b->time > time) break;

    a = b - 1;
    /* if an element was not found or if times are the same, then we can return
     * the last one */
    if (b == points.end() || a->time == b->time) return *a;

    /* interpolate between a and b */
    double t = (double(time) - a->time) / (b->time - a->time);
    return *a + (*b - *a) * t;
}

QRectF PathData::boundingRect() const
{
    return QRectF(QPointF(latMin, lonMin), QPointF(latMax, lonMax));
}

void PathData::optimize()
{
    int tolerance = 8;

    /* for every point, set the zoom level at which the point must be rendered
     */

    if (points.isEmpty()) return;

    QVector<PathPoint>::iterator curr = points.begin() + pointsOptimized;
    if (pointsOptimized == 0)
    {
        curr->zoom = SCHAR_MAX;
        pointsOptimized = 1;
        curr++;
    }

    QVector<PathPoint>::iterator prev;

    for (; curr != points.end(); curr++)
    {
        int dx, dy, dmax, zoom;

        prev = curr - 1;

        dx = curr->unit.x() - prev->unit.x();
        dy = curr->unit.y() - prev->unit.y();
        dmax = qMax(qAbs(dx), qAbs(dy));

        for (zoom = 0; dmax > tolerance << zoom; zoom++) {}

        /* We got the zoom level for this point, supposing that the previous
         * one is always drawn.
         * But we need go back in the path to find the last point which is
         * _surely_ drawn when this point is; that is, we look for the last
         * point having a zoom value bigger than that of the current point. */

        while (zoom >= prev->zoom)
        {
            QVector<PathPoint>::iterator prevBefore;
            /* going back is safe (we don't risk going past the head) because
             * the first point will always have zoom set to 127 */
            for (prevBefore = prev; prev->zoom <= zoom; prev--) {}

            if (prev == prevBefore) break;

            /* now, find the distance between these two points */
            dx = curr->unit.x() - prev->unit.x();
            dy = curr->unit.y() - prev->unit.y();
            dmax = qMax(qAbs(dx), qAbs(dy));

            for (; dmax > tolerance << zoom; zoom++) {}
        }

        curr->zoom = zoom;
    }

    pointsOptimized = points.count();
}
