/*
 * Copyright (C) 2006, 2007 John Costigan.
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
 * along with libMappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "gpx.h"

#include "debug.h"
#include "util.h"

#include <string.h>
#include <math.h>
#include <libxml/parser.h>

#define BUFFER_SIZE 2048
#define XML_DATE_FORMAT "%FT%T"

/** This enum defines the states of the SAX parsing state machine. */
typedef enum
{
    START,
    INSIDE_GPX,
    INSIDE_WPT,
    INSIDE_WPT_NAME,
    INSIDE_WPT_DESC,
    INSIDE_WPT_CMT,
    INSIDE_PATH,
    INSIDE_PATH_SEGMENT,
    INSIDE_PATH_POINT,
    INSIDE_PATH_POINT_ELE,
    INSIDE_PATH_POINT_TIME,
    INSIDE_PATH_POINT_DESC,
    INSIDE_PATH_POINT_CMT,
    FINISH,
    UNKNOWN,
    ERROR,
} SaxState;

/** Data used during the SAX parsing operation. */
typedef struct _SaxData SaxData;
struct _SaxData {
    SaxState state;
    SaxState prev_state;
    gint unknown_depth;
    gboolean at_least_one_trkpt;
    GString *chars;
};

typedef struct _PathSaxData PathSaxData;
struct _PathSaxData {
    SaxData sax_data;
    MapPath *path;
    MapPathPoint pt;
    MapDirection dir;
    gchar *desc;
};

typedef struct _PoiSaxData PoiSaxData;
struct _PoiSaxData {
    SaxData sax_data;
    GList *poi_list;
    MapPoiInfo *curr_poi;
};

/**
 * Handle a start tag in the parsing of a GPX file.
 */
#define MACRO_SET_UNKNOWN() { \
    data->sax_data.prev_state = data->sax_data.state; \
    data->sax_data.state = UNKNOWN; \
    data->sax_data.unknown_depth = 1; \
}

static gchar XML_TZONE[7];

/**
 * Handle char data in the parsing of a GPX file.
 */
static void
gpx_chars(SaxData *data, const xmlChar *ch, int len)
{
    gint i;

    switch(data->state)
    {
        case ERROR:
        case UNKNOWN:
            break;
        case INSIDE_WPT_NAME:
        case INSIDE_WPT_DESC:
        case INSIDE_PATH_POINT_ELE:
        case INSIDE_PATH_POINT_TIME:
        case INSIDE_PATH_POINT_DESC:
        case INSIDE_PATH_POINT_CMT:
            for(i = 0; i < len; i++)
                data->chars = g_string_append_c(data->chars, ch[i]);
            DEBUG("%s", data->chars->str);
            break;
        default:
            break;
    }
}

/**
 * Handle an entity in the parsing of a GPX file.  We don't do anything
 * special here.
 */
static xmlEntityPtr
gpx_get_entity(SaxData *data, const xmlChar *name)
{
    return xmlGetPredefinedEntity(name);
}

/**
 * Handle an error in the parsing of a GPX file.
 */
static void
gpx_error(SaxData *data, const gchar *msg, ...)
{
    DEBUG("");
    data->state = ERROR;
}

static gboolean
gpx_write_string(GOutputStream *stream, const gchar *str)
{
    GError *error = NULL;

    g_output_stream_write(stream, str, strlen(str), NULL, &error);
    if (G_UNLIKELY(error != NULL))
    {
        g_warning("Error while writing to file: %s",
                  error->message);
        g_error_free(error);
        /* TODO: return error to client */
        return FALSE;
    }
    return TRUE;
}

static gboolean
gpx_write_escaped(GOutputStream *stream, const gchar *str)
{
    const gchar *ptr = str;
    const gchar *nullchr = ptr + strlen(ptr);
    while(ptr < nullchr)
    {
        gchar *newptr = strpbrk(ptr, "&<>");
        if(newptr != NULL)
        {
            /* First, write out what we have so far. */
            const gchar *to_write;
            GError *error = NULL;

            g_output_stream_write(stream, ptr, newptr - ptr, NULL, &error);
            if (G_UNLIKELY(error != NULL))
            {
                g_warning("Error while writing to file: %s",
                          error->message);
                g_error_free(error);
                /* TODO: return error to client */
                return FALSE;
            }

            /* Now, write the XML entity. */
            switch(*newptr)
            {
                case '&':
                    to_write = "&amp;";
                    break;
                case '<':
                    to_write = "&lt;";
                    break;
                case '>':
                    to_write = "&gt;";
                    break;
                default:
                    to_write = "";
            }
            gpx_write_string(stream, to_write);

            /* Advance the pointer to continue searching for entities. */
            ptr = newptr + 1;
        }
        else
        {
            /* No characters need escaping - write the whole thing. */
            gpx_write_string(stream, ptr);
            ptr = nullchr;
        }
    }
    return TRUE;
}

/****************************************************************************
 * BELOW: OPEN PATH *********************************************************
 ****************************************************************************/

static void
gpx_path_start_element(PathSaxData *data,
        const xmlChar *name, const xmlChar **attrs)
{
    switch(data->sax_data.state)
    {
        case ERROR:
            printf("ERROR!\n");
            break;
        case START:
            if(!strcmp((gchar*)name, "gpx"))
                data->sax_data.state = INSIDE_GPX;
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case INSIDE_GPX:
            if(!strcmp((gchar*)name, "trk"))
                data->sax_data.state = INSIDE_PATH;
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case INSIDE_PATH:
            if(!strcmp((gchar*)name, "trkseg"))
            {
                data->sax_data.state = INSIDE_PATH_SEGMENT;
                data->sax_data.at_least_one_trkpt = FALSE;
            }
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case INSIDE_PATH_SEGMENT:
            if(!strcmp((gchar*)name, "trkpt"))
            {
                const xmlChar **curr_attr;
                gchar *error_check;
                MapGeo lat = 0.0, lon = 0.0;
                gboolean has_lat, has_lon;
                has_lat = FALSE;
                has_lon = FALSE;
                for(curr_attr = attrs; *curr_attr != NULL; )
                {
                    const gchar *attr_name = (const gchar *)*curr_attr++;
                    const gchar *attr_val = (const gchar *)*curr_attr++;
                    if(!strcmp(attr_name, "lat"))
                    {
                        lat = g_ascii_strtod(attr_val, &error_check);
                        if(error_check != attr_val)
                            has_lat = TRUE;
                    }
                    else if(!strcmp(attr_name, "lon"))
                    {
                        lon = g_ascii_strtod(attr_val, &error_check);
                        if(error_check != attr_val)
                            has_lon = TRUE;
                    }
                }
                if(has_lat && has_lon)
                {
                    map_latlon2unit(lat, lon,
                                    data->pt.unit.x, data->pt.unit.y);
                    data->pt.time = 0;
                    data->pt.altitude = 0;
                    data->sax_data.state = INSIDE_PATH_POINT;
                }
                else
                    data->sax_data.state = ERROR;
                data->dir = MAP_DIRECTION_UNKNOWN;
            }
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case INSIDE_PATH_POINT:
            if(!strcmp((gchar*)name, "time"))
                data->sax_data.state = INSIDE_PATH_POINT_TIME;
            else if(!strcmp((gchar*)name, "ele"))
                data->sax_data.state = INSIDE_PATH_POINT_ELE;
            else if(!strcmp((gchar*)name, "desc"))
                data->sax_data.state = INSIDE_PATH_POINT_DESC;
            else if(!strcmp((gchar*)name, "cmt"))
                data->sax_data.state = INSIDE_PATH_POINT_CMT;

            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case UNKNOWN:
            data->sax_data.unknown_depth++;
            break;
        default:
            ;
    }
}

/**
 * Handle an end tag in the parsing of a GPX file.
 */
static void
gpx_path_end_element(PathSaxData *data, const xmlChar *name)
{
    switch(data->sax_data.state)
    {
        case ERROR:
            printf("ERROR!\n");
            break;
        case START:
            data->sax_data.state = ERROR;
            break;
        case INSIDE_GPX:
            if(!strcmp((gchar*)name, "gpx"))
                data->sax_data.state = FINISH;
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH:
            if(!strcmp((gchar*)name, "trk"))
                data->sax_data.state = INSIDE_GPX;
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH_SEGMENT:
            if(!strcmp((gchar*)name, "trkseg"))
            {
                if(data->sax_data.at_least_one_trkpt)
                {
                    map_path_append_break(data->path);
                }
                data->sax_data.state = INSIDE_PATH;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH_POINT:
            if(!strcmp((gchar*)name, "trkpt"))
            {
                MapPathPoint *p;
                p = map_path_append_point_fast(data->path, &data->pt);
                if (data->desc)
                {
                    map_path_make_waypoint_full(data->path, p,
                                                data->dir, data->desc);
                    data->desc = NULL;
                }
                data->sax_data.state = INSIDE_PATH_SEGMENT;
                data->sax_data.at_least_one_trkpt = TRUE;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH_POINT_ELE:
            if(!strcmp((gchar*)name, "ele"))
            {
                gchar *error_check;
                data->pt.altitude
                    = g_ascii_strtod(data->sax_data.chars->str, &error_check);
                if(error_check == data->sax_data.chars->str)
                    data->pt.altitude = 0;
                data->sax_data.state = INSIDE_PATH_POINT;
                g_string_free(data->sax_data.chars, TRUE);
                data->sax_data.chars = g_string_new("");
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH_POINT_TIME:
            if(!strcmp((gchar*)name, "time"))
            {
                struct tm time;
                gchar *ptr;

                if(NULL == (ptr = strptime(data->sax_data.chars->str,
                            XML_DATE_FORMAT, &time)))
                    /* Failed to parse dateTime format. */
                    data->sax_data.state = ERROR;
                else
                {
                    /* Parse was successful. Now we have to parse timezone.
                     * From here on, if there is an error, I just assume local
                     * timezone.  Yes, this is not proper XML, but I don't
                     * care. */
                    gchar *error_check;

                    /* First, set time in "local" time zone. */
                    data->pt.time = (mktime(&time));

                    /* Now, skip inconsequential characters */
                    while(*ptr && *ptr != 'Z' && *ptr != '-' && *ptr != '+')
                        ptr++;

                    /* Check if we ran to the end of the string. */
                    if(*ptr)
                    {
                        /* Next character is either 'Z', '-', or '+' */
                        if(*ptr == 'Z')
                            /* Zulu (UTC) time. Undo the local time zone's
                             * offset. */
                            data->pt.time += time.tm_gmtoff;
                        else
                        {
                            /* Not Zulu (UTC). Must parse hours and minutes. */
                            gint offhours = strtol(ptr, &error_check, 10);
                            if(error_check != ptr
                                    && *(ptr = error_check) == ':')
                            {
                                /* Parse of hours worked. Check minutes. */
                                gint offmins = strtol(ptr + 1,
                                        &error_check, 10);
                                if(error_check != (ptr + 1))
                                {
                                    /* Parse of minutes worked. Calculate. */
                                    data->pt.time
                                        += (time.tm_gmtoff
                                                - (offhours * 60 * 60
                                                    + offmins * 60));
                                }
                            }
                        }
                    }
                    /* Successfully parsed dateTime. */
                    data->sax_data.state = INSIDE_PATH_POINT;
                }

                g_string_free(data->sax_data.chars, TRUE);
                data->sax_data.chars = g_string_new("");
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH_POINT_CMT:
            /* only parse description for routes */
            if(!strcmp((gchar*)name, "cmt"))
            {
                if (data->desc == NULL)
                    data->desc = g_string_free(data->sax_data.chars, FALSE);
                else
                    g_string_free(data->sax_data.chars, TRUE);

                data->sax_data.chars = g_string_new("");
                data->sax_data.state = INSIDE_PATH_POINT;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_PATH_POINT_DESC:
            /* only parse description for routes */
            if(!strcmp((gchar*)name, "desc"))
            {
                /* If we already have a desc (e.g. from cmt), then overwrite */
                g_free(data->desc);
                data->desc = g_string_free(data->sax_data.chars, FALSE);
                data->sax_data.chars = g_string_new("");
                data->sax_data.state = INSIDE_PATH_POINT;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case UNKNOWN:
            if(!--data->sax_data.unknown_depth)
                data->sax_data.state = data->sax_data.prev_state;
            break;
        default:
            ;
    }
}

static void
parse_xml_stream(GInputStream *stream, xmlSAXHandler *sax_handler, void *data)
{
    xmlParserCtxtPtr ctx;
    gchar buffer[1024];
    gssize len;

    len = g_input_stream_read(stream, buffer, 4, NULL, NULL);
    if (len <= 0) return;

    ctx = xmlCreatePushParserCtxt(sax_handler, data,
                                  buffer, len, NULL);
    while ((len = g_input_stream_read(stream, buffer, sizeof(buffer),
                                      NULL, NULL)) > 0)
    {
        DEBUG("Read %d bytes", len);
        xmlParseChunk(ctx, buffer, len, 0);
    }
    xmlParseChunk(ctx, buffer, 0, 1);

    if (ctx->myDoc)
        xmlFreeDoc(ctx->myDoc);
    xmlFreeParserCtxt(ctx);
}

gboolean
map_gpx_path_parse(GInputStream *stream, MapPath *path)
{
    PathSaxData data;
    xmlSAXHandler sax_handler;

    data.path = path;
    data.desc = NULL;
    data.sax_data.state = START;
    data.sax_data.chars = g_string_new("");

    memset(&sax_handler, 0, sizeof(sax_handler));
    sax_handler.characters = (charactersSAXFunc)gpx_chars;
    sax_handler.startElement = (startElementSAXFunc)gpx_path_start_element;
    sax_handler.endElement = (endElementSAXFunc)gpx_path_end_element;
    sax_handler.entityDecl = (entityDeclSAXFunc)gpx_get_entity;
    sax_handler.warning = (warningSAXFunc)gpx_error;
    sax_handler.error = (errorSAXFunc)gpx_error;
    sax_handler.fatalError = (fatalErrorSAXFunc)gpx_error;

    parse_xml_stream(stream, &sax_handler, &data);
    g_string_free(data.sax_data.chars, TRUE);

    if(data.sax_data.state != FINISH)
    {
        return FALSE;
    }

    map_path_append_point_end(data.path);

    return TRUE;
}

/****************************************************************************
 * ABOVE: OPEN PATH *********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: SAVE PATH *********************************************************
 ****************************************************************************/

gboolean
map_gpx_path_write(MapPath *path, GOutputStream *handle)
{
    MapPathPoint *curr = NULL;
    MapPathWayPoint *wcurr = NULL;
    MapLineIter line;

    /* Write the header. */
    gpx_write_string(handle,
            "<?xml version=\"1.0\"?>\n"
            "<gpx version=\"1.0\" creator=\"maemo-mapper\" "
            "xmlns=\"http://www.topografix.com/GPX/1/0\">\n"
            "  <trk>\n");

    map_path_line_iter_first(path, &line);
    wcurr = path->whead;
    do
    {
        MapPathPoint *start, *end;
        gpx_write_string(handle, "    <trkseg>\n");
        start = map_path_line_first(&line);
        end = start + map_path_line_len(&line);
        for (curr = start; curr < end; curr++)
        {
            MapGeo lat, lon;
            gchar buffer[80];
            gboolean first_sub = TRUE;
            map_unit2latlon(curr->unit.x, curr->unit.y, lat, lon);
            gpx_write_string(handle, "      <trkpt lat=\"");
            g_ascii_formatd(buffer, sizeof(buffer), "%.06f", lat);
            gpx_write_string(handle, buffer);
            gpx_write_string(handle, "\" lon=\"");
            g_ascii_formatd(buffer, sizeof(buffer), "%.06f", lon);
            gpx_write_string(handle, buffer);
            gpx_write_string(handle, "\"");

            /* write the elevation */
            if(curr->altitude != 0)
            {
                if(first_sub)
                {
                    gpx_write_string(handle, ">\n");
                    first_sub = FALSE;
                }
                gpx_write_string(handle, "        <ele>");
                {
                    g_ascii_formatd(buffer, 80, "%.2f", curr->altitude);
                    gpx_write_string(handle, buffer);
                }
                gpx_write_string(handle, "</ele>\n");
            }

            /* write the time */
            if(curr->time)
            {
                if(first_sub)
                {
                    gpx_write_string(handle, ">\n");
                    first_sub = FALSE;
                }
                gpx_write_string(handle, "        <time>");
                strftime(buffer, 80, XML_DATE_FORMAT, localtime(&curr->time));
                gpx_write_string(handle, buffer);
                gpx_write_string(handle, XML_TZONE);
                gpx_write_string(handle, "</time>\n");
            }

            if (wcurr <= path->wtail && curr == wcurr->point)
            {
                if(first_sub)
                {
                    gpx_write_string(handle, ">\n");
                    first_sub = FALSE;
                }
                gpx_write_string(handle, "        <desc>");
                gpx_write_escaped(handle, wcurr->desc);
                gpx_write_string(handle, "</desc>\n");
                wcurr++;
            }
            if(first_sub)
            {
                gpx_write_string(handle, "/>\n");
            }
            else
            {
                gpx_write_string(handle, "      </trkpt>\n");
            }
        }

        gpx_write_string(handle, "    </trkseg>\n");
    }
    while (map_path_line_iter_next(&line));

    /* Write the footer. */
    gpx_write_string(handle,
            "  </trk>\n"
            "</gpx>\n");


    return TRUE;
}

/****************************************************************************
 * ABOVE: SAVE PATH *********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: OPEN POI **********************************************************
 ****************************************************************************/

static void
gpx_poi_start_element(PoiSaxData *data,
        const xmlChar *name, const xmlChar **attrs)
{
    DEBUG("%s", name);

    switch(data->sax_data.state)
    {
        case ERROR:
            printf("ERROR!\n");
            break;
        case START:
            if(!strcmp((gchar*)name, "gpx"))
                data->sax_data.state = INSIDE_GPX;
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case INSIDE_GPX:
            if(!strcmp((gchar*)name, "wpt"))
            {
                const xmlChar **curr_attr;
                gchar *error_check;
                MapGeo lat = 0.0, lon = 0.0;
                gboolean has_lat, has_lon;
                has_lat = FALSE;
                has_lon = FALSE;

                /* Parse the attributes - there should be lat and lon. */
                for(curr_attr = attrs; *curr_attr != NULL; )
                {
                    const gchar *attr_name = (const gchar *)*curr_attr++;
                    const gchar *attr_val = (const gchar *)*curr_attr++;
                    if(!strcmp(attr_name, "lat"))
                    {
                        lat = g_ascii_strtod(attr_val, &error_check);
                        if(error_check != attr_val)
                            has_lat = TRUE;
                    }
                    else if(!strcmp(attr_name, "lon"))
                    {
                        lon = g_ascii_strtod(attr_val, &error_check);
                        if(error_check != attr_val)
                            has_lon = TRUE;
                    }
                }
                if(has_lat && has_lon)
                {
                    data->sax_data.state = INSIDE_WPT;
                    data->curr_poi = g_slice_new0(MapPoiInfo);
                    data->curr_poi->lat = lat;
                    data->curr_poi->lon = lon;
                    data->poi_list = g_list_append(
                            data->poi_list, data->curr_poi);
                }
                else
                    data->sax_data.state = ERROR;
            }
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case INSIDE_WPT:
            if(!strcmp((gchar*)name, "name"))
                data->sax_data.state = INSIDE_WPT_NAME;
            else if(!strcmp((gchar*)name, "desc"))
                data->sax_data.state = INSIDE_WPT_DESC;
            else if(!strcmp((gchar*)name, "cmt"))
                data->sax_data.state = INSIDE_WPT_CMT;
            else
            {
                MACRO_SET_UNKNOWN();
            }
            break;
        case UNKNOWN:
            data->sax_data.unknown_depth++;
            break;
        default:
            ;
    }
}

/**
 * Handle an end tag in the parsing of a GPX file.
 */
static void
gpx_poi_end_element(PoiSaxData *data, const xmlChar *name)
{
    DEBUG("%s", name);

    switch(data->sax_data.state)
    {
        case ERROR:
            printf("ERROR!\n");
            break;
        case START:
            data->sax_data.state = ERROR;
            break;
        case INSIDE_GPX:
            if(!strcmp((gchar*)name, "gpx"))
            {
                data->sax_data.state = FINISH;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_WPT:
            if(!strcmp((gchar*)name, "wpt"))
                data->sax_data.state = INSIDE_GPX;
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_WPT_NAME:
            if(!strcmp((gchar*)name, "name"))
            {
                data->curr_poi->label
                    = g_string_free(data->sax_data.chars, FALSE);
                data->sax_data.chars = g_string_new("");
                data->sax_data.state = INSIDE_WPT;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_WPT_CMT:
            if(!strcmp((gchar*)name, "cmt"))
            {
                /* Only use if we don't already have a desc */
                if(!data->curr_poi->desc)
                {
                    data->curr_poi->desc
                        = g_string_free(data->sax_data.chars, FALSE);
                    data->sax_data.chars = g_string_new("");
                    data->sax_data.state = INSIDE_WPT;
                }
            }
            else
                data->sax_data.state = ERROR;
            break;
        case INSIDE_WPT_DESC:
            if(!strcmp((gchar*)name, "desc"))
            {
                /* If we already have a desc (e.g. from cmt), then overwrite */
                if(data->curr_poi->desc)
                    g_free(data->curr_poi->desc);
                data->curr_poi->desc
                    = g_string_free(data->sax_data.chars, FALSE);
                data->sax_data.chars = g_string_new("");
                data->sax_data.state = INSIDE_WPT;
            }
            else
                data->sax_data.state = ERROR;
            break;
        case UNKNOWN:
            if(!--data->sax_data.unknown_depth)
                data->sax_data.state = data->sax_data.prev_state;
            break;
        default:
            ;
    }
}

gboolean
map_gpx_poi_parse(GInputStream *stream, GList **poi_list)
{
    PoiSaxData data;
    xmlSAXHandler sax_handler;

    data.poi_list = *poi_list;
    data.sax_data.state = START;
    data.sax_data.chars = g_string_new("");

    memset(&sax_handler, 0, sizeof(sax_handler));
    sax_handler.characters = (charactersSAXFunc)gpx_chars;
    sax_handler.startElement = (startElementSAXFunc)gpx_poi_start_element;
    sax_handler.endElement = (endElementSAXFunc)gpx_poi_end_element;
    sax_handler.entityDecl = (entityDeclSAXFunc)gpx_get_entity;
    sax_handler.warning = (warningSAXFunc)gpx_error;
    sax_handler.error = (errorSAXFunc)gpx_error;
    sax_handler.fatalError = (fatalErrorSAXFunc)gpx_error;

    parse_xml_stream(stream, &sax_handler, &data);
    g_string_free(data.sax_data.chars, TRUE);
    *poi_list = data.poi_list;

    if(data.sax_data.state != FINISH)
    {
        return FALSE;
    }

    return TRUE;
}

/****************************************************************************
 * ABOVE: OPEN POI **********************************************************
 ****************************************************************************/

/****************************************************************************
 * BELOW: SAVE POI **********************************************************
 ****************************************************************************/

gint
map_gpx_poi_write(GtkTreeModel *model, GOutputStream *handle)
{
    gint num_written = 0;
    GtkTreeIter iter;

    /* Write the header. */
    gpx_write_string(handle,
            "<?xml version=\"1.0\"?>\n"
            "<gpx version=\"1.0\" creator=\"maemo-mapper\" "
            "xmlns=\"http://www.topografix.com/GPX/1/0\">\n");

    /* Iterate through the data model and import as desired. */
    if(gtk_tree_model_get_iter_first(model, &iter)) do
    {   
        MapPoiInfo poi;
        gboolean selected;
        memset(&poi, 0, sizeof(poi));

        gtk_tree_model_get(model, &iter,
                MAP_POI_SELECTED, &selected,
                MAP_POI_POIID, &(poi.poi_id),
                MAP_POI_CATID, &(poi.cat_id),
                MAP_POI_LAT, &(poi.lat),
                MAP_POI_LON, &(poi.lon),
                MAP_POI_LABEL, &(poi.label),
                MAP_POI_DESC, &(poi.desc),
                MAP_POI_CLABEL, &(poi.clabel),
                -1);

        if(selected)
        {
            gchar buffer[80];

            gpx_write_string(handle, "  <wpt lat=\"");
            g_ascii_formatd(buffer, sizeof(buffer), "%.06f", poi.lat);
            gpx_write_string(handle, buffer);
            gpx_write_string(handle, "\" lon=\"");
            g_ascii_formatd(buffer, sizeof(buffer), "%.06f", poi.lon);
            gpx_write_string(handle, buffer);
            gpx_write_string(handle, "\">\n");

            if(poi.label && *poi.label)
            {
                gpx_write_string(handle, "    <name>");
                gpx_write_escaped(handle, poi.label);
                gpx_write_string(handle, "</name>\n");
            }

            if(poi.desc && *poi.desc)
            {
                gpx_write_string(handle, "    <desc>");
                gpx_write_escaped(handle, poi.desc);
                gpx_write_string(handle, "</desc>\n");
            }
            gpx_write_string(handle, "  </wpt>\n");
            ++ num_written;
        }
    } while(gtk_tree_model_iter_next(model, &iter));

    /* Write the footer. */
    gpx_write_string(handle, "</gpx>\n");

    return num_written;
}

/****************************************************************************
 * ABOVE: SAVE POI **********************************************************
 ****************************************************************************/

void
gpx_init()
{
    /* set XML_TZONE */
    {   
        time_t time1;
        struct tm time2;
        time1 = time(NULL);
        localtime_r(&time1, &time2);
        snprintf(XML_TZONE, sizeof(XML_TZONE), "%+03ld:%02ld",
                (time2.tm_gmtoff / 60 / 60), (time2.tm_gmtoff / 60) % 60);
    }
}
