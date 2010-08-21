/* vi: set et sw=4 ts=8 cino=t0,(0: */
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

#include "view.h"
#include "view.moc.hpp"

#include "osm.h"

#include <QResizeEvent>
#include <QGLWidget>
#include <QGraphicsScene>
#include <QGraphicsView>

using namespace Map;

View::View(QGraphicsScene *scene):
    QGraphicsView(scene)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);
    setAttribute(Qt::WA_Maemo5NonComposited, true);
    setRenderHints(QPainter::Antialiasing);
    setViewport(new QGLWidget);

    /* create the OSM */
    osm = new Map::Osm();
    scene->addItem(osm);
}

void View::resizeEvent(QResizeEvent *event)
{
    osm->setSize(event->size());
}

