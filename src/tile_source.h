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
#ifndef __TILE_SOURCE_H__
#define __TILE_SOURCE_H__

#include "types.h"

#include <hildon/hildon-touch-selector.h>

const gchar *tile_source_format_extention(TileFormat format);
const gchar *tile_source_format_name(TileFormat format);
TileFormat   tile_source_format_by_name(const gchar *name);

const TileSourceType* tile_source_type_find_by_name(const gchar *name);
const TileSourceType* tile_source_get_primary_type();

gchar* tile_source_list_to_xml(GList *tile_sources);
GList* tile_source_xml_to_list(const gchar *data);

gboolean tile_source_compare(TileSource *ts1, TileSource *ts2);

void tile_source_free(TileSource *ts);

void tile_source_list_edit_dialog();
gboolean tile_source_edit_dialog(GtkWindow *parent, TileSource *ts);

void tile_source_fill_selector(HildonTouchSelector *selector, gboolean filter, gboolean transparent, TileSource *active);

#endif /* __TILE_SOURCE_H__ */
