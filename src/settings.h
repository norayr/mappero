/*
 * Copyright (C) 2006, 2007 John Costigan.
 *
 * POI and GPS-Info code originally written by Cezary Jackiewicz.
 *
 * Default map data provided by http://www.openstreetmap.org/
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

#ifndef MAEMO_MAPPER_SETTINGS_H
#define MAEMO_MAPPER_SETTINGS_H

void settings_init(GConfClient *gconf_client);
void settings_save(void);

gboolean settings_dialog(void);

#define GCONF_KEY_PREFIX "/apps/maemo/maemo-mapper"
#define GCONF_KEY_GPS_RCVR_TYPE GCONF_KEY_PREFIX"/gps_rcvr_type"
#define GCONF_KEY_GPS_BT_MAC GCONF_KEY_PREFIX"/receiver_mac"
#define GCONF_KEY_GPS_GPSD_HOST GCONF_KEY_PREFIX"/gps_gpsd_host"
#define GCONF_KEY_GPS_GPSD_PORT GCONF_KEY_PREFIX"/gps_gpsd_port"
#define GCONF_KEY_GPS_FILE_PATH GCONF_KEY_PREFIX"/gps_file_path"
#define GCONF_KEY_AUTO_DOWNLOAD GCONF_KEY_PREFIX"/auto_download"
#define GCONF_KEY_AUTO_DOWNLOAD_PRECACHE \
                                      GCONF_KEY_PREFIX"/auto_download_precache"
#define GCONF_KEY_CENTER_SENSITIVITY GCONF_KEY_PREFIX"/center_sensitivity"
#define GCONF_KEY_ENABLE_ANNOUNCE GCONF_KEY_PREFIX"/enable_announce"
#define GCONF_KEY_ANNOUNCE_NOTICE GCONF_KEY_PREFIX"/announce_notice"
#define GCONF_KEY_ROTATE_SENSITIVITY GCONF_KEY_PREFIX"/rotate_sensitivity"
#define GCONF_KEY_AC_MIN_SPEED GCONF_KEY_PREFIX"/autocenter_min_speed"
#define GCONF_KEY_ROTATE_DIR GCONF_KEY_PREFIX"/rotate_direction"
#define GCONF_KEY_DRAW_WIDTH GCONF_KEY_PREFIX"/draw_width"
#define GCONF_KEY_ENABLE_VOICE GCONF_KEY_PREFIX"/enable_voice"
#define GCONF_KEY_VOICE_SPEED GCONF_KEY_PREFIX"/voice_speed"
#define GCONF_KEY_VOICE_PITCH GCONF_KEY_PREFIX"/voice_pitch"
#define GCONF_KEY_FULLSCREEN GCONF_KEY_PREFIX"/fullscreen"
#define GCONF_KEY_UNITS GCONF_KEY_PREFIX"/units"
#define GCONF_KEY_SPEED_LIMIT_ON GCONF_KEY_PREFIX"/speed_limit_on"
#define GCONF_KEY_SPEED_LIMIT GCONF_KEY_PREFIX"/speed_limit"
#define GCONF_KEY_SPEED_LOCATION GCONF_KEY_PREFIX"/speed_location"
#define GCONF_KEY_UNBLANK_SIZE GCONF_KEY_PREFIX"/unblank_option"
#define GCONF_KEY_INFO_FONT_SIZE GCONF_KEY_PREFIX"/info_font_size"

#define GCONF_KEY_POI_DB GCONF_KEY_PREFIX"/poi_db"
#define GCONF_KEY_POI_ZOOM GCONF_KEY_PREFIX"/poi_zoom"

#define GCONF_KEY_AUTOCENTER_MODE GCONF_KEY_PREFIX"/autocenter_mode"
#define GCONF_KEY_AUTOCENTER_ROTATE GCONF_KEY_PREFIX"/autocenter_rotate"
#define GCONF_KEY_LEAD_AMOUNT GCONF_KEY_PREFIX"/lead_amount"
#define GCONF_KEY_LEAD_IS_FIXED GCONF_KEY_PREFIX"/lead_is_fixed"
#define GCONF_KEY_LAST_LAT GCONF_KEY_PREFIX"/last_latitude"
#define GCONF_KEY_LAST_LON GCONF_KEY_PREFIX"/last_longitude"
#define GCONF_KEY_LAST_ALT GCONF_KEY_PREFIX"/last_altitude"
#define GCONF_KEY_LAST_SPEED GCONF_KEY_PREFIX"/last_speed"
#define GCONF_KEY_LAST_HEADING GCONF_KEY_PREFIX"/last_heading"
#define GCONF_KEY_LAST_TIME GCONF_KEY_PREFIX"/last_timestamp"
#define GCONF_KEY_CENTER_LAT GCONF_KEY_PREFIX"/center_latitude"
#define GCONF_KEY_CENTER_LON GCONF_KEY_PREFIX"/center_longitude"
#define GCONF_KEY_CENTER_ANGLE GCONF_KEY_PREFIX"/center_angle"
#define GCONF_KEY_ZOOM GCONF_KEY_PREFIX"/zoom"
#define GCONF_KEY_ROUTEDIR GCONF_KEY_PREFIX"/route_directory"
#define GCONF_KEY_TRACKFILE GCONF_KEY_PREFIX"/track_file"
#define GCONF_KEY_SHOWZOOMLEVEL GCONF_KEY_PREFIX"/show_zoomlevel"
#define GCONF_KEY_SHOWSCALE GCONF_KEY_PREFIX"/show_scale"
#define GCONF_KEY_SHOWCOMPROSE GCONF_KEY_PREFIX"/show_comprose"
#define GCONF_KEY_SHOWTRACKS GCONF_KEY_PREFIX"/show_tracks"
#define GCONF_KEY_SHOWROUTES GCONF_KEY_PREFIX"/show_routes"
#define GCONF_KEY_SHOWVELVEC GCONF_KEY_PREFIX"/show_velocity_vector"
#define GCONF_KEY_SHOWPOIS GCONF_KEY_PREFIX"/show_poi"
#define GCONF_KEY_ENABLE_GPS GCONF_KEY_PREFIX"/enable_gps"
#define GCONF_KEY_ENABLE_TRACKING GCONF_KEY_PREFIX"/enable_tracking"
#define GCONF_KEY_ROUTE_LOCATIONS GCONF_KEY_PREFIX"/route_locations"
#define GCONF_KEY_ACTIVE_REPOSITORY GCONF_KEY_PREFIX"/active_repository"
#define GCONF_KEY_REPOSITORIES GCONF_KEY_PREFIX"/repositories_data"
#define GCONF_KEY_TILE_SOURCES GCONF_KEY_PREFIX"/tile_sources_data"
#define GCONF_KEY_CURRREPO GCONF_KEY_PREFIX"/curr_repo"
#define GCONF_KEY_GPS_INFO GCONF_KEY_PREFIX"/gps_info"
#define GCONF_KEY_ROUTER_NAME GCONF_KEY_PREFIX"/router_name"
#define GCONF_KEY_ROUTE_DL_RADIUS GCONF_KEY_PREFIX"/route_dl_radius"
#define GCONF_KEY_POI_DL_URL GCONF_KEY_PREFIX"/poi_dl_url"
#define GCONF_KEY_DEG_FORMAT GCONF_KEY_PREFIX"/deg_format"

#endif /* ifndef MAEMO_MAPPER_SETTINGS_H */
