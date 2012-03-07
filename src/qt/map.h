/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2009-2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_MAP_H
#define MAP_MAP_H

#include "types.h"

#include <QDeclarativeItem>

namespace Mappero {

class GpsPosition;
class Layer;
class MapObject;
class Path;
class Projection;

class MapPrivate;
class Map: public QDeclarativeItem {
    Q_OBJECT
    Q_PROPERTY(QPointF center READ center WRITE setCenter \
               NOTIFY centerChanged);
    Q_PROPERTY(QPointF animatedCenterUnits READ animatedCenterUnits \
               WRITE setAnimatedCenterUnits \
               NOTIFY animatedCenterUnitsChanged);
    Q_PROPERTY(QPointF requestedCenter READ requestedCenter \
               WRITE setRequestedCenter \
               NOTIFY requestedCenterChanged);
    Q_PROPERTY(QPointF requestedCenterUnits READ requestedCenterUnits \
               NOTIFY requestedCenterChanged);
    Q_PROPERTY(QObject *flickable READ flickable WRITE setFlickable);
    Q_PROPERTY(qreal zoomLevel READ zoomLevel \
               NOTIFY zoomLevelChanged);
    Q_PROPERTY(qreal animatedZoomLevel \
               READ animatedZoomLevel WRITE setAnimatedZoomLevel \
               NOTIFY animatedZoomLevelChanged);
    Q_PROPERTY(qreal requestedZoomLevel \
               READ requestedZoomLevel WRITE setRequestedZoomLevel \
               NOTIFY requestedZoomLevelChanged);
    Q_PROPERTY(QString mainLayerId READ mainLayerId WRITE setMainLayerId \
               NOTIFY mainLayerIdChanged);
    Q_PROPERTY(bool followGps READ followGps WRITE setFollowGps \
               NOTIFY followGpsChanged);
public:
    Map();
    ~Map();

    void setCenter(const QPointF &center);
    QPointF center() const;

    void setRequestedCenter(const QPointF &center);
    QPointF requestedCenter() const;
    QPointF requestedCenterUnits() const;

    void setAnimatedCenterUnits(const QPointF &center);
    QPointF animatedCenterUnits() const;

    void setFlickable(QObject *flickable);
    QObject *flickable() const;

    Point centerUnits() const;

    const Projection *projection() const;

    void setZoomLevel(qreal zoom);
    qreal zoomLevel() const;

    void setAnimatedZoomLevel(qreal zoom);
    qreal animatedZoomLevel() const;

    void setRequestedZoomLevel(qreal zoom);
    qreal requestedZoomLevel() const;

    void setMainLayerId(const QString &layerId);
    QString mainLayerId() const;

    void setMainLayer(Layer *layer);
    Layer *mainLayer() const;

    void setRoute(const Path &route);

    void setFollowGps(bool followGps);
    bool followGps() const;

    // reimplemented virtual functions:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

Q_SIGNALS:
    void centerChanged(const QPointF &center);
    void animatedCenterUnitsChanged(const QPointF &center);
    void requestedCenterChanged(const QPointF &center);
    void zoomLevelChanged(qreal zoomLevel);
    void animatedZoomLevelChanged(qreal zoomLevel);
    void requestedZoomLevelChanged(qreal zoomLevel);
    void sizeChanged();
    void mainLayerIdChanged();
    void followGpsChanged(bool followGps);

protected Q_SLOTS:
    virtual void gpsPositionUpdated(const GpsPosition &gpsPosition);

protected:
    // reimplemented virtual methods
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    bool sceneEvent(QEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *e);

private:
    friend class MapObject;
    void addObject(MapObject *mapObject);

    MapPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Map)
};

};

#endif /* MAP_MAP_H */
