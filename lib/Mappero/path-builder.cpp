/*
 * Copyright (C) 2013-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "path-builder.h"

#include <QDateTime>

using namespace Mappero;

namespace Mappero {

class PathBuilderPrivate
{
    Q_DECLARE_PUBLIC(PathBuilder)

    inline PathBuilderPrivate(PathBuilder *q);
    ~PathBuilderPrivate() {};

private:
    mutable PathBuilder *q_ptr;
    Path path;
    QString source;
};

} // namespace

PathBuilderPrivate::PathBuilderPrivate(PathBuilder *q):
    q_ptr(q)
{
}

PathBuilder::PathBuilder(QObject *parent):
    QObject(parent),
    d_ptr(new PathBuilderPrivate(this))
{
}

PathBuilder::~PathBuilder()
{
    delete d_ptr;
}

Path PathBuilder::path() const
{
    Q_D(const PathBuilder);
    return d->path;
}

void PathBuilder::setSource(const QString &source)
{
    Q_D(PathBuilder);
    d->source = source;
    d->path.setSource(source);
    Q_EMIT sourceChanged();
}

QString PathBuilder::source() const
{
    Q_D(const PathBuilder);
    return d->source;
}

void PathBuilder::clear()
{
    Q_D(PathBuilder);
    d->path.clear();
    d->path.setSource(d->source);
    Q_EMIT pathChanged();
}

void PathBuilder::addPoint(const QVariantMap &pointData)
{
    Q_D(PathBuilder);

    if (d->path.addPoint(pointData)) {
        Q_EMIT pathChanged();
    }
}
