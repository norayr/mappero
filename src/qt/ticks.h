/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_TICKS_H
#define MAP_TICKS_H

#include <QQuickPaintedItem>

namespace Mappero {

class Ticks: public QQuickPaintedItem
{
    Q_OBJECT

public:
    Ticks(QQuickItem *parent = 0);
    virtual ~Ticks();

    // reimplemented methods
    void paint(QPainter *painter);
};

}; // namespace

#endif /* MAP_TICKS_H */
