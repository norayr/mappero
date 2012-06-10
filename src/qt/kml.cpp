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
#include "kml.h"

#include <QDateTime>
#include <QStringList>
#include <QXmlStreamReader>

using namespace Mappero;

struct KmlWayPoint {
    KmlWayPoint(const PathPoint &p, const QString &desc):
        p(p), desc(desc) {}

    PathPoint p;
    QString desc;
};

static void
makeWayPoints(PathData *pathData,
              const QList<KmlWayPoint> &wayPoints)
{
    int index = 0;
    foreach (const KmlWayPoint &wp, wayPoints) {
        int wpIndex = pathData->points.indexOf(wp.p, index);
        if (wpIndex < 0) {
            qWarning() << "Waypoint not found:" << wp.desc;
            continue;
        }

        pathData->makeWayPoint(wp.desc, wpIndex);
        index = wpIndex + 1;
    }
}

Kml::Kml(QXmlStreamReader &xml):
    xml(xml)
{
}

Kml::~Kml()
{
}

void Kml::parseCoordinates()
{
    QStringList pts = xml.readElementText().split(" ");
    foreach (const QString &point, pts) {
        if (point.isEmpty()) continue;
        QStringList coords = point.split(",");
        if (coords.count() < 2) {
            DEBUG() << "Incomplete coordinates" << point;
            continue;
        }

        PathPoint p(GeoPoint(coords[0].toDouble(),
                             coords[1].toDouble()));
        if (coords.count() > 2) {
            p.altitude = coords[2].toInt();
        }

        points.append(p);
    }
}

void Kml::parseLineString()
{
    while (xml.readNextStartElement()) {
        if (xml.name() == "coordinates") {
            parseCoordinates();
        } else {
            xml.skipCurrentElement();
        }
    }
}

void Kml::parsePoint()
{
    while (xml.readNextStartElement()) {
        if (xml.name() == "coordinates") {
            parseCoordinates();
        } else {
            xml.skipCurrentElement();
        }
    }
}

void Kml::parseGeometryCollection()
{
    while (xml.readNextStartElement()) {
        if (xml.name() == "LineString") {
            parseLineString();
        } else {
            xml.skipCurrentElement();
        }
    }
}

bool Kml::appendToPath(Path *path)
{
    PathData *pathData = path->d;
    QList<KmlWayPoint> waypoints;

    xml.readNextStartElement();
    if (xml.name() != "Document") return false;

    while (xml.readNextStartElement()) {
        if (xml.name() == "Placemark") {
            QString desc;

            points.clear();
            while (xml.readNextStartElement()) {
                if (xml.name() == "name") {
                    desc = xml.readElementText();
                } else if (xml.name() == "Point") {
                    parsePoint();
                } else if (xml.name() == "LineString") {
                    parseLineString();
                } else if (xml.name() == "GeometryCollection") {
                    parseGeometryCollection();
                } else {
                    xml.skipCurrentElement();
                }
            }

            /* is it a waypoint? */
            if (!desc.isEmpty() && points.count() == 1) {
                waypoints.append(KmlWayPoint(points[0], desc));
            } else {
                pathData->points += points;
            }
        } else {
            xml.skipCurrentElement();
        }
    }

    makeWayPoints(pathData, waypoints);
    return true;
}
