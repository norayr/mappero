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

#include <QGraphicsItem>

namespace Mappero {

struct Projection;
class Map;
class MapEvent;

class LayerPrivate;
class Layer: public QGraphicsItem {
public:
    Layer(const QString &id, const Projection *projection);
    virtual ~Layer();

    static Layer *fromId(const QString &id);
    QString id() const;

    const Projection *projection();

    void setMap(Map *map);
    Map *map() const;

    virtual void mapEvent(MapEvent *e);

    // reimplemented virtual functions:
    QRectF boundingRect() const;

private:
    LayerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Layer)
};

};

#endif /* MAP_LAYER_H */
