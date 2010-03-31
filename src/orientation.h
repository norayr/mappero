/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Maemo Mapper.
 *
 * Maemo Mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Maemo Mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Maemo Mapper.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAP_ORIENTATION_H
#define MAP_ORIENTATION_H

typedef enum {
    MAP_ORIENTATION_AUTO,
    MAP_ORIENTATION_LANDSCAPE,
    MAP_ORIENTATION_PORTRAIT,
} MapOrientation;

void map_controller_set_orientation(MapController *self,
                                    MapOrientation orientation);
MapOrientation map_controller_get_orientation(MapController *self);

#endif /* MAP_ORIENTATION_H */
