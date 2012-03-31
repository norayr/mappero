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
#include "map.h"
#include "path-item.h"
#include "path.h"

#include <QPainter>
#include <QPen>

using namespace Mappero;

namespace Mappero {

class PathItemPrivate
{
    Q_DECLARE_PUBLIC(PathItem)

    PathItemPrivate(PathItem *mark):
        q_ptr(mark),
        route(0),
        routePen(Qt::green, 4)
    {
    }
    ~PathItemPrivate() {};

private:
    mutable PathItem *q_ptr;
    QRectF boundingRect;
    QPainterPath routePath;
    const Path *route;
    QPen routePen;
};
}; // namespace

PathItem::PathItem(Map *map):
    MapObject(),
    d_ptr(new PathItemPrivate(this))
{
    setCacheMode(QGraphicsItem::ItemCoordinateCache);
    setMap(map);
}

PathItem::~PathItem()
{
    delete d_ptr;
}

void PathItem::setRoute(const Path &route)
{
    Q_D(PathItem);

    d->route = &route;

    update();
}

QRectF PathItem::boundingRect() const
{
    Q_D(const PathItem);
    return d->boundingRect;
}

void PathItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                     QWidget *widget)
{
    Q_D(const PathItem);

    Q_UNUSED(option);
    Q_UNUSED(widget);

    QTransform transformation = painter->transform();

    qreal zoom = map()->zoomLevel();

    Point offset = -map()->centerUnits().toPixel(zoom);
    transformation.translate(offset.x(), offset.y());

    painter->setTransform(transformation);
    painter->setPen(d->routePen);
    painter->drawPath(d->routePath);
}

void PathItem::mapEvent(MapEvent *event)
{
    Q_D(PathItem);
    if (event->zoomLevelChanged() ||
        event->centerChanged()) {
        setPos(0, 0);

        if (d->route) {
            d->routePath = d->route->toPainterPath(map()->zoomLevel());
        }
        update();
    }

    if (event->sizeChanged()) {
        QSizeF size = map()->boundingRect().size();
        qreal radius = qMax(size.width(), size.height());
        d->boundingRect = QRectF(-radius, -radius, radius * 2, radius * 2);
    }
}
