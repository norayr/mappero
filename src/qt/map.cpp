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
#include "controller.h"
#include "debug.h"
#include "gps.h"
#include "layer.h"
#include "map.h"
#include "map-object.h"
#include "mark.h"
#include "path-item.h"
#include "path.h"
#include "projection.h"

#include <QEvent>
#include <QGestureEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneWheelEvent>
#include <QPanGesture>
#include <QPinchGesture>
#include <QStyleOptionGraphicsItem>
#include <math.h>

using namespace Mappero;

namespace Mappero {

typedef QDeclarativeListProperty<MapItem> MapItemList;

class LayerGroup: public MapGraphicsItem
{
public:
    LayerGroup():
        MapGraphicsItem()
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

    MapPrivate(Map *q);

    void setRequestedCenter(const Point &centerUnits);
    void onPanning(QPanGesture *pan);
    void onPinching(QPinchGesture *pinch);
    Point pixel2unit(const QPointF &pixel) const {
        return (pixel * pow(2, zoomLevel)).toPoint();
    }

    GeoPoint unit2geo(const Point &u) {
        if (mainLayer == 0) return GeoPoint(0, 0);

        const Projection *projection = mainLayer->projection();
        return projection->unitToGeo(u);
    }

    void setupFlickable();

    static void itemAppend(MapItemList *p, MapItem *o) {
        MapPrivate *d = reinterpret_cast<MapPrivate*>(p->data);
        d->items.append(o);
        o->setMap(d->q_ptr);
        static_cast<QGraphicsItem*>(o)->setParentItem(d->layerGroup);
    }
    static int itemCount(MapItemList *p) {
        MapPrivate *d = reinterpret_cast<MapPrivate*>(p->data);
        return d->items.count();
    }
    static MapItem *itemAt(MapItemList *p, int idx) {
        MapPrivate *d = reinterpret_cast<MapPrivate*>(p->data);
        return d->items.at(idx);
    }
    static void itemClear(MapItemList *p) {
        MapPrivate *d = reinterpret_cast<MapPrivate*>(p->data);
        foreach (MapItem *o, d->items) {
            o->setMap(0);
            static_cast<QGraphicsItem*>(o)->setParentItem(0);
        }
        d->items.clear();
    }

private Q_SLOTS:
    void deliverMapEvent();
    void onFlickablePan();
    void onFlickablePanFinished();

protected:
    bool eventFilter(QObject *watched, QEvent *e);

private:
    QList<MapObject*> mapObjects;
    QList<MapItem*> items;
    LayerGroup *layerGroup;
    Layer *mainLayer;
    QPointF center;
    QPointF animatedCenterUnits;
    QPointF requestedCenter;
    QPointF requestedCenterUnits;
    Point centerUnits;
    QPointF pan;
    bool ignoreNextPan;
    QObject *flickable;
    qreal zoomLevel;
    qreal animatedZoomLevel;
    qreal requestedZoomLevel;
    bool followGps;
    MapEvent mapEvent;
    Mark *mark;
    PathItem *pathItem;
    mutable Map *q_ptr;
};
};

MapPrivate::MapPrivate(Map *q):
    layerGroup(new LayerGroup),
    mainLayer(0),
    center(0, 0),
    animatedCenterUnits(0, 0),
    requestedCenter(0, 0),
    requestedCenterUnits(0, 0),
    centerUnits(0, 0),
    ignoreNextPan(false),
    zoomLevel(-1),
    animatedZoomLevel(-1),
    requestedZoomLevel(-1),
    followGps(true),
    mapEvent(q),
    mark(0),
    pathItem(0),
    q_ptr(q)
{
    layerGroup->setParentItem(q);

    QObject::connect(q, SIGNAL(centerChanged(const QPointF&)),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    QObject::connect(q, SIGNAL(zoomLevelChanged(qreal)),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    QObject::connect(q, SIGNAL(sizeChanged()),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
}

void MapPrivate::setRequestedCenter(const Point &centerUnits)
{
    Q_Q(Map);
    if (mainLayer != 0) {
        const Projection *projection = mainLayer->projection();
        q->setRequestedCenter(projection->unitToGeo(centerUnits).toPointF());
    }
}

void MapPrivate::onPanning(QPanGesture *pan)
{
    DEBUG() << "Panning, delta:" << pan->delta();
}

void MapPrivate::onPinching(QPinchGesture *pinch)
{
    Q_Q(Map);
    DEBUG() << "Rotating around" << pinch->centerPoint() <<
        ", angle:" << pinch->rotationAngle();

    if (pinch->totalChangeFlags() & QPinchGesture::ScaleFactorChanged) {
        DEBUG() << "Scale" << pinch->scaleFactor() <<
            ", total:" << pinch->totalScaleFactor();
        qreal zoom = zoomLevel - log(pinch->totalScaleFactor());
        if (pinch->state() == Qt::GestureFinished) {
            q->setRequestedZoomLevel(zoom);
        } else {
            q->setAnimatedZoomLevel(zoom);
        }
    }
}

void MapPrivate::deliverMapEvent()
{
    if (mapEvent.centerChanged() ||
        mapEvent.zoomLevelChanged() ||
        mapEvent.sizeChanged()) {

        foreach (MapObject *object, mapObjects) {
            object->mapEvent(&mapEvent);
        }
        mapEvent.clear();
    }
}

void MapPrivate::onFlickablePan()
{
    Q_Q(Map);
    if (ignoreNextPan) {
        ignoreNextPan = false;
        return;
    }
    pan = flickable->property("position").toPointF();
    if (pan == QPointF(0, 0)) return;
    layerGroup->setPos(q->boundingRect().center() - pan);
}

void MapPrivate::onFlickablePanFinished()
{
    Q_Q(Map);
    Point newCenterUnits = centerUnits.translated(pixel2unit(pan));
    q->setCenter(unit2geo(newCenterUnits).toPointF());
    ignoreNextPan = true;
}

bool MapPrivate::eventFilter(QObject *watched, QEvent *e)
{
    if (watched == flickable &&
        e->type() == QEvent::GraphicsSceneWheel) {
        e->ignore();
        return true;
    }
    return QObject::eventFilter(watched, e);
}

void MapPrivate::setupFlickable()
{
    flickable->installEventFilter(this);
    QObject::connect(flickable, SIGNAL(positionChanged()),
                     this, SLOT(onFlickablePan()));
    QObject::connect(flickable, SIGNAL(panFinished()),
                     this, SLOT(onFlickablePanFinished()));
}

Map::Map():
    QDeclarativeItem(0),
    d_ptr(new MapPrivate(this))
{
    Q_D(Map);
    d->layerGroup->setMap(this);

    d->mark = new Mark(this);
    d->mark->setParentItem(d->layerGroup);
    d->mark->setZValue(2);

    d->pathItem = new PathItem(this);
    d->pathItem->setParentItem(d->layerGroup);
    d->pathItem->setZValue(1);

    setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    Gps *gps = Gps::instance();
    QObject::connect(gps,
                     SIGNAL(positionUpdated(const GpsPosition &)),
                     this,
                     SLOT(gpsPositionUpdated(const GpsPosition &)));
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
        Controller::instance()->setProjection(d->mainLayer->projection());
    }

    Q_EMIT mainLayerIdChanged();
}

void Map::setRoute(const Path &route)
{
    Q_D(Map);
    d->pathItem->setRoute(route);
}

Layer *Map::mainLayer() const
{
    Q_D(const Map);
    return d->mainLayer;
}

void Map::addObject(MapObject *mapObject)
{
    Q_D(Map);
    d->mapObjects.append(mapObject);
}

void Map::removeObject(MapObject *mapObject)
{
    Q_D(Map);
    d->mapObjects.removeOne(mapObject);
}

void Map::setCenter(const QPointF &center)
{
    Q_D(Map);

    d->center = center;
    d->requestedCenter = center;
    if (d->mainLayer != 0) {
        const Projection *projection = d->mainLayer->projection();
        d->centerUnits = projection->geoToUnit(GeoPoint(center));
        d->animatedCenterUnits = d->centerUnits;
        d->requestedCenterUnits = d->centerUnits;
        DEBUG() << "Transformed:" << d->centerUnits;
    }

    d->mapEvent.m_centerChanged = true;
    Q_EMIT centerChanged(center);
}

QPointF Map::center() const
{
    Q_D(const Map);
    return d->center;
}

void Map::setAnimatedCenterUnits(const QPointF &center)
{
    Q_D(Map);

    DEBUG() << center;
    if (center != d->animatedCenterUnits) {
        Point diffUnits = d->centerUnits - center.toPoint();
        QPointF viewCenter = boundingRect().center();
        d->layerGroup->setPos(diffUnits.toPixel(d->animatedZoomLevel) + viewCenter);

        d->animatedCenterUnits = center;
        Q_EMIT animatedCenterUnitsChanged(center);
    }

    if (center == d->requestedCenterUnits)
        setCenter(d->requestedCenter);
}

QPointF Map::animatedCenterUnits() const
{
    Q_D(const Map);
    return d->animatedCenterUnits;
}

void Map::setRequestedCenter(const QPointF &center)
{
    Q_D(Map);

    if (center != d->requestedCenter) {
        DEBUG() << center;
        d->requestedCenter = center;
        if (d->mainLayer != 0) {
            const Projection *projection = d->mainLayer->projection();
            d->requestedCenterUnits = projection->geoToUnit(GeoPoint(center));
        }
        Q_EMIT requestedCenterChanged(center);
    }
}

QPointF Map::requestedCenter() const
{
    Q_D(const Map);
    return d->requestedCenter;
}

QPointF Map::requestedCenterUnits() const
{
    Q_D(const Map);
    return d->requestedCenterUnits;
}

Point Map::centerUnits() const
{
    Q_D(const Map);
    return d->centerUnits;
}

const Projection *Map::projection() const
{
    Q_D(const Map);
    return (d->mainLayer != 0) ? d->mainLayer->projection() : 0;
}

void Map::setFlickable(QObject *flickable)
{
    Q_D(Map);
    d->flickable = flickable;
    if (flickable != 0)
        d->setupFlickable();
}

QObject *Map::flickable() const
{
    Q_D(const Map);
    return d->flickable;
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

    /* approximate the zoom level to the nearest integer.
     * TODO add a protected method so that subclasses can decide whether this
     * should happen.
     */
    zoom = round(zoom);
    d->requestedZoomLevel = zoom;
    Q_EMIT requestedZoomLevelChanged(zoom);
}

qreal Map::requestedZoomLevel() const
{
    Q_D(const Map);
    return d->requestedZoomLevel;
}

void Map::setFollowGps(bool followGps)
{
    Q_D(Map);
    if (followGps == d->followGps) return;

    d->followGps = followGps;
    Q_EMIT followGpsChanged(followGps);
}

bool Map::followGps() const
{
    Q_D(const Map);
    return d->followGps;
}

void Map::setTracker(Tracker *tracker)
{
    Q_D(Map);
    d->pathItem->setTracker(tracker);
}

Tracker *Map::tracker() const
{
    Q_D(const Map);
    return d->pathItem->tracker();
}

QDeclarativeListProperty<MapItem> Map::items()
{
    return QDeclarativeListProperty<MapItem>(this, d_ptr,
                                             MapPrivate::itemAppend,
                                             MapPrivate::itemCount,
                                             MapPrivate::itemAt,
                                             MapPrivate::itemClear);
}

void Map::lookAt(const QRectF &area, int offsetX, int offsetY, int margin)
{
    if (area.width() < 0 || area.height() < 0) return;

    /* compute the best zoom level for framing the area */
    Q_D(Map);
    if (d->mainLayer == 0) return;

    const Projection *projection = d->mainLayer->projection();
    Point southEast = projection->geoToUnit(GeoPoint(area.bottomLeft()));
    Point northWest = projection->geoToUnit(GeoPoint(area.topRight()));

    Point size = southEast - northWest;

    int fitWidth = width() - 2 * margin;
    int fitHeight = height() - 2 * margin;
    int zoom;
    for (zoom = d->mainLayer->minZoom();
         zoom < d->mainLayer->maxZoom(); zoom++) {
        QPoint pixelSize = size.toPixel(zoom);
        if (pixelSize.x() <= fitWidth && pixelSize.y() <= fitHeight) {
            break;
        }
    }
    /* don't zoom too much in */
    if (zoom < 4) zoom = 4;

    Point centerUnits =
        QRect(northWest, southEast).center() -
        Point::fromPixel(QPoint(offsetX, offsetY), zoom);
    d->setRequestedCenter(centerUnits);
    setRequestedZoomLevel(zoom);
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

bool Map::sceneEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Gesture:
        {
            Q_D(Map);
            QGestureEvent *e = static_cast<QGestureEvent*>(event);
            bool active = false;
            foreach (QGesture *gesture, e->activeGestures()) {
                Qt::GestureState state = gesture->state();
                if (state == Qt::GestureStarted ||
                    state == Qt::GestureUpdated)
                    active = true;
            }
            d->flickable->setProperty("interactive", !active);
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

void Map::gpsPositionUpdated(const GpsPosition &pos) {
    Q_D(Map);

    d->mark->setPosition(pos);
    if (d->followGps) {
        setRequestedCenter(pos.geo().toPointF());
    }
}

#include "map.moc"
