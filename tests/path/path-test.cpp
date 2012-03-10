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
    QCOMPARE(path.points.count(), 524);
}

QTEST_MAIN(PathTest)
