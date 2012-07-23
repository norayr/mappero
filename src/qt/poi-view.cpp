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

#include "debug.h"
#include "map.h"
#include "poi-view.h"
#include "projection.h"

#include <QAbstractListModel>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeProperty>
#include <QDeclarativePropertyMap>
#include <QVector>

using namespace Mappero;

namespace Mappero {

class VisualModelItem;

class PoiViewPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(PoiView)

public:
    PoiViewPrivate(PoiView *poiView);

    void setModel(QAbstractListModel *newModel);
    void updateItems(int start, int end);
    void updateItemsPosition();
    QDeclarativeItem *createItem(VisualModelItem *modelItem);

    inline VisualModelItem *visualItemAt(int index);
    inline QDeclarativeItem *itemAt(int index);

    QVariant modelValue(int index, int role) {
        return model->data(model->index(index, 0), role);
    }

    QHash<int,QByteArray> roleNames() const { return model->roleNames(); }

    QPoint geoToPos(const GeoPoint &geo) const;

private Q_SLOTS:
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsRemoved(const QModelIndex &parent, int start, int end);
    void onDataChanged(const QModelIndex &topLeft,
                       const QModelIndex &bottomRight);

private:
    QAbstractListModel *model;
    QVector<VisualModelItem*> items;
    QDeclarativeComponent *delegate;
    int geoPointRole;
    int currentIndex;
    int highestZ;
    mutable PoiView *q_ptr;
};

class VisualModelItem: public QDeclarativePropertyMap
{
    Q_OBJECT

public:
    VisualModelItem(PoiViewPrivate *poiViewPriv, int index,
                    const QPoint &position):
        QDeclarativePropertyMap(poiViewPriv),
        poiViewPriv(poiViewPriv),
        _index(index),
        _item(0)
    {
        refreshRoles();
        insert("index", index);
        _item = poiViewPriv->createItem(this);
        setItemPosition(position);

        QDeclarativeProperty originProp(_item, "transformOriginPoint");
        originProp.connectNotifySignal(this,
                                       VisualModelItem::staticMetaObject.
                                       indexOfSlot("onOriginChanged()"));
    }

    ~VisualModelItem() {
        delete _item;
        _item = 0;
    }

    void updatePosition(const QPoint &position) {
        _item->setPos(position + origin());
    }

    QDeclarativeItem *item() const { return _item; }

    void setIndex(int index) { _index = index; insert("index", index); }

    void setGeoPoint(const GeoPoint &geo) { _geo = geo; }
    GeoPoint geoPoint() const { return _geo; }

    QPointF origin() const {
        if (_item == 0) return QPointF();
        QVariant origin = _item->property("transformOriginPoint");
        return origin.isValid() ? origin.toPointF() : QPointF();
    }

private:
    void refreshRoles();
    void setItemPosition(const QPointF &position);

private Q_SLOTS:
    void onOriginChanged() {
        DEBUG() << "origin changed";
        QPoint position = poiViewPriv->geoToPos(_geo);
        setItemPosition(position);
    }

private:
    PoiViewPrivate *poiViewPriv;
    int _index;
    QDeclarativeItem *_item;
    GeoPoint _geo;
};

}; // namespace

void VisualModelItem::refreshRoles()
{
    // TODO: this can be optimized
    QHash<int,QByteArray> roleNames = poiViewPriv->roleNames();

    for (QHash<int,QByteArray>::const_iterator i = roleNames.begin();
         i != roleNames.end();
         i++) {
        insert(i.value(), poiViewPriv->modelValue(_index, i.key()));
    }
}

void VisualModelItem::setItemPosition(const QPointF &position)
{
    QPointF origin = this->origin();
    _item->setTransform(QTransform::fromTranslate(-origin.x(), -origin.y()));
    _item->setPos(position + origin);
}

PoiViewPrivate::PoiViewPrivate(PoiView *poiView):
    QObject(poiView),
    model(0),
    delegate(0),
    geoPointRole(0),
    currentIndex(-1),
    highestZ(1),
    q_ptr(poiView)
{
}

void PoiViewPrivate::setModel(QAbstractListModel *newModel)
{
    if (newModel == model) return;

    if (model != 0) {
        model->disconnect(this);
    }

    model = newModel;

    if (model != 0) {
        geoPointRole = model->roleNames().key("geoPoint", 0);
        if (geoPointRole == 0) {
            // we cannot use this model
            return;
        }

        QObject::connect(model,
                         SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                         this,
                         SLOT(onRowsInserted(const QModelIndex&,int,int)));
        QObject::connect(model,
                         SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
                         this,
                         SLOT(onRowsRemoved(const QModelIndex&,int,int)));
        QObject::connect(model,
                         SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
                         this,
                         SLOT(onDataChanged(const QModelIndex&,const QModelIndex&)));

        items.resize(model->rowCount());
        updateItems(0, model->rowCount());
    }
}

void PoiViewPrivate::updateItems(int start, int end)
{
    Q_Q(PoiView);
    Map *map = q->map();
    if (map == 0) return;

    const Projection *projection = map->projection();
    if (projection == 0) return;

    if (geoPointRole == 0 || delegate == 0) return;

    Point mapCenter = map->centerUnits();
    qreal zoom = map->zoomLevel();

    QModelIndex parent;
    for (int i = start; i < end; i++) {
        GeoPoint geoPoint;
        QVariant data = model->data(model->index(i, 0, parent), geoPointRole);
        if (data.isValid()) {
            geoPoint = data.value<GeoPoint>();
        }

        if (geoPoint.isValid()) {
            VisualModelItem *item = items.at(i);
            Point position = projection->geoToUnit(geoPoint) - mapCenter;
            if (item != 0) {
                item->updatePosition(position.toPixel(zoom));
            } else {
                item = new VisualModelItem(this, i, position.toPixel(zoom));
                items[i] = item;
            }
            item->setGeoPoint(geoPoint);
        } else {
            delete items.at(i);
            items[i] = 0;
        }
    }
}

void PoiViewPrivate::updateItemsPosition()
{
    Q_Q(PoiView);
    Map *map = q->map();
    if (map == 0) return;

    const Projection *projection = map->projection();
    if (projection == 0) return;

    if (geoPointRole == 0 || delegate == 0) return;

    Point mapCenter = map->centerUnits();
    qreal zoom = map->zoomLevel();

    QModelIndex parent;
    foreach (VisualModelItem *item, items) {
        if (item == 0) continue;

        GeoPoint geoPoint = item->geoPoint();
        Point position = projection->geoToUnit(geoPoint) - mapCenter;
        item->updatePosition(position.toPixel(zoom));
    }
}

QDeclarativeItem *PoiViewPrivate::createItem(VisualModelItem *modelItem)
{
    Q_Q(PoiView);

    QDeclarativeContext *context =
        new QDeclarativeContext(delegate->creationContext(), modelItem);
    context->setContextObject(modelItem);
    context->setContextProperty("model", modelItem);
    context->setContextProperty("view", q);
    QObject *object = delegate->beginCreate(context);

    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(object);
    item->setParentItem(q);

    delegate->completeCreate();
    return item;
}

VisualModelItem *PoiViewPrivate::visualItemAt(int index)
{
    if (index < 0 || index >= items.count()) return 0;

    return items.at(index);
}

QDeclarativeItem *PoiViewPrivate::itemAt(int index)
{
    VisualModelItem *visualItem = visualItemAt(index);
    if (visualItem == 0) return 0;

    return visualItem->item();
}

QPoint PoiViewPrivate::geoToPos(const GeoPoint &geo) const
{
    Q_Q(const PoiView);
    Map *map = q->map();
    const Projection *projection = map->projection();
    qreal zoom = map->zoomLevel();
    Point position = projection->geoToUnit(geo) - map->centerUnits();
    return position.toPixel(zoom);
}

void PoiViewPrivate::onRowsInserted(const QModelIndex &, int first, int last)
{
    int begin = first;
    int end = last + 1;

    /* insert empty items */
    items.reserve(items.size() + end - begin);
    for (int i = begin; i < end; i++) {
        items.insert(i, 0);
    }

    /* update the index on the following items */
    int lastItem = items.count();
    for (int i = end; i < lastItem; i++) {
        VisualModelItem *item = items.at(i);
        if (item != 0) item->setIndex(i);
    }
    updateItems(begin, end);
}

void PoiViewPrivate::onRowsRemoved(const QModelIndex &, int first, int last)
{
    Q_UNUSED(first);
    Q_UNUSED(last);
    // FIXME TODO
}

void PoiViewPrivate::onDataChanged(const QModelIndex &topLeft,
                                   const QModelIndex &bottomRight)
{
    int begin = topLeft.row();
    int end = bottomRight.row() + 1;
    updateItems(begin, end);
}

PoiView::PoiView(QDeclarativeItem *parent):
    MapItem(parent),
    d_ptr(new PoiViewPrivate(this))
{
}

PoiView::~PoiView()
{
    delete d_ptr;
}

void PoiView::setModel(QAbstractListModel *model)
{
    Q_D(PoiView);
    return d->setModel(model);
}

QAbstractListModel *PoiView::model() const
{
    Q_D(const PoiView);
    return d->model;
}

void PoiView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(PoiView);

    if (delegate == d->delegate) return;

    // TODO: rebuild existing delegates
    d->delegate = delegate;
}

QDeclarativeComponent *PoiView::delegate() const
{
    Q_D(const PoiView);
    return d->delegate;
}

QRectF PoiView::itemArea() const
{
    Q_D(const PoiView);
    /* TODO cache the rect, emit the changed signal */
    Geo latMin = 100, latMax = -100, lonMin = 200, lonMax = -200;
    foreach (VisualModelItem *item, d->items) {
        if (item == 0) continue;

        GeoPoint geo = item->geoPoint();
        if (geo.lat < latMin)
            latMin = geo.lat;
        else if (geo.lat > latMax)
            latMax = geo.lat;
        if (geo.lon < lonMin)
            lonMin = geo.lon;
        else if (geo.lon > lonMax)
            lonMax = geo.lon;
    }
    if (latMin == 100) return QRectF(0, 0, -1, -1);
    if (latMax < latMin) latMax = latMin;
    if (lonMax < lonMin) lonMax = lonMin;
    return QRectF(latMin, lonMin, latMax - latMin, lonMax - lonMin);
}

void PoiView::setCurrentIndex(int index)
{
    Q_D(PoiView);

    if (index == d->currentIndex) return;

    d->currentIndex = index;
    Q_EMIT currentIndexChanged();

    VisualModelItem *visualItem = d->visualItemAt(index);
    if (visualItem != 0 && visualItem->geoPoint().isValid()) {
        QPoint origin = visualItem->origin().toPoint();
        QDeclarativeItem *item = visualItem->item();
        map()->ensureVisible(visualItem->geoPoint(),
                             item->width() / 2 - origin.x(),
                             item->height() / 2 - origin.y(),
                             qMax(item->width(), item->height()));
        item->setZValue(d->highestZ++);
    }
}

int PoiView::currentIndex() const
{
    Q_D(const PoiView);
    return d->currentIndex;
}

GeoPoint PoiView::itemPos(int index) const
{
    Q_D(const PoiView);
    if (index >= d->items.count()) return GeoPoint();

    VisualModelItem *visualItem = d->items.at(index);
    if (visualItem == 0) return GeoPoint();

    /* get the geoposition from the item position */
    Point mapCenter = map()->centerUnits();
    qreal zoom = map()->zoomLevel();
    const Projection *projection = map()->projection();

    QPointF p = visualItem->item()->pos() - visualItem->origin();
    return projection->unitToGeo(Point::fromPixel(p, zoom) + mapCenter);
}

void PoiView::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

QRectF PoiView::boundingRect() const
{
    return QRectF(-1.0e6, -1.0e6, 2.0e6, 2.0e6);
}

void PoiView::mapEvent(MapEvent *e)
{
    Q_D(PoiView);

    if (e->centerChanged() ||
        e->zoomLevelChanged()) {
        d->updateItemsPosition();
    }
}

#include "poi-view.moc"
