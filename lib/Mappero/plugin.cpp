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

#include "debug.h"
#include "plugin.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QStringList>

using namespace Mappero;

namespace Mappero {

class PluginPrivate
{
    Q_DECLARE_PUBLIC(Plugin)

    inline PluginPrivate(Plugin *q, const QVariantMap &manifestData);
    ~PluginPrivate() {};

    static QUrl resolvePath(const QVariantMap &manifestData,
                            const QString &path);

private:
    mutable Plugin *q_ptr;
    QVariantMap m_data;
    QMap<QString,QQmlComponent*> m_loadedComponents;
};

} // namespace

PluginPrivate::PluginPrivate(Plugin *q, const QVariantMap &manifestData):
    q_ptr(q),
    m_data(manifestData)
{
}

QUrl PluginPrivate::resolvePath(const QVariantMap &manifestData,
                                const QString &path)
{
    static const QString keyBaseDir =
        QStringLiteral(MAPPERO_PLUGIN_KEY_BASEDIR);

    if (path.isEmpty()) return QUrl();

    if (path.startsWith("/")) {
        return QUrl::fromLocalFile(path);
    } else if (manifestData.contains(keyBaseDir)) {
        /* The PluginManager has already computed the location for us */
        return QUrl::fromLocalFile(manifestData.value(keyBaseDir).toString() +
                                   '/' + path);
    } else {
        /* Assume it's a URL */
        return QUrl::fromUserInput(path);
    }
}

Plugin::Plugin(const QVariantMap &manifestData, QObject *parent):
    QObject(parent),
    d_ptr(new PluginPrivate(this, manifestData))
{
}

Plugin::~Plugin()
{
    delete d_ptr;
}

QString Plugin::name(const QVariantMap &manifestData)
{
    static const QString keyName = QStringLiteral(MAPPERO_PLUGIN_KEY_NAME);
    return manifestData.value(keyName).toString();
}

QString Plugin::displayName(const QVariantMap &manifestData)
{
    static const QString keyName =
        QStringLiteral(MAPPERO_PLUGIN_KEY_DISPLAY_NAME);
    return manifestData.value(keyName).toString();
}

QUrl Plugin::icon(const QVariantMap &manifestData)
{
    static const QString keyIcon = QStringLiteral(MAPPERO_PLUGIN_KEY_ICON);
    QString iconName = manifestData.value(keyIcon).toString();
    return PluginPrivate::resolvePath(manifestData, iconName);
}

QString Plugin::type(const QVariantMap &manifestData)
{
    static const QString keyType = QStringLiteral(MAPPERO_PLUGIN_KEY_TYPE);
    return manifestData.value(keyType).toString();
}

QString Plugin::translations(const QVariantMap &manifestData)
{
    static const QString keyTranslations =
        QStringLiteral(MAPPERO_PLUGIN_KEY_TRANSLATIONS);
    return manifestData.value(keyTranslations).toString();
}

QString Plugin::name() const
{
    Q_D(const Plugin);
    return name(d->m_data);
}

QString Plugin::displayName() const
{
    Q_D(const Plugin);
    return displayName(d->m_data);
}

QUrl Plugin::icon() const
{
    Q_D(const Plugin);
    return icon(d->m_data);
}

QString Plugin::type() const
{
    Q_D(const Plugin);
    return type(d->m_data);
}

QString Plugin::translations() const
{
    Q_D(const Plugin);
    return translations(d->m_data);
}

const QVariantMap &Plugin::manifestData() const
{
    Q_D(const Plugin);
    return d->m_data;
}

QUrl Plugin::resolvePath(const QString &path) const
{
    Q_D(const Plugin);
    return PluginPrivate::resolvePath(d->m_data, path);
}

QQmlComponent *Plugin::loadComponent(const QString &key)
{
    Q_D(Plugin);

    if (d->m_loadedComponents.contains(key))
        return d->m_loadedComponents[key];

    QString path = manifestData().value(key).toString();
    QUrl componentUrl = resolvePath(path);
    if (componentUrl.isEmpty()) return 0;

    QQmlContext *context = QQmlEngine::contextForObject(this);
    if (Q_UNLIKELY(context == 0)) return 0;

    QQmlComponent *component =
        new QQmlComponent(context->engine(), componentUrl, this);
    if (Q_UNLIKELY(component->isError())) {
        qWarning() << "Error:" << component->errors();
        delete component;
        component = 0;
        /* We don't just return here: we add the NULL pointer to the
         * m_loadedComponents map, to avoid reloading this invalid component
         * again. */
    }

    d->m_loadedComponents.insert(key, component);
    return component;
}
