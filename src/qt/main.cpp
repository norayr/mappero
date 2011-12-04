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
#include "map.h"
#include "tiled-layer.h"
#include "view.h"

#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsScene>

static void addMap(QGraphicsScene *scene)
{
    Mappero::Map *map = new Mappero::Map;

    const Mappero::TiledLayer::Type *type =
        Mappero::TiledLayer::Type::get("XYZ_INV");
    Mappero::TiledLayer *layer =
        new Mappero::TiledLayer("OpenStreet", "OpenStreetMap I",
                                "http://tile.openstreetmap.org/%0d/%d/%d.png",
                                "png", type);

    map->setMainLayer(layer);

    Mappero::GeoPoint center;
    center.lat = 60.19997;
    center.lon = 24.94057;
    map->setCenter(center);

    scene->addItem(map);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    /* TODO: set search path depending on installation prefix */
    QDir::addSearchPath("qml", "../qml");
    QDir::addSearchPath("icon", "../../data/icons/scalable/");
    Mappero::Controller controller;
    Mappero::View view;
    controller.setView(&view);
    QFileInfo fi("qml:mappero.qml");
    view.rootContext()->setContextProperty("view", &view);
    view.setSource(QUrl::fromLocalFile(fi.absoluteFilePath()));
    view.setWindowTitle("Mappero");
    view.show();

    addMap(view.scene());
    view.centerOn(0, 0);
    //view.centerOn(305629531, 155308849);

    return app.exec();
}
