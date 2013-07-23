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

#ifndef MAP_POI_VIEW_H
#define MAP_POI_VIEW_H

#include "map-object.h"
#include "types.h"

class QAbstractListModel;
class QQmlComponent;

namespace Mappero {

class PoiViewPrivate;
class PoiView: public MapItem
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *model READ model WRITE setModel \
               NOTIFY modelChanged);
    Q_PROPERTY(QQmlComponent *delegate READ delegate \
               WRITE setDelegate NOTIFY delegateChanged);
    Q_PROPERTY(QRectF itemArea READ itemArea NOTIFY itemAreaChanged);
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex \
               NOTIFY currentIndexChanged);

public:
    PoiView(QQuickItem *parent = 0);
    virtual ~PoiView();

    void setModel(QAbstractListModel *model);
    QAbstractListModel *model() const;

    void setDelegate(QQmlComponent *delegate);
    QQmlComponent *delegate() const;

    QRectF itemArea() const;

    void setCurrentIndex(int index);
    int currentIndex() const;

    Q_INVOKABLE GeoPoint itemPos(int index) const;

    // reimplemented virtual methods
    void mapEvent(MapEvent *e);

Q_SIGNALS:
    void modelChanged();
    void delegateChanged();
    void itemAreaChanged();
    void currentIndexChanged();

private:
    PoiViewPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PoiView)
};

}; // namespace

#endif /* MAP_POI_VIEW_H */
