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
};

} // namespace

inline PathItemPrivate::PathItemPrivate(PathItem *tracker):
    q_ptr(tracker)
{
}

PathItem::PathItem(QObject *parent):
    QObject(parent),
    d_ptr(new PathItemPrivate(this))
{
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
    Q_EMIT colorChanged();
}

QColor PathItem::color() const
{
    Q_D(const PathItem);
    return d->color;
}

void PathItem::loadFile(const QString &fileName)
{
    Path path;
    path.load(fileName);
    setPath(path);
}

void PathItem::saveFile(const QString &fileName) const
{
    path().save(fileName);
}
