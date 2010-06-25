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

#ifndef MAEMO_MAPPER_POI_H
#define MAEMO_MAPPER_POI_H

void poi_db_connect(void);

gboolean get_nearest_poi(const MapPoint *point, PoiInfo *poi);

gboolean select_poi(const MapPoint *point, PoiInfo *poi, gboolean quick);

gboolean category_list_dialog(GtkWidget *parent);

gboolean poi_add_dialog(GtkWidget *parent, const MapPoint *point);
gboolean poi_view_dialog(GtkWidget *parent, PoiInfo *poi);
gboolean poi_import_dialog(const MapPoint *point);
gboolean poi_download_dialog(const MapPoint *point);
gboolean poi_browse_dialog(const MapPoint *point);

gboolean poi_run_select_dialog(GtkTreeModel *model, PoiInfo *poi);
GtkTreeModel *poi_get_model_for_area(MapArea *area);

typedef void (*MapPoiRenderCb)(const MapPoint *point, GdkPixbuf *pixbuf,
                               gpointer user_data);
void map_poi_render(MapArea *area, MapPoiRenderCb callback, gpointer user_data);

void poi_destroy(void);

#endif /* ifndef MAEMO_MAPPER_POI_H */
