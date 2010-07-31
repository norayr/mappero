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

#ifndef MAP_KML_H
#define MAP_KML_H

#include <mappero/path.h>

#include <gio/gio.h>

typedef struct _MapKml MapKml;

MapKml *map_kml_new_from_stream(GInputStream *stream);
void map_kml_free(MapKml *kml);

MapPath *map_kml_get_path(MapKml *kml);

gboolean map_kml_path_parse(GInputStream *stream, MapPath *path);

#endif /* MAP_KML_H */
