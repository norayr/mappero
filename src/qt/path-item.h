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

#ifndef MAP_PATH_ITEM_H
#define MAP_PATH_ITEM_H

#include "map-object.h"

namespace Mappero {

class Path;
class Tracker;

class PathItemPrivate;
class PathItem: public MapGraphicsItem
{
public:
    PathItem(Map *map);
    ~PathItem();

    void setTracker(Tracker *tracker);
    Tracker *tracker() const;

    void setRoute(const Path &route);

protected:
    // reimplemented methods
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void mapEvent(MapEvent *event);

private:
    PathItemPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PathItem)
};

}; // namespace

#endif /* MAP_PATH_ITEM_H */
