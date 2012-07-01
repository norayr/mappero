/* vi: set et sw=4 ts=4 cino=t0,(0: */
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

#ifndef MAP_POI_ITEM_H
#define MAP_POI_ITEM_H

#include <QDeclarativeItem>
#include <QPointF>

namespace Mappero {

class PoiItemPrivate;
class PoiItem: public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF transformOriginPoint \
               READ transformOriginPoint \
               WRITE setTransformOriginPoint \
               NOTIFY transformOriginPointChaned);

public:
    PoiItem(QDeclarativeItem *parent = 0);
    virtual ~PoiItem();

Q_SIGNALS:
    void transformOriginPointChaned();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

}; // namespace

#endif /* MAP_POI_ITEM_H */
