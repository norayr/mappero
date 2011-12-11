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
#include "view.h"

#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsScene>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<Mappero::GeoPoint>("GeoPoint");
    qmlRegisterType<Mappero::Map>("Mappero", 1, 0, "Map");

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

    view.centerOn(0, 0);

    return app.exec();
}
