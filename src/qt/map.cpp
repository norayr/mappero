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
#include "map.h.moc"
#include "projection.h"

#include <QEvent>
#include <QGestureEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneWheelEvent>
#include <QPanGesture>
#include <QPinchGesture>
#include <QStyleOptionGraphicsItem>
#include <QtScroller>
#include <QtScrollEvent>
#include <QtScrollPrepareEvent>
#include <math.h>

using namespace Mappero;

namespace Mappero {

MapEvent::MapEvent(Map *map, bool dirty):
    m_map(map),
    m_centerChanged(dirty),
    m_zoomLevelChanged(dirty),
    m_sizeChanged(dirty)
{
}

void MapEvent::clear()
{
    m_centerChanged = false;
    m_zoomLevelChanged = false;
    m_sizeChanged = false;
}

class LayerGroup: public QGraphicsItem
{
public:
    LayerGroup(QGraphicsItem *parent):
        QGraphicsItem(parent)
    {
        setFlags(QGraphicsItem::ItemHasNoContents);
    }

    void mapEvent(MapEvent *event) {
        if (event->centerChanged() ||
            event->sizeChanged()) {
            QPointF viewCenter = event->map()->boundingRect().center();
            setPos(viewCenter);
        }
        if (event->zoomLevelChanged()) {
            setScale(1.0);
        }

        // propagate event to children
        foreach (QGraphicsItem *item, childItems()) {
            ((Layer *)item)->mapEvent(event);
        }
    }

    // reimplemented virtual methods
    QRectF boundingRect() const { return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6); }
    void paint(QPainter *,
               const QStyleOptionGraphicsItem *,
               QWidget *) {}
};

class MapPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Map)

    MapPrivate(Map *q):
        layerGroup(new LayerGroup(q)),
        mainLayer(0),
        center(0, 0),
        centerUnits(0, 0),
        zoomLevel(-1),
        animatedZoomLevel(-1),
        requestedZoomLevel(-1),
        mapEvent(q),
        q_ptr(q)
    {
        QObject::connect(q, SIGNAL(centerChanged(const GeoPoint&)),
                         this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
        QObject::connect(q, SIGNAL(zoomLevelChanged(qreal)),
                         this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
        QObject::connect(q, SIGNAL(sizeChanged()),
                         this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    }

    void onPanning(QPanGesture *pan);
    void onPinching(QPinchGesture *pinch);
    Point pixel2unit(const QPointF &pixel) const {
        return (pixel * pow(2, zoomLevel)).toPoint();
    }

    GeoPoint unit2geo(const Point &u) {
        if (mainLayer == 0) return GeoPoint(0, 0);

        const Projection *projection = mainLayer->projection();
        GeoPoint geo;
        projection->unitToGeo(u.x, u.y, &geo.lat, &geo.lon);
        return geo;
    }

private Q_SLOTS:
    void deliverMapEvent();

private:
    LayerGroup *layerGroup;
    Layer *mainLayer;
    GeoPoint center;
    Point centerUnits;
    qreal zoomLevel;
    qreal animatedZoomLevel;
    qreal requestedZoomLevel;
    MapEvent mapEvent;
    mutable Map *q_ptr;
};
};

void MapPrivate::onPanning(QPanGesture *pan)
{
    DEBUG() << "Panning, delta:" << pan->delta();
}

void MapPrivate::onPinching(QPinchGesture *pinch)
{
    DEBUG() << "Rotating around" << pinch->centerPoint() <<
        ", angle:" << pinch->rotationAngle();
}

void MapPrivate::deliverMapEvent()
{
    if (mapEvent.centerChanged() ||
        mapEvent.zoomLevelChanged() ||
        mapEvent.sizeChanged()) {
        layerGroup->mapEvent(&mapEvent);
        mapEvent.clear();
    }
}

Map::Map():
    QDeclarativeItem(0),
    d_ptr(new MapPrivate(this))
{
    setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    QtScroller::grabGesture(this, QtScroller::LeftMouseButtonGesture);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

Map::~Map()
{
    delete d_ptr;
}

void Map::setMainLayerId(const QString &layerId)
{
    Q_D(Map);

    if (d->mainLayer != 0 && d->mainLayer->id() == layerId) return;

    Layer *layer = Layer::fromId(layerId);
    if (layer == 0) {
        qWarning() << "Error loading layer: " << layerId;
        return;
    }

    setMainLayer(layer);
}

QString Map::mainLayerId() const
{
    Q_D(const Map);
    return d->mainLayer != 0 ? d->mainLayer->id() : QString();
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
        layer->setParentItem(d->layerGroup);
        layer->setMap(this);
    }

    Q_EMIT mainLayerIdChanged();
}

Layer *Map::mainLayer() const
{
    Q_D(const Map);
    return d->mainLayer;
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

    d->mapEvent.m_centerChanged = true;
    Q_EMIT centerChanged(center);
}

GeoPoint Map::center() const
{
    Q_D(const Map);
    return d->center;
}

void Map::setCenter(const QPointF &center)
{
    GeoPoint p(center.x(), center.y());
    setCenter(p);
}

QPointF Map::centerPoint() const
{
    GeoPoint p = center();
    return QPointF(p.lat, p.lon);
}

Point Map::centerUnits() const
{
    Q_D(const Map);
    return d->centerUnits;
}

void Map::setZoomLevel(qreal zoom)
{
    Q_D(Map);
    if (zoom == d->zoomLevel) return;

    d->zoomLevel = zoom;
    d->animatedZoomLevel = zoom;
    d->mapEvent.m_zoomLevelChanged = true;
    Q_EMIT zoomLevelChanged(zoom);
}

qreal Map::zoomLevel() const
{
    Q_D(const Map);
    return d->zoomLevel;
}

void Map::setAnimatedZoomLevel(qreal zoom)
{
    Q_D(Map);
    if (zoom == d->animatedZoomLevel) return;

    d->animatedZoomLevel = zoom;
    d->layerGroup->setScale(exp2(d->zoomLevel - zoom));
    Q_EMIT animatedZoomLevelChanged(zoom);

    /* if we reached the requested zoom level, change the effective zoom level
     */
    if (zoom == d->requestedZoomLevel)
        setZoomLevel(zoom);
}

qreal Map::animatedZoomLevel() const
{
    Q_D(const Map);
    return d->animatedZoomLevel;
}

void Map::setRequestedZoomLevel(qreal zoom)
{
    Q_D(Map);
    if (zoom == d->requestedZoomLevel) return;

    d->requestedZoomLevel = zoom;
    Q_EMIT requestedZoomLevelChanged(zoom);
}

qreal Map::requestedZoomLevel() const
{
    Q_D(const Map);
    return d->requestedZoomLevel;
}

void Map::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void Map::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(oldGeometry);
    DEBUG() << "Geometry:" << newGeometry;
    Q_D(Map);
    d->mapEvent.m_sizeChanged = true;
    Q_EMIT sizeChanged();
}

bool Map::event(QEvent *e)
{
    Q_D(Map);

    switch (e->type()) {
    case QtScrollPrepareEvent::ScrollPrepare:
        {
            QtScrollPrepareEvent *se = static_cast<QtScrollPrepareEvent *>(e);
            se->setContentPos(-d->layerGroup->pos());
            se->setContentPosRange(d->layerGroup->boundingRect());
            se->setViewportSize(boundingRect().size());
            se->accept();
            return true;
        }
    case QtScrollEvent::Scroll:
        {
            QtScrollEvent *se = static_cast<QtScrollEvent *>(e);
            QtScrollEvent::ScrollState state = se->scrollState();
            if (state == QtScrollEvent::ScrollUpdated) {
                d->layerGroup->setPos(-se->contentPos());
            } else if (state == QtScrollEvent::ScrollFinished) {
                QPointF viewCenter = boundingRect().center();
                Point newCenterUnits =
                    d->centerUnits.translated(d->pixel2unit(se->contentPos() +
                                                            viewCenter));
                setCenter(d->unit2geo(newCenterUnits));
            }
            return true;
        }
    default:
        break;
    }
    return QDeclarativeItem::event(e);
}

bool Map::sceneEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Gesture:
        {
            Q_D(Map);
            QGestureEvent *e = static_cast<QGestureEvent*>(event);
            QGesture *gesture;

            if ((gesture = e->gesture(Qt::PanGesture)) != 0)
                d->onPanning(static_cast<QPanGesture*>(gesture));

            if ((gesture = e->gesture(Qt::PinchGesture)) != 0)
                d->onPinching(static_cast<QPinchGesture*>(gesture));

            return true;
        }
    case QEvent::TouchBegin:
    case QEvent::GraphicsSceneMousePress:
        return true;
    default:
        break;
    }
    return QDeclarativeItem::sceneEvent(event);
}

void Map::wheelEvent(QGraphicsSceneWheelEvent *e)
{
    Q_D(Map);
    setRequestedZoomLevel(d->requestedZoomLevel - e->delta() / 120);
}

#include "map.cpp.moc"
