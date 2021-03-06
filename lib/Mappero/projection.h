/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2009-2020 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAPPERO_PROJECTION_H
#define MAPPERO_PROJECTION_H

#include "types.h"

namespace Mappero {

class MAPPERO_EXPORT Projection {
public:
    typedef Point (*GeoToUnit)(const GeoPoint &geo);
    typedef GeoPoint (*UnitToGeo)(const Point &unit);
    enum Type {
        GOOGLE,
        YANDEX,
        LAST
    };
    const char *name;
    GeoToUnit geoToUnit;
    UnitToGeo unitToGeo;
    static const Projection *get(Type type);
};

};

#endif /* MAPPERO_PROJECTION_H */
