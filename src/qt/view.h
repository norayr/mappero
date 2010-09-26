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

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <QGraphicsView>

namespace Mappero {
class Osm;

class View: public QGraphicsView
{
    Q_OBJECT
public:
    View(QGraphicsScene *scene);

    virtual void resizeEvent(QResizeEvent *event);

public slots:
    void switchFullscreen();

private:
    Osm *osm;
};

};

#endif /* MAP_VIEW_H */
