/*
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
 * along with libMappero. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <glib.h>
#include <glib-object.h>

void map_loader_read_dir(const gchar *path);
GList *map_loader_get_objects();

/* To fine-tune the loading, the following APIs could be added:
 *
 * list plugin IDs (could be the plugin filename):
 * const gchar * const *map_loader_list_plugins();
 *
 * list plugin object IDs:
 * const gchar * const *map_loader_list_plugin_objects(const gchar *plugin_id);
 *
 * instantiate an object:
 * GObject *map_loader_get_object(const gchar *plugin_id,
 *                                const gchar *object_id);
 */


#endif /* MAP_LOADER_H */
