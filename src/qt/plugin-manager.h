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

#ifndef MAPPERO_PLUGIN_MANAGER_H
#define MAPPERO_PLUGIN_MANAGER_H

#include <QObject>
#include <QQmlParserStatus>
#include <QStringList>

class QAbstractListModel;

namespace Mappero {

class Plugin;

class PluginManagerPrivate;
class PluginManager: public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

public:
    PluginManager(QObject *parent = 0);
    ~PluginManager();

    Q_INVOKABLE QObject *loadPlugin(const QString &name);
    Q_INVOKABLE QAbstractListModel *pluginModel(const QString &type);

    // reimplemented virtual methods
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

private:
    PluginManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PluginManager)
};

} // namespace

#endif // MAPPERO_PLUGIN_MANAGER_H
