/* vi: set et sw=4 ts=8 cino=t0,(0: */
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

#ifndef MAP_OSM_H
#define MAP_OSM_H

#include <QGraphicsItem>

namespace Mappero {

class Osm: public QGraphicsItem
{
public:
    Osm();

    void setSize(const QSize &size);

protected:
    QRectF boundingRect() const
    {
        return QRectF(0, 0, 0, 0);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget)
    {
    }
};

};


#endif /* MAP_OSM_H */
