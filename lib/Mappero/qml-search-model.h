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

#ifndef MAPPERO_QML_SEARCH_MODEL_H
#define MAPPERO_QML_SEARCH_MODEL_H

#include <QAbstractListModel>
#include <QStringList>

namespace Mappero {

class QmlSearchModelPrivate;
class QmlSearchModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList roles READ roles WRITE setRoles NOTIFY rolesChanged)

public:
    QmlSearchModel(QObject *parent = 0);
    ~QmlSearchModel();

    void setRoles(const QStringList &roles);
    QStringList roles() const;

    Q_INVOKABLE void append(const QVariantMap &rowData);
    Q_INVOKABLE void clear();

    // reimplemented virtual methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void rolesChanged();

private:
    QmlSearchModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QmlSearchModel)
};

} // namespace

#endif // MAPPERO_QML_SEARCH_MODEL_H
