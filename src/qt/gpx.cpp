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

#include <QDateTime>
#include <QXmlStreamReader>

using namespace Mappero;

Gpx::Gpx():
    PathStream()
{
}

Gpx::~Gpx()
{
}

void Gpx::parseTrkseg(QXmlStreamReader &xml, PathData &pathData)
{
    QVector<PathPoint> &points = pathData.points;

    while (xml.readNextStartElement()) {
        if (xml.name() == "trkpt") {
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
                    xml.skipCurrentElement();
                }
            }
            pathData.addPoint(p);

            /* is it a waypoint? */
            if (!desc.isEmpty()) {
                pathData.makeWayPoint(desc, points.count() - 1);
            }
        } else {
            xml.skipCurrentElement();
        }
    }
}

bool Gpx::read(QXmlStreamReader &xml, PathData &pathData)
{
    while (xml.readNextStartElement()) {
        if (xml.name() == "trk") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "trkseg") {
                    pathData.appendBreak();
                    parseTrkseg(xml, pathData);
                } else {
                    xml.skipCurrentElement();
                }
            }
        } else {
            xml.skipCurrentElement();
        }
    }
    return true;
}

bool Gpx::write(QXmlStreamWriter &xml, const PathData &pathData)
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

    /* this is outside of the loop, in order to reuse it */
    QDateTime time;
    time.setTimeSpec(Qt::UTC);

    QList<PathSegment>::const_iterator curr, next;
    for (curr = pathData.segments.constBegin();
         curr != pathData.segments.constEnd();
         curr++) {
        next = curr + 1;
        int lastIndex = (next == pathData.segments.constEnd()) ?
            pathData.points.count() : next->startIndex;

        if (lastIndex <= curr->startIndex) continue;

        xml.writeStartElement("trkseg");

        for (int i = curr->startIndex + 1; i < lastIndex; i++) {
            const PathPoint &point = pathData.points.at(i);
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
        xml.writeEndElement();
    }
    xml.writeEndDocument();

    return true;
}
