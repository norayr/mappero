/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of libMappero.
 *
 * libMappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libMappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libMappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAP_UTIL_H
#define MAP_UTIL_H

#include <mappero/globals.h>

MapGeo map_calculate_distance(MapGeo lat1, MapGeo lon1,
                              MapGeo lat2, MapGeo lon2);
MapGeo map_calculate_bearing(MapGeo lat1, MapGeo lon1,
                             MapGeo lat2, MapGeo lon2);

#endif /* MAP_UTIL_H */
