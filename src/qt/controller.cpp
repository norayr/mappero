/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2010-2012 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "projection.h"
#include "tile-cache.h"
#include "tile-download.h"

#include <QElapsedTimer>

using namespace Mappero;

static Controller *controller = 0;
static QElapsedTimer elapsedTimer;

namespace Mappero {
class ControllerPrivate
{
    Q_DECLARE_PUBLIC(Controller)

    ControllerPrivate():
        projection(Projection::get(Projection::GOOGLE)),
        view(0),
        tileDownload(0),
        tileCache(0),
        configuration(0)
    {
    }

    ~ControllerPrivate()
    {
        delete tileCache;
        tileCache = 0;
    }

    const Projection *projection;
    View *view;
private:
    mutable Controller *q_ptr;
    mutable TileDownload *tileDownload;
    mutable TileCache *tileCache;
    mutable Configuration *configuration;
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

void Controller::setView(View *view)
{
    Q_D(Controller);
    Q_ASSERT(d->view == 0);
    d->view = view;
}

View *Controller::view() const
{
    Q_D(const Controller);
    return d->view;
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
