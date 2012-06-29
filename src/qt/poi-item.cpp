/* vi: set et sw=4 ts=8 cino=t0,(0: */
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

#include "debug.h"
#include "poi-item.h"

using namespace Mappero;

#define DEFAULT_WIDTH 200
#define DEFAULT_HEIGHT 200

namespace Mappero {

class PoiItemPrivate
{
    Q_DECLARE_PUBLIC(PoiItem)

public:
    PoiItemPrivate(PoiItem *poiItem);

    static QPoint originFromSize(const QSize &size) {
        return QPoint(size.width() / 2, size.height());
    }

private:
    QPoint location;
    QPoint origin;
    mutable PoiItem *q_ptr;
};

}; // namespace

PoiItemPrivate::PoiItemPrivate(PoiItem *poiItem):
    location(QPoint()),
    origin(originFromSize(QSize(DEFAULT_WIDTH, DEFAULT_HEIGHT))),
    q_ptr(poiItem)
{
}

PoiItem::PoiItem(QDeclarativeItem *parent):
    QDeclarativeItem(parent),
    d_ptr(new PoiItemPrivate(this))
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations);

    setImplicitWidth(DEFAULT_WIDTH);
    setImplicitHeight(DEFAULT_HEIGHT);
}

PoiItem::~PoiItem()
{
    delete d_ptr;
}

void PoiItem::setLocation(const QPoint &location)
{
    Q_D(PoiItem);

    if (location == d->location) return;

    d->location = location;
    setPos(d->location + d->origin);
    Q_EMIT locationChanged();
}

QPoint PoiItem::location() const
{
    Q_D(const PoiItem);
    return d->location;
}

void PoiItem::geometryChanged(const QRectF &newGeometry,
                               const QRectF &oldGeometry)
{
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    QSize newSize = newGeometry.size().toSize();
    QSize oldSize = oldGeometry.size().toSize();
    if (newSize == oldSize) return;

    Q_D(PoiItem);
    d->origin = d->originFromSize(newSize);
    setTransform(QTransform::fromTranslate(-d->origin.x(), -d->origin.y()));
    setPos(d->location + d->origin);
}

