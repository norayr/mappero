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

#ifndef MAP_TRACKER_H
#define MAP_TRACKER_H

#include "path.h"

#include <QObject>

namespace Mappero {

class Gps;

class TrackerPrivate;
class Tracker: public QObject
{
    Q_OBJECT
    Q_PROPERTY(Path track READ track WRITE setTrack NOTIFY trackChanged);
    Q_PROPERTY(bool tracking READ isTracking WRITE setTracking \
               NOTIFY trackingChanged);

public:
    Tracker(QObject *parent = 0);
    ~Tracker();

    void setGps(Gps *gps);

    void setTracking(bool isTracking);
    bool isTracking() const;

    void setTrack(const Path &track);
    Path track() const;

public Q_SLOTS:
    void loadFile(const QString &fileName);
    void saveFile(const QString &fileName) const;

Q_SIGNALS:
    void trackingChanged();
    void trackChanged();

private:
    TrackerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Tracker)
};

}; // namespace

#endif /* MAP_TRACKER_H */
