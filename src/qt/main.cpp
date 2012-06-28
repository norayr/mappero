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

#include "controller.h"
#include "gps.h"
#include "map.h"
#include "poi-view.h"
#ifdef GEOTAGGING_ENABLED
#include "taggable.h"
#include "taggable-area.h"
#endif
#include "tracker.h"
#include "view.h"

#include <QAbstractListModel>
#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsScene>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("mardy.it");
    app.setApplicationName("mappero");

    qRegisterMetaType<Mappero::GeoPoint>("GeoPoint");
    qmlRegisterType<Mappero::Map>("Mappero", 1, 0, "MapView");
    qmlRegisterType<Mappero::Tracker>("Mappero", 1, 0, "Tracker");
    qmlRegisterType<Mappero::PoiView>("Mappero", 1, 0, "PoiView");
    qmlRegisterUncreatableType<Mappero::MapItem>("Mappero", 1, 0, "MapItem",
                                                 "C++ creation only");
    qmlRegisterType<QAbstractListModel>();

    Mappero::Controller controller;
    Mappero::View view;
    controller.setView(&view);
    view.rootContext()->setContextProperty("view", &view);
    view.rootContext()->setContextProperty("gps", Mappero::Gps::instance());

    QString firstPage = "MainPage.qml";
    if (app.arguments().contains("--geotag")) {
        firstPage = "GeoTagPage.qml";
    }
    view.rootContext()->setContextProperty("firstPage", firstPage);

#ifdef GEOTAGGING_ENABLED
    qmlRegisterType<Mappero::TaggableArea>("Mappero", 1, 0, "TaggableArea");
    QDeclarativeEngine *engine = view.rootContext()->engine();
    engine->addImageProvider(Mappero::Taggable::ImageProvider::name(),
                             Mappero::Taggable::ImageProvider::instance());
#endif

    view.setSource(QUrl("qrc:/mappero.qml"));
    view.setWindowTitle("Mappero");
#if defined MEEGO || defined MAEMO5
    view.showFullScreen();
#else
    view.show();
#endif

    view.centerOn(0, 0);

    return app.exec();
}
