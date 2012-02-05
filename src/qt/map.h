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

class Layer;
class LayerPrivate;
class Map;
class MapPrivate;

class MapEvent {
public:
    ~MapEvent() {}

    Map *map() const { return m_map; }
    bool centerChanged() const { return m_centerChanged; }
    bool zoomLevelChanged() const { return m_zoomLevelChanged; }
    bool sizeChanged() const { return m_sizeChanged; }

private:
    friend class LayerPrivate;
    friend class Map;
    friend class MapPrivate;
    MapEvent(Map *map, bool dirty = false);
    void clear();
    Map *m_map;
    bool m_centerChanged;
    bool m_zoomLevelChanged;
    bool m_sizeChanged;
};

class Map: public QDeclarativeItem {
    Q_OBJECT
    Q_PROPERTY(QPointF center READ centerPoint WRITE setCenter \
               NOTIFY centerChanged);
    Q_PROPERTY(QPointF pan READ pan WRITE setPan);
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
public:
    Map();
    ~Map();

    void setCenter(const GeoPoint &center);
    GeoPoint center() const;

    /* For QML: same as above, but use QPointF */
    void setCenter(const QPointF &center);
    QPointF centerPoint() const;

    void setPan(const QPointF &pan);
    QPointF pan() const;

    Point centerUnits() const;

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

    // reimplemented virtual functions:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

Q_SIGNALS:
    void centerChanged(const GeoPoint &center);
    void zoomLevelChanged(qreal zoomLevel);
    void animatedZoomLevelChanged(qreal zoomLevel);
    void requestedZoomLevelChanged(qreal zoomLevel);
    void sizeChanged();
    void mainLayerIdChanged();

public Q_SLOTS:
    void panFinished();

protected:
    // reimplemented virtual methods
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    bool sceneEvent(QEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *e);

private:
    MapPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Map)
};

};

#endif /* MAP_MAP_H */
