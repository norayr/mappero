/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2010-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "configuration.h"
#include "controller.h"
#include "debug.h"
#include "tile-cache.h"
#include "tile-download.h"

#include <Mappero/Projection>
#include <QElapsedTimer>
#include <QStringList>
#include <cstdlib>

using namespace Mappero;

static Controller *controller = 0;
static QElapsedTimer elapsedTimer;

namespace Mappero {
class ControllerPrivate
{
    Q_DECLARE_PUBLIC(Controller)

    ControllerPrivate():
        projection(Projection::get(Projection::GOOGLE)),
        tileDownload(0),
        tileCache(0),
        configuration(0),
        uiScale(1.0)
    {
        /* Ubuntu Touch might define this */
        float uiScaleTest = qgetenv("GRID_UNIT_PX").toFloat();
        if (uiScaleTest != 0.0) {
            uiScale = uiScaleTest / 8;
        }
    }

    ~ControllerPrivate()
    {
        delete tileCache;
        tileCache = 0;
    }

    const Projection *projection;
private:
    mutable Controller *q_ptr;
    mutable TileDownload *tileDownload;
    mutable TileCache *tileCache;
    mutable Configuration *configuration;
    qreal uiScale;
};
};

Controller::Controller(QObject *parent):
    QObject(parent),
    d_ptr(new ControllerPrivate())
{
    Q_ASSERT(controller == 0);

    controller = this;
    elapsedTimer.start();
}

Controller::~Controller()
{
    delete d_ptr;
    controller = 0;
}

Controller *Controller::instance()
{
    return controller;
}

void Controller::setProjection(const Projection *projection)
{
    Q_D(Controller);
    d->projection = projection;
}

const Projection *Controller::projection() const
{
    Q_D(const Controller);
    return d->projection;
}

TileDownload *Controller::tileDownload() const
{
    Q_D(const Controller);

    if (d->tileDownload == 0) {
        d->tileDownload = new TileDownload(const_cast<Controller *>(this));
    }

    return d->tileDownload;
}

TileCache *Controller::tileCache() const
{
    Q_D(const Controller);

    if (d->tileCache == 0) {
        d->tileCache = new TileCache();
    }

    return d->tileCache;
}

Configuration *Controller::configuration() const
{
    Q_D(const Controller);

    if (d->configuration == 0) {
        d->configuration = new Configuration(const_cast<Controller *>(this));
    }

    return d->configuration;
}

qint64 Controller::clock()
{
    return elapsedTimer.elapsed();
}

QString Controller::formatOffset(int seconds)
{
    if (seconds / 30 == 0) return "=";
    if (seconds > 0) {
        return QString::fromLatin1("+%1:00").arg((seconds + 30) / 60);
    } else {
        return QString::fromLatin1("%1:00").arg((seconds - 30) / 60);
    }
}

QString Controller::formatLength(qreal metres)
{
    const char *unit;
    int decimals;

    if (metres >= 1000) {
        metres /= 1000;
        decimals = 1;
        unit = "km";
    } else {
        decimals = 0;
        unit = "m";
    }
    return QString::fromLatin1("%1%2").arg(metres, 0, 'f', decimals).arg(unit);
}

QString Controller::formatDuration(int ms)
{
    int seconds = ms / 1000;

    div_t d = div(seconds, 60 * 60 * 24);
    int days = d.quot;
    seconds = d.rem;

    d = div(seconds, 60 * 60);
    int hours = d.quot;
    seconds = d.rem;

    d = div(seconds, 60);
    int minutes = d.quot;
    seconds = d.rem;

    QStringList components;
    if (days > 0) {
        components.append(tr("%1 d").arg(days));
    }
    if (hours > 0) {
        components.append(tr("%1 h").arg(hours));
    }
    components.append(tr("%1 min").arg(minutes));
    if (days == 0 && hours == 0 && seconds > 0) {
        components.append(tr("%1 sec").arg(seconds));
    }
    return components.join(tr(", ", "time components separator"));
}

qreal Controller::uiScale() const
{
    Q_D(const Controller);
    return d->uiScale;
}
