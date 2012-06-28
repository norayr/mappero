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
#include "map.h"
#include "poi-view.h"
#include "projection.h"
#include "types.h"

#include <QAbstractListModel>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
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
    QObject *createItem(VisualModelItem *modelItem, const QPoint &position);

    QVariant modelValue(int index, int role) {
        return model->data(model->index(index, 0), role);
    }

    QHash<int,QByteArray> roleNames() const { return model->roleNames(); }

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
        _item = poiViewPriv->createItem(this, position);
    }

    void updatePosition(const QPoint &position) {
        _item->setProperty("location", position);
    }

    void setItem(QObject *item) { _item = item; }
    QObject *item() const { return _item; }

    void setIndex(int index) { _index = index; }

    void setGeoPoint(const GeoPoint &geo) { _geo = geo; }
    GeoPoint geoPoint() const { return _geo; }

private:
    void refreshRoles();

private:
    PoiViewPrivate *poiViewPriv;
    int _index;
    QObject *_item;
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

PoiViewPrivate::PoiViewPrivate(PoiView *poiView):
    QObject(poiView),
    model(0),
    delegate(0),
    geoPointRole(0),
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

QObject *PoiViewPrivate::createItem(VisualModelItem *modelItem,
                                    const QPoint &position)
{
    Q_Q(PoiView);

    QDeclarativeContext *context =
        new QDeclarativeContext(delegate->creationContext(), modelItem);
    context->setContextObject(modelItem);
    context->setContextProperty("model", modelItem);
    QObject *object = delegate->beginCreate(context);
    object->setProperty("location", position);

    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(object);
    item->setParentItem(q);

    delegate->completeCreate();
    return object;
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
