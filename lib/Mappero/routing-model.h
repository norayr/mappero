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

#ifndef MAPPERO_ROUTING_MODEL_H
#define MAPPERO_ROUTING_MODEL_H

#include "path.h"

#include <QAbstractListModel>
#include <QPointF>
#include <QQmlListProperty>

namespace Mappero {

class Path;

class RoutingModelPrivate;
class RoutingModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QPointF from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QPointF to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning \
               NOTIFY isRunningChanged)
    Q_PROPERTY(QQmlListProperty<QObject> resources READ resources);
    Q_CLASSINFO("DefaultProperty", "resources");

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
    };

    RoutingModel(QObject *parent = 0);
    ~RoutingModel();

    void setFrom(const QPointF &point);
    QPointF from() const;

    void setTo(const QPointF &point);
    QPointF to() const;

    void setRunning(bool isRunning);
    bool isRunning() const;

    QQmlListProperty<QObject> resources();

    Q_INVOKABLE virtual void run();

    Q_INVOKABLE void appendPath(const Path &path);
    Q_INVOKABLE Path getPath(int row) const;
    Q_INVOKABLE void clear();

    // reimplemented virtual methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void countChanged();
    void fromChanged();
    void toChanged();
    void isRunningChanged();

private:
    RoutingModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(RoutingModel)
};

} // namespace

#endif // MAPPERO_ROUTING_MODEL_H
