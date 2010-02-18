/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
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
#define _XOPEN_SOURCE
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include "reittiopas.h"

#include "data.h"
#include "defines.h"
#include "dialog.h"
#include "error.h"
#include "path.h"

#include <libxml/xmlreader.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define REITTIOPAS_ROUTER_URL \
    "http://api.reittiopas.fi/public-ytv/fi/api/?user=maemomapper&pass=b4zmhurp"

static void router_iface_init(MapRouterIface *iface);
static void map_reittiopas_geocode(MapRouter *router, const gchar *address,
                                   MapRouterGeocodeCb callback,
                                   gpointer user_data);

G_DEFINE_TYPE_WITH_CODE(MapReittiopas, map_reittiopas, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MAP_TYPE_ROUTER,
                                              router_iface_init));

/* a point in Reittiopas coordinate system */
typedef struct {
    gfloat p;
    gfloat i;
} KKJ2;

typedef struct {
    MapPoint from;
    MapPoint to;
    gchar *dest_address;
    MapRouterCalculateRouteCb callback;
    gpointer user_data;
} CalculateRouteData;

typedef enum {
    EL_NONE,
    EL_MTRXML,
    EL_ROUTE,
    EL_WALK,
    EL_LINE,
    EL_POINT,
    EL_MAPLOC,
    EL_STOP,
    EL_ARRIVAL,
    EL_DEPARTURE,
    EL_NAME,
    EL_GEOCODE,
    EL_LOC,
    EL_ERROR,
    EL_UNKNOWN,
} RoElement;

#define EL_IS_POINT(e)  (e >= EL_POINT && e <= EL_STOP)
#define EL_IS_TIME(e)  (e == EL_ARRIVAL || e == EL_DEPARTURE)
#define EL_IS_SEGMENT(e)  (e == EL_LINE || e == EL_WALK)

#define MAX_LEVELS  16

typedef struct {
    MapPoint p;
    gint type;
    gint mobility;
    const gchar *code;
    const gchar *id;
    const gchar *lang;
    const gchar *val;
    time_t time;
    gfloat lat;
    gfloat lon;
} Attributes;

typedef struct {
    MapPoint p;
    time_t arrival;
    time_t departure;
    gchar *code;
    gchar *id;
    gchar *name;
    gboolean is_waypoint;
} RoPoint;

typedef struct {
    gint type;
    gint mobility;
    gchar *code;
    gchar *id;
} RoLine;

typedef struct {
    Path *path;
    gfloat lat;
    gfloat lon;
    gboolean error;
    RoElement elements[MAX_LEVELS];
    gint level;
    gboolean segment_has_points;
    RoPoint point;
    RoLine line;
} SaxData;

#define strsame(s1, s2)  (strcmp((const gchar *)s1, s2) == 0)

static gboolean
location2units(const MapLocation *loc, MapPoint *u)
{
    gboolean ret = FALSE;

    if (loc->address)
    {
        gdouble lat, lon;
        gchar *end;
        gchar *str = loc->address;

        lat = g_ascii_strtod(str, &end);
        if (end != str)
        {
            str = end;
            if (str[0] == ',')
            {
                str ++;
                lon = g_ascii_strtod(str, &end);
                if (end != str)
                    ret = TRUE;
            }
        }
        if (ret)
        {
            latlon2unit(lat, lon, u->x, u->y);
        }
    }
    else
    {
        *u = loc->point;
        ret = TRUE;
    }
    return ret;
}

static void
wgslatlon2kkjlatlon(gdouble wlat, gdouble wlon, gdouble *klat, gdouble *klon)
{
    gdouble la, lo;
    gdouble dla, dlo;

    la = rad2deg(wlat);
    lo = rad2deg(wlon);

    dla = -0.124766E+01 +
        0.269941E+00 * la +
        -0.191342E+00 * lo +
        -0.356086E-02 * la * la +
        0.122353E-02 * la * lo +
        0.335456E-03 * lo * lo;
    dla = deg2rad(dla) / 3600.0;
    
    dlo = 0.286008E+02 +
        -0.114139E+01 * la +
        0.581329E+00 * lo +
        0.152376E-01 * la * la +
        -0.118166E-01 * la * lo +
        -0.826201E-03 * lo * lo;
    dlo = deg2rad(dlo) / 3600.0;

    *klat = wlat + dla;
    *klon = wlon + dlo;
}

static void
kkjlatlon2kkjxy(gdouble KKJradla, gdouble KKJradlo,
                gint ZoneNumber, gdouble Long0, KKJ2 *k)
{
    gdouble Lo, a, b, f, bb, c, ee, n, nn, cosLa, cosLaF, LaF, t, A, A1, A2, A3, A4, X, Y, NN;

    Lo = KKJradlo - Long0;
    a  = 6378388.0;
    f  = 1/297.0;
    b  = (1.0 - f) * a;
    bb = b * b;
    c  = (a / b) * a;
    ee = (a * a - bb) / bb;
    n = (a - b)/(a + b);
    nn = n * n;
    cosLa = cos(KKJradla);
    NN = ee * cosLa * cosLa;
    LaF = atan(tan(KKJradla) / cos(Lo * sqrt(1 + NN)));
    cosLaF = cos(LaF);
    t   = (tan(Lo) * cosLaF) / sqrt(1 + ee * cosLaF * cosLaF);
    A   = a / ( 1 + n );
    A1  = A * (1 + nn / 4 + nn * nn / 64);
    A2  = A * 1.5 * n * (1 - nn / 8);
    A3  = A * 0.9375 * nn * (1 - nn / 4);
    A4  = A * 35/48.0 * nn * n;
    X = A1 * LaF - A2 * sin(2*LaF) + A3 * sin(4*LaF) - A4 * sin(6*LaF);
    Y = c * log(t + sqrt(1+t*t)) + 500000.0 + ZoneNumber * 1000000.0;

    k->p = X;
    k->i = Y;
}

static void
KKJlalo_to_WGSlalo(gdouble kla, gdouble klo, gdouble *lat, gdouble *lon)
{
    gdouble La, Lo, dLa, dLo;

    La = rad2deg(kla);
    Lo = rad2deg(klo);

    dLa = deg2rad(  0.124867E+01       +
                     -0.269982E+00 * La +
                     0.191330E+00 * Lo +
                     0.356119E-02 * La * La +
                     -0.122312E-02 * La * Lo +
                     -0.335514E-03 * Lo * Lo ) / 3600.0;
    dLo = deg2rad( -0.286111E+02       +
                    0.114183E+01 * La +
                    -0.581428E+00 * Lo +
                    -0.152421E-01 * La * La +
                    0.118177E-01 * La * Lo +
                    0.826646E-03 * Lo * Lo ) / 3600.0;

    *lat = kla + dLa;
    *lon = klo + dLo;
}

static void
KKJxy_to_KKJlalo(const KKJ2 *k, gint ZoneNumber, gdouble Long0,
                 gdouble *kla, gdouble *klo)
{
    gdouble klat, klon;
    gdouble MinLa, MaxLa, MinLo, MaxLo;
    gdouble DeltaLa, DeltaLo;
    KKJ2 KKJt;
    gint i;

    MinLa = deg2rad(60.0);
    MaxLa = deg2rad(61.0);
    MinLo = deg2rad(24.0);
    MaxLo = deg2rad(25.5);

    for (i = 1; i < 35; ++i) {

        DeltaLa = MaxLa - MinLa;
        DeltaLo = MaxLo - MinLo;

        klat = MinLa + 0.5 * DeltaLa;
        klon = MinLo + 0.5 * DeltaLo;

        kkjlatlon2kkjxy(klat, klon, ZoneNumber, Long0, &KKJt);

        if (fabs(KKJt.p - k->p) < 1 && fabs(KKJt.i - k->i) < 1)
            break;

        if (KKJt.p < k->p) {
            MinLa = MinLa + 0.45 * DeltaLa;
        } else {
            MaxLa = MinLa + 0.55 * DeltaLa;
        }

        if (KKJt.i < k->i) {
            MinLo = MinLo + 0.45 * DeltaLo;
        } else {
            MaxLo = MinLo + 0.55 * DeltaLo;
        }

    }

    *kla = klat;
    *klo = klon;
}

static void
kkj22latlon(const KKJ2 *k, gdouble *lat, gdouble *lon)
{
    gdouble rlat, rlon;
    gdouble klat, klon;
    gdouble long0;

    long0 = deg2rad(24);
    KKJxy_to_KKJlalo(k, 2, long0, &klat, &klon);
    KKJlalo_to_WGSlalo(klat, klon, &rlat, &rlon);
    *lat = rad2deg(rlat);
    *lon = rad2deg(rlon);
}

static void
kkj22unit(const KKJ2 *k, MapPoint *p)
{
    gdouble lat, lon;

    kkj22latlon(k, &lat, &lon);

    latlon2unit(lat, lon, p->x, p->y);
}

static void
latlon2kkj2(gdouble lat, gdouble lon, KKJ2 *k)
{
    gdouble rlat, rlon;
    gdouble klat, klon;
    gdouble long0;

    rlat = deg2rad(lat);
    rlon = deg2rad(lon);

    wgslatlon2kkjlatlon(rlat, rlon, &klat, &klon);
    long0 = deg2rad(24);
    kkjlatlon2kkjxy(klat, klon, 2, long0, k);
}

static void
unit2kkj2(const MapPoint *p, KKJ2 *k)
{
    gdouble lat, lon;

    unit2latlon(p->x, p->y, lat, lon);

    latlon2kkj2(lat, lon, k);
}

/* XML handling */

static void
free_point(SaxData *data)
{
    g_free(data->point.code);
    g_free(data->point.id);
    g_free(data->point.name);
    memset(&data->point, 0, sizeof(RoPoint));
}

static void
free_line(SaxData *data)
{
    g_free(data->line.code);
    g_free(data->line.id);
    memset(&data->line, 0, sizeof(RoLine));
}

static xmlEntityPtr
handle_get_entity(SaxData *data, const xmlChar *name)
{
    return xmlGetPredefinedEntity(name);
}

static void
handle_error(SaxData *data, const gchar *msg, ...)
{
    g_debug("%s: %s", G_STRFUNC, msg);
    data->error = TRUE;
}

static const gchar *
transport_type(gint type)
{
    switch (type) {
    case 1: return "Helsinki/bus";
    case 2: return "Helsinki/tram";
    case 3: return "Espoo internal";
    case 4: return "Vantaa internal";
    case 5: return "Regional traffic";
    case 6: return "Metro traffic";
    case 7: return "Ferry";
    case 8: return "U-lines";
    case 9: return "Other local traffic";
    case 10: return "Long-distance traffic";
    case 11: return "Express";
    case 12: return "VR local traffic";
    case 13: return "VR long-distance traffic";
    case 14: return "All";
    case 21: return "Helsinki service lines";
    case 22: return "Helsinki night traffic";
    case 23: return "Espoo service lines";
    case 24: return "Vantaa service lines";
    case 25: return "Regional night traffic";
    };

    return "Unknown";
}

static RoElement
ro_element_from_name(const xmlChar *name)
{
    if (strsame(name, "MTRXML"))
        return EL_MTRXML;
    if (strsame(name, "ROUTE"))
        return EL_ROUTE;
    if (strsame(name, "POINT"))
        return EL_POINT;
    if (strsame(name, "WALK"))
        return EL_WALK;
    if (strsame(name, "LINE"))
        return EL_LINE;
    if (strsame(name, "MAPLOC"))
        return EL_MAPLOC;
    if (strsame(name, "STOP"))
        return EL_STOP;
    if (strsame(name, "ARRIVAL"))
        return EL_ARRIVAL;
    if (strsame(name, "DEPARTURE"))
        return EL_DEPARTURE;
    if (strsame(name, "NAME"))
        return EL_NAME;
    if (strsame(name, "GEOCODE"))
        return EL_GEOCODE;
    if (strsame(name, "LOC"))
        return EL_LOC;
    if (strsame(name, "ERROR"))
        return EL_ERROR;
    return EL_UNKNOWN;
}

static void
parse_attributes(Attributes *a, RoElement el, const gchar **attrs)
{
    const gchar **attr;
    KKJ2 kkj;
    gboolean got_x = FALSE, got_y = FALSE;
    const gchar *date = NULL, *time = NULL;

    for (attr = attrs; *attr != NULL; attr += 2)
    {
        const gchar *name = attr[0];
        const gchar *value = attr[1];

        if (EL_IS_POINT(el))
        {
            if (strsame(name, "x"))
            {
                kkj.i = g_ascii_strtod(value, NULL);
                got_x = TRUE;
            }
            else if (strsame(name, "y"))
            {
                kkj.p = g_ascii_strtod(value, NULL);
                got_y = TRUE;
            }
            else if (strsame(name, "code"))
                a->code = value;
            else if (strsame(name, "id"))
                a->id = value;
        }
        else if (el == EL_DEPARTURE || el == EL_ARRIVAL)
        {
            if (strsame(name, "date"))
                date = value;
            else if (strsame(name, "time"))
                time = value;
        }
        else if (el == EL_NAME)
        {
            if (strsame(name, "lang"))
                a->lang = value;
            else if (strsame(name, "val"))
                a->val = value;
        }
        else if (el == EL_LINE)
        {
            if (strsame(name, "type"))
                a->type = atoi(value);
            else if (strsame(name, "mobility"))
                a->mobility = atoi(value);
            else if (strsame(name, "code"))
                a->code = value;
            else if (strsame(name, "id"))
                a->id = value;
        }
        else if (el == EL_LOC)
        {
            if (strsame(name, "lat"))
                a->lat = g_ascii_strtod(value, NULL);
            else if (strsame(name, "lon"))
                a->lon = g_ascii_strtod(value, NULL);
        }
    }

    if (got_x && got_y)
        kkj22unit(&kkj, &a->p);

    if (date && time)
    {
        struct tm tm;

        memset(&tm, 0, sizeof(tm));
        strptime(date, "%Y%m%d", &tm);
        strptime(time, "%H%M", &tm);
        a->time = mktime(&tm);
    }
}

static void
handle_start_element(SaxData *data, const xmlChar *name, const xmlChar **attrs)
{
    RoElement el, parent;
    Attributes a;

    if (G_UNLIKELY(data->error)) return;

    parent = data->elements[data->level];
    el = ro_element_from_name(name);
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

    if (EL_IS_SEGMENT(el))
        data->segment_has_points = FALSE;

    if (attrs)
    {
        memset(&a, 0, sizeof(a));
        parse_attributes(&a, el, (const gchar **)attrs);

        if (EL_IS_SEGMENT(parent))
        {
            if (EL_IS_POINT(el))
            {
                data->point.p = a.p;
                data->point.code = g_strdup(a.code);
                data->point.id = g_strdup(a.id);

                /* if this is the first point in a segment, it's a waypoint */
                if (!data->segment_has_points)
                {
                    data->segment_has_points = TRUE;
                    data->point.is_waypoint = TRUE;
                }
            }
        }
        else if (EL_IS_POINT(parent))
        {
            if (el == EL_DEPARTURE)
                data->point.departure = a.time;
            else if (el == EL_ARRIVAL)
                data->point.arrival = a.time;
            else if (el == EL_NAME)
            {
                /* pick the first, which is Finnish.
                 * TODO: check the locale and use Swedish accordingly */
                if (!data->point.name)
                    data->point.name = g_strdup(a.val);
            }
        }
        else if (el == EL_LINE)
        {
            data->line.code = g_strdup(a.code);
            data->line.id = g_strdup(a.id);
            data->line.type = a.type;
            data->line.mobility = a.mobility;
        }
        else if (el == EL_LOC && data->lat == 0)
        {
            data->lat = a.lat;
            data->lon = a.lon;
            g_debug("Got lat lon: %.6f, %.6f", data->lat, data->lon);
        }
    }
}

/**
 * Handle an end tag in the parsing of a GPX file.
 */
static void
handle_end_element(SaxData *data, const xmlChar *name)
{
    RoElement el;

    if (G_UNLIKELY(data->error)) return;

    if (G_UNLIKELY(data->level == 0))
    {
        g_warning("Error in XML document");
        data->error = TRUE;
        return;
    }

    el = data->elements[data->level];

    if (EL_IS_POINT(el))
    {
        if (data->point.p.x != 0)
        {
            MACRO_PATH_INCREMENT_TAIL(*data->path);
            data->path->tail->unit = data->point.p;
            data->path->tail->time = data->point.departure;
            data->path->tail->altitude = 0;

            if (data->point.is_waypoint)
            {
                MACRO_PATH_INCREMENT_WTAIL(*data->path);
                data->path->wtail->point = data->path->tail;
                if (data->line.code)
                {
                    data->path->wtail->desc =
                        g_strdup_printf("%s\n%s %.4s", data->point.name,
                                        transport_type(data->line.type),
                                        data->line.code + 1);
                }
                else
                    data->path->wtail->desc = g_strdup(data->point.name);
            }

            free_point(data);
        }
    }
    else if (el == EL_LINE)
    {
        free_line(data);
    }
    else if (el == EL_ROUTE)
    {
        /* Add a null point at the end of the route */
        MACRO_PATH_INCREMENT_TAIL(*data->path);
        *data->path->tail = _point_null;
    }

    data->level--;
}

static gboolean
parse_xml(SaxData *data, gchar *buffer, gsize len)
{
    xmlSAXHandler handler;
    gint ret;

    memset(&handler, 0, sizeof(handler));
    handler.startElement = (startElementSAXFunc)handle_start_element;
    handler.endElement = (endElementSAXFunc)handle_end_element;
    handler.entityDecl = (entityDeclSAXFunc)handle_get_entity;
    handler.warning = (warningSAXFunc)handle_error;
    handler.error = (errorSAXFunc)handle_error;
    handler.fatalError = (fatalErrorSAXFunc)handle_error;

    data->elements[0] = EL_NONE;
    ret = xmlSAXUserParseMemory(&handler, data, buffer, len);

    return ret == 0 && !data->error;
}

static void
download_route(Path *path, const KKJ2 *from, const KKJ2 *to, GError **error)
{
    gchar *query, *bytes;
    GString *string;
    gint size;
    GnomeVFSResult vfs_result;
    SaxData data;

    string = g_string_new_len(REITTIOPAS_ROUTER_URL,
                              sizeof(REITTIOPAS_ROUTER_URL) - 1);

    g_string_append_printf(string, "&a=%.0f,%.0f&b=%.0f,%.0f&show=1",
                           from->i, from->p, to->i, to->p);

    query = g_string_free(string, FALSE);
    g_debug("URL: %s", query);

    /* Attempt to download the route from the server. */
    vfs_result = gnome_vfs_read_entire_file(query, &size, &bytes);
    g_free(query);

    if (vfs_result != GNOME_VFS_OK)
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_NETWORK,
                    gnome_vfs_result_to_string(vfs_result));
        goto finish;
    }

    if (strncmp(bytes, "<?xml", 5))
    {
        /* Not an XML document - must be bad locations. */
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

    memset(&data, 0, sizeof(data));
    data.path = path;
    if (!parse_xml(&data, bytes, size))
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

finish:
    g_free(bytes);
}

static void
fetch_geocode(const gchar *address, gfloat *lat, gfloat *lon, GError **error)
{
    gchar *query, *address_latin1, *address_escaped, *bytes;
    gint size;
    GnomeVFSResult vfs_result;
    SaxData data;

    address_latin1 =
        g_convert(address, -1, "iso-8859-1", "utf-8", NULL, NULL, NULL);
    address_escaped = gnome_vfs_escape_string(address_latin1);
    g_free(address_latin1);
    query = g_strdup_printf("%s&key=%s", REITTIOPAS_ROUTER_URL,
                            address_escaped);
    g_free(address_escaped);

    g_debug("URL: %s", query);

    vfs_result = gnome_vfs_read_entire_file(query, &size, &bytes);
    g_free(query);

    if (vfs_result != GNOME_VFS_OK)
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_NETWORK,
                    gnome_vfs_result_to_string(vfs_result));
        goto finish;
    }

    if (strncmp(bytes, "<?xml", 5))
    {
        /* Not an XML document - must be bad locations. */
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

    memset(&data, 0, sizeof(data));
    if (!parse_xml(&data, bytes, size))
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        goto finish;
    }

    *lat = data.lat;
    *lon = data.lon;

finish:
    g_free(bytes);
}

static void
calculate_route_with_units(MapRouter *router,
                           const MapPoint *from_u, const MapPoint *to_u,
                           MapRouterCalculateRouteCb callback,
                           gpointer user_data)
{
    KKJ2 to, from;
    Path path;
    GError *error = NULL;

    unit2kkj2(from_u, &from);
    unit2kkj2(to_u, &to);

    MACRO_PATH_INIT(path);
    download_route(&path, &from, &to, &error);
    if (!error)
    {
        callback(router, &path, NULL, user_data);
    }
    else
    {
        callback(router, NULL, error, user_data);
        MACRO_PATH_FREE(path);
        g_error_free(error);
    }
}

static void
calculate_route_data_free(CalculateRouteData *crd)
{
    g_free(crd->dest_address);
    g_slice_free(CalculateRouteData, crd);
}

static void
route_address_retrieved(MapRouter *router, MapPoint point,
                        const GError *error, CalculateRouteData *crd)
{
    if (error)
    {
        crd->callback(router, NULL, error, crd->user_data);
        calculate_route_data_free(crd);
        return;
    }

    if (crd->from.x == 0)
    {
        crd->from = point;
        if (crd->dest_address)
        {
            map_reittiopas_geocode(router, crd->dest_address,
                                   (MapRouterGeocodeCb)route_address_retrieved,
                                   crd);
            return;
        }
    }
    else
        crd->to = point;

    /* at this point, we should have origin and destination in units: we can
     * calculate the route  */
    calculate_route_with_units(router, &crd->from, &crd->to,
                               crd->callback, crd->user_data);
    calculate_route_data_free(crd);
}

static const gchar *
map_reittiopas_get_name(MapRouter *router)
{
    return "Reittiopas";
}

static void
map_reittiopas_geocode(MapRouter *router, const gchar *address,
                       MapRouterGeocodeCb callback, gpointer user_data)
{
    GError *error = NULL;
    gfloat lat, lon;
    MapPoint p = { 0, 0 };

    fetch_geocode(address, &lat, &lon, &error);
    if (!error)
    {
        latlon2unit(lat, lon, p.x, p.y);
        g_debug("Got lat lon: %.6f, %.6f, units %u, %u",
                lat, lon, p.x, p.y);
        callback(router, p, NULL, user_data);
    }
    else
    {
        callback(router, p, error, user_data);
        g_error_free(error);
    }
}

static void
map_reittiopas_calculate_route(MapRouter *router, const MapRouterQuery *query,
                               MapRouterCalculateRouteCb callback,
                               gpointer user_data)
{
    MapPoint to_u, from_u;
    gboolean from_ok, to_ok;

    from_ok = location2units(&query->from, &from_u);
    to_ok = location2units(&query->to, &to_u);

    if (!from_ok || !to_ok)
    {
        CalculateRouteData *crd = g_slice_new0(CalculateRouteData);
        const gchar *address = NULL;

        if (from_ok)
            crd->from = from_u;
        else
            address = query->from.address;

        if (to_ok)
            crd->to = to_u;
        else
        {
            if (address == NULL)
                address = query->to.address;
            else
                crd->dest_address = g_strdup(query->to.address);
        }

        crd->callback = callback;
        crd->user_data = user_data;
        map_reittiopas_geocode(router, address,
                               (MapRouterGeocodeCb)route_address_retrieved,
                               crd);
        return;
    }

    calculate_route_with_units(router, &from_u, &to_u, callback, user_data);
}

static void
router_iface_init(MapRouterIface *iface)
{
    iface->get_name = map_reittiopas_get_name;
    iface->calculate_route = map_reittiopas_calculate_route;
    iface->geocode = map_reittiopas_geocode;
}

static void
map_reittiopas_init(MapReittiopas *reittiopas)
{
}

static void
map_reittiopas_class_init(MapReittiopasClass *klass)
{
}

