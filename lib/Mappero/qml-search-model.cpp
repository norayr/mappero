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
#include "qml-search-model.h"

using namespace Mappero;

namespace Mappero {

enum Roles {
    GeoPointRole = Qt::UserRole + 1,
};

class QmlSearchModelPrivate
{
    friend class QmlSearchModel;

    inline QmlSearchModelPrivate();
    ~QmlSearchModelPrivate() {};

private:
    QStringList m_roles;
    QList<QVariantMap> m_data;
};

} // namespace

QmlSearchModelPrivate::QmlSearchModelPrivate()
{
    m_roles.append("geoPoint");
}

QmlSearchModel::QmlSearchModel(QObject *parent):
    QAbstractListModel(parent),
    d_ptr(new QmlSearchModelPrivate)
{
    QObject::connect(this, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    QObject::connect(this, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
}

QmlSearchModel::~QmlSearchModel()
{
    delete d_ptr;
}

void QmlSearchModel::setRoles(const QStringList &roles)
{
    Q_D(QmlSearchModel);
    if (roles == d->m_roles) return;

    Q_FOREACH(const QString &role, roles) {
        if (!d->m_roles.contains(role)) {
            d->m_roles.append(role);
        }
    }
    Q_EMIT rolesChanged();
}

QStringList QmlSearchModel::roles() const
{
    Q_D(const QmlSearchModel);
    return d->m_roles;
}

void QmlSearchModel::append(const QVariantMap &rowData)
{
    Q_D(QmlSearchModel);
    int count = d->m_data.count();
    beginInsertRows(QModelIndex(), count, count);
    d->m_data.append(rowData);
    endInsertRows();
}

void QmlSearchModel::clear()
{
    Q_D(QmlSearchModel);
    beginRemoveRows(QModelIndex(), 0, d->m_data.count() - 1);
    d->m_data.clear();
    endRemoveRows();
}

int QmlSearchModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QmlSearchModel);
    Q_UNUSED(parent);
    return d->m_data.count();
}

QVariant QmlSearchModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QmlSearchModel);

    if (index.row() >= d->m_data.count()) return QVariant();

    const QVariantMap &rowData = d->m_data.at(index.row());
    QString roleName = d->m_roles.at(role - (Qt::UserRole + 1));

    return rowData.value(roleName);
}

QHash<int, QByteArray> QmlSearchModel::roleNames() const
{
    Q_D(const QmlSearchModel);

    QHash<int, QByteArray> roles;
    int i = Qt::UserRole + 1;
    Q_FOREACH(const QString &role, d->m_roles) {
        roles.insert(i++, role.toUtf8());
    }
    return roles;
}
