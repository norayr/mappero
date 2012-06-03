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

    Path route;
    route.load(":/Route.gpx");
    QCOMPARE(route.d->points.count(), 1509);
    QCOMPARE(route.d->wayPoints.count(), 15);
}

void PathTest::loadKml()
{
    Path route;
    route.load(":/Route.kml");
    QCOMPARE(route.d->points.count(), 2105);
    QCOMPARE(route.d->wayPoints.count(), 2);
    QCOMPARE(route.d->wayPoints[0].desc, UTF8("Vilhonkatu 13, Helsinki"));
    QCOMPARE(route.d->wayPoints[1].desc,
             UTF8("Helsinki-vantaan Lentoasema 2, Vantaa"));

    route.clear();

    route.load(":/HelsinkiTikkurila.kml");
    QCOMPARE(route.d->points.count(), 276);
    QCOMPARE(route.d->wayPoints.count(), 22);
    QCOMPARE(route.d->wayPoints[0].desc,
             UTF8("Head northeast on Brunnsgatan/Kaivokatu toward "
                  "Mannerheimintie/Mannerheimvägen/E12"));
    QCOMPARE(route.d->wayPoints[1].desc,
             UTF8("Continue onto Kaisaniemenkatu/Kajsaniemigatan"));
    QCOMPARE(route.d->wayPoints[21].desc,
             UTF8("Arrive at: Tikkurila, Vantaa, Finland"));
}

void PathTest::saveGpx()
{
    Path path;

    path.load(":/Lauttasaari.gpx");
    QCOMPARE(path.d->points.count(), 524);

    path.save("/tmp/Lauttasaari.gpx");
}

QTEST_MAIN(PathTest)
