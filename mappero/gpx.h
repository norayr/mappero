/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifndef MAP_GPX_H
#define MAP_GPX_H

#include <mappero/path.h>
#include <mappero/poi.h>

#include <libgnomevfs/gnome-vfs.h> /* TODO: use GIO */
#include <gtk/gtk.h>

gboolean map_gpx_path_parse(MapPath *to_replace, gchar *buffer, gint size,
                            gint policy_old);
gboolean map_gpx_path_write(MapPath *path, GnomeVFSHandle *handle);

gint map_gpx_poi_parse(gchar *buffer, gint size, GList **list);
gint map_gpx_poi_write(GtkTreeModel *model, GnomeVFSHandle *handle);

#endif /* MAP_GPX_H */
