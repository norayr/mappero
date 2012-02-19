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

#ifndef MAP_GPS_H
#define MAP_GPS_H

#include "types.h"

#include <QDateTime>
#include <QDebug>
#include <QFlags>
#include <QObject>

namespace Mappero {

class GpsPrivate;

class GpsPosition
{
public:
    enum Field {
        Coordinates = 1 << 0,
        Direction = 1 << 1,
        Speed = 1 << 2,
        VerticalSpeed = 1 << 3,
    };
    Q_DECLARE_FLAGS(Fields, Field)

    Fields validFields() const { return m_validFields; }
    const QDateTime &time() const { return m_time; }
    qreal latitude() const { return m_latitude; }
    qreal longitude() const { return m_longitude; }
    GeoPoint geo() const { return GeoPoint(m_latitude, m_longitude); }
    qreal altitude() const { return m_altitude; }
    qreal direction() const { return m_direction; }
    qreal speed() const { return m_speed; }
    qreal verticalSpeed() const { return m_verticalSpeed; }
    qreal accuracy() const { return m_accuracy; }
    qreal verticalAccuracy() const { return m_verticalAccuracy; }

private:
    friend class GpsPrivate;
    QDateTime m_time;
    qreal m_latitude;
    qreal m_longitude;
    qreal m_altitude;
    qreal m_direction;
    qreal m_speed;
    qreal m_verticalSpeed;
    qreal m_accuracy;
    qreal m_verticalAccuracy;
    Fields m_validFields;
};

class Gps: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int updateInterval \
               READ updateInterval WRITE setUpdateInterval);
    Q_PROPERTY(QString sourceName READ sourceName WRITE setSourceName);
    Q_PROPERTY(bool active READ isActive NOTIFY activated);

public:
    Gps(QObject *parent = 0);
    ~Gps();

    void setSourceName(const QString &sourceName);
    QString sourceName() const;

    void setUpdateInterval(int interval);
    int updateInterval() const;

    bool isActive() const;

public Q_SLOTS:
    void start();
    void stop();

Q_SIGNALS:
    void positionUpdated(const GpsPosition &position);
    void activated(bool isActive);

private:
    GpsPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Gps)
};

}; // namespace

QDebug operator<<(QDebug dbg, const Mappero::GpsPosition &p);

#endif /* MAP_GPS_H */
