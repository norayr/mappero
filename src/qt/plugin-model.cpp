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
#include "plugin-model.h"

#include "Mappero/Plugin"

using namespace Mappero;

namespace Mappero {

class PluginModelPrivate
{
    friend class PluginModel;

    inline PluginModelPrivate();
    inline ~PluginModelPrivate();

private:
    QHash<int, QByteArray> m_roleNames;
    QList<QVariantMap> m_plugins;
};

} // namespace

PluginModelPrivate::PluginModelPrivate()
{
    m_roleNames[Qt::DisplayRole] = "displayName";
    m_roleNames[PluginModel::NameRole] = "name";
    m_roleNames[PluginModel::IconRole] = "icon";
    m_roleNames[PluginModel::TypeRole] = "type";
    m_roleNames[PluginModel::TranslationsRole] = "translations";
}

PluginModelPrivate::~PluginModelPrivate()
{
}

PluginModel::PluginModel(QObject *parent):
    QAbstractListModel(parent),
    d_ptr(new PluginModelPrivate)
{
}

PluginModel::~PluginModel()
{
    delete d_ptr;
}

void PluginModel::setPlugins(const QList<QVariantMap> &plugins)
{
    Q_D(PluginModel);
    beginResetModel();
    d->m_plugins = plugins;
    endResetModel();
}

QVariant PluginModel::get(int row, const QString &roleName) const
{
    int role = roleNames().key(roleName.toLatin1(), -1);
    return data(index(row, 0), role);
}

int PluginModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const PluginModel);
    Q_UNUSED(parent);
    return d->m_plugins.count();
}

QVariant PluginModel::data(const QModelIndex &index, int role) const
{
    Q_D(const PluginModel);

    if (index.row() >= d->m_plugins.count()) return QVariant();

    const QVariantMap &pluginData = d->m_plugins.at(index.row());
    QVariant ret;

    switch (role) {
    case Qt::DisplayRole:
        ret = Plugin::displayName(pluginData);
        break;
    case NameRole:
        ret = Plugin::name(pluginData);
        break;
    case IconRole:
        ret = Plugin::icon(pluginData);
        break;
    case TypeRole:
        ret = Plugin::type(pluginData);
        break;
    case TranslationsRole:
        ret = Plugin::translations(pluginData);
        break;
    }

    return ret;
}

QHash<int, QByteArray> PluginModel::roleNames() const
{
    Q_D(const PluginModel);
    return d->m_roleNames;
}
