/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef MAP_PATH_H
#define MAP_PATH_H

#include "types.h"

#include <QPainterPath>
#include <QVector>

class QIODevice;
class QXmlStreamReader;

namespace Mappero {

struct PathPoint
{
    inline PathPoint();
    inline PathPoint(const Point &p);

    Point unit;
    time_t time;
    qint8 zoom; /* zoom level at which this point becomes visible */
    qint16 altitude;
    float distance; /* distance from previous point */
};

struct PathWayPoint
{
    /* Navigation directions */
    enum Direction {
        DIRECTION_UNKNOWN = 0,
        DIRECTION_CS, /* continue straight */
        DIRECTION_TR, /* turn right */
        DIRECTION_TL,
        DIRECTION_STR, /* slight turn right */
        DIRECTION_STL,
        DIRECTION_EX1, /* first exit */
        DIRECTION_EX2,
        DIRECTION_EX3,
        DIRECTION_LAST,
    };

    int pointIndex;
    Direction dir;
    QString desc;
};

class PathTest;

class Path
{
public:
    Path();
    ~Path();

    bool load(const QString &fileName);
    bool load(QIODevice *device);

    QPainterPath toPainterPath(int zoomLevel) const;

private:
    friend class PathTest;
    bool loadGpx(QXmlStreamReader &xml);

    QVector<PathPoint> points;
    QVector<PathWayPoint> wayPoints;
};

}; // namespace

#endif /* MAP_PATH_H */
