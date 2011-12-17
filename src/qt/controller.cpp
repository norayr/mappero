/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "controller.h"
#include "controller.moc.hpp"
#include "debug.h"
#include "tile-download.h"

using namespace Mappero;

static Controller *controller = 0;

namespace Mappero {
class ControllerPrivate
{
    Q_DECLARE_PUBLIC(Controller)

    ControllerPrivate():
        view(0),
        tileDownload(0)
    {
    }

    View *view;
private:
    mutable Controller *q_ptr;
    mutable TileDownload *tileDownload;
};
};

Controller::Controller(QObject *parent):
    QObject(parent),
    d_ptr(new ControllerPrivate())
{
    Q_ASSERT(controller == 0);

    controller = this;
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

TileDownload *Controller::tileDownload() const
{
    Q_D(const Controller);

    if (d->tileDownload == 0) {
        d->tileDownload = new TileDownload(const_cast<Controller *>(this));
    }

    return d->tileDownload;
}
