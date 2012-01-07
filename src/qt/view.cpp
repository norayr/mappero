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
#include "view.h.moc"

#include <QDebug>
#include <QResizeEvent>
#include <QGLWidget>

using namespace Mappero;

View::View():
    QDeclarativeView()
{
    setResizeMode(SizeRootObjectToView);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifdef Q_WS_MAEMO5
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);
    setAttribute(Qt::WA_Maemo5NonComposited, true);
#endif
    setRenderHints(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setViewport(new QGLWidget);
}

void View::switchFullscreen()
{
    bool isFullScreen = windowState() & Qt::WindowFullScreen;
    if (isFullScreen)
        showNormal();
    else
        showFullScreen();
}

