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

#ifndef MAPPERO_MODEL_AGGREGATOR_H
#define MAPPERO_MODEL_AGGREGATOR_H

#include "global.h"

#include <QAbstractItemModel>
#include <QQmlListProperty>

namespace Mappero {

class ModelAggregatorPrivate;
class MAPPERO_EXPORT ModelAggregator: public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QQmlListProperty<QObject> models READ models)
    Q_CLASSINFO("DefaultProperty", "models")

public:
    ModelAggregator(QObject *parent = 0);
    ~ModelAggregator();

    QQmlListProperty<QObject> models();

    Q_INVOKABLE void clear();

    // reimplemented virtual methods
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void countChanged();

protected:
    QModelIndex mapToSource(const QModelIndex &index) const;

private:
    ModelAggregatorPrivate *d_ptr;
    Q_DECLARE_PRIVATE(ModelAggregator)
};

} // namespace

#endif // MAPPERO_MODEL_AGGREGATOR_H
