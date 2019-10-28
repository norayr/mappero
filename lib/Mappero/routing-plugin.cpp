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
#include "path.h"
#include "routing-model.h"
#include "routing-plugin.h"

#include <QQmlContext>
#include <QQmlEngine>

using namespace Mappero;

namespace Mappero {

class RoutingPluginPrivate
{
    friend class RoutingPlugin;

    inline RoutingPluginPrivate(RoutingPlugin *q);
    ~RoutingPluginPrivate() {};

private:
    mutable RoutingPlugin *q_ptr;
};

} // namespace

RoutingPluginPrivate::RoutingPluginPrivate(RoutingPlugin *q):
    q_ptr(q)
{
}

RoutingPlugin::RoutingPlugin(const QVariantMap &manifestData, QObject *parent):
    Plugin(manifestData, parent),
    d_ptr(new RoutingPluginPrivate(this))
{
}

RoutingPlugin::~RoutingPlugin()
{
    delete d_ptr;
}

QQmlComponent *RoutingPlugin::optionsUi()
{
    /* The default implementation loads the delegate defined in the manifest
     * file, if any. */
    static const QString key =
        QStringLiteral(MAPPERO_ROUTING_PLUGIN_KEY_OPTIONS_UI_DELEGATE);
    if (manifestData().contains(key)) {
        return loadComponent(key);
    }

    return 0;
}

RoutingModel *RoutingPlugin::routingModel()
{
    /* The default implementation loads the delegate defined in the manifest
     * file, if any. */
    static const QString key =
        QStringLiteral(MAPPERO_ROUTING_PLUGIN_KEY_ROUTING_MODEL);
    if (!manifestData().contains(key)) return 0;

    QQmlComponent *component = loadComponent(key);
    if (Q_UNLIKELY(!component)) return 0;

    QQmlContext *context =
        new QQmlContext(QQmlEngine::contextForObject(this), this);
    context->setContextProperty(QStringLiteral("plugin"), this);
    QObject *model = component->create(context);
    return qobject_cast<RoutingModel*>(model);
}
