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
#include "plugin-manager.h"
#include "plugin-model.h"

#include "Mappero/Plugin"
#include "Mappero/RoutingPlugin"
#include "Mappero/SearchPlugin"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMap>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QStringList>

using namespace Mappero;

namespace Mappero {

struct PluginData {
    QVariantMap manifestData;
    Plugin *plugin;
};

class PluginManagerPrivate
{
    Q_DECLARE_PUBLIC(PluginManager)

    inline PluginManagerPrivate(PluginManager *q);
    inline ~PluginManagerPrivate();

    QVariantMap parseManifest(const QFileInfo &manifest);
    Plugin *createPlugin(const QVariantMap &manifestData);

    void clear();
    void reload();

private:
    mutable PluginManager *q_ptr;
    QString m_baseDir;
    bool m_isUninstalled;
    QMap<QString,PluginData> m_plugins;
    QHash<QString,PluginModel*> m_models;
};

} // namespace

PluginManagerPrivate::PluginManagerPrivate(PluginManager *q):
    q_ptr(q),
    m_baseDir(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                     PLUGIN_MANIFEST_DIR,
                                     QStandardPaths::LocateDirectory)),
    m_isUninstalled(false)
{
    DEBUG() << "basedir:" << m_baseDir;
}

PluginManagerPrivate::~PluginManagerPrivate()
{
    clear();
}

QVariantMap PluginManagerPrivate::parseManifest(const QFileInfo &manifest)
{
    QVariantMap ret;

    QFile file(manifest.filePath());
    if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly | QIODevice::Text))) {
        qWarning() << "Couldn't open file" << manifest.filePath();
        return ret;
    }

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &error);
    if (Q_UNLIKELY(json.isEmpty())) {
        qWarning() << "File is empty:" << manifest.filePath() <<
            error.errorString();
        return ret;
    }

    ret = json.toVariant().toMap();
    // Add the manifest base name as ID */
    ret.insert(QStringLiteral(MAPPERO_PLUGIN_KEY_NAME),
               manifest.completeBaseName());
    return ret;
}

Plugin *PluginManagerPrivate::createPlugin(const QVariantMap &manifestData)
{
    Q_Q(PluginManager);

    /* TODO: implement loading of binary plugins */
    QString pluginType = Plugin::type(manifestData);
    if (pluginType == QStringLiteral(MAPPERO_PLUGIN_TYPE_SEARCH)) {
        return new SearchPlugin(manifestData, q);
    } else if (pluginType == QStringLiteral(MAPPERO_PLUGIN_TYPE_ROUTING)) {
        return new RoutingPlugin(manifestData, q);
    } else {
        return 0;
    }
}

void PluginManagerPrivate::clear()
{
    QMutableMapIterator<QString,PluginData> it(m_plugins);
    while (it.hasNext()) {
        it.next();
        PluginData &pluginData = it.value();
        delete pluginData.plugin;
    }
    m_plugins.clear();
}

void PluginManagerPrivate::reload()
{
    clear();
    QDir path(m_baseDir);
    QStringList manifestDirComponents =
        QString(PLUGIN_MANIFEST_DIR).split('/', QString::SkipEmptyParts);
    QString relativePluginPath;
    for (int i = 0; i < manifestDirComponents.count(); i++) {
        relativePluginPath += "../";
    }
    relativePluginPath += PLUGIN_QML_DIR;

    Q_FOREACH(QFileInfo fileInfo, path.entryInfoList()) {
        if (fileInfo.fileName()[0] == '.') continue;
        QVariantMap manifestData;
        if (fileInfo.suffix() == "manifest") {
            manifestData = parseManifest(fileInfo);
            QString name =
                manifestData.value(QStringLiteral(MAPPERO_PLUGIN_KEY_NAME))
                .toString();
            QDir manifestDir = fileInfo.dir();
            if (!manifestDir.cd(relativePluginPath) || !manifestDir.cd(name)) {
                qWarning() <<
                    "Cannot find QML dir for plugin" << fileInfo.fileName() <<
                    " - manifest:" << manifestDir <<
                    " - relative:" << relativePluginPath;
            }
            /* Append the name of the directory, so that plugins will load from
             * there */
            manifestData.insert(QLatin1String(MAPPERO_PLUGIN_KEY_BASEDIR),
                                manifestDir.absolutePath());
        }
        if (manifestData.isEmpty()) continue;

        QString name = Plugin::name(manifestData);
        PluginData pluginData;
        pluginData.manifestData = manifestData;
        pluginData.plugin = 0;
        m_plugins.insert(name, pluginData);
    }
}

PluginManager::PluginManager(QObject *parent):
    QObject(parent),
    d_ptr(new PluginManagerPrivate(this))
{
}

PluginManager::~PluginManager()
{
    delete d_ptr;
}

QObject *PluginManager::loadPlugin(const QString &name)
{
    Q_D(PluginManager);

    QMap<QString,PluginData>::iterator it = d->m_plugins.find(name);
    if (Q_UNLIKELY(it == d->m_plugins.end())) return 0;

    PluginData &pluginData = it.value();

    if (pluginData.plugin == 0) {
        Plugin *plugin = d->createPlugin(pluginData.manifestData);
        QQmlEngine::setContextForObject(plugin,
                                        QQmlEngine::contextForObject(this));
        pluginData.plugin = plugin;
    }

    return pluginData.plugin;
}

QAbstractListModel *PluginManager::pluginModel(const QString &type)
{
    Q_D(PluginManager);
    PluginModel *&model = d->m_models[type];
    if (model == 0) {
        QList<QVariantMap> plugins;
        Q_FOREACH(const PluginData &pluginData, d->m_plugins) {
            if (Plugin::type(pluginData.manifestData) == type) {
                plugins.append(pluginData.manifestData);
            }
        }
        model = new PluginModel(this);
        model->setPlugins(plugins);
    }

    return model;
}

void PluginManager::classBegin()
{
    Q_D(PluginManager);
    d->reload();
}

void PluginManager::componentComplete()
{
}
