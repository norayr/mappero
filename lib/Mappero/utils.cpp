/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "utils.h"

#include "debug.h"

#include <cstdlib>

using namespace Mappero;

namespace {

qreal computeUiScale()
{
    qreal uiScale = 1.0;
    /* Ubuntu Touch might define this */
    qreal uiScaleTest = qgetenv("GRID_UNIT_PX").toFloat();
    if (uiScaleTest != 0.0) {
        uiScale = uiScaleTest / 8;
    }
    return uiScale;
}

} // namespace

Utils::Utils(QObject *parent):
    QObject(parent)
{
}

Utils::~Utils() = default;

QString Utils::formatOffset(int seconds)
{
    if (seconds / 30 == 0) return "=";
    if (seconds > 0) {
        return QString::fromLatin1("+%1:00").arg((seconds + 30) / 60);
    } else {
        return QString::fromLatin1("%1:00").arg((seconds - 30) / 60);
    }
}

QString Utils::formatLength(qreal metres)
{
    const char *unit;
    int decimals;

    if (metres >= 1000) {
        metres /= 1000;
        decimals = 1;
        unit = "km";
    } else {
        decimals = 0;
        unit = "m";
    }
    return QString::fromLatin1("%1%2").arg(metres, 0, 'f', decimals).arg(unit);
}

QString Utils::formatDuration(int ms)
{
    int seconds = ms / 1000;

    div_t d = div(seconds, 60 * 60 * 24);
    int days = d.quot;
    seconds = d.rem;

    d = div(seconds, 60 * 60);
    int hours = d.quot;
    seconds = d.rem;

    d = div(seconds, 60);
    int minutes = d.quot;
    seconds = d.rem;

    QStringList components;
    if (days > 0) {
        components.append(tr("%1 d").arg(days));
    }
    if (hours > 0) {
        components.append(tr("%1 h").arg(hours));
    }
    components.append(tr("%1 min").arg(minutes));
    if (days == 0 && hours == 0 && seconds > 0) {
        components.append(tr("%1 sec").arg(seconds));
    }
    return components.join(tr(", ", "time components separator"));
}

qreal Utils::uiScale() const
{
    static qreal uiScale = 0.0;
    if (uiScale == 0.0) {
        uiScale = computeUiScale();
    }
    return uiScale;
}
