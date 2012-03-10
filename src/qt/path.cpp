/* vi: set et sw=4 ts=8 cino=t0,(0: */
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
#include "path.h"
#include "projection.h"

#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QXmlStreamReader>

using namespace Mappero;

static const Projection *global_projection = 0;

static inline void ensure_projection()
{
    if (global_projection == 0) {
        global_projection = Projection::get(Projection::GOOGLE);
    }
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

PathPoint::PathPoint(const GeoPoint &p):
    geo(p),
    time(0),
    zoom(SCHAR_MAX),
    altitude(0),
    distance(0.0)
{
    ensure_projection();
    unit = global_projection->geoToUnit(p);
}

Path::Path()
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

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == "gpx") {
            return loadGpx(xml);
        }
    }
    return false;
}

QPainterPath Path::toPainterPath(int zoomLevel) const
{
    if (points.isEmpty()) return QPainterPath();

    QPainterPath pp(points[0].unit.toPixel(zoomLevel));
    foreach (const PathPoint &point, points) {
        // TODO: skip points based on their zoom value
        pp.lineTo(point.unit.toPixel(zoomLevel));
    }
    return pp;
}

void Path::setProjection(const Projection *projection)
{
    global_projection = projection;
    // maybe TODO: keep track of the existing paths, and update all of them
}

bool Path::loadGpx(QXmlStreamReader &xml)
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

            while (xml.readNextStartElement()) {
                if (xml.name() == "ele") {
                    p.altitude = xml.readElementText().toInt();
                } else if (xml.name() == "time") {
                    QDateTime time =
                        QDateTime::fromString(xml.readElementText(),
                                              Qt::ISODate);
                    p.time = time.toTime_t();
                }
            }
            points.append(p);
        }
    }
    return false;
}
