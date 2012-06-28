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
#include "gps.h"
#include "map.h"
#include "mark.h"
#include "projection.h"
#include "types.h"

#include <QBrush>
#include <QPainter>

using namespace Mappero;

namespace Mappero {

class MarkPrivate
{
    Q_DECLARE_PUBLIC(Mark)

    MarkPrivate(Mark *mark):
        q_ptr(mark),
        accuracyRadius(0),
        spotRadius(5),
        accuracy(0),
        latitude(0),
        accuracyBrush(Qt::SolidPattern),
        spotBrush(Qt::blue)
    {
        QColor accuracyColor = QColor(spotBrush.color());
        accuracyColor.setAlphaF(1.0 / 8);
        accuracyBrush.setColor(accuracyColor);
    }
    ~MarkPrivate() {};

    void updateAccuracyRadius(qreal zoomLevel) {
        accuracyRadius = metre2unit(accuracy, latitude) / exp2(zoomLevel);
    }

private:
    mutable Mark *q_ptr;
    qreal accuracyRadius;
    qreal spotRadius;
    qreal accuracy;
    qreal latitude;
    qreal speed;
    Point position;
    QBrush accuracyBrush;
    QBrush spotBrush;
};
}; // namespace

Mark::Mark(Map *map):
    MapGraphicsItem(),
    d_ptr(new MarkPrivate(this))
{
    setMap(map);
}

Mark::~Mark()
{
    delete d_ptr;
}

void Mark::setPosition(const GpsPosition &position)
{
    Q_D(Mark);

    d->accuracy = position.accuracy();
    d->latitude = position.latitude();
    Map *map = this->map();
    qreal zoom = map->zoomLevel();
    d->updateAccuracyRadius(zoom);
    d->speed = position.speed();

    // convert geocoordinates into Units
    const Projection *projection = map->projection();
    if (projection == 0) return;

    d->position = projection->geoToUnit(position.geo());
    Point offset = d->position - map->centerUnits();
    setPos(offset.toPixel(zoom));

    update();
}

QRectF Mark::boundingRect() const
{
    Q_D(const Mark);

    qreal radius = qMax (d->accuracyRadius, d->spotRadius);
    return QRectF(-radius, -radius, radius * 2, radius * 2);
}

void Mark::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                 QWidget *widget)
{
    Q_D(const Mark);

    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(Qt::NoPen);

    painter->setBrush(d->accuracyBrush);
    painter->drawEllipse(-d->accuracyRadius, -d->accuracyRadius,
                         d->accuracyRadius * 2, d->accuracyRadius * 2);

    painter->setBrush(d->spotBrush);
    qreal radius = d->spotRadius / parentItem()->scale();
    painter->drawEllipse(-radius, -radius,
                         radius * 2, radius * 2);
}

void Mark::mapEvent(MapEvent *event)
{
    Q_D(Mark);

    qreal zoom = map()->zoomLevel();

    if (event->zoomLevelChanged()) {
        d->updateAccuracyRadius(zoom);
    }

    if (event->zoomLevelChanged() ||
        event->centerChanged()) {
        Point offset = d->position - map()->centerUnits();
        setPos(offset.toPixel(zoom));
    }
}
