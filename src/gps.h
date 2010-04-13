/*
 * Copyright (C) 2006, 2007 John Costigan.
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

#ifndef MAEMO_MAPPER_GPS_H
#define MAEMO_MAPPER_GPS_H

void set_conn_state(ConnState new_conn_state);

void map_controller_gps_connect(MapController *self);
void map_controller_gps_disconnect(MapController *self);

void map_controller_gps_set_effective_interval(MapController *self,
                                               gint interval);

void map_controller_gps_init(MapController *self, GConfClient *gconf_client);
void map_controller_gps_dispose(MapController *self);

#endif /* ifndef MAEMO_MAPPER_GPS_H */
