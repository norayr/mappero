/* vi: set et sw=4 ts=4 cino=t0,(0: */
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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "debug.h"
#include "gps.h"

#ifdef QT_LOCATION_LIB
#define HAS_QT_LOCATION
#endif

#ifdef HAS_QT_LOCATION
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#endif

#include <QStringList>

using namespace Mappero;

static Gps *m_instance = 0;

namespace Mappero {

class GpsPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Gps)

    GpsPrivate(Gps *gps):
        QObject(gps),
        q_ptr(gps),
#ifdef HAS_QT_LOCATION
        source(0),
#endif
        updateInterval(0),
        isActive(false)
    {}
    ~GpsPrivate() {};

#ifdef HAS_QT_LOCATION
private:
    void setupSource();

private Q_SLOTS:
    void onPositionUpdated(const QGeoPositionInfo &update);
#endif

private:
    mutable Gps *q_ptr;
#ifdef HAS_QT_LOCATION
    QGeoPositionInfoSource *source;
#endif
    QString sourceName;
    int updateInterval;
    GpsPosition pos;
    bool isActive;
};
}; // namespace

QDebug operator<<(QDebug dbg, const GpsPosition &p)
{
    dbg.nospace() << "(" << p.latitude() << ", " << p.longitude() <<
        " e" << p.accuracy() <<
        ", " << p.altitude() << " e" << p.verticalAccuracy();

    if (p.validFields() & GpsPosition::Direction) {
        dbg.nospace() << " dir:" << p.direction();
    }
    if (p.validFields() & GpsPosition::Speed) {
        dbg.nospace() << " spd:" << p.speed();
    }
    dbg.nospace() << ")";
    return dbg.space();
}

#ifdef HAS_QT_LOCATION
void GpsPrivate::setupSource()
{
    if (source != 0) return;

    DEBUG() << "available sources:" <<
        QGeoPositionInfoSource::availableSources();
    source = sourceName.isEmpty() ?
        QGeoPositionInfoSource::createDefaultSource(this) :
        QGeoPositionInfoSource::createSource(sourceName, this);
    if (source == 0) {
        qWarning() << "Couldn't create GPS source" << sourceName;
        return;
    }

    QObject::connect(source, SIGNAL(positionUpdated(const QGeoPositionInfo&)),
                     this, SLOT(onPositionUpdated(const QGeoPositionInfo&)));
}

void GpsPrivate::onPositionUpdated(const QGeoPositionInfo &update)
{
    Q_Q(Gps);

    if (!update.isValid()) return;

    pos.m_validFields = GpsPosition::Coordinates;
    QGeoCoordinate coordinates = update.coordinate();
    pos.m_latitude = coordinates.latitude();
    pos.m_longitude = coordinates.longitude();
    pos.m_altitude = coordinates.altitude();
    pos.m_accuracy =
        update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) ?
        update.attribute(QGeoPositionInfo::HorizontalAccuracy) : 1000;
    pos.m_verticalAccuracy =
        update.hasAttribute(QGeoPositionInfo::VerticalAccuracy) ?
        update.attribute(QGeoPositionInfo::VerticalAccuracy) : 1000;

    pos.m_time = update.timestamp().toUTC();
    if (update.hasAttribute(QGeoPositionInfo::Direction)) {
        pos.m_validFields |= GpsPosition::Direction;
        pos.m_direction = update.attribute(QGeoPositionInfo::Direction);
        if (update.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
            pos.m_validFields |= GpsPosition::Speed;
            pos.m_speed = update.attribute(QGeoPositionInfo::GroundSpeed);
        }
    }

    if (update.hasAttribute(QGeoPositionInfo::VerticalSpeed)) {
        pos.m_validFields |= GpsPosition::VerticalSpeed;
        pos.m_verticalSpeed =
            update.attribute(QGeoPositionInfo::VerticalSpeed);
    }

    Q_EMIT q->positionUpdated(pos);
}
#endif

Gps::Gps(QObject *parent):
    QObject(parent),
    d_ptr(new GpsPrivate(this))
{
    Q_ASSERT(m_instance == 0);
    m_instance = this;
}

Gps::~Gps()
{
    m_instance = 0;
    delete d_ptr;
}

Gps *Gps::instance()
{
    if (m_instance == 0) {
        m_instance = new Gps();
    }
    return m_instance;
}

void Gps::setSourceName(const QString &sourceName)
{
    Q_D(Gps);

    if (sourceName == d->sourceName) return;

#ifdef HAS_QT_LOCATION
    // TODO: handle changing source on the fly
    if (d->source != 0) {
        qWarning() << "Ignoring GPS source change while GPS running";
        return;
    }
#endif
    d->sourceName = sourceName;
}

QString Gps::sourceName() const
{
    Q_D(const Gps);
    return d->sourceName;
}

void Gps::setUpdateInterval(int interval)
{
    Q_D(Gps);

    if (interval == d->updateInterval) return;

    d->updateInterval = interval;
#ifdef HAS_QT_LOCATION
    if (d->source != 0)
        d->source->setUpdateInterval(interval * 1000);
#endif
}

int Gps::updateInterval() const
{
    Q_D(const Gps);
    return d->updateInterval;
}

bool Gps::isActive() const
{
    Q_D(const Gps);
    return d->isActive;
}

void Gps::start()
{
#ifdef HAS_QT_LOCATION
    Q_D(Gps);

    if (d->source == 0)
        d->setupSource();

    if (d->source != 0) {
        d->source->startUpdates();
        d->isActive = true;
        Q_EMIT activated(true);
    }
#endif
}

void Gps::stop()
{
    Q_D(Gps);
#ifdef HAS_QT_LOCATION
    if (d->source != 0)
        d->source->stopUpdates();
    d->isActive = false;
#endif
    Q_EMIT activated(d->isActive);
}

#include "gps.moc"
