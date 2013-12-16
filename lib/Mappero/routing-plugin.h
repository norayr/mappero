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

#ifndef MAPPERO_ROUTING_PLUGIN_H
#define MAPPERO_ROUTING_PLUGIN_H

#include "plugin.h"

#include <QObject>

namespace Mappero {

class RoutingModel;

#define MAPPERO_ROUTING_PLUGIN_KEY_ROUTING_MODEL "routingModel"
#define MAPPERO_ROUTING_PLUGIN_KEY_OPTIONS_UI_DELEGATE "optionsUiDelegate"

class RoutingPluginPrivate;
class RoutingPlugin: public Plugin
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *optionsUi READ optionsUi CONSTANT)

public:
    RoutingPlugin(const QVariantMap &manifestData, QObject *parent = 0);
    ~RoutingPlugin();

    virtual QQmlComponent *optionsUi();
    Q_INVOKABLE virtual Mappero::RoutingModel *routingModel();

private:
    RoutingPluginPrivate *d_ptr;
    Q_DECLARE_PRIVATE(RoutingPlugin)
};

} // namespace

#endif // MAPPERO_ROUTING_PLUGIN_H
