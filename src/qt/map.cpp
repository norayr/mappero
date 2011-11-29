/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2010-2011 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "projection.h"

#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>

using namespace Mappero;

namespace Mappero {
class MapPrivate
{
    Q_DECLARE_PUBLIC(Map)

    MapPrivate(Map *q):
        mainLayer(0),
        q_ptr(q)
    {
    }

    Layer *mainLayer;
    GeoPoint center;
    Point centerUnits;
private:
    mutable Map *q_ptr;
};
};

Map::Map():
    QGraphicsItem(0),
    d_ptr(new MapPrivate(this))
{
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
}

Map::~Map()
{
    delete d_ptr;
}

void Map::setMainLayer(Layer *layer)
{
    Q_D(Map);

    if (d->mainLayer != 0) {
        scene()->removeItem(d->mainLayer);
        delete d->mainLayer;
    }
    d->mainLayer = layer;
    if (layer != 0) {
        layer->setParentItem(this);
    }
}

void Map::setCenter(const GeoPoint &center)
{
    Q_D(Map);

    d->center = center;
    if (d->mainLayer != 0) {
        const Projection *projection = d->mainLayer->projection();
        Unit x, y;
        projection->geoToUnit(center.lat, center.lon, &x, &y);

        DEBUG() << "Transformed:" << x << y;
        d->centerUnits = Point(x, y);
    }
}

GeoPoint Map::center() const
{
    Q_D(const Map);
    return d->center;
}

Point Map::centerUnits() const
{
    Q_D(const Map);
    return d->centerUnits;
}

QRectF Map::boundingRect() const
{
    return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6);
}

void Map::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget)
{
    DEBUG() << option->exposedRect;
}

