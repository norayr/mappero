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
#include "debug.h"
#include "layer.h"


using namespace Mappero;

namespace Mappero {
class LayerPrivate
{
    Q_DECLARE_PUBLIC(Layer)

    LayerPrivate(const Projection *projection):
        projection(projection)
    {
    }

    const Projection *projection;
private:
    mutable Layer *q_ptr;
};
};

Layer::Layer(const Projection *projection):
    QGraphicsItem(0),
    d_ptr(new LayerPrivate(projection))
{
}

Layer::~Layer()
{
    delete d_ptr;
}

const Projection *Layer::projection()
{
    Q_D(Layer);
    return d->projection;
}

QRectF Layer::boundingRect() const
{
    return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6);
}

