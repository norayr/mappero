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
#include "controller.h"
#include "debug.h"
#include "gps.h"
#include "layer.h"
#include "map.h"
#include "map-object.h"
#include "mark.h"
#include "path-item.h"

#include <Mappero/Path>
#include <Mappero/Projection>
#include <QEvent>
#include <math.h>

using namespace Mappero;

namespace Mappero {

typedef QQmlListProperty<QObject> MapItemList;

class LayerGroup: public MapItem
{
public:
    LayerGroup():
        MapItem()
    {
        setScalable(false);
    }

    void mapEvent(MapEvent *event) {
        if (event->zoomLevelChanged() || event->animated()) {
            qreal scale;
            if (event->animated()) {
                scale = exp2(map()->zoomLevel() - map()->animatedZoomLevel());
            } else {
                scale = 1.0;
            }
            setScale(scale);
        }
        if (event->centerChanged() ||
            event->sizeChanged() ||
            event->animated()) {
            QPointF pos = event->map()->boundingRect().center();
            if (event->animated()) {
                Point diffUnits = map()->animatedCenterUnits().toPoint() -
                    map()->centerUnits();
                if (!diffUnits.isNull()) {
                    pos -= diffUnits.toPixelF(map()->animatedZoomLevel());
                }
            }
            setPos(pos);
        }
    }

    void setPos(const QPointF &pos) {
        setX(pos.x());
        setY(pos.y());
    }

    // reimplemented virtual methods
    QRectF boundingRect() const { return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6); }
};

class MapPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Map)

    MapPrivate(Map *q);

    void setRequestedCenter(const Point &centerUnits);
    Point pixel2unit(const QPoint &pixel) const {
        return Point::fromPixel(pixel, zoomLevel);
    }

    GeoPoint unit2geo(const Point &u) const {
        if (mainLayer == 0) return GeoPoint(0, 0);

        const Projection *projection = mainLayer->projection();
        return projection->unitToGeo(u);
    }

    void setupFlickable();

    void itemAdded(QQuickItem *item);
    void itemRemoved(QQuickItem *item);

private Q_SLOTS:
    void deliverMapEvent();
    void onFlickablePan();
    void onFlickablePanFinished();

protected:
    bool eventFilter(QObject *watched, QEvent *e);

private:
    bool reparenting;
    QList<MapObject*> mapObjects;
    LayerGroup *layerGroup;
    Layer *mainLayer;
    GeoPoint center;
    QPointF animatedCenterUnits;
    GeoPoint requestedCenter;
    QPoint requestedCenterUnits;
    Point centerUnits;
    QPoint panOffset;
    bool ignoreNextPan;
    QObject *flickable;
    qreal zoomLevel;
    qreal animatedZoomLevel;
    qreal requestedZoomLevel;
    qreal pinchScale;
    bool followGps;
    MapEvent mapEvent;
    Mark *mark;
    mutable Map *q_ptr;
};
};

MapPrivate::MapPrivate(Map *q):
    reparenting(false),
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
    q_ptr(q)
{
    QObject::connect(q, SIGNAL(centerChanged(const GeoPoint&)),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    QObject::connect(q, SIGNAL(zoomLevelChanged(qreal)),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    QObject::connect(q, SIGNAL(sizeChanged()),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    QObject::connect(q, SIGNAL(animatedZoomLevelChanged(qreal)),
                     this, SLOT(deliverMapEvent()), Qt::QueuedConnection);
    QObject::connect(q, SIGNAL(animatedCenterUnitsChanged(const QPointF&)),
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

void MapPrivate::itemAdded(QQuickItem *item)
{
    if (item == layerGroup) return;

    MapObject *object = qobject_cast<MapObject*>(item);
    if (object) {
        Q_Q(Map);
        object->setMap(q);
        if (object->isScalable()) {
            reparenting = true;
            item->setParentItem(layerGroup);
            reparenting = false;
        }
    }
}

void MapPrivate::itemRemoved(QQuickItem *item)
{
    if (reparenting) return;

    MapObject *object = qobject_cast<MapObject*>(item);
    if (object) {
        object->setMap(0);
    }
}

void MapPrivate::deliverMapEvent()
{
    bool sendToAll = mapEvent.centerChanged() ||
        mapEvent.zoomLevelChanged() ||
        mapEvent.sizeChanged();
    bool sendToUnscalable = mapEvent.animated();

    if (sendToAll || sendToUnscalable) {
        foreach (MapObject *object, mapObjects) {
            if (sendToAll || !object->isScalable()) {
                object->mapEvent(&mapEvent);
            }
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
    panOffset = flickable->property("position").toPointF().toPoint();
    if (panOffset == QPoint(0, 0)) return;
    q->setAnimatedCenterUnits(centerUnits.translated(pixel2unit(panOffset)));
}

void MapPrivate::onFlickablePanFinished()
{
    Q_Q(Map);
    Point newCenterUnits = centerUnits.translated(pixel2unit(panOffset));
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
    QQuickItem(0),
    d_ptr(new MapPrivate(this))
{
    Q_D(Map);
    d->layerGroup->setParentItem(this);
    d->layerGroup->setMap(this);

    d->mark = new Mark(this);
    d->mark->setParentItem(d->layerGroup);
    d->mark->setZ(2);

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

void Map::setMainLayer(Layer *layer)
{
    Q_D(Map);

    if (layer == d->mainLayer) return;

    if (d->mainLayer != 0) {
        d->mainLayer->setMap(0);
        delete d->mainLayer;
    }
    d->mainLayer = layer;
    if (layer != 0) {
        static_cast<QQuickItem*>(layer)->setParentItem(d->layerGroup);
        layer->setZ(-10);
        layer->setMap(this);
        const Projection *projection = d->mainLayer->projection();
        Controller::instance()->setProjection(projection);
        d->centerUnits = projection->geoToUnit(d->center);
        d->animatedCenterUnits = d->centerUnits;
        d->requestedCenterUnits = d->centerUnits;
        d->mapEvent.m_centerChanged = true;
        d->deliverMapEvent();
    }

    Q_EMIT mainLayerChanged();
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

void Map::setCenter(const GeoPoint &center)
{
    Q_D(Map);

    d->center = center.normalized();
    if (d->center == d->requestedCenter) {
        d->centerUnits = d->requestedCenterUnits;
        d->animatedCenterUnits = d->centerUnits;
    } else {
        d->requestedCenter = d->center;
        if (d->mainLayer != 0) {
            const Projection *projection = d->mainLayer->projection();
            d->centerUnits = projection->geoToUnit(d->center);
            d->animatedCenterUnits = d->centerUnits;
            d->requestedCenterUnits = d->centerUnits;
            DEBUG() << "Transformed:" << d->centerUnits;
        }
    }

    d->mapEvent.m_centerChanged = true;
    Q_EMIT centerChanged(d->center);
}

GeoPoint Map::center() const
{
    Q_D(const Map);
    return d->center;
}

void Map::setAnimatedCenterUnits(const QPointF &center)
{
    Q_D(Map);

    if (center != d->animatedCenterUnits) {
        d->animatedCenterUnits = center;

        if (center.toPoint() == d->requestedCenterUnits) {
            setCenter(d->requestedCenter);
        } else {
            d->mapEvent.m_animated = true;
            Q_EMIT animatedCenterUnitsChanged(center);
        }
    }
}

QPointF Map::animatedCenterUnits() const
{
    Q_D(const Map);
    return d->animatedCenterUnits;
}

void Map::setRequestedCenter(const GeoPoint &center)
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

GeoPoint Map::requestedCenter() const
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
    d->mapEvent.m_animated = true;
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

    if (d->mainLayer != 0) {
        if (zoom < d->mainLayer->minZoom() ||
            zoom > d->mainLayer->maxZoom())
            return;
    }

    /* approximate the zoom level to the nearest integer.
     * TODO add a protected method so that subclasses can decide whether this
     * should happen.
     */
    zoom = (zoom > d->requestedZoomLevel) ? ceil(zoom) : floor(zoom);
    d->requestedZoomLevel = zoom;
    Q_EMIT requestedZoomLevelChanged(zoom);
}

qreal Map::requestedZoomLevel() const
{
    Q_D(const Map);
    return d->requestedZoomLevel;
}

qreal Map::minZoomLevel() const
{
    Q_D(const Map);

    if (d->mainLayer == 0) return MIN_ZOOM;
    return d->mainLayer->minZoom();
}

qreal Map::maxZoomLevel() const
{
    Q_D(const Map);

    if (d->mainLayer == 0) return MAX_ZOOM;
    return d->mainLayer->maxZoom();
}

void Map::setPinchScale(qreal scale)
{
    Q_D(Map);

    if (scale == d->pinchScale) return;

    if (scale != 0) {
        qreal zoom = d->zoomLevel - log(scale);
        setAnimatedZoomLevel(zoom);
    } else {
        qreal zoom = d->zoomLevel - log(d->pinchScale);
        setRequestedZoomLevel(zoom);
    }
    d->pinchScale = scale;
    Q_EMIT pinchScaleChanged();
}

qreal Map::pinchScale() const
{
    Q_D(const Map);
    return d->pinchScale;
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

void Map::ensureVisible(const GeoPoint &geoPoint, int offsetX, int offsetY,
                        int margin)
{
    Q_D(Map);
    if (d->mainLayer == 0) return;

    const Projection *projection = d->mainLayer->projection();
    Point point = projection->geoToUnit(geoPoint);

    Point offsetUnits = point - d->centerUnits;
    QPoint offsetPixels = offsetUnits.toPixel(d->zoomLevel) +
        boundingRect().center().toPoint();
    offsetPixels += QPoint(offsetX, offsetY);

    int scrollX = 0, scrollY = 0;
    if (offsetPixels.x() + margin > width()) {
        scrollX = width() - (offsetPixels.x() + margin);
    } else if (offsetPixels.x() - margin < 0) {
        scrollX = - (offsetPixels.x() - margin);
    }

    if (offsetPixels.y() + margin > height()) {
        scrollY = height() - (offsetPixels.y() + margin);
    } else if (offsetPixels.y() - margin < 0) {
        scrollY = - (offsetPixels.y() - margin);
    }

    // Convert the pixels back to units
    Point scrollUnits = d->pixel2unit(QPoint(scrollX, scrollY));
    d->setRequestedCenter(d->centerUnits - scrollUnits);
}

GeoPoint Map::pixelsToGeo(const QPointF &pixel) const
{
    Q_D(const Map);
    QPoint pixelsFromCenter =
        (pixel - QPointF(width() / 2, height() / 2)).toPoint();
    Point units = d->centerUnits.translated(d->pixel2unit(pixelsFromCenter));
    return d->unit2geo(units);
}

GeoPoint Map::pixelsToGeo(qreal x, qreal y) const
{
    return pixelsToGeo(QPointF(x, y));
}

void Map::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(Map);
    d->mapEvent.m_sizeChanged = true;
    Q_EMIT sizeChanged();
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void Map::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(Map);

    if (change == QQuickItem::ItemChildAddedChange) {
        d->itemAdded(value.item);
    } else if (change == QQuickItem::ItemChildRemovedChange) {
        d->itemRemoved(value.item);
    }

    QQuickItem::itemChange(change, value);
}

void Map::gpsPositionUpdated(const GpsPosition &pos) {
    Q_D(Map);

    d->mark->setPosition(pos);
    if (d->followGps) {
        setRequestedCenter(pos.geo().toPointF());
    }
}

#include "map.moc"
