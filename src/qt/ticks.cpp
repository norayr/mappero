/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2012-2019 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "debug.h"
#include "ticks.h"

#include <QPainter>

using namespace Mappero;

Ticks::Ticks(QQuickItem *parent):
    QQuickPaintedItem(parent)
{
}

Ticks::~Ticks()
{
}

void Ticks::paint(QPainter *painter)
{
    int w = width();
    int h = height();

    int intervals = 12;
    for (int i = 0; i <= intervals; i++)
    {
        int x = w * i / intervals;
        qreal ratio;
        if (i % 6 == 0) ratio = 1.0;
        else if (i % 3 == 0) ratio = 0.75;
        else ratio = 0.65;

        painter->drawLine(x, (1 - ratio) * h, x, ratio * h);
    }
    painter->drawLine(0, h / 2, w, h / 2);
}
