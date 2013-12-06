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
#include "path-item.h"

#include <QDateTime>
#include <QUrl>

using namespace Mappero;

namespace Mappero {

class PathItemPrivate
{
    Q_DECLARE_PUBLIC(PathItem)

public:
    inline PathItemPrivate(PathItem *tracker);
    PathItemPrivate() {}

private:
    mutable PathItem *q_ptr;
    Path path;
    QColor color;
    qreal opacity;
    int offset;
};

} // namespace

inline PathItemPrivate::PathItemPrivate(PathItem *tracker):
    q_ptr(tracker),
    opacity(1.0),
    offset(0)
{
}

PathItem::PathItem(QObject *parent):
    QObject(parent),
    d_ptr(new PathItemPrivate(this))
{
    QObject::connect(this, SIGNAL(pathChanged()),
                     this, SIGNAL(timesChanged()));
}

PathItem::~PathItem()
{
    delete d_ptr;
}

void PathItem::setPath(const Path &path)
{
    Q_D(PathItem);
    d->path = path;
    Q_EMIT pathChanged();
}

Path PathItem::path() const
{
    Q_D(const PathItem);
    return d->path;
}

Path &PathItem::path()
{
    Q_D(PathItem);
    return d->path;
}

void PathItem::setColor(const QColor &color)
{
    Q_D(PathItem);
    d->color = color;
    d->color.setAlphaF(d->opacity);
    Q_EMIT colorChanged();
}

QColor PathItem::color() const
{
    Q_D(const PathItem);
    return d->color;
}

void PathItem::setOpacity(qreal opacity)
{
    Q_D(PathItem);
    d->opacity = opacity;
    d->color.setAlphaF(opacity);
    Q_EMIT opacityChanged();
}

qreal PathItem::opacity() const
{
    Q_D(const PathItem);
    return d->opacity;
}

bool PathItem::isEmpty() const
{
    Q_D(const PathItem);
    return d->path.isEmpty();
}

void PathItem::setTimeOffset(int offset)
{
    Q_D(PathItem);

    if (offset == d->offset) return;

    d->offset = offset;
    Q_EMIT timeOffsetChanged();
    Q_EMIT timesChanged();
}

int PathItem::timeOffset() const
{
    Q_D(const PathItem);
    return d->offset;
}


QDateTime PathItem::startTime() const
{
    Q_D(const PathItem);
    if (d->path.isEmpty()) return QDateTime();

    return QDateTime::fromTime_t(d->path.firstPoint().time).addSecs(d->offset);
}

QDateTime PathItem::endTime() const
{
    Q_D(const PathItem);
    if (d->path.isEmpty()) return QDateTime();

    return QDateTime::fromTime_t(d->path.lastPoint().time).addSecs(d->offset);
}

qreal PathItem::length() const
{
    Q_D(const PathItem);
    return d->path.length();
}

GeoPoint PathItem::positionAt(const QDateTime &time) const
{
    Q_D(const PathItem);

    time_t t = time.addSecs(-d->offset).toTime_t();
    PathPoint p = d->path.positionAt(t);
    return p.geo;
}

QRectF PathItem::itemArea() const
{
    Q_D(const PathItem);
    return d->path.boundingRect();
}

void PathItem::clear()
{
    Path path;
    setPath(path);
}

void PathItem::loadFile(const QUrl &fileName)
{
    Path path;
    path.load(fileName.toLocalFile());
    setPath(path);
}

void PathItem::saveFile(const QUrl &fileName) const
{
    path().save(fileName.toLocalFile());
}
