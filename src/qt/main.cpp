/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "configuration.h"
#include "controller.h"
#include "gps.h"
#include "map.h"
#include "path-item.h"
#include "path-layer.h"
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
#include "view.h"

#include <QAbstractListModel>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("mardy.it");
    app.setApplicationName("mappero");

    qRegisterMetaType<Mappero::GeoPoint>("GeoPoint");
    qRegisterMetaTypeStreamOperators<Mappero::GeoPoint>("GeoPoint");
    qmlRegisterType<Mappero::Map>("Mappero", 1, 0, "MapView");
    qmlRegisterType<Mappero::TiledLayer>("Mappero", 1, 0, "TiledLayer");
    qmlRegisterType<Mappero::Tracker>("Mappero", 1, 0, "Tracker");
    qmlRegisterType<Mappero::PathItem>("Mappero", 1, 0, "PathItem");
    qmlRegisterType<Mappero::PathLayer>("Mappero", 1, 0, "PathLayer");
    qmlRegisterType<Mappero::PoiItem>("Mappero", 1, 0, "PoiItem");
    qmlRegisterType<Mappero::PoiView>("Mappero", 1, 0, "PoiView");
    qmlRegisterUncreatableType<Mappero::MapItem>("Mappero", 1, 0, "MapItem",
                                                 "C++ creation only");
    qmlRegisterType<Mappero::Layer>();
    qmlRegisterType<QAbstractListModel>();
    qmlRegisterType<Mappero::Configuration>();

    Mappero::Controller controller;
    Mappero::View view;
    controller.setView(&view);
    view.rootContext()->setContextProperty("view", &view);
    view.rootContext()->setContextProperty("gps", Mappero::Gps::instance());
    view.rootContext()->setContextProperty("Mappero", &controller);

    QString firstPage = "MainPage.qml";
#ifdef Q_WS_MAC
    if (1) {
#else
    if (app.arguments().contains("--geotag")) {
#endif
        firstPage = "GeoTagPage.qml";
    }
    view.rootContext()->setContextProperty("firstPage", firstPage);

#ifdef GEOTAGGING_ENABLED
    qmlRegisterType<Mappero::Taggable>();
    qmlRegisterType<Mappero::TaggableModel>();
    qmlRegisterType<Mappero::TaggableArea>("Mappero", 1, 0, "TaggableArea");
    qmlRegisterType<Mappero::TaggableSelection>();
    QQmlEngine *engine = view.rootContext()->engine();
    engine->addImageProvider(Mappero::Taggable::ImageProvider::name(),
                             Mappero::Taggable::ImageProvider::instance());
    qmlRegisterType<Mappero::Ticks>("Mappero", 1, 0, "Ticks");
#endif

    view.setSource(QUrl("qrc:/mappero.qml"));
    view.setTitle("Mappero");
#if defined MEEGO || defined MAEMO5
    view.showFullScreen();
#else
    view.show();
#endif

    return app.exec();
}
