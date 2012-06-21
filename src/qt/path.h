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
#include <QSharedDataPointer>
#include <QVector>

class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace Mappero {

class Projection;

struct PathPoint
{
    PathPoint();
    PathPoint(const GeoPoint &p, int altitude = 0);

    bool isValid() { return geo.isValid() && altitude != 0; }
    inline bool operator==(const PathPoint &other) const { return geo == other.geo; }

    GeoPoint geo;
    Point unit;
    time_t time;
    qint8 zoom; /* zoom level at which this point becomes visible */
    qint16 altitude;
    float distance; /* distance from previous point */
};

struct PathWayPoint
{
    PathWayPoint();
    inline PathWayPoint(const QString &desc, int pointIndex);

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

class Kml;
class PathTest;

class PathData: public QSharedData
{
public:
    PathData(): pointsOptimized(0) {}
    inline PathData(const PathData &other);
    ~PathData() {}

    void makeWayPoint(const QString &desc, int pointIndex);
    PathPoint positionAt(time_t time) const;

private:
    friend class Kml;
    friend class Path;
    bool loadGpx(QXmlStreamReader &xml);
    bool saveGpx(QXmlStreamWriter &xml) const;

    void optimize();

public:
    QVector<PathPoint> points;
    QVector<PathWayPoint> wayPoints;
    int pointsOptimized;
};

class Path
{
public:
    Path();
    ~Path();

    bool load(const QString &fileName);
    bool load(QIODevice *device);

    bool save(const QString &fileName) const;
    bool save(QIODevice *device) const;

    bool isEmpty() const;
    const PathPoint &lastPoint() const;

    PathPoint positionAt(time_t time) const;

    void clear();

    void addPoint(const GeoPoint &geo, int altitude, time_t time = 0,
                  Geo distance = 0);

    QPainterPath toPainterPath(int zoomLevel) const;

    static void setProjection(const Projection *projection);

private:
    friend class PathTest;
    friend class Kml;

    QSharedDataPointer<PathData> d;
};

inline PathData::PathData(const PathData &other):
    QSharedData(other), points(other.points), wayPoints(other.wayPoints),
    pointsOptimized(other.pointsOptimized) {}

}; // namespace

#endif /* MAP_PATH_H */
