/*
 * Copyright (C) 2010 Max Lapan
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
#ifndef __REPOSITORY_H__
#define __REPOSITORY_H__

#include "types.h"

gchar* repository_list_to_xml(GList *repositories);
GList* repository_xml_to_list(const gchar *data);

Repository* repository_create_default_lists(GList **tile_sources,
                                            GList **repositories);
gboolean repository_tile_sources_can_expire(Repository *repository);

void repository_free(Repository *repo);
void repository_list_edit_dialog();

#endif /* __REPOSITORY_H__ */
