/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2012-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAPPERO_PATH_H
#define MAPPERO_PATH_H

#include "types.h"

#include <QPainterPath>
#include <QSharedDataPointer>
#include <QVector>

class QIODevice;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace Mappero {

class Projection;

struct MAPPERO_EXPORT PathPoint
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

#define PATH_POINT_KEY_LATITUDE "lat"
#define PATH_POINT_KEY_LONGITUDE "lon"
#define PATH_POINT_KEY_ALTITUDE "altitude"
#define PATH_POINT_KEY_DISTANCE "distance"
#define PATH_POINT_KEY_TIME "time"
#define PATH_POINT_KEY_TEXT "text"
#define PATH_POINT_KEY_DIRECTION "dir"

struct PathWayPoint
{
    PathWayPoint();
    inline PathWayPoint(const QVariantMap &data, int pointIndex);

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
    QVariantMap data;
};

struct PathSegment
{
    PathSegment(int startIndex = 0): startIndex(startIndex) {}
    int startIndex;
};

class Kml;
class PathTest;
class PathStream;

class PathData: public QSharedData
{
public:
    PathData();
    ~PathData() {}

    void addPoint(const PathPoint &point);
    void makeWayPoint(const QString &desc, int pointIndex);
    void makeWayPoint(const QVariantMap &data, int pointIndex);
    bool appendBreak();
    PathPoint positionAt(time_t time) const;
    QRectF boundingRect() const;
    Geo length() const { return m_length; }

    void setSource(const QString &source) { m_source = source; }
    QString source() const { return m_source; }

private:
    friend class Path;
    bool load(QXmlStreamReader &xml, PathStream *stream);

    void optimize();

public:
    QVector<PathPoint> points;
    QVector<PathWayPoint> wayPoints;
    QList<PathSegment> segments;
    QString m_source;
    int pointsOptimized;
    Geo latMin, latMax, lonMin, lonMax;
    Geo m_length;
};

class MAPPERO_EXPORT Path
{
public:
    Path();
    ~Path();

    bool load(const QString &fileName);
    bool load(QIODevice *device);

    bool save(const QString &fileName) const;
    bool save(QIODevice *device) const;

    bool isEmpty() const;
    const PathPoint &firstPoint() const;
    const PathPoint &lastPoint() const;

    int wayPointCount() const;
    const PathPoint &wayPointAt(int index) const;
    QString wayPointText(int index) const;
    QVariantMap wayPointData(int index) const;

    PathPoint positionAt(time_t time) const;
    QRectF boundingRect() const;
    Geo length() const;
    int totalTime() const;

    void clear();

    void addPoint(const GeoPoint &geo, int altitude, time_t time = 0,
                  Geo distance = 0);
    bool addPoint(const QVariantMap &pointData);
    void appendBreak();

    void setSource(const QString &source);
    QString source() const;

    QPainterPath toPainterPath(int zoomLevel) const;

    static void setProjection(const Projection *projection);

private:
    friend class PathTest;
    friend class Kml;

    QSharedDataPointer<PathData> d;
};

class MAPPERO_EXPORT PathStream
{
public:
    PathStream();
    virtual ~PathStream();

    virtual bool read(QXmlStreamReader &xml, PathData &pathData) = 0;
    virtual bool write(QXmlStreamWriter &xml, const PathData &pathData) = 0;
};

}; // namespace

Q_DECLARE_METATYPE(Mappero::Path)

#endif /* MAPPERO_PATH_H */
