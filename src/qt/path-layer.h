/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef MAP_PATH_LAYER_H
#define MAP_PATH_LAYER_H

#include "map-object.h"

namespace Mappero {

class Path;
class PathItem;

class PathLayerPrivate;
class PathLayer: public MapItem
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeListProperty<Mappero::PathItem> items READ items);
    Q_CLASSINFO("DefaultProperty", "items");

public:
    PathLayer(QDeclarativeItem *parent = 0);
    ~PathLayer();

    QDeclarativeListProperty<PathItem> items();

protected:
    // reimplemented methods
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void mapEvent(MapEvent *event);

private:
    PathLayerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PathLayer)
};

}; // namespace

#endif /* MAP_PATH_LAYER_H */
