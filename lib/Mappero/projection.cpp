/* vi: set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) 2010-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "projection.h"

#include <QDebug>
#include <QtMath>

using namespace Mappero;

static Point geo2unit_google(const GeoPoint &geo);
static GeoPoint unit2geo_google(const Point &unit);

static Point geo2unit_yandex(const GeoPoint &geo);
static GeoPoint unit2geo_yandex(const Point &unit);

// Keep this elements in sync with the Projection::Type enum!
static const Projection projections[] = {
    { "google", geo2unit_google, unit2geo_google },
    { "yandex", geo2unit_yandex, unit2geo_yandex },
};

const Projection *Projection::get(Type type)
{
    Q_ASSERT(type < LAST);
    return &projections[type];
}

#define MERCATOR_SPAN (-6.28318377773622)
#define MERCATOR_TOP (3.14159188886811)

static Point geo2unit_google(const GeoPoint &geo)
{
    Geo tmp;
    Unit x, y;

    x = (geo.lon + 180.0) * (WORLD_SIZE_UNITS / 360.0) + 0.5;
    tmp = GSIN(deg2rad(geo.lat));
    y = 0.5 + (WORLD_SIZE_UNITS / MERCATOR_SPAN) *
        (GLOG((1.0 + tmp) / (1.0 - tmp)) * 0.5 - MERCATOR_TOP);
    return Point(x, y);
}

static GeoPoint unit2geo_google(const Point &unit)
{
    GeoPoint geo;
    Geo tmp;
    geo.lon = (unit.x() * (360.0 / WORLD_SIZE_UNITS)) - 180.0;
    tmp = (unit.y() * (MERCATOR_SPAN / WORLD_SIZE_UNITS)) + MERCATOR_TOP;
    geo.lat = (360.0 * (GATAN(GEXP(tmp)))) * (1.0 / PI) - 90.0;
    return geo;
}

#define YANDEX_Rn (6378137.0)
#define YANDEX_E (0.0818191908426)
#define YANDEX_A (20037508.342789)
#define YANDEX_F (53.5865938 / 4) /* the 4 divisor accounts for 2 zoom levels */
#define YANDEX_AB (0.00335655146887969400)
#define YANDEX_BB (0.00000657187271079536)
#define YANDEX_CB (0.00000001764564338702)
#define YANDEX_DB (0.00000000005328478445)

static Point geo2unit_yandex(const GeoPoint &geo)
{
    Geo tmp, pow_tmp;
    Point unit;

    tmp = GTAN(M_PI_4 + deg2rad(geo.lat) / 2.0);
    pow_tmp = GPOW(GTAN(M_PI_4 + GASIN(YANDEX_E * GSIN(deg2rad(geo.lat))) /
                        2.0),
                   YANDEX_E);
    unit.setX((YANDEX_Rn * deg2rad(geo.lon) + YANDEX_A) * YANDEX_F);
    unit.setY((YANDEX_A - (YANDEX_Rn * GLOG(tmp / pow_tmp))) * YANDEX_F);
    return unit;
}

static GeoPoint unit2geo_yandex(const Point &unit)
{
    Geo xphi, x, y, lat, lon;

    x = (unit.x() / YANDEX_F) - YANDEX_A;
    y = YANDEX_A - (unit.y() / YANDEX_F);
    xphi = M_PI_2 - 2.0 * GATAN(1.0 / GEXP(y / YANDEX_Rn));
    lat = xphi + YANDEX_AB * GSIN(2.0 * xphi) + YANDEX_BB * GSIN(4.0 * xphi) +
        YANDEX_CB * GSIN(6.0 * xphi) + YANDEX_DB * GSIN(8.0 * xphi);
    lon = x / YANDEX_Rn;
    lat = rad2deg(qAbs(lat) > M_PI_2 ? M_PI_2 : lat);
    lon = rad2deg(qAbs(lon) > M_PI_2 ? M_PI_2 : lon);
    return GeoPoint(lat, lon);
}
