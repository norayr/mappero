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

#ifndef MAP_POI_H
#define MAP_POI_H

#include <mappero/globals.h>

/* FIXME: all the types defined here should be reviewed */

typedef enum
{
    MAP_POI_SELECTED,
    MAP_POI_POIID,
    MAP_POI_CATID,
    MAP_POI_LAT,
    MAP_POI_LON,
    MAP_POI_LATLON,
    MAP_POI_BEARING,
    MAP_POI_DISTANCE,
    MAP_POI_LABEL,
    MAP_POI_DESC,
    MAP_POI_CLABEL,
    MAP_POI_NUM_COLUMNS
} MapPoiList;

typedef struct _MapPoiInfo MapPoiInfo;
struct _MapPoiInfo {
    gint poi_id;
    gint cat_id;
    MapGeo lat;
    MapGeo lon;
    gchar *label;
    gchar *desc;
    gchar *clabel;
};

#endif /* MAP_POI_H */
