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
#include "tiled-layer.h"


using namespace Mappero;

namespace Mappero {
class LayerPrivate
{
    Q_DECLARE_PUBLIC(Layer)

    LayerPrivate(const QString &id, const Projection *projection):
        id(id),
        projection(projection)
    {
    }

    QString id;
    const Projection *projection;
private:
    mutable Layer *q_ptr;
};
};

Layer::Layer(const QString &id, const Projection *projection):
    QGraphicsItem(0),
    d_ptr(new LayerPrivate(id, projection))
{
}

Layer::~Layer()
{
    delete d_ptr;
}

Layer *Layer::fromId(const QString &id)
{
    /* TODO: load available layers from config file/plugins */
    if (id == "OpenStreetMap I") {
        const Mappero::TiledLayer::Type *type =
            Mappero::TiledLayer::Type::get("XYZ_INV");
        return new TiledLayer("OpenStreet", "OpenStreetMap I",
                              "http://tile.openstreetmap.org/%0d/%d/%d.png",
                              "png", type);
    } else {
        return 0;
    }
}

QString Layer::id() const
{
    Q_D(const Layer);
    return d->id;
}

const Projection *Layer::projection()
{
    Q_D(Layer);
    return d->projection;
}

Map *Layer::map() const
{
    return (Map *)parentObject();
}

QRectF Layer::boundingRect() const
{
    return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6);
}

