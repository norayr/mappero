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
#include "debug.h"
#include "layer.h"
#include "map.h"
#include "tiled-layer.h"

#include <QObject>

using namespace Mappero;

static QHash<QString,Layer*> layerIdMap;

namespace Mappero {
class LayerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Layer)

    LayerPrivate(Layer *layer):
        QObject(),
        q_ptr(layer),
        projection(0),
        minZoom(MIN_ZOOM),
        maxZoom(MAX_ZOOM),
        layerChangedQueued(false)
    {
    }

private Q_SLOTS:
    void emitLayerChanged();

private:
    mutable Layer *q_ptr;
    QString id;
    QString name;
    const Projection *projection;
    int minZoom;
    int maxZoom;
    bool layerChangedQueued;
};
};

void LayerPrivate::emitLayerChanged()
{
    Q_Q(Layer);
    layerChangedQueued = false;
    Q_EMIT q->layerChanged();
}

Layer::Layer():
    MapItem(),
    d_ptr(new LayerPrivate(this))
{
}

Layer::~Layer()
{
    Q_D(Layer);
    if (!d->id.isEmpty()) {
        layerIdMap.remove(d->id);
    }
    delete d_ptr;
}

Layer *Layer::find(const QString &id)
{
    return layerIdMap.value(id);
}

void Layer::setId(const QString &id)
{
    Q_D(Layer);

    if (id == d->id) return;

    if (!d->id.isEmpty()) {
        layerIdMap.remove(d->id);
    }
    d->id = id;
    if (!id.isEmpty()) {
        layerIdMap.insert(id, this);
    }
    queueLayerChanged();
}

QString Layer::id() const
{
    Q_D(const Layer);
    return d->id;
}

void Layer::setName(const QString &name)
{
    Q_D(Layer);
    d->name = name;
    queueLayerChanged();
}

QString Layer::name() const
{
    Q_D(const Layer);
    return d->name;
}

void Layer::setProjection(const Projection *projection)
{
    Q_D(Layer);
    d->projection = projection;
    queueLayerChanged();
}

const Projection *Layer::projection() const
{
    Q_D(const Layer);
    return d->projection;
}

void Layer::setMinZoom(int minZoom)
{
    Q_D(Layer);
    d->minZoom = minZoom;
    queueLayerChanged();
}

int Layer::minZoom() const
{
    Q_D(const Layer);
    return d->minZoom;
}

void Layer::setMaxZoom(int maxZoom)
{
    Q_D(Layer);
    d->maxZoom = maxZoom;
    queueLayerChanged();
}

int Layer::maxZoom() const
{
    Q_D(const Layer);
    return d->maxZoom;
}

QRectF Layer::boundingRect() const
{
    return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6);
}

void Layer::mapEvent(MapEvent *e)
{
    Q_UNUSED(e);
    // virtual method
}

void Layer::queueLayerChanged()
{
    Q_D(Layer);

    if (!d->layerChangedQueued) {
        d->layerChangedQueued = true;
        QMetaObject::invokeMethod(d, "emitLayerChanged", Qt::QueuedConnection);
    }
}

#include "layer.moc"
