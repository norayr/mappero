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

#include <gconf/gconf-client.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#include <libxml/xmlreader.h>
#include <mappero/debug.h>
#include <mappero/path.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define GCONF_REITTIOPAS_KEY_PREFIX GCONF_ROUTER_KEY_PREFIX"/reittiopas"
#define GCONF_REITTIOPAS_KEY_MARGIN GCONF_REITTIOPAS_KEY_PREFIX"/margin"
#define GCONF_REITTIOPAS_KEY_WALKSPEED GCONF_REITTIOPAS_KEY_PREFIX"/walkspeed"
#define GCONF_REITTIOPAS_KEY_OPTIMIZE GCONF_REITTIOPAS_KEY_PREFIX"/optimize"
#define GCONF_REITTIOPAS_KEY_ALLOWED GCONF_REITTIOPAS_KEY_PREFIX"/allowed"

#define REITTIOPAS_ROUTER_URL \
    "http://api.reittiopas.fi/public-ytv/fi/api/?user=maemomapper&pass=b4zmhurp"

static void router_iface_init(MapRouterIface *iface);
static void map_reittiopas_geocode(MapRouter *router, const gchar *address,
                                   MapRouterGeocodeCb callback,
                                   gpointer user_data);

G_DEFINE_TYPE_WITH_CODE(MapReittiopas, map_reittiopas, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(MAP_TYPE_ROUTER,
                                              router_iface_init));

#define WALKSPEED_TO_SELECTOR(ws)   (ws - 1)
#define WALKSPEED_FROM_SELECTOR(idx)    (idx + 1)

enum {
    COL_PIXBUF = 0,
    COL_LAST
};

typedef enum {
    RO_TRANSPORT_HELSINKI_BUS = 1,
    RO_TRANSPORT_HELSINKI_TRAM,
    RO_TRANSPORT_ESPOO_INTERNAL,
    RO_TRANSPORT_VANTAA_INTERNAL,
    RO_TRANSPORT_REGIONAL,
    RO_TRANSPORT_METRO,
    RO_TRANSPORT_FERRY,
    RO_TRANSPORT_U_LINES,
    RO_TRANSPORT_OTHER_REGIONAL,
    RO_TRANSPORT_LONG_DISTANCE,
    RO_TRANSPORT_EXPRESS,
    RO_TRANSPORT_VR_REGIONAL,
    RO_TRANSPORT_VR_LONG_DISTANCE,
    RO_TRANSPORT_ALL,
    RO_TRANSPORT_HELSINKI_SERVICE_LINES = 21,
    RO_TRANSPORT_HELSINKI_NIGHT,
    RO_TRANSPORT_ESPOO_SERVICE_LINES,
    RO_TRANSPORT_VANTAA_SERVICE_LINES,
    RO_TRANSPORT_REGIONAL_NIGHT,
} RoTransport;

typedef enum {
    RO_AREA_TYPE_HELSINKI_INTERNAL = '1',
    RO_AREA_TYPE_ESPOO,
    RO_AREA_TYPE_LOCAL_TRAIN,
    RO_AREA_TYPE_VANTAA,
    RO_AREA_TYPE_REGIONAL,
    RO_AREA_TYPE_UNUSED,
    RO_AREA_TYPE_U_LINES,
} RoAreaType;

/* a point in Reittiopas coordinate system */
typedef struct {
    gfloat p;
    gfloat i;
} KKJ2;

typedef struct {
    KKJ2 from;
    KKJ2 to;
    time_t time;
    gboolean arrival;
} RoQuery;

typedef struct {
    MapPoint from;
    MapPoint to;
    gchar *dest_address;
    GtkWindow *parent;
    MapRouterCalculateRouteCb callback;
    gpointer user_data;
} CalculateRouteData;

typedef enum {
    EL_NONE,
    EL_MTRXML,
    EL_ROUTE,
    EL_LENGTH,
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
#define MAX_ROUTES  5

typedef struct {
    KKJ2 kkj;
    time_t arrival;
    time_t departure;
    gchar *code;
    gchar *id;
    gchar *name;
} RoPoint;

typedef struct {
    guint time; /* in seconds */
    guint distance; /* in metres */
} RoLength;

#define LINE_CODE_LEN 6
typedef struct {
    RoLength length;
    gboolean walk;
    RoTransport transport;
    RoAreaType area_type;
    gint mobility;
    gchar code[LINE_CODE_LEN];
    gchar *id;
    GArray *points;
} RoLine;

typedef struct {
    RoLength length;
    GList *lines;
} RoRoute;

typedef struct {
    KKJ2 kkj;
    gboolean got_coords;
    RoTransport type;
    gint mobility;
    const gchar *code;
    const gchar *id;
    const gchar *lang;
    const gchar *val;
    time_t time;
    gfloat lat;
    gfloat lon;
    RoLength length;
} Attributes;

typedef struct {
    RoRoute routes[MAX_ROUTES];
    gint n_routes;
} RoRoutes;

typedef struct {
    gfloat lat;
    gfloat lon;
    gboolean error;
    RoElement elements[MAX_LEVELS];
    gint level;
    RoPoint point;
    RoRoutes *routes;
    /* current elements */
    RoRoute *route;
    RoLine *line;
} SaxData;

typedef struct {
    MapReittiopas *reittiopas;
    GtkWidget *dialog;
    RoRoutes *routes;
    const RoQuery *query;
    GtkListStore *store;
} RouteSelectionData;

#define strsame(s1, s2)  (strcmp((const gchar *)s1, s2) == 0)

static void
download_route(MapReittiopas *self, RoRoutes *routes, const RoQuery *q,
               GError **error);

static gboolean
location2units(const MapLocation *loc, MapPoint *u)
{
    gboolean ret = FALSE;

    if (loc->address)
    {
        MapGeo lat, lon;
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
wgslatlon2kkjlatlon(MapGeo wlat, MapGeo wlon, MapGeo *klat, MapGeo *klon)
{
    MapGeo la, lo;
    MapGeo dla, dlo;

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
kkjlatlon2kkjxy(MapGeo KKJradla, MapGeo KKJradlo,
                gint ZoneNumber, MapGeo Long0, KKJ2 *k)
{
    MapGeo Lo, a, b, f, bb, c, ee, n, nn, cosLa, cosLaF, LaF, t, A, A1, A2, A3, A4, X, Y, NN;

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
KKJlalo_to_WGSlalo(MapGeo kla, MapGeo klo, MapGeo *lat, MapGeo *lon)
{
    MapGeo La, Lo, dLa, dLo;

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
KKJxy_to_KKJlalo(const KKJ2 *k, gint ZoneNumber, MapGeo Long0,
                 MapGeo *kla, MapGeo *klo)
{
    MapGeo klat, klon;
    MapGeo MinLa, MaxLa, MinLo, MaxLo;
    MapGeo DeltaLa, DeltaLo;
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
kkj22latlon(const KKJ2 *k, MapGeo *lat, MapGeo *lon)
{
    MapGeo rlat, rlon;
    MapGeo klat, klon;
    MapGeo long0;

    long0 = deg2rad(24);
    KKJxy_to_KKJlalo(k, 2, long0, &klat, &klon);
    KKJlalo_to_WGSlalo(klat, klon, &rlat, &rlon);
    *lat = rad2deg(rlat);
    *lon = rad2deg(rlon);
}

static void
kkj22unit(const KKJ2 *k, MapPoint *p)
{
    MapGeo lat, lon;

    kkj22latlon(k, &lat, &lon);

    latlon2unit(lat, lon, p->x, p->y);
}

static void
latlon2kkj2(MapGeo lat, MapGeo lon, KKJ2 *k)
{
    MapGeo rlat, rlon;
    MapGeo klat, klon;
    MapGeo long0;

    rlat = deg2rad(lat);
    rlon = deg2rad(lon);

    wgslatlon2kkjlatlon(rlat, rlon, &klat, &klon);
    long0 = deg2rad(24);
    kkjlatlon2kkjxy(klat, klon, 2, long0, k);
}

static void
unit2kkj2(const MapPoint *p, KKJ2 *k)
{
    MapGeo lat, lon;

    unit2latlon(p->x, p->y, lat, lon);

    latlon2kkj2(lat, lon, k);
}

/* XML handling */

static void
ro_point_free(RoPoint *point)
{
    g_free(point->code);
    g_free(point->id);
    g_free(point->name);
    memset(point, 0, sizeof(RoPoint));
}

static RoLine *
ro_line_new()
{
    RoLine *line = g_slice_new0(RoLine);
    line->points = g_array_sized_new(FALSE, FALSE, sizeof(RoPoint), 4);
    return line;
}

static void
ro_line_free(RoLine *line)
{
    gint i;

    g_free(line->id);

    for (i = 0; i < line->points->len; i++)
    {
        RoPoint *point = &g_array_index(line->points, RoPoint, i);
        ro_point_free(point);
    }
    g_array_free(line->points, TRUE);

    g_slice_free(RoLine, line);
}

static void
ro_route_free(RoRoute *route)
{
    while (route->lines)
    {
        RoLine *line = route->lines->data;
        ro_line_free(line);
        route->lines = g_list_delete_link(route->lines, route->lines);
    }
}

static void
ro_routes_free(RoRoutes *routes)
{
    gint i;

    for (i = 0; i < routes->n_routes; i++)
        ro_route_free(&routes->routes[i]);
}

static void
sax_data_free(SaxData *data)
{
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

static const gchar *
transport_type(RoTransport type)
{
    switch (type) {
    case RO_TRANSPORT_HELSINKI_BUS: return "Helsinki/bus";
    case RO_TRANSPORT_HELSINKI_TRAM: return "Helsinki/tram";
    case RO_TRANSPORT_ESPOO_INTERNAL: return "Espoo internal";
    case RO_TRANSPORT_VANTAA_INTERNAL: return "Vantaa internal";
    case RO_TRANSPORT_REGIONAL: return "Regional traffic";
    case RO_TRANSPORT_METRO: return "Metro traffic";
    case RO_TRANSPORT_FERRY: return "Ferry";
    case RO_TRANSPORT_U_LINES: return "U-lines";
    case RO_TRANSPORT_OTHER_REGIONAL: return "Other local traffic";
    case RO_TRANSPORT_LONG_DISTANCE: return "Long-distance traffic";
    case RO_TRANSPORT_EXPRESS: return "Express";
    case RO_TRANSPORT_VR_REGIONAL: return "VR local traffic";
    case RO_TRANSPORT_VR_LONG_DISTANCE: return "VR long-distance traffic";
    case RO_TRANSPORT_ALL: return "All";
    case RO_TRANSPORT_HELSINKI_SERVICE_LINES: return "Helsinki service lines";
    case RO_TRANSPORT_HELSINKI_NIGHT: return "Helsinki night traffic";
    case RO_TRANSPORT_ESPOO_SERVICE_LINES: return "Espoo service lines";
    case RO_TRANSPORT_VANTAA_SERVICE_LINES: return "Vantaa service lines";
    case RO_TRANSPORT_REGIONAL_NIGHT: return "Regional night traffic";
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
    if (strsame(name, "LENGTH"))
        return EL_LENGTH;
    if (strsame(name, "ERROR"))
        return EL_ERROR;
    return EL_UNKNOWN;
}

static void
parse_attributes(Attributes *a, RoElement el, const gchar **attrs)
{
    const gchar **attr;
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
                a->kkj.i = g_ascii_strtod(value, NULL);
                got_x = TRUE;
            }
            else if (strsame(name, "y"))
            {
                a->kkj.p = g_ascii_strtod(value, NULL);
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
        else if (el == EL_LENGTH)
        {
            if (strsame(name, "time"))
                a->length.time = g_ascii_strtod(value, NULL) * 60;
            else if (strsame(name, "dist"))
                a->length.distance = g_ascii_strtod(value, NULL);
        }
    }

    if (got_x && got_y)
        a->got_coords = TRUE;

    if (date && time)
    {
        struct tm tm;

        memset(&tm, 0, sizeof(tm));
        strptime(date, "%Y%m%d", &tm);
        strptime(time, "%H%M", &tm);
        tm.tm_isdst = -1;
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

    if (el == EL_ROUTE)
    {
        if (!data->routes || data->routes->n_routes >= MAX_ROUTES)
            data->error = TRUE;
        else
            data->route = data->routes->routes + data->routes->n_routes;
    }
    else if (EL_IS_SEGMENT(el))
    {
        if (!data->route)
        {
            data->error = TRUE;
            return;
        }
        data->line = ro_line_new();
        if (el == EL_WALK)
            data->line->walk = TRUE;
    }

    if (attrs)
    {
        memset(&a, 0, sizeof(a));
        parse_attributes(&a, el, (const gchar **)attrs);

        if (EL_IS_SEGMENT(parent))
        {
            if (EL_IS_POINT(el))
            {
                if (a.got_coords)
                {
                    data->point.kkj = a.kkj;
                    data->point.code = g_strdup(a.code);
                    data->point.id = g_strdup(a.id);
                }
            }
            else if (el == EL_LENGTH)
            {
                data->line->length = a.length;
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
        else if (parent == EL_ROUTE)
        {
            if (el == EL_LINE)
            {
                gchar buffer[6], *ptr;

                data->line->area_type = a.code[0];
                sprintf(buffer, "%.4s", a.code + 1);
                /* remove leading spaces/zeroes and trailing spaces */
                for (ptr = buffer + strlen(buffer) - 1; ptr[0] == ' '; ptr--)
                    ptr[0] = '\0';
                for (ptr = buffer; ptr[0] == ' ' || ptr[0] == '0'; ptr++);
                sprintf(data->line->code, "%.4s", ptr);
                data->line->id = g_strdup(a.id);
                data->line->transport = a.type;
                data->line->mobility = a.mobility;
            }
            else if (el == EL_LENGTH)
            {
                data->route->length = a.length;
            }
        }
        else if (el == EL_LOC && data->lat == 0)
        {
            data->lat = a.lat;
            data->lon = a.lon;
            DEBUG("Got lat lon: %.6f, %.6f", data->lat, data->lon);
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
        if (data->line)
        {
            g_array_append_val(data->line->points, data->point);
            memset(&data->point, 0, sizeof(RoPoint));
        }
        else
            ro_point_free(&data->point);
    }
    else if (EL_IS_SEGMENT(el))
    {
        data->route->lines = g_list_append(data->route->lines, data->line);
        data->line = NULL;
    }
    else if (el == EL_ROUTE)
    {
        data->routes->n_routes++;
        data->route = NULL;
    }

    data->level--;
}

static void
ro_point_to_path_point(const RoPoint *point, MapPathPoint *p)
{
    kkj22unit(&point->kkj, &p->unit);
    p->time = point->departure;
    p->altitude = 0;
}

static void
ro_route_to_path(const RoRoute *route, MapPath *path)
{
    MapPathPoint *last = NULL;
    GList *list;

    for (list = route->lines; list != NULL; list = list->next)
    {
        RoLine *line = list->data;
        RoPoint *point;
        MapPathPoint path_point;
        gint i;

        if (line->points->len < 2) continue;

        /* the first point is somehow special: handle it out of the for loop */
        point = &g_array_index(line->points, RoPoint, 0);

        memset(&path_point, 0, sizeof(path_point));
        ro_point_to_path_point(point, &path_point);
        if (last != NULL &&
            last->unit.x == path_point.unit.x &&
            last->unit.y == path_point.unit.y)
        {
            /* if this point has the same coordinates of the previous one,
             * overwrite the previous: do not increment the path */
            *last = path_point;
        }
        else
            last = map_path_append_point_fast(path, &path_point);

        if (point->name)
        {
            gchar *desc;
            /* the first point is always a waypoint */
            if (line->code[0] != '\0')
            {
                desc = g_strdup_printf("%s\n%s %.4s", point->name,
                                       transport_type(line->transport),
                                       line->code);
            }
            else
                desc = g_strdup(point->name);
            map_path_make_waypoint(path, last, desc);
        }

        for (i = 1; i < line->points->len; i++)
        {
            MapPathPoint pt;
            point = &g_array_index(line->points, RoPoint, i);

            ro_point_to_path_point(point, &pt);
            last = map_path_append_point_fast(path, &pt);
        }
    }

    /* Add a null point at the end of the route */
    map_path_append_break(path);
    map_path_append_point_end(path);
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

static size_t
time_to_string(gchar *string, size_t size, time_t time)
{
    struct tm tm;

    tm = *localtime(&time);
    return strftime(string, size, "%H:%M", &tm);
}

static inline guint8
convert_color_channel(guint8 src, guint8 alpha)
{
        return alpha ? ((src << 8) - src) / alpha : 0;
}

/**
 * cairo_convert_to_pixbuf:
 * Converts from a Cairo image surface to a GdkPixbuf. Why does GTK+ not
 * implement this?
 */
GdkPixbuf *
cairo_convert_to_pixbuf(cairo_surface_t *surface)
{
    GdkPixbuf *pixbuf;
    int width, height;
    int srcstride, dststride;
    guchar *srcpixels, *dstpixels;
    guchar *srcpixel, *dstpixel;
    int n_channels;
    int x, y;

    width = cairo_image_surface_get_width (surface);
    height = cairo_image_surface_get_height (surface);
    srcstride = cairo_image_surface_get_stride (surface);
    srcpixels = cairo_image_surface_get_data (surface);

    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                             width, height);
    dststride = gdk_pixbuf_get_rowstride (pixbuf);
    dstpixels = gdk_pixbuf_get_pixels (pixbuf);
    n_channels = gdk_pixbuf_get_n_channels (pixbuf);

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            srcpixel = srcpixels + y * srcstride + x * 4;
            dstpixel = dstpixels + y * dststride + x * n_channels;

            dstpixel[0] = convert_color_channel (srcpixel[2],
                                                 srcpixel[3]);
            dstpixel[1] = convert_color_channel (srcpixel[1],
                                                 srcpixel[3]);
            dstpixel[2] = convert_color_channel (srcpixel[0],
                                                 srcpixel[3]);
            dstpixel[3] = srcpixel[3];
        }
    }

    return pixbuf;
}

static void
draw_text(cairo_t *cr, PangoFontDescription *desc, const gchar *text,
          gboolean centered, gboolean top)
{
    PangoLayout *layout;
    gint width, height;

    layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, text, -1);
    pango_layout_get_pixel_size(layout, &width, &height);
    if (top)
        cairo_rel_move_to(cr, 0, -height);
    if (centered)
        cairo_rel_move_to(cr, -width / 2, 0);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
}

static const gchar *
ro_line_to_icon_name(RoLine *line)
{
#define ICON_BASE   "/usr/share/icons/hicolor/scalable/hildon/maemo-mapper-"
    if (line->walk)
        return ICON_BASE "walk.png";

    if (line->area_type == RO_AREA_TYPE_LOCAL_TRAIN)
        return ICON_BASE "train.png";

    switch (line->transport) {
    case RO_TRANSPORT_METRO:
        return ICON_BASE "metro.png";
    case RO_TRANSPORT_FERRY:
        return ICON_BASE "ferry.png";
    case RO_TRANSPORT_HELSINKI_TRAM:
        return ICON_BASE "tram.png";
    case RO_TRANSPORT_EXPRESS:
    case RO_TRANSPORT_VR_REGIONAL:
    case RO_TRANSPORT_VR_LONG_DISTANCE:
        return ICON_BASE "train.png";
    default:
        return ICON_BASE "bus.png";
    }
}

static void
put_tranport_icon(cairo_t *cr, RoLine *line, gint x, gint y)
{
    cairo_surface_t *icon;
    gint width, height;
    const gchar *icon_name;

    icon_name = ro_line_to_icon_name(line);
    icon = cairo_image_surface_create_from_png(icon_name);
    width = cairo_image_surface_get_width(icon);
    height = cairo_image_surface_get_height(icon);

    cairo_save(cr);

    cairo_translate(cr, x, y);
    cairo_set_source_surface(cr, icon, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(icon);

    cairo_restore(cr);
}

static inline void
set_source_color(cairo_t *cr, GdkColor *color)
{
    cairo_set_source_rgb(cr,
                         color->red / 65535.0,
                         color->green / 65535.0,
                         color->blue / 65535.0);
}

static void
ro_line_transport_to_text(gchar *buffer, gsize len, RoLine *line)
{
    if (line->walk)
        g_snprintf(buffer, len, "%.1fkm", line->length.distance / 1000.0);
    else if (line->transport == RO_TRANSPORT_METRO)
        strncpy(buffer, _("Metro"), len);
    else if (line->transport == RO_TRANSPORT_FERRY)
        strncpy(buffer, _("Ferry"), len);
    else
        g_snprintf(buffer, len, "%.4s", line->code);
}

static GdkPixbuf *
ro_route_to_pixbuf(RoRoute *route)
{
    GdkPixbuf *pixbuf;
    gchar buffer[128];
    RoPoint *point = NULL;
    RoLine *line = NULL;
    gint width, height, seg_x = 0;
    GList *list;
    cairo_surface_t *surface;
    cairo_t *cr;
    PangoFontDescription *font_time, *font_small;
    GdkColor color_default, color_secondary;
    GtkStyle *style;

#define SEGMENT_WIDTH   150
#define ROUTE_HEIGHT    80
#define TICK_HEIGHT 4
#define TIME_WIDTH  64
#define LINE_X  (TIME_WIDTH / 2)
#define LINE_Y  56
#define TIME_X          LINE_X
#define TIME_Y          (LINE_Y - TICK_HEIGHT)
#define ICON_SIZE   20
#define ICON_X  (LINE_X + SEGMENT_WIDTH / 2 - ICON_SIZE / 2)
#define ICON_Y  (LINE_Y - ICON_SIZE - TICK_HEIGHT)
#define TRANSPORT_X (LINE_X + SEGMENT_WIDTH / 2)
#define TRANSPORT_Y ICON_Y
#define PLACE_X LINE_X
#define PLACE_Y LINE_Y

    style = gtk_rc_get_style_by_paths(gtk_settings_get_default(),
                                      "SystemFont", NULL, G_TYPE_NONE);
    font_time = pango_font_description_copy(style->font_desc);

    style = gtk_rc_get_style_by_paths(gtk_settings_get_default(),
                                      "SmallSystemFont", NULL, G_TYPE_NONE);
    font_small = pango_font_description_copy(style->font_desc);

    style = gtk_rc_get_style_by_paths(gtk_settings_get_default(),
                                      NULL, "GtkWidget", GTK_TYPE_WIDGET);
    gtk_style_lookup_color(style, "DefaultTextColor", &color_default);
    gtk_style_lookup_color(style, "SecondaryTextColor", &color_secondary);

    width = g_list_length(route->lines) * SEGMENT_WIDTH + TIME_WIDTH;
    height = ROUTE_HEIGHT;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    cr = cairo_create(surface);

    for (list = route->lines; list != NULL; list = list->next)
    {
        line = list->data;

        /* draw the timeline */
        set_source_color (cr, &color_secondary);
        cairo_set_line_width (cr, 2);
        cairo_move_to (cr, seg_x + LINE_X, LINE_Y - TICK_HEIGHT);
        cairo_rel_line_to (cr, 0, TICK_HEIGHT * 2);
        cairo_move_to (cr, seg_x + LINE_X, LINE_Y);
        cairo_rel_line_to (cr, SEGMENT_WIDTH, 0);
        cairo_rel_move_to (cr, 0, -TICK_HEIGHT);
        cairo_rel_line_to (cr, 0, TICK_HEIGHT * 2);
        cairo_stroke(cr);

        set_source_color (cr, &color_default);

        point = &g_array_index(line->points, RoPoint, 0);
        time_to_string(buffer, sizeof(buffer), point->departure);

        cairo_move_to(cr, seg_x + TIME_X, TIME_Y);
        draw_text(cr, font_time, buffer, TRUE, TRUE);

        put_tranport_icon(cr, line, seg_x + ICON_X, ICON_Y);

        cairo_move_to(cr, seg_x + TRANSPORT_X, TRANSPORT_Y);
        ro_line_transport_to_text(buffer, sizeof(buffer), line);
        draw_text(cr, line->walk ? font_small : font_time, buffer, TRUE, TRUE);

        if (point->name)
        {
            set_source_color (cr, &color_secondary);
            cairo_move_to(cr, seg_x + PLACE_X, PLACE_Y);
            draw_text(cr, font_small, point->name, FALSE, FALSE);
        }

        seg_x += SEGMENT_WIDTH;
    }

    /* print the time of the last point */
    if (line && line->points->len > 0)
    {
        point = &g_array_index(line->points, RoPoint, line->points->len - 1);
        time_to_string(buffer, sizeof(buffer), point->arrival);

        cairo_move_to(cr, seg_x + TIME_X, TIME_Y);
        set_source_color (cr, &color_default);
        draw_text(cr, font_time, buffer, TRUE, TRUE);
    }

    cairo_destroy(cr);

    pixbuf = cairo_convert_to_pixbuf(surface);
    cairo_surface_destroy(surface);

    pango_font_description_free(font_time);
    pango_font_description_free(font_small);
    return pixbuf;
}

static void
put_routes_in_store(RoRoutes *routes, GtkListStore *store)
{
    GdkPixbuf *pixbuf;
    gint i;

    for (i = 0; i < routes->n_routes; i++)
    {
        RoRoute *route = routes->routes + i;
        GtkTreeIter iter;

        gtk_list_store_append(store, &iter);
        pixbuf = ro_route_to_pixbuf(route);
        gtk_list_store_set(store, &iter,
                           COL_PIXBUF, pixbuf,
                           -1);
        g_object_unref(pixbuf);
    }
}

static void
refresh_dialog(RouteSelectionData *sd, RoQuery *query, gboolean invert)
{
    GError *error = NULL;
    RoRoutes new_routes;

    download_route(sd->reittiopas, &new_routes, query, &error);
    if (error)
    {
        map_error_show_and_clear(GTK_WINDOW(sd->dialog), &error);
        return;
    }

    gtk_list_store_clear(sd->store);
    ro_routes_free(sd->routes);

    if (invert)
    {
        gint i;

        for (i = 0; i < new_routes.n_routes; i++)
            memcpy(sd->routes->routes + i,
                   new_routes.routes + (new_routes.n_routes - 1 - i),
                   sizeof(RoRoute));
        sd->routes->n_routes = new_routes.n_routes;
    }
    else
        memcpy(sd->routes, &new_routes, sizeof(RoRoutes));
    put_routes_in_store(sd->routes, sd->store);
}

static void
on_earlier_clicked(GtkWidget *button, RouteSelectionData *sd)
{
    RoRoute *route;
    RoLine *line;
    RoPoint *point;
    RoQuery query;

    if (sd->routes->n_routes == 0) return;

    /* find the first final time */
    route = sd->routes->routes;
    if (route->lines)
    {
        line = g_list_last(route->lines)->data;
        if (line->points->len > 0)
        {
            point = &g_array_index(line->points, RoPoint,
                                   line->points->len - 1);

            query = *sd->query;
            query.time = point->arrival - 60;
            query.arrival = TRUE;

            refresh_dialog(sd, &query, TRUE);
        }
    }
}

static void
on_later_clicked(GtkWidget *button, RouteSelectionData *sd)
{
    RoRoute *route;
    RoLine *line;
    RoPoint *point;
    RoQuery query;

    if (sd->routes->n_routes == 0) return;

    /* find the last time */
    route = sd->routes->routes + sd->routes->n_routes - 1;
    if (route->lines)
    {
        line = route->lines->data;
        if (line->points->len > 0)
        {
            point = &g_array_index(line->points, RoPoint, 0);

            query = *sd->query;
            query.time = point->departure + 60;
            query.arrival = FALSE;

            refresh_dialog(sd, &query, FALSE);
        }
    }
}

static void
on_pannable_size_request(GtkWidget *pannable, GtkRequisition *req,
                         GtkWidget *child)
{
    gtk_widget_get_child_requisition(child, req);
}

static void
on_route_selected(GtkTreeView *treeview, GtkTreePath *path,
                  GtkTreeViewColumn *column, GtkDialog *dialog)
{
    gint *p_i;

    p_i = gtk_tree_path_get_indices(path);
    if (p_i)
        gtk_dialog_response(dialog, *p_i);
}

static gint
route_selection_dialog(MapReittiopas *self, GtkWindow *parent, RoRoutes *routes,
                       const RoQuery *query)
{
    GtkWidget *dialog;
    GtkWidget *tree_view;
    GtkWidget *pannable, *viewport, *hbox, *button;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkListStore *store;
    gint response;
    RouteSelectionData sd;

    dialog = gtk_dialog_new_with_buttons(_("Select route"), parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        NULL);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_widget_hide(GTK_DIALOG(dialog)->action_area);
    gtk_widget_set_no_show_all(GTK_DIALOG(dialog)->action_area, TRUE);

    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
                       FALSE, FALSE, 0);

    button = gtk_button_new_with_label(_("Earlier"));
    hildon_gtk_widget_set_theme_size(button, HILDON_SIZE_FINGER_HEIGHT);
    g_signal_connect(button, "clicked",
                     G_CALLBACK(on_earlier_clicked), &sd);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

    button = gtk_button_new_with_label(_("Later"));
    hildon_gtk_widget_set_theme_size(button, HILDON_SIZE_FINGER_HEIGHT);
    g_signal_connect(button, "clicked",
                     G_CALLBACK(on_later_clicked), &sd);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

    store = gtk_list_store_new(COL_LAST, GDK_TYPE_PIXBUF);

    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_set_name(tree_view, "mapper::reittiopas::tree-view");
    gtk_rc_parse_string("style \"reittiopas\" = \"fremantle-touchlist\"\n{\n"
                        "GtkTreeView::row-height = 80\n"
                        "GtkTreeView::row-ending-details = 1\n"
                        "GtkWidget::hildon-mode = 1\n}\n"
                        "widget \"*.mapper::reittiopas::tree-view\" "
                        "style \"reittiopas\"");
    g_signal_connect(tree_view, "row-activated",
                     G_CALLBACK(on_route_selected), dialog);

    column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
                          "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                          NULL);
    renderer = g_object_new(GTK_TYPE_CELL_RENDERER_PIXBUF,
                            "xalign", 0.0,
                            NULL);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", COL_PIXBUF);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    pannable = hildon_pannable_area_new();
    g_object_set(pannable,
                 "mov-mode", HILDON_MOVEMENT_MODE_BOTH,
                 NULL);
    g_signal_connect(pannable, "size-request",
                     G_CALLBACK(on_pannable_size_request), tree_view);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), pannable,
                       TRUE, TRUE, 0);

    viewport = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(viewport), tree_view);
    gtk_container_add(GTK_CONTAINER(pannable), viewport);

    put_routes_in_store(routes, store);

    memset(&sd, 0, sizeof(sd));
    sd.reittiopas = self;
    sd.dialog = dialog;
    sd.store = store;
    sd.routes = routes;
    sd.query = query;

    gtk_widget_show_all(dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);

    if (response < 0) return -1;

    return response;
}

static void
download_route(MapReittiopas *self, RoRoutes *routes, const RoQuery *q,
               GError **error)
{
    gchar *query, *bytes;
    GString *string;
    gint size;
    GnomeVFSResult vfs_result;
    SaxData data;

    memset(routes, 0, sizeof(RoRoutes));

    string = g_string_new_len(REITTIOPAS_ROUTER_URL,
                              sizeof(REITTIOPAS_ROUTER_URL) - 1);

    g_string_append_printf(string, "&a=%.0f,%.0f&b=%.0f,%.0f&show=5",
                           q->from.i, q->from.p, q->to.i, q->to.p);

    if (!self->transport_allowed[RO_TRANSPORT_TYPE_BUS])
        g_string_append(string, "&use_bus=0");
    if (!self->transport_allowed[RO_TRANSPORT_TYPE_TRAIN])
        g_string_append(string, "&use_train=0");
    if (!self->transport_allowed[RO_TRANSPORT_TYPE_FERRY])
        g_string_append(string, "&use_ferry=0");
    if (!self->transport_allowed[RO_TRANSPORT_TYPE_METRO])
        g_string_append(string, "&use_metro=0");
    if (!self->transport_allowed[RO_TRANSPORT_TYPE_TRAM])
        g_string_append(string, "&use_tram=0");

    g_string_append_printf(string, "&margin=%d&optimize=%d&walkspeed=%d",
                           self->margin, self->optimize, self->walkspeed);

    if (q->time != 0)
    {
        struct tm tm;
        gchar time[6], date[10];
        tm = *localtime(&q->time);
        strftime(time, sizeof(time), "%H%M", &tm);
        strftime(date, sizeof(date), "%Y%m%d", &tm);
        g_string_append_printf(string, "&time=%s&date=%s&timemode=%d",
                               time, date, q->arrival + 1);
    }

    query = g_string_free(string, FALSE);
    DEBUG("URL: %s", query);

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
    data.routes = routes;
    if (!parse_xml(&data, bytes, size) || routes->n_routes == 0)
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        sax_data_free(&data);
        goto finish;
    }

    sax_data_free(&data);
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

    DEBUG("URL: %s", query);

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
        sax_data_free(&data);
        goto finish;
    }

    *lat = data.lat;
    *lon = data.lon;
    sax_data_free(&data);

finish:
    g_free(bytes);
}

static void
calculate_route_with_units(MapRouter *router,
                           const MapPoint *from_u, const MapPoint *to_u,
                           GtkWindow *parent,
                           MapRouterCalculateRouteCb callback,
                           gpointer user_data)
{
    MapReittiopas *self = MAP_REITTIOPAS(router);
    MapPath path;
    RoRoutes routes;
    RoQuery query;
    GError *error = NULL;

    memset(&query, 0, sizeof(query));
    unit2kkj2(from_u, &query.from);
    unit2kkj2(to_u, &query.to);

    download_route(self, &routes, &query, &error);
    if (!error)
    {
        gint id = 0;
        if (routes.n_routes > 1 && parent != NULL)
            id = route_selection_dialog(self, parent, &routes, &query);

        if (id >= 0)
        {
            map_path_init(&path);
            ro_route_to_path(&routes.routes[id], &path);
            callback(router, &path, NULL, user_data);
        }
        else
        {
            GError err = { MAP_ERROR, MAP_ERROR_USER_CANCELED, "" };
            callback(router, NULL, &err, user_data);
        }

        ro_routes_free(&routes);
    }
    else
    {
        callback(router, NULL, error, user_data);
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
    calculate_route_with_units(router, &crd->from, &crd->to, crd->parent,
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
        DEBUG("Got lat lon: %.6f, %.6f, units %u, %u", lat, lon, p.x, p.y);
        callback(router, p, NULL, user_data);
    }
    else
    {
        callback(router, p, error, user_data);
        g_error_free(error);
    }
}

static void
hack_sel(GtkWidget *selector, gint column, gboolean *hacked)
{
    GtkWidget *dialog;
    guint id, ret;

    if (*hacked) return;

    /* HILDON HACK: HildonPickerDialog blocks the emission of the "response"
     * signal if no item is selected. Since this blocking happens on a signal
     * handler for the "response" signal itself, we disconnect it :-) */
    dialog = gtk_widget_get_toplevel(selector);
    id = g_signal_lookup("response", GTK_TYPE_DIALOG);
    ret = g_signal_handlers_disconnect_matched(dialog,
        G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA, id, 0, NULL, NULL, NULL);
    if (ret != 1)
        g_warning("%s: disconnected %u signals!", G_STRFUNC, ret);

    *hacked = TRUE;
}

static gchar *
transport_print_func(HildonTouchSelector *selector, gpointer user_data)
{
    GList *selected_rows, *list;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar text[256], *transport;
    gint i_text = 0;

    selected_rows = hildon_touch_selector_get_selected_rows(selector, 0);
    model = hildon_touch_selector_get_model(selector, 0);
    for (list = selected_rows; list != NULL; list = list->next)
    {
        GtkTreePath *path = list->data;

        gtk_tree_model_get_iter(model, &iter, path);
        gtk_tree_path_free(path);

        gtk_tree_model_get(model, &iter, 0, &transport, -1);
        g_assert(i_text + strlen(transport) + 1 < sizeof(text));
        i_text += sprintf(text + i_text, (i_text == 0) ? "%s" : ", %s",
                          transport);
        g_free(transport);
    }
    g_list_free(selected_rows);

    if (i_text == 0)
    {
        return g_strdup(_("Walking only"));
    }
    else
        return g_strdup(text);
}

static void
map_reittiopas_run_options_dialog(MapRouter *router, GtkWindow *parent)
{
    MapReittiopas *self = MAP_REITTIOPAS(router);
    GtkWidget *dialog;
    GtkWidget *widget;
    HildonTouchSelector *transport, *optimize, *walkspeed, *margin;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean selector_hacked = FALSE;
    gchar buffer[16];
    gint i;

    dialog = map_dialog_new(_("Reittiopas router options"), parent, TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          H_("wdgt_bd_save"), GTK_RESPONSE_ACCEPT);

    /* transfer margin */
    margin = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    for (i = 0; i <= 10; i++)
    {
        sprintf(buffer, "%d", i);
        hildon_touch_selector_append_text(margin, buffer);
    }
    hildon_touch_selector_set_active(margin, 0, self->margin);

    widget =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Transfer margin (mins)"),
                     "touch-selector", margin,
                     "xalign", 0.0,
                     NULL);
    map_dialog_add_widget(MAP_DIALOG(dialog), widget);

    /* Optimization goal */

    optimize = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(optimize, _("default"));
    hildon_touch_selector_append_text(optimize, _("fastest"));
    hildon_touch_selector_append_text(optimize, _("least transfers"));
    hildon_touch_selector_append_text(optimize, _("least walking"));
    hildon_touch_selector_set_active(optimize, 0, self->optimize);

    widget =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Routing algorithm"),
                     "touch-selector", optimize,
                     "xalign", 0.0,
                     NULL);
    map_dialog_add_widget(MAP_DIALOG(dialog), widget);

    /* Walking speed */

    walkspeed = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(walkspeed, _("slow"));
    hildon_touch_selector_append_text(walkspeed, _("normal"));
    hildon_touch_selector_append_text(walkspeed, _("fast"));
    hildon_touch_selector_append_text(walkspeed, _("running"));
    hildon_touch_selector_append_text(walkspeed, _("cycling"));
    hildon_touch_selector_set_active(walkspeed, 0,
                                     WALKSPEED_TO_SELECTOR(self->walkspeed));

    widget =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Walking speed"),
                     "touch-selector", walkspeed,
                     "xalign", 0.0,
                     NULL);
    map_dialog_add_widget(MAP_DIALOG(dialog), widget);

    transport=
        HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    hildon_touch_selector_append_text(transport, _("bus"));
    hildon_touch_selector_append_text(transport, _("train"));
    hildon_touch_selector_append_text(transport, _("ferry"));
    hildon_touch_selector_append_text(transport, _("metro"));
    hildon_touch_selector_append_text(transport, _("tram"));
    hildon_touch_selector_set_column_selection_mode(transport,
        HILDON_TOUCH_SELECTOR_SELECTION_MODE_MULTIPLE);
    hildon_touch_selector_set_print_func(transport, transport_print_func);
    hildon_touch_selector_unselect_all(transport, 0);
    model = hildon_touch_selector_get_model(transport, 0);
    for (i = 0; i < RO_TRANSPORT_TYPE_LAST; i++)
    {
        if (self->transport_allowed[i])
        {
            gtk_tree_model_iter_nth_child(model, &iter, NULL, i);
            hildon_touch_selector_select_iter(transport, 0, &iter, FALSE);
        }
    }
    g_signal_connect(transport, "changed", G_CALLBACK(hack_sel),
                     &selector_hacked);

    widget =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Allowed transport means"),
                     "touch-selector", transport,
                     "xalign", 0.0,
                     NULL);
    map_dialog_add_widget(MAP_DIALOG(dialog), widget);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        GList *selected_rows, *list;

        for (i = 0; i < RO_TRANSPORT_TYPE_LAST; i++)
            self->transport_allowed[i] = FALSE;
        selected_rows = hildon_touch_selector_get_selected_rows(transport, 0);
        for (list = selected_rows; list != NULL; list = list->next)
        {
            GtkTreePath *path = list->data;
            gint *p_i;

            p_i = gtk_tree_path_get_indices(path);
            if (*p_i < RO_TRANSPORT_TYPE_LAST)
                self->transport_allowed[*p_i] = TRUE;
            gtk_tree_path_free(path);
        }
        g_list_free(selected_rows);

        self->optimize = hildon_touch_selector_get_active(optimize, 0);

        self->walkspeed = WALKSPEED_FROM_SELECTOR(
            hildon_touch_selector_get_active(walkspeed, 0));

        self->margin = hildon_touch_selector_get_active(margin, 0);
    }

    gtk_widget_destroy(dialog);
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
        crd->parent = query->parent;
        map_reittiopas_geocode(router, address,
                               (MapRouterGeocodeCb)route_address_retrieved,
                               crd);
        return;
    }

    calculate_route_with_units(router, &from_u, &to_u, query->parent,
                               callback, user_data);
}

static void
map_reittiopas_load_options(MapRouter *router, GConfClient *gconf_client)
{
    MapReittiopas *self = MAP_REITTIOPAS(router);
    gchar key_buf[sizeof(GCONF_REITTIOPAS_KEY_ALLOWED) + 5];
    GConfValue *value;
    gint i;

    value = gconf_client_get(gconf_client, GCONF_REITTIOPAS_KEY_MARGIN, NULL);
    if (value) {
        self->margin = gconf_value_get_int(value);
        gconf_value_free(value);
    }

    value = gconf_client_get(gconf_client, GCONF_REITTIOPAS_KEY_WALKSPEED, NULL);
    if (value) {
        self->walkspeed = gconf_value_get_int(value);
        gconf_value_free(value);
    }

    value = gconf_client_get(gconf_client, GCONF_REITTIOPAS_KEY_OPTIMIZE, NULL);
    if (value) {
        self->optimize = gconf_value_get_int(value);
        gconf_value_free(value);
    }

    for (i = 0; i < RO_TRANSPORT_TYPE_LAST; i++) {
        g_snprintf(key_buf, sizeof(key_buf), "%s/%d", GCONF_REITTIOPAS_KEY_ALLOWED, i);
        value = gconf_client_get(gconf_client, key_buf, NULL);
        if (value) {
            self->transport_allowed[i] = gconf_value_get_bool(value);
            gconf_value_free(value);
        }
    }
}

static void
map_reittiopas_save_options(MapRouter *router, GConfClient *gconf_client)
{
    MapReittiopas *self = MAP_REITTIOPAS(router);
    gchar key_buf[sizeof(GCONF_REITTIOPAS_KEY_ALLOWED) + 5];
    gint i;

    gconf_client_set_int(gconf_client, GCONF_REITTIOPAS_KEY_MARGIN, self->margin, NULL);
    gconf_client_set_int(gconf_client, GCONF_REITTIOPAS_KEY_WALKSPEED, self->walkspeed, NULL);
    gconf_client_set_int(gconf_client, GCONF_REITTIOPAS_KEY_OPTIMIZE, self->optimize, NULL);

    for (i = 0; i < RO_TRANSPORT_TYPE_LAST; i++) {
        g_snprintf(key_buf, sizeof(key_buf), "%s/%d", GCONF_REITTIOPAS_KEY_ALLOWED, i);
        gconf_client_set_bool(gconf_client, key_buf, self->transport_allowed[i], NULL);
    }
}

static void
router_iface_init(MapRouterIface *iface)
{
    iface->get_name = map_reittiopas_get_name;
    iface->run_options_dialog = map_reittiopas_run_options_dialog;
    iface->calculate_route = map_reittiopas_calculate_route;
    iface->geocode = map_reittiopas_geocode;
    iface->load_options = map_reittiopas_load_options;
    iface->save_options = map_reittiopas_save_options;
}

static void
map_reittiopas_init(MapReittiopas *self)
{
    gint i;

    for (i = 0; i < RO_TRANSPORT_TYPE_LAST; i++)
        self->transport_allowed[i] = TRUE;

    self->walkspeed = RO_WALKSPEED_NORMAL;
    self->margin = 3;
}

static void
map_reittiopas_class_init(MapReittiopasClass *klass)
{
}

