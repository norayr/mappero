/* vi: set et sw=4 ts=4 cino=t0,(0: */
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
#include "path-layer.h"
#include "path.h"
#include "tracker.h"

#include <QPainter>
#include <QPen>

using namespace Mappero;

namespace Mappero {

class PathLayerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(PathLayer)

    PathLayerPrivate(PathLayer *mark):
        QObject(0),
        q_ptr(mark),
        tracker(0),
        route(0),
        routePen(Qt::green, 4),
        trackPen(Qt::red, 4)
    {
        routePen.setCosmetic(true);
        trackPen.setCosmetic(true);
    }
    ~PathLayerPrivate() {};

public Q_SLOTS:
    void onTrackChanged();

private:
    mutable PathLayer *q_ptr;
    Tracker *tracker;
    QRectF boundingRect;
    QPainterPath routePath;
    QPainterPath trackPath;
    const Path *route;
    QPen routePen;
    QPen trackPen;
};
}; // namespace

void PathLayerPrivate::onTrackChanged()
{
    Q_Q(PathLayer);
    trackPath = tracker->path().toPainterPath(q->map()->zoomLevel());
    q->update();
}

PathLayer::PathLayer(QDeclarativeItem *parent):
    MapItem(parent),
    d_ptr(new PathLayerPrivate(this))
{
    setCacheMode(QGraphicsItem::ItemCoordinateCache);
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}

PathLayer::~PathLayer()
{
    delete d_ptr;
}

void PathLayer::setTracker(Tracker *tracker)
{
    Q_D(PathLayer);
    d->tracker = tracker;
    QObject::connect(d->tracker, SIGNAL(pathChanged()),
                     d, SLOT(onTrackChanged()));
}

Tracker *PathLayer::tracker() const
{
    Q_D(const PathLayer);
    return d->tracker;
}

void PathLayer::setRoute(const Path &route)
{
    Q_D(PathLayer);

    d->route = &route;

    update();
}

QRectF PathLayer::boundingRect() const
{
    Q_D(const PathLayer);
    return d->boundingRect;
}

void PathLayer::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem *option,
                      QWidget *widget)
{
    Q_D(const PathLayer);

    Q_UNUSED(option);
    Q_UNUSED(widget);

    QTransform transformation = painter->transform();

    qreal zoom = map()->zoomLevel();

    Point offset = -map()->centerUnits().toPixel(zoom);
    transformation.translate(offset.x(), offset.y());

    painter->setTransform(transformation);
    painter->setPen(d->routePen);
    painter->drawPath(d->routePath);

    painter->setPen(d->trackPen);
    painter->drawPath(d->trackPath);
}

void PathLayer::mapEvent(MapEvent *event)
{
    Q_D(PathLayer);
    if (event->zoomLevelChanged() ||
        event->centerChanged()) {
        setPos(0, 0);

        if (d->route) {
            d->routePath = d->route->toPainterPath(map()->zoomLevel());
        }
        if (d->tracker) {
            d->trackPath =
                d->tracker->path().toPainterPath(map()->zoomLevel());
        }
        update();
    }

    if (event->sizeChanged()) {
        QSizeF size = map()->boundingRect().size();
        qreal radius = qMax(size.width(), size.height());
        d->boundingRect = QRectF(-radius, -radius, radius * 2, radius * 2);
    }
}

#include "path-layer.moc"
