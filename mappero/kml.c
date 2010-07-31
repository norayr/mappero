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
 * along with libMappero.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "gpx.h"

#include "debug.h"

#include <string.h>
#include <libxml/parser.h>

#define BUFFER_SIZE 2048
#define MAX_LEVELS 32

/** This enum defines the states of the SAX parsing state machine. */
typedef enum
{
    /* containers */
    EL_KML,
    EL_DOCUMENT,
    EL_PLACEMARK,
    EL_POINT,
    EL_LINESTRING,
    /* text data elements */
    EL_NAME,
    EL_DESCRIPTION,
    EL_COORDINATES,
    EL_ALTITUDE_MODE,
    /* end of supported elements */
    EL_UNKNOWN,
    EL_NONE,
    EL_ERROR,
} KmlElement;

typedef enum {
    KML_COORDS_NULL = 0,
    KML_COORDS_2D,
    KML_COORDS_3D,
} KmlCoords;

typedef struct {
    MapPathPoint point;
    gchar *name;
    gchar *description;
} KmlPlacemark;

typedef struct {
    gboolean error;
    KmlElement elements[MAX_LEVELS];
    gint level;
    MapPath *path;
    GList *placemarks;
    /* current elements */
    MapPathPoint point;
    gchar *name;
    gchar *description;
    GString *chars;
} SaxData;

#define CURRENT_ELEMENT(data) (data->elements[data->level])
#define EL_HAS_TEXT(el) (el >= EL_NAME && el <= EL_ALTITUDE_MODE)
#define strsame(s1, s2)  (strcmp((const gchar *)s1, s2) == 0)

const static gchar *kml_elements[] = {
    [EL_KML] = "kml",
    [EL_DOCUMENT] = "Document",
    [EL_PLACEMARK] = "Placemark",
    [EL_POINT] = "Point",
    [EL_LINESTRING] = "LineString",
    [EL_NAME] = "name",
    [EL_DESCRIPTION] = "description",
    [EL_COORDINATES] = "coordinates",
    [EL_ALTITUDE_MODE] = "altitudeMode",
    NULL
};

static void
link_waypoints(SaxData *data)
{
    MapPathPoint *p;
    GList *list;

    if (data->path == NULL) return;

    /* Placemarks can be waypoints, if they also appear as points in the
     * route; here we do the link */
    for (list = data->placemarks; list != NULL; list = list->next)
    {
        KmlPlacemark *placemark = list->data;
        for (p = map_path_first(data->path);
             p < map_path_end(data->path);
             p = map_path_next(data->path, p))
        {
            if (placemark->point.unit.x == p->unit.x &&
                placemark->point.unit.y == p->unit.y)
            {
                map_path_make_waypoint(data->path, p, placemark->name);
                placemark->name = NULL;
                break;
            }
        }
    }
}

static KmlPlacemark *
kml_placemark_new(const MapPathPoint *point, gchar *name, gchar *description)
{
    KmlPlacemark *placemark;

    placemark = g_slice_new0(KmlPlacemark);
    placemark->point = *point;
    placemark->name = name;
    placemark->description = description;

    return placemark;
}

static void
kml_placemark_free(KmlPlacemark *placemark)
{
    g_free(placemark->name);
    g_free(placemark->description);
    g_slice_free(KmlPlacemark, placemark);
}

static void
add_placemark(SaxData *data)
{
    KmlPlacemark *placemark = kml_placemark_new(&data->point,
                                                data->name,
                                                data->description);
    if (placemark)
    {
        data->name = NULL;
        data->description = NULL;
        data->placemarks = g_list_prepend(data->placemarks, placemark);
    }
}

static KmlCoords
parse_coordinates(gchar **string, MapGeo *lat, MapGeo *lon, MapGeo *alt)
{
    gdouble d;
    gchar *token, *end = NULL;
    KmlCoords coords_type = KML_COORDS_2D;

    token = *string;

    d = g_ascii_strtod(token, &end);
    if (G_UNLIKELY(end <= token)) return KML_COORDS_NULL;
    *lon = d;

    if (G_LIKELY(end[0] == ','))
    {
        token = end + 1;
        d = g_ascii_strtod(token, &end);
        if (G_UNLIKELY(end <= token)) return KML_COORDS_NULL;
        *lat = d;

        if (end[0] == ',')
        {
            token = end + 1;
            d = g_ascii_strtod(token, &end);
            if (G_UNLIKELY(end <= token)) return KML_COORDS_NULL;
            *alt = d;
            coords_type = KML_COORDS_3D;
        }
    }
    else
    {
        g_warning("%s: invalid coordinates data: %s", G_STRFUNC, *string);
        return KML_COORDS_NULL;
    }

    *string = end;
    return coords_type;
}

static void
add_coordinates_to_path(SaxData *data)
{
    gchar *entry;
    MapGeo lat, lon, alt;
    MapPathPoint p;
    KmlCoords coords_type;

    if (data->path == NULL) return;

    memset(&p, 0, sizeof(p));
    entry = data->chars->str;
    for (coords_type = parse_coordinates(&entry, &lat, &lon, &alt);
         coords_type != KML_COORDS_NULL;
         coords_type = parse_coordinates(&entry, &lat, &lon, &alt))
    {
        map_latlon2unit(lat, lon, p.unit.x, p.unit.y);
        /* skip duplicate points */
        if (map_path_len(data->path) > 0)
        {
            const MapPathPoint *last = map_path_last(data->path);
            if (last->unit.x == p.unit.x &&
                last->unit.y == p.unit.y)
                continue;
        }
        map_path_append_point_fast(data->path, &p);
    }
    map_path_append_point_end(data->path);
}

static void
add_coordinates_to_point(SaxData *data)
{
    gchar *entry;
    MapGeo lat, lon, alt;
    KmlCoords coords_type;

    entry = data->chars->str;
    coords_type = parse_coordinates(&entry, &lat, &lon, &alt);
    map_latlon2unit(lat, lon, data->point.unit.x, data->point.unit.y);
}

static gboolean
has_ancestor(SaxData *data, KmlElement ancestor)
{
    gint l;

    for (l = data->level - 1; l >= 0; l--)
        if (data->elements[l] == ancestor)
            return TRUE;
    return FALSE;
}

/**
 * Handle char data in the parsing of a GPX file.
 */
static void
handle_chars(SaxData *data, const xmlChar *ch, int len)
{
    switch (CURRENT_ELEMENT(data))
    {
        case EL_NAME:
        case EL_DESCRIPTION:
        case EL_COORDINATES:
        case EL_ALTITUDE_MODE:
            data->chars = g_string_append_len(data->chars,
                                              (const gchar *)ch, len);
            DEBUG("%s", data->chars->str);
            break;
        default:
            break;
    }
}

static KmlElement
element_from_name(const xmlChar *name)
{
    gint i;

    for (i = 0; kml_elements[i] != NULL; i++)
    {
        if (strsame(name, kml_elements[i]))
            return i;
    }
    return EL_UNKNOWN;
}

static void
handle_start_element(SaxData *data, const xmlChar *name, const xmlChar **attrs)
{
    KmlElement el, parent;

    DEBUG("%s", name);
    if (G_UNLIKELY(data->error)) return;

    parent = CURRENT_ELEMENT(data);
    el = element_from_name(name);
    if (el == EL_ERROR)
    {
        data->error = TRUE;
        return;
    }

    data->level++;
    if (G_UNLIKELY(data->level >= MAX_LEVELS))
    {
        g_warning("Max number of levels reached");
        data->error = TRUE;
        return;
    }
    data->elements[data->level] = el;

    if (EL_HAS_TEXT(el))
        g_string_truncate(data->chars, 0);
}

/**
 * Handle an end tag in the parsing of a GPX file.
 */
static void
handle_end_element(SaxData *data, const xmlChar *name)
{
    KmlElement el;

    DEBUG("%s", name);
    if (G_UNLIKELY(data->error)) return;

    if (G_UNLIKELY(data->level == 0))
    {
        g_warning("Error in XML document");
        data->error = TRUE;
        return;
    }

    el = CURRENT_ELEMENT(data);

    if (el == EL_COORDINATES)
    {
        if (has_ancestor(data, EL_LINESTRING))
            add_coordinates_to_path(data);
        else if (has_ancestor(data, EL_PLACEMARK))
            add_coordinates_to_point(data);
    }
    else if (el == EL_NAME)
    {
        if (has_ancestor(data, EL_PLACEMARK))
        {
            g_free(data->name);
            data->name = g_string_free(data->chars, FALSE);
            data->chars = g_string_new("");
        }
    }
    else if (el == EL_PLACEMARK)
    {
        add_placemark(data);
    }
    else if (el == EL_KML)
    {
        data->placemarks = g_list_reverse(data->placemarks);
        link_waypoints(data);
    }

    data->level--;
}

static xmlEntityPtr
handle_get_entity(SaxData *data, const xmlChar *name)
{
    return xmlGetPredefinedEntity(name);
}

static void
handle_error(SaxData *data, const gchar *msg, ...)
{
    DEBUG("%s", msg);
    data->error = TRUE;
}

static gboolean
parse_xml_stream(GInputStream *stream, xmlSAXHandler *sax_handler, void *data)
{
    xmlParserCtxtPtr ctx;
    gchar buffer[1024];
    gssize len;
    gboolean ok = TRUE;
    int ret;

    len = g_input_stream_read(stream, buffer, 4, NULL, NULL);
    if (len <= 0) return FALSE;

    ctx = xmlCreatePushParserCtxt(sax_handler, data,
                                  buffer, len, NULL);
    while ((len = g_input_stream_read(stream, buffer, sizeof(buffer),
                                      NULL, NULL)) > 0)
    {
        ret = xmlParseChunk(ctx, buffer, len, 0);
        if (G_UNLIKELY(ret != 0)) ok = FALSE;
    }
    ret = xmlParseChunk(ctx, buffer, 0, 1);
    if (G_UNLIKELY(ret != 0)) ok = FALSE;

    if (ctx->myDoc)
        xmlFreeDoc(ctx->myDoc);
    xmlFreeParserCtxt(ctx);
    return ok;
}

static void
sax_data_free(SaxData *data)
{
    while (data->placemarks)
    {
        KmlPlacemark *placemark = data->placemarks->data;
        kml_placemark_free(placemark);
        data->placemarks = g_list_delete_link(data->placemarks,
                                              data->placemarks);
    }

    g_free(data->name);
    g_free(data->description);
    g_string_free(data->chars, TRUE);
}

gboolean
map_kml_path_parse(GInputStream *stream, MapPath *path)
{
    SaxData data;
    xmlSAXHandler handler;
    gboolean ok = FALSE;

    memset(&data, 0, sizeof(data));
    data.path = path;
    data.chars = g_string_new("");

    memset(&handler, 0, sizeof(handler));
    handler.characters = (charactersSAXFunc)handle_chars;
    handler.startElement = (startElementSAXFunc)handle_start_element;
    handler.endElement = (endElementSAXFunc)handle_end_element;
    handler.entityDecl = (entityDeclSAXFunc)handle_get_entity;
    handler.warning = (warningSAXFunc)handle_error;
    handler.error = (errorSAXFunc)handle_error;
    handler.fatalError = (fatalErrorSAXFunc)handle_error;

    if (parse_xml_stream(stream, &handler, &data))
    {
        ok = !data.error;
        /* Do not accept empty paths */
        if (ok && map_path_len(path) <= 0)
            ok = FALSE;
    }
    sax_data_free(&data);

    return ok;
}

