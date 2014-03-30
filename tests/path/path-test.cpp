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

#include "path-test.h"

#include "path.h"
#include <QDebug>

#define UTF8(s) QString::fromUtf8(s)

using namespace Mappero;

void PathTest::initTestCase()
{
}

void PathTest::cleanupTestCase()
{
}

void PathTest::loadGpx()
{
    Path path;

    path.load(":/Lauttasaari.gpx");
    QCOMPARE(path.d->points.count(), 524);
    QCOMPARE(path.d->segments.count(), 2);

    Path route;
    route.load(":/Route.gpx");
    QCOMPARE(route.d->points.count(), 1509);
    QCOMPARE(route.wayPointCount(), 15);
    QCOMPARE(route.d->segments.count(), 1);
}

void PathTest::loadKml()
{
    Path route;
    route.load(":/Route.kml");
    QCOMPARE(route.d->points.count(), 2105);
    QCOMPARE(route.wayPointCount(), 2);
    QCOMPARE(route.wayPointText(0), UTF8("Vilhonkatu 13, Helsinki"));
    QCOMPARE(route.wayPointText(1),
             UTF8("Helsinki-vantaan Lentoasema 2, Vantaa"));
    QCOMPARE(route.d->segments.count(), 1);

    route.clear();

    route.load(":/HelsinkiTikkurila.kml");
    QCOMPARE(route.d->points.count(), 276);
    QCOMPARE(route.wayPointCount(), 22);
    QCOMPARE(route.wayPointText(0),
             UTF8("Head northeast on Brunnsgatan/Kaivokatu toward "
                  "Mannerheimintie/Mannerheimvägen/E12"));
    QCOMPARE(route.wayPointText(1),
             UTF8("Continue onto Kaisaniemenkatu/Kajsaniemigatan"));
    QCOMPARE(route.wayPointText(21),
             UTF8("Arrive at: Tikkurila, Vantaa, Finland"));
    QCOMPARE(route.d->segments.count(), 1);
}

void PathTest::saveGpx()
{
    Path path;

    path.load(":/Lauttasaari.gpx");
    QCOMPARE(path.d->points.count(), 524);

    path.save("/tmp/Lauttasaari.gpx");
}

void PathTest::positionAt()
{
    Path path;

    path.load(":/Lauttasaari.gpx");

    PathPoint &first = path.d->points[0];
    QCOMPARE(path.positionAt(first.time), first);

    PathPoint &last = path.d->points[0];
    QCOMPARE(path.positionAt(last.time), last);

    time_t time =
        QDateTime::fromString("2011-08-25T18:33:02", Qt::ISODate).toTime_t();
    PathPoint p = path.positionAt(time);
    QCOMPARE(p.geo, GeoPoint(60.164066, 24.865551));
}

QTEST_MAIN(PathTest)
