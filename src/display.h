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

#ifndef MAEMO_MAPPER_DISPLAY_H
#define MAEMO_MAPPER_DISPLAY_H

#include "maps.h"

typedef struct {
    gdouble glat, glon;
    GtkWidget *fmt_combo;
    GtkWidget *lat;
    GtkWidget *lon;
    GtkWidget *lat_title;
    GtkWidget *lon_title;
} LatlonDialog;


gboolean gps_display_details(void);
void gps_display_data(void);
void gps_show_info(void);
void gps_details(void);

void map_render_segment(GdkGC *gc_norm, GdkGC *gc_alt,
        gint unitx1, gint unity1, gint unitx2, gint unity2);
void map_render_paths(void);

void update_gcs(void);

void map_move_mark(void);
void map_refresh_mark(gboolean force_redraw);
void map_force_redraw(void);


void map_center_unit_full(MapPoint new_center, gint zoom, gint rotate_angle);
void map_rotate(gint rotate_angle);
MapPoint map_calc_new_center(gint zoom);

gboolean thread_render_map(MapRenderTask *mrt);

gboolean latlon_dialog(gdouble lat, gdouble lon);

gboolean display_open_file(GtkWindow *parent,
                           GInputStream **input, GOutputStream **output,
                           gchar **dir, gchar **file,
                           GtkFileChooserAction chooser_action);

void map_download_refresh_idle(MapTileSpec *tile, GdkPixbuf *pixbuf,
                               const GError *error, gpointer user_data);

void display_init(void);

void map_display_on(void);

#endif /* ifndef MAEMO_MAPPER_DISPLAY_H */
