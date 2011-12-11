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
#include "map.h"
#include "tiled-layer.h"

#include <QObject>

using namespace Mappero;

namespace Mappero {
class LayerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Layer)

    LayerPrivate(Layer *layer, const QString &id, const Projection *projection):
        QObject(),
        q_ptr(layer),
        id(id),
        projection(projection),
        map(0)
    {
    }

    void setMap(Map *newMap);

private Q_SLOTS:
    void onMapChanged();

private:
    mutable Layer *q_ptr;
    QString id;
    const Projection *projection;
    Map *map;
};
};

void LayerPrivate::setMap(Map *newMap)
{
    Q_Q(Layer);
    if (map != newMap) {
        map = newMap;
        q->setParentItem(map);
        QObject::connect(map, SIGNAL(centerChanged(const GeoPoint&)),
                         this, SLOT(onMapChanged()), Qt::QueuedConnection);
        QObject::connect(map, SIGNAL(zoomLevelChanged(qreal)),
                         this, SLOT(onMapChanged()), Qt::QueuedConnection);
        onMapChanged();
    }
}

void LayerPrivate::onMapChanged()
{
    Q_Q(Layer);
    q->mapChanged();
}

Layer::Layer(const QString &id, const Projection *projection):
    QGraphicsItem(0),
    d_ptr(new LayerPrivate(this, id, projection))
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

void Layer::setMap(Map *map)
{
    Q_D(Layer);
    d->setMap(map);
}

Map *Layer::map() const
{
    Q_D(const Layer);
    return d->map;
}

QRectF Layer::boundingRect() const
{
    return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6);
}

void Layer::mapChanged()
{
    // virtual method
}

#include "layer.moc.cpp"
