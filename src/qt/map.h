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

#include <QQuickItem>

namespace Mappero {

class GpsPosition;
class Layer;
class MapItem;
class MapObject;
class Path;
class Projection;
class Tracker;

class MapPrivate;
class Map: public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(GeoPoint center READ center WRITE setCenter \
               NOTIFY centerChanged);
    Q_PROPERTY(QPointF animatedCenterUnits READ animatedCenterUnits \
               WRITE setAnimatedCenterUnits \
               NOTIFY animatedCenterUnitsChanged);
    Q_PROPERTY(GeoPoint requestedCenter READ requestedCenter \
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
    Q_PROPERTY(qreal minZoomLevel READ minZoomLevel
               NOTIFY mainLayerChanged);
    Q_PROPERTY(qreal maxZoomLevel READ maxZoomLevel
               NOTIFY mainLayerChanged);
    Q_PROPERTY(qreal pinchScale READ pinchScale WRITE setPinchScale \
               NOTIFY pinchScaleChanged);
    Q_PROPERTY(Mappero::Layer *mainLayer READ mainLayer WRITE setMainLayer \
               NOTIFY mainLayerChanged);
    Q_PROPERTY(bool followGps READ followGps WRITE setFollowGps \
               NOTIFY followGpsChanged);

public:
    Map();
    ~Map();

    void setCenter(const GeoPoint &center);
    GeoPoint center() const;

    void setRequestedCenter(const GeoPoint &center);
    GeoPoint requestedCenter() const;
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

    qreal minZoomLevel() const;
    qreal maxZoomLevel() const;

    void setPinchScale(qreal scale);
    qreal pinchScale() const;

    void setMainLayer(Layer *layer);
    Layer *mainLayer() const;

    void setFollowGps(bool followGps);
    bool followGps() const;

    Q_INVOKABLE void lookAt(const QRectF &area,
                            int offsetX, int offsetY, int margin = 0);
    Q_INVOKABLE void ensureVisible(const GeoPoint &geoPoint,
                                   int offsetX, int offsetY, int margin = 0);

    Q_INVOKABLE GeoPoint pixelsToGeo(const QPointF &pixel) const;
    Q_INVOKABLE GeoPoint pixelsToGeo(qreal x, qreal y) const;

Q_SIGNALS:
    void centerChanged(const GeoPoint &center);
    void animatedCenterUnitsChanged(const QPointF &center);
    void requestedCenterChanged(const GeoPoint &center);
    void zoomLevelChanged(qreal zoomLevel);
    void animatedZoomLevelChanged(qreal zoomLevel);
    void requestedZoomLevelChanged(qreal zoomLevel);
    void pinchScaleChanged();
    void sizeChanged();
    void mainLayerChanged();
    void followGpsChanged(bool followGps);

protected Q_SLOTS:
    virtual void gpsPositionUpdated(const GpsPosition &gpsPosition);

protected:
    // reimplemented virtual methods
    void geometryChanged(const QRectF &newGeometry,
                         const QRectF &oldGeometry) Q_DECL_OVERRIDE;
    void itemChange(ItemChange change,
                    const ItemChangeData &value) Q_DECL_OVERRIDE;

private:
    friend class MapObject;
    void addObject(MapObject *mapObject);
    void removeObject(MapObject *mapObject);

    MapPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Map)
};

};

#endif /* MAP_MAP_H */
