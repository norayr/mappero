/* vi: set et sw=4 ts=8 cino=t0,(0: */
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
#include "gps.h"
#include "tracker.h"

/* uncertainty below which a GPS position is considered to be precise enough
 * to be added to the track (in metres) */
#define MAX_UNCERTAINTY 200

/* minimum distance the GPS position must have from the previous track point in
 * order to be registered in the track (in metres) */
#define MIN_TRACK_DISTANCE 10

using namespace Mappero;

namespace Mappero {

class TrackerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Tracker)

public:
    inline TrackerPrivate(Tracker *tracker);
    TrackerPrivate() {}

    void setGps(Gps *gps);

private:
    bool pointIsSignificant(const GpsPosition &position);

private Q_SLOTS:
    void onPositionUpdated(const GpsPosition &position);

private:
    mutable Tracker *q_ptr;
    bool isTracking;
    Geo maximumUncertainty;
    Geo minimumDistance;
    Geo distanceFromLast;
    Path track;
};

} // namespace

inline TrackerPrivate::TrackerPrivate(Tracker *tracker):
    QObject(tracker),
    q_ptr(tracker),
    isTracking(false),
    maximumUncertainty(MAX_UNCERTAINTY),
    minimumDistance(MIN_TRACK_DISTANCE),
    distanceFromLast(0)
{
    Gps *gps = Gps::instance();
    QObject::connect(gps, SIGNAL(positionUpdated(const GpsPosition &)),
                     this, SLOT(onPositionUpdated(const GpsPosition &)));
}

bool TrackerPrivate::pointIsSignificant(const GpsPosition &position)
{
    if (track.isEmpty()) return true;

    /* TODO */
#if 0
    /* check if the track was in a break */
    map_path_line_iter_last(path, &line);
    if (map_path_line_len(&line) == 0)
        return TRUE;
#endif
    const PathPoint &last = track.lastPoint();

    if (position.time().toTime_t() - last.time > 60) return true;

    distanceFromLast = position.geo().distanceTo(last.geo);
    if (distanceFromLast < minimumDistance) return false;

    return true;
}

void TrackerPrivate::onPositionUpdated(const GpsPosition &position)
{
    Q_Q(Tracker);

    if (!isTracking) return;

    if (!(position.validFields() & GpsPosition::Coordinates) ||
        position.accuracy() > maximumUncertainty)
        return;

    distanceFromLast = -1;
    if (!pointIsSignificant(position)) return;

    track.addPoint(position.geo(), position.altitude(),
                   position.time().toTime_t(),
                   distanceFromLast);
    Q_EMIT q->trackChanged();
}

Tracker::Tracker(QObject *parent):
    QObject(parent),
    d_ptr(new TrackerPrivate(this))
{
}

Tracker::~Tracker()
{
    delete d_ptr;
}

void Tracker::setTracking(bool isTracking)
{
    Q_D(Tracker);
    if (d->isTracking != isTracking) {
        d->isTracking = isTracking;
        Q_EMIT trackingChanged();
    }
}

bool Tracker::isTracking() const
{
    Q_D(const Tracker);
    return d->isTracking;
}

void Tracker::setTrack(const Path &track)
{
    Q_D(Tracker);
    d->track = track;
    Q_EMIT trackChanged();
}

Path Tracker::track() const
{
    Q_D(const Tracker);
    return d->track;
}

void Tracker::loadFile(const QString &fileName)
{
    Path path;
    path.load(fileName);
    setTrack(path);
}

void Tracker::saveFile(const QString &fileName) const
{
    track().save(fileName);
}

#include "tracker.moc"
