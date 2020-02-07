/*
 * Copyright (C) 2013-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAPPERO_PLUGIN_MODEL_H
#define MAPPERO_PLUGIN_MODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QVariantMap>

namespace Mappero {

class PluginModelPrivate;
class PluginModel: public QAbstractListModel
{
    Q_OBJECT

public:
    PluginModel(QObject *parent = 0);
    ~PluginModel();

    enum Roles {
        NameRole = Qt::UserRole + 1,
        IconRole,
        TypeRole,
        TranslationsRole,
        ManifestRole,
    };
    void setPlugins(const QList<QVariantMap> &plugins);

    Q_INVOKABLE QVariant get(int row, const QString &roleName) const;

    // reimplemented virtual methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

private:
    PluginModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PluginModel)
};

} // namespace

#endif // MAPPERO_PLUGIN_MODEL_H
