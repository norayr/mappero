/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2009-2012 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include <Mappero/types.h>

#include <QObject>

namespace Mappero {

class Configuration;
class Projection;
class TileCache;
class TileDownload;
class View;

class ControllerPrivate;
class Controller: public QObject
{
    Q_OBJECT
    Q_PROPERTY(Mappero::Configuration *conf READ configuration CONSTANT);
    Q_PROPERTY(qreal uiScale READ uiScale CONSTANT);

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

    static qint64 clock();

public Q_SLOTS:
    GeoPoint geo(qreal lat, qreal lon) { return GeoPoint(lat, lon); }
    QPointF point(const GeoPoint &geo) { return geo.toPointF(); }
    QString formatOffset(int seconds);
    QString formatLength(qreal metres);
    qreal uiScale() const;

private:
    ControllerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Controller)
};

};


#endif /* MAP_CONTROLLER_H */
