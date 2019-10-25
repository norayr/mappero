/*
 * Copyright (C) 2010-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "application.h"
#include "configuration.h"
#include "controller.h"
#include "gps.h"
#include "map.h"
#include "path-item.h"
#include "path-layer.h"
#include "plugin-manager.h"
#include "poi-item.h"
#include "poi-view.h"
#ifdef GEOTAGGING_ENABLED
#include "taggable-area.h"
#include "taggable-selection.h"
#include "taggable.h"
#include "ticks.h"
#endif
#include "tiled-layer.h"
#include "tracker.h"

#include "Mappero/types.h"
#include "MapperoUi/types.h"
#include <QAbstractListModel>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QFileInfo>

static QObject *createPluginManager(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine);

    Mappero::PluginManager *manager = new Mappero::PluginManager();
    QQmlEngine::setContextForObject(manager, engine->rootContext());
    manager->classBegin();
    return manager;
}

int main(int argc, char *argv[])
{
    Mappero::Application app(argc, argv);
    Q_INIT_RESOURCE(mappero_ui);

    Mappero::registerTypes();
    MapperoUi::registerTypes();
    qmlRegisterType<Mappero::Map>("Mappero", 1, 0, "MapView");
    qmlRegisterType<Mappero::TiledLayer>("Mappero", 1, 0, "TiledLayer");
    qmlRegisterType<Mappero::Tracker>("Mappero", 1, 0, "Tracker");
    qmlRegisterType<Mappero::PathItem>("Mappero", 1, 0, "PathItem");
    qmlRegisterType<Mappero::PathLayer>("Mappero", 1, 0, "PathLayer");
    qmlRegisterSingletonType<Mappero::PluginManager>("Mappero", 1, 0,
                                                     "PluginManager",
                                                     createPluginManager);
    qmlRegisterType<Mappero::PoiItem>("Mappero", 1, 0, "PoiItem");
    qmlRegisterType<Mappero::PoiView>("Mappero", 1, 0, "PoiView");
    qmlRegisterUncreatableType<Mappero::MapItem>("Mappero", 1, 0, "MapItem",
                                                 "C++ creation only");
    qmlRegisterType<Mappero::Layer>();
    qmlRegisterType<QAbstractItemModel>();
    qmlRegisterType<QAbstractListModel>();
    qmlRegisterType<Mappero::Configuration>();

    Mappero::Controller controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("gps", Mappero::Gps::instance());
    engine.rootContext()->setContextProperty("Mappero", &controller);

    engine.rootContext()->setContextProperty("firstPage", app.firstPage());

#ifdef GEOTAGGING_ENABLED
    qmlRegisterUncreatableType<Mappero::Taggable>("Mappero", 1, 0, "Taggable",
                                                  "C++ creation only");
    qmlRegisterType<Mappero::TaggableModel>();
    qmlRegisterType<Mappero::TaggableArea>("Mappero", 1, 0, "TaggableArea");
    qmlRegisterType<Mappero::TaggableSelection>();
    engine.addImageProvider(Mappero::Taggable::ImageProvider::name(),
                            Mappero::Taggable::ImageProvider::instance());
    qmlRegisterType<Mappero::Ticks>("Mappero", 1, 0, "Ticks");
#endif

    engine.addImportPath("qrc:/");
    engine.load(QUrl("qrc:/mappero.qml"));

    return app.exec();
}

#ifdef STATIC_BUILD
    #include <QQmlExtensionPlugin>
    Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
    Q_IMPORT_PLUGIN(QmlFolderListModelPlugin)
    Q_IMPORT_PLUGIN(QmlSettingsPlugin)
    Q_IMPORT_PLUGIN(QtQuick2Plugin)
    Q_IMPORT_PLUGIN(QtQuick2DialogsPlugin)
    Q_IMPORT_PLUGIN(QtQuick2DialogsPrivatePlugin)
    Q_IMPORT_PLUGIN(QtQuick2WindowPlugin)
    Q_IMPORT_PLUGIN(QtQuickControls1Plugin)
    Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin)
    // Image formsts
    Q_IMPORT_PLUGIN(QJpegPlugin)
    Q_IMPORT_PLUGIN(QSvgPlugin)
#endif
