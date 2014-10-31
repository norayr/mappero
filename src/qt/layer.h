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

#ifndef MAP_LAYER_H
#define MAP_LAYER_H

#include "map-object.h"

namespace Mappero {

struct Projection;

class LayerPrivate;
class Layer: public MapItem {
    Q_OBJECT
    Q_PROPERTY(QString uid READ id WRITE setId NOTIFY layerChanged);
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY layerChanged);
    Q_PROPERTY(int minZoom READ minZoom WRITE setMinZoom NOTIFY layerChanged);
    Q_PROPERTY(int maxZoom READ maxZoom WRITE setMaxZoom NOTIFY layerChanged);

public:
    Layer();
    virtual ~Layer();

    static Layer *find(const QString &id);

    void setId(const QString &id);
    QString id() const;

    void setName(const QString &name);
    QString name() const;

    const Projection *projection() const;

    void setMinZoom(int minZoom);
    int minZoom() const;
    void setMaxZoom(int maxZoom);
    int maxZoom() const;

    virtual void mapEvent(MapEvent *e);

    // reimplemented virtual functions:
    QRectF boundingRect() const;

Q_SIGNALS:
    void layerChanged();

protected:
    void queueLayerChanged();
    void setProjection(const Projection *projection);

private:
    LayerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Layer)
};

};

#endif /* MAP_LAYER_H */
