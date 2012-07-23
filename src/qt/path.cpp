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
    geo(0, 0),
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
            return d->loadGpx(xml);
        } else if (xml.name() == "kml") {
            Kml kml(xml);
            return kml.appendToPath(this);
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
    return d->saveGpx(xml);
}

bool Path::isEmpty() const
{
    return d->points.isEmpty();
}

const PathPoint &Path::lastPoint() const
{
    return d->points.last();
}

PathPoint Path::positionAt(time_t time) const
{
    return d->positionAt(time);
}

void Path::clear()
{
    d->points.clear();
    d->wayPoints.clear();
}

void Path::addPoint(const GeoPoint &geo, int altitude, time_t time,
                    Geo distance)
{
    DEBUG() << geo;
    PathPoint p(geo);
    p.altitude = altitude;
    p.time = time;
    if (!isEmpty()) {
        if (distance < 0) {
            distance = geo.distanceTo(lastPoint().geo);
        }
        p.distance = distance;
    }
    d->points.append(p);
    d->optimize();
}

QPainterPath Path::toPainterPath(int zoomLevel) const
{
    if (d->points.isEmpty()) return QPainterPath();

    QPainterPath pp(d->points[0].unit.toPixel(zoomLevel));
    foreach (const PathPoint &point, d->points) {
        // skip points based on their zoom value
        if (point.zoom <= zoomLevel) continue;

        pp.lineTo(point.unit.toPixel(zoomLevel));
    }
    return pp;
}

void Path::setProjection(const Projection *projection)
{
    global_projection = projection;
    // maybe TODO: keep track of the existing paths, and update all of them
}

bool PathData::loadGpx(QXmlStreamReader &xml)
{
    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == "trkpt") {
            QXmlStreamAttributes attributes = xml.attributes();
            QStringRef lat = attributes.value("lat");
            QStringRef lon = attributes.value("lon");
            if (lat.isEmpty() || lon.isEmpty()) continue;

            PathPoint p(GeoPoint(lat.toString().toDouble(),
                                 lon.toString().toDouble()));
            QString desc;

            while (xml.readNextStartElement()) {
                if (xml.name() == "ele") {
                    p.altitude = xml.readElementText().toFloat();
                } else if (xml.name() == "time") {
                    QDateTime time =
                        QDateTime::fromString(xml.readElementText(),
                                              Qt::ISODate);
                    p.time = time.toTime_t();
                } else if (xml.name() == "cmt") {
                    desc = xml.readElementText();
                } else {
                    DEBUG() << "Unrecognized element:" << xml.name();
                }
            }
            points.append(p);

            /* is it a waypoint? */
            if (!desc.isEmpty()) {
                makeWayPoint(desc, points.count() - 1);
            }
        }
    }
    optimize();
    return true;
}

bool PathData::saveGpx(QXmlStreamWriter &xml) const
{
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();

    /* GPX element */
    xml.writeStartElement("gpx");
    xml.writeAttribute("version", "1.0");
    xml.writeAttribute("creator", "mappero");
    xml.writeAttribute("xmlns", "http://www.topografix.com/GPX/1/0");

    /* track element */
    xml.writeStartElement("trk");

    /* TODO handle track segments */
    xml.writeStartElement("trkseg");

    /* this is outside of the loop, in order to reuse it */
    QDateTime time;
    time.setTimeSpec(Qt::UTC);

    foreach (const PathPoint &point, points) {
        xml.writeStartElement("trkpt");
        xml.writeAttribute("lat", QString::number(point.geo.lat, 'f'));
        xml.writeAttribute("lon", QString::number(point.geo.lon, 'f'));

        if (point.altitude != 0) {
            xml.writeTextElement("ele",
                                 QString::number(point.altitude));
        }

        if (point.time != 0) {
            time.setTime_t(point.time);
            xml.writeTextElement("time", time.toString(Qt::ISODate));
        }
        xml.writeEndElement();
    }
    xml.writeEndDocument();

    return true;
}

void PathData::makeWayPoint(const QString &desc, int pointIndex)
{
    wayPoints.append(PathWayPoint(desc, pointIndex));
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

        for (zoom = 0; dmax > tolerance << zoom; zoom++);

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
            for (prevBefore = prev; prev->zoom <= zoom; prev--);

            if (prev == prevBefore) break;

            /* now, find the distance between these two points */
            dx = curr->unit.x() - prevBefore->unit.x();
            dy = curr->unit.y() - prevBefore->unit.y();
            dmax = qMax(qAbs(dx), qAbs(dy));

            for (; dmax > tolerance << zoom; zoom++);
        }

        curr->zoom = zoom;
    }

    pointsOptimized = points.count();
}
