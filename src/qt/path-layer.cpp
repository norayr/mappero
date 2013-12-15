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
#include "path-item.h"
#include "path-layer.h"

#include <Mappero/Path>
#include <QPainter>
#include <QPen>

using namespace Mappero;

namespace Mappero {

typedef QQmlListProperty<PathItem> PathList;

struct PathItemData {
    inline PathItemData(PathItem *pathItem);
    PathItem *pathItem;
    QPainterPath painterPath;
    QPen pen;
};

class PathLayerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(PathLayer)

    PathLayerPrivate(PathLayer *pathLayer):
        QObject(0),
        q_ptr(pathLayer)
    {
    }
    ~PathLayerPrivate() {};

    static void itemAppend(PathList *p, PathItem *o) {
        PathLayerPrivate *d = reinterpret_cast<PathLayerPrivate*>(p->data);
        d->addPathItem(o);
    }
    static int itemCount(PathList *p) {
        PathLayerPrivate *d = reinterpret_cast<PathLayerPrivate*>(p->data);
        return d->items.count();
    }
    static PathItem *itemAt(PathList *p, int idx) {
        PathLayerPrivate *d = reinterpret_cast<PathLayerPrivate*>(p->data);
        return d->items.at(idx)->pathItem;
    }
    static void itemClear(PathList *p) {
        PathLayerPrivate *d = reinterpret_cast<PathLayerPrivate*>(p->data);
        foreach (PathItemData *data, d->items) {
            delete data;
        }
        d->items.clear();
        Q_EMIT d->q_ptr->itemsChanged();
    }

    void itemAdded(QQuickItem *item);
    void itemRemoved(QQuickItem *item);
    void addPathItem(PathItem *pathItem);

    PathItemData *pathItemData(const PathItem *item);

public Q_SLOTS:
    void onPathChanged();
    void onPathPenChanged();

private:
    mutable PathLayer *q_ptr;
    QList<PathItemData *> items;
};
}; // namespace

PathItemData::PathItemData(PathItem *pathItem):
    pathItem(pathItem),
    pen(pathItem->color(), 4)
{
    pen.setCosmetic(true);
}

void PathLayerPrivate::itemAdded(QQuickItem *item)
{
    PathItem *pathItem = qobject_cast<PathItem*>(item);
    if (pathItem) {
        addPathItem(pathItem);
    }
}

void PathLayerPrivate::itemRemoved(QQuickItem *item)
{
    PathItem *pathItem = qobject_cast<PathItem*>(item);
    PathItemData *data = pathItemData(pathItem);
    if (data) {
        Q_Q(PathLayer);
        QObject::disconnect(pathItem, 0, this, 0);
        QObject::disconnect(pathItem, 0, q, 0);
        items.removeOne(data);
        Q_EMIT q->itemsChanged();
    }
}

void PathLayerPrivate::addPathItem(PathItem *pathItem)
{
    Q_Q(PathLayer);
    items.append(new PathItemData(pathItem));
    QObject::connect(pathItem, SIGNAL(pathChanged()),
                     this, SLOT(onPathChanged()));
    QObject::connect(pathItem, SIGNAL(colorChanged()),
                     this, SLOT(onPathPenChanged()));
    QObject::connect(pathItem, SIGNAL(opacityChanged()),
                     q, SLOT(update()));
    Q_EMIT q->itemsChanged();
}

PathItemData *PathLayerPrivate::pathItemData(const PathItem *item)
{
    /* Find the PathItemData; using a QMap would be more efficient, but only
     * when many paths are loaded -- which is unlikely. */
    int length = items.count();
    for (int i = 0; i < length; i++) {
        if (items[i]->pathItem == item) {
            return items[i];
        }
    }
    return 0;
}

void PathLayerPrivate::onPathChanged()
{
    Q_Q(PathLayer);

    PathItem *pathItem = static_cast<PathItem*>(sender());
    PathItemData *data = pathItemData(pathItem);
    if (Q_UNLIKELY(data == 0)) {
        qWarning() << "pathChanged() signal from unknown PathItem";
        return;
    }

    data->painterPath = pathItem->path().toPainterPath(q->map()->zoomLevel());
    q->update();
}

void PathLayerPrivate::onPathPenChanged()
{
    Q_Q(PathLayer);

    PathItem *pathItem = static_cast<PathItem*>(sender());
    PathItemData *data = pathItemData(pathItem);
    if (Q_UNLIKELY(data == 0)) {
        qWarning() << "signal from unknown PathItem";
        return;
    }

    data->pen = QPen(pathItem->color(), 4);
    q->update();
}

PathLayer::PathLayer(QQuickItem *parent):
    QQuickPaintedItem(parent),
    d_ptr(new PathLayerPrivate(this))
{
}

PathLayer::~PathLayer()
{
    delete d_ptr;
}

QQmlListProperty<PathItem> PathLayer::items()
{
    return PathList(this, d_ptr,
                    PathLayerPrivate::itemAppend,
                    PathLayerPrivate::itemCount,
                    PathLayerPrivate::itemAt,
                    PathLayerPrivate::itemClear);
}

void PathLayer::paint(QPainter *painter)
{
    Q_D(const PathLayer);

    QTransform transformation = painter->transform();

    qreal zoom = map()->zoomLevel();

    Point offset = -map()->centerUnits().toPixel(zoom);
    transformation.translate(offset.x() + width() / 2, offset.y() + height() / 2);

    painter->setTransform(transformation);
    painter->setRenderHints(QPainter::Antialiasing, true);
    foreach (PathItemData *data, d->items) {
        painter->setOpacity(data->pathItem->opacity());
        painter->setPen(data->pen);
        painter->drawPath(data->painterPath);
    }
}

void PathLayer::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(PathLayer);

    if (change == QQuickItem::ItemChildAddedChange) {
        d->itemAdded(value.item);
    } else if (change == QQuickItem::ItemChildRemovedChange) {
        d->itemRemoved(value.item);
    }

    QQuickItem::itemChange(change, value);
}

void PathLayer::mapEvent(MapEvent *event)
{
    Q_D(PathLayer);
    if (event->zoomLevelChanged() ||
        event->centerChanged()) {
        setX(-width() / 2);
        setY(-height() / 2);

        foreach (PathItemData *data, d->items) {
            data->painterPath =
                data->pathItem->path().toPainterPath(map()->zoomLevel());
        }
        update();
    }

    if (event->sizeChanged()) {
        QSizeF size = map()->boundingRect().size();
        qreal radius = qMax(size.width(), size.height());
        setWidth(radius);
        setHeight(radius);
        setX(-width() / 2);
        setY(-height() / 2);
    }
}

#include "path-layer.moc"
