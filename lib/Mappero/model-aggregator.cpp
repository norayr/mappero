/*
 * Copyright (C) 2013-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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
#include "model-aggregator.h"

using namespace Mappero;

namespace Mappero {

typedef QQmlListProperty<QObject> ModelList;
typedef QMap<const QAbstractItemModel*,int> ModelRoleNames;

class ModelAggregatorPrivate: public QObject
{
    Q_DECLARE_PUBLIC(ModelAggregator)

    enum Roles {
        ModelNameRole = Qt::UserRole + 1,
        LastStaticRole
    };

    inline ModelAggregatorPrivate(ModelAggregator *q);
    ~ModelAggregatorPrivate() {};

    void updateRolesMapping();
    void watchModel(QAbstractItemModel *model);

    static void itemAppend(ModelList *p, QObject *o);
    static int itemCount(ModelList *p);
    static QObject *itemAt(ModelList *p, int idx);
    static void itemClear(ModelList *p);

private:
    mutable ModelAggregator *q_ptr;
    QList<QAbstractItemModel*> m_models;
    QHash<int, ModelRoleNames> m_rolesMapping;
    QHash<int, QByteArray> m_roleNames;
};

} // namespace

void ModelAggregatorPrivate::updateRolesMapping()
{
    m_roleNames.clear();
    m_rolesMapping.clear();

    /* First, insert the static roles */
    m_roleNames[ModelNameRole] = "modelName";
    int nextFreeRole = LastStaticRole + 1;


    QHash<QByteArray,int> allRoleNames;
    Q_FOREACH(const QAbstractItemModel *model, m_models) {
        QHash<int, QByteArray> roleNames = model->roleNames();

        /* Insert all this model's roles into the allRoleNames map. */
        QHash<int, QByteArray>::const_iterator i;
        for (i = roleNames.begin(); i != roleNames.end(); i++) {
            const QByteArray &roleName = i.value();
            int localRole = i.key();
            int globalRole = allRoleNames.value(roleName, 0);
            if (globalRole == 0) {
                globalRole = nextFreeRole++;
                allRoleNames.insert(roleName, globalRole);
                m_roleNames.insert(globalRole, roleName);
            }

            ModelRoleNames &modelRoleNames = m_rolesMapping[globalRole];
            modelRoleNames.insert(model, localRole);
        }
    }
}

void ModelAggregatorPrivate::watchModel(QAbstractItemModel *model)
{
    Q_Q(ModelAggregator);
    // TODO: rows added and removed
    QObject::connect(model, SIGNAL(modelReset()),
                     q, SIGNAL(modelReset()));
}

void ModelAggregatorPrivate::itemAppend(ModelList *p, QObject *o) {
    ModelAggregatorPrivate *d =
        reinterpret_cast<ModelAggregatorPrivate*>(p->data);
    QAbstractItemModel *listModel = qobject_cast<QAbstractItemModel*>(o);
    if (listModel) {
        d->m_models.append(listModel);
        d->watchModel(listModel);
        d->updateRolesMapping();
    }
}

int ModelAggregatorPrivate::itemCount(ModelList *p) {
    ModelAggregatorPrivate *d =
        reinterpret_cast<ModelAggregatorPrivate*>(p->data);
    return d->m_models.count();
}

QObject *ModelAggregatorPrivate::itemAt(ModelList *p, int idx) {
    ModelAggregatorPrivate *d =
        reinterpret_cast<ModelAggregatorPrivate*>(p->data);
    return d->m_models.at(idx);
}

void ModelAggregatorPrivate::itemClear(ModelList *p) {
    ModelAggregatorPrivate *d =
        reinterpret_cast<ModelAggregatorPrivate*>(p->data);
    d->m_models.clear();
    d->updateRolesMapping();
}

ModelAggregatorPrivate::ModelAggregatorPrivate(ModelAggregator *q):
    q_ptr(q)
{
}

ModelAggregator::ModelAggregator(QObject *parent):
    QAbstractItemModel(parent),
    d_ptr(new ModelAggregatorPrivate(this))
{
    QObject::connect(this, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    QObject::connect(this, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    QObject::connect(this, SIGNAL(modelReset()),
                     this, SIGNAL(countChanged()));
}

ModelAggregator::~ModelAggregator()
{
    delete d_ptr;
}

QQmlListProperty<QObject> ModelAggregator::models()
{
    return QQmlListProperty<QObject>(this, d_ptr,
                                     ModelAggregatorPrivate::itemAppend,
                                     ModelAggregatorPrivate::itemCount,
                                     ModelAggregatorPrivate::itemAt,
                                     ModelAggregatorPrivate::itemClear);
}

void ModelAggregator::clear()
{
    Q_D(ModelAggregator);
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    d->m_models.clear();
    endRemoveRows();
}

QModelIndex ModelAggregator::index(int row, int column,
                                   const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // TODO: use parent
    return createIndex(row, column);
}

QModelIndex ModelAggregator::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    // TODO use index
    return QModelIndex();
}

int ModelAggregator::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // TODO use parent
    return 1;
}

int ModelAggregator::rowCount(const QModelIndex &parent) const
{
    Q_D(const ModelAggregator);

    int count = 0;
    Q_FOREACH(const QAbstractItemModel *model, d->m_models) {
        count += model->rowCount(parent);
    }
    return count;
}

QVariant ModelAggregator::data(const QModelIndex &index, int role) const
{
    Q_D(const ModelAggregator);

    QModelIndex sourceIndex = mapToSource(index);
    if (Q_UNLIKELY(!sourceIndex.isValid())) return QVariant();

    const QAbstractItemModel *model = sourceIndex.model();
    if (role == ModelAggregatorPrivate::ModelNameRole) {
        return model->objectName();
    }

    /* Find the role numeric ID specific to this model. */
    const ModelRoleNames &modelRoleNames = d->m_rolesMapping[role];
    int localRole = modelRoleNames[model];
    return model->data(sourceIndex, localRole);
}

QHash<int, QByteArray> ModelAggregator::roleNames() const
{
    Q_D(const ModelAggregator);
    return d->m_roleNames;
}

QModelIndex ModelAggregator::mapToSource(const QModelIndex &index) const
{
    Q_D(const ModelAggregator);

    int row = index.row();

    Q_FOREACH(const QAbstractItemModel *model, d->m_models) {
        int count = model->rowCount(index.parent());
        if (row < count) {
            return model->index(row, index.column());
        } else {
            row -= count;
        }
    }

    return QModelIndex();
}
