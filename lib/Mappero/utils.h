/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAPPERO_UTILS_H
#define MAPPERO_UTILS_H

#include "types.h"

#include <QObject>

namespace Mappero {

class Utils: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal uiScale READ uiScale CONSTANT)
    Q_PROPERTY(GeoPoint invalidGeoPoint READ invalidGeoPoint CONSTANT)

public:
    Utils(QObject *parent = 0);
    ~Utils();

    qreal uiScale() const;
    GeoPoint invalidGeoPoint() const { return GeoPoint(); }

public Q_SLOTS:
    GeoPoint geo(qreal lat, qreal lon) { return GeoPoint(lat, lon); }
    QPointF point(const GeoPoint &geo) { return geo.toPointF(); }
    bool isValid(const GeoPoint &geo) { return geo.isValid(); }
    QString formatOffset(int seconds);
    QString formatLength(qreal metres);
    QString formatDuration(int ms);
};

};


#endif /* MAPPERO_UTILS_H */
