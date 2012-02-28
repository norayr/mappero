/* vi: set et sw=4 ts=8 cino=t0,(0: */
/*
 * Copyright (C) 2009-2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_CONTROLLER_H
#define MAP_CONTROLLER_H

#include <QObject>

namespace Mappero {

class Configuration;
class Gps;
class Projection;
class TileCache;
class TileDownload;
class View;

class ControllerPrivate;
class Controller: public QObject
{
    Q_OBJECT

public:
    Controller(QObject *parent = 0);
    ~Controller();

    static Controller *instance();

    void setView(View *view);
    View *view() const;

    void setProjection(const Projection *projection);
    const Projection *projection() const;

    TileDownload *tileDownload() const;
    TileCache *tileCache() const;
    Configuration *configuration() const;
    Gps *gps() const;

private:
    ControllerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Controller)
};

};


#endif /* MAP_CONTROLLER_H */
