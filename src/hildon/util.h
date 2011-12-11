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

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#ifndef MAEMO_MAPPER_UTIL_H
#define MAEMO_MAPPER_UTIL_H

#ifndef LEGACY
#    include <hildon/hildon-controlbar.h>
#else
#    include <hildon-widgets/hildon-controlbar.h>
#endif

void popup_error(GtkWidget *window, const gchar *error);

MapPoint locate_address(GtkWidget *parent, const gchar *address);

MapGeo calculate_distance(MapGeo lat1, MapGeo lon1,
        MapGeo lat2, MapGeo lon2);
MapGeo calculate_bearing(MapGeo lat1, MapGeo lon1,
        MapGeo lat2, MapGeo lon2);

void force_min_visible_bars(HildonControlbar *control_bar, gint num_bars);

void deg_format(MapGeo coor, gchar *scoor, gchar neg_char, gchar pos_char);

MapGeo strdmstod(const gchar *nptr, gchar **endptr);

gboolean parse_coords(const gchar* txt_lat, const gchar* txt_lon,
                      MapGeo *lat, MapGeo *lon);
void format_lat_lon(MapGeo d_lat, MapGeo d_lon, gchar* lat, gchar* lon);

gboolean coord_system_check_lat_lon (MapGeo lat, MapGeo lon, gint *fallback_deg_format);

gint64 g_ascii_strtoll(const gchar *nptr, gchar **endptr, guint base);
gint convert_str_to_int(const gchar *str);

gint gint_sqrt(gint num);

size_t time_to_string(gchar *string, size_t size, const gchar *format,
                      time_t time);
size_t duration_to_string(gchar *string, size_t size, guint duration);
size_t distance_to_string(gchar *string, size_t size, gfloat distance);
void map_util_format_distance(gfloat *distance, const gchar **unit,
                              gint *decimals);

void map_set_fast_mode(gboolean fast);

#endif /* ifndef MAEMO_MAPPER_UTIL_H */