/*
 * Copyright (C) 2013 Alberto Mardegan <mardy@users.sourceforge.net>
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

void PathBuilder::addPoint(const QVariantMap &pointData)
{
    Q_D(PathBuilder);

    static const QString keyLat = QStringLiteral("lat");
    static const QString keyLon = QStringLiteral("lon");
    static const QString keyAltitude = QStringLiteral("altitude");
    static const QString keyTime = QStringLiteral("time");

    if (!pointData.contains(keyLat) || !pointData.contains(keyLon)) return;

    time_t time = 0;
    if (pointData.contains(keyTime)) {
        QVariant vTime = pointData[keyTime];
        if (vTime.canConvert(QMetaType::Int)) {
            time = vTime.toInt();
        } else if (vTime.canConvert(QMetaType::QDateTime)) {
            time = vTime.toDateTime().toMSecsSinceEpoch();
        }
    }

    d->path.addPoint(GeoPoint(pointData[keyLat].toReal(),
                              pointData[keyLon].toReal()),
                     pointData[keyAltitude].toInt(),
                     time);

    // TODO: support adding waypoints

    Q_EMIT pathChanged();
}
