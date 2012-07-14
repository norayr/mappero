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

#include "controller.h"
#include "geotag-test.h"

#include "taggable.h"
#include <QDebug>

#define UTF8(s) QString::fromUtf8(s)

#define JPEG_FILE UTF8("12060025.jpg")
#define ARW_FILE UTF8("a700.arw")

using namespace Mappero;

qint64 Controller::clock() { return 0; }

void GeotagTest::initTestCase()
{
}

void GeotagTest::cleanupTestCase()
{
}

void GeotagTest::loadTaggable()
{
    Taggable *taggable = new Taggable(this);

    taggable->setFileName(JPEG_FILE);
    QCOMPARE(taggable->fileName(), JPEG_FILE);
    QCOMPARE(QDateTime::fromTime_t(taggable->time()),
             QDateTime::fromString("Thu Jun 21 16:10:14 2012"));

    taggable->setFileName(ARW_FILE);
    QCOMPARE(taggable->fileName(), ARW_FILE);
}

void GeotagTest::pixmap()
{
    Taggable *taggable = new Taggable(this);

    taggable->setFileName(ARW_FILE);

    QSize imageSize;
    QPixmap pixmap;

    pixmap = taggable->pixmap(&imageSize, QSize(10, 10));
    QCOMPARE(imageSize, QSize(160, 120));

    pixmap = taggable->pixmap(&imageSize, QSize(200, 200));
    QCOMPARE(imageSize, QSize(1616, 1080));
}

QTEST_MAIN(GeotagTest)
