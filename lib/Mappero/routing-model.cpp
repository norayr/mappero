/*
 * Copyright (C) 2013 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "routing-model.h"

using namespace Mappero;

namespace Mappero {

typedef QQmlListProperty<QObject> ResourcesList;

class RoutingModelPrivate
{
    friend class RoutingModel;

    inline RoutingModelPrivate(RoutingModel *q);
    ~RoutingModelPrivate() {};

    static void itemAppend(ResourcesList *p, QObject *o) {
        RoutingModelPrivate *d = reinterpret_cast<RoutingModelPrivate*>(p->data);
        d->m_resources.append(o);
    }
    static int itemCount(ResourcesList *p) {
        RoutingModelPrivate *d = reinterpret_cast<RoutingModelPrivate*>(p->data);
        return d->m_resources.count();
    }
    static QObject *itemAt(ResourcesList *p, int idx) {
        RoutingModelPrivate *d = reinterpret_cast<RoutingModelPrivate*>(p->data);
        return d->m_resources.at(idx);
    }
    static void itemClear(ResourcesList *p) {
        RoutingModelPrivate *d = reinterpret_cast<RoutingModelPrivate*>(p->data);
        d->m_resources.clear();
    }

private:
    mutable RoutingModel *q_ptr;
    QList<Path> m_paths;
    QList<QObject*> m_resources;
    QPointF m_from;
    QPointF m_to;
    bool m_isRunning;
    QQmlComponent *m_wayPointDelegate;
    QQmlComponent *m_routeDelegate;
    QHash<int, QByteArray> m_roles;
};

} // namespace

RoutingModelPrivate::RoutingModelPrivate(RoutingModel *q):
    q_ptr(q),
    m_isRunning(false)
{
    m_roles[RoutingModel::PathRole] = "path";
}

RoutingModel::RoutingModel(QObject *parent):
    QAbstractListModel(parent),
    d_ptr(new RoutingModelPrivate(this))
{
    QObject::connect(this, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    QObject::connect(this, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
}

RoutingModel::~RoutingModel()
{
    delete d_ptr;
}

void RoutingModel::setFrom(const QPointF &point)
{
    Q_D(RoutingModel);
    if (point == d->m_from) return;
    d->m_from = point;
    Q_EMIT fromChanged();
}

QPointF RoutingModel::from() const
{
    Q_D(const RoutingModel);
    return d->m_from;
}

void RoutingModel::setTo(const QPointF &point)
{
    Q_D(RoutingModel);
    if (point == d->m_to) return;
    d->m_to = point;
    Q_EMIT toChanged();
}

QPointF RoutingModel::to() const
{
    Q_D(const RoutingModel);
    return d->m_to;
}

void RoutingModel::setRunning(bool isRunning)
{
    Q_D(RoutingModel);
    if (isRunning == d->m_isRunning) return;
    d->m_isRunning = isRunning;
    Q_EMIT isRunningChanged();
}

bool RoutingModel::isRunning() const
{
    Q_D(const RoutingModel);
    return d->m_isRunning;
}

void RoutingModel::setWayPointDelegate(QQmlComponent *delegate)
{
    Q_D(RoutingModel);

    if (delegate == d->m_wayPointDelegate) return;

    d->m_wayPointDelegate = delegate;
    Q_EMIT wayPointDelegateChanged();
}

QQmlComponent *RoutingModel::wayPointDelegate() const
{
    Q_D(const RoutingModel);
    return d->m_wayPointDelegate;
}

void RoutingModel::setRouteDelegate(QQmlComponent *delegate)
{
    Q_D(RoutingModel);

    if (delegate == d->m_routeDelegate) return;

    d->m_routeDelegate = delegate;
    Q_EMIT routeDelegateChanged();
}

QQmlComponent *RoutingModel::routeDelegate() const
{
    Q_D(const RoutingModel);
    return d->m_routeDelegate;
}

QQmlListProperty<QObject> RoutingModel::resources()
{
    return QQmlListProperty<QObject>(this, d_ptr,
                                     RoutingModelPrivate::itemAppend,
                                     RoutingModelPrivate::itemCount,
                                     RoutingModelPrivate::itemAt,
                                     RoutingModelPrivate::itemClear);
}

void RoutingModel::run()
{
    /* This is a virtual method. Nothing to be done here in the base class. */
}

void RoutingModel::appendPath(const Path &path)
{
    Q_D(RoutingModel);
    int count = d->m_paths.count();
    beginInsertRows(QModelIndex(), count, count);
    d->m_paths.append(path);
    endInsertRows();
}

Path RoutingModel::getPath(int row) const
{
    Q_D(const RoutingModel);
    return d->m_paths[row];
}

void RoutingModel::clear()
{
    Q_D(RoutingModel);
    beginRemoveRows(QModelIndex(), 0, d->m_paths.count() - 1);
    d->m_paths.clear();
    endRemoveRows();
}

int RoutingModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const RoutingModel);
    Q_UNUSED(parent);
    return d->m_paths.count();
}

QVariant RoutingModel::data(const QModelIndex &index, int role) const
{
    Q_D(const RoutingModel);
    Q_UNUSED(role);

    if (index.row() >= d->m_paths.count()) return QVariant();

    return QVariant::fromValue(d->m_paths.at(index.row()));
}

QHash<int, QByteArray> RoutingModel::roleNames() const
{
    Q_D(const RoutingModel);
    return d->m_roles;
}
