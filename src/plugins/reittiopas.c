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

#include <hildon/hildon-picker-button.h>
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

#define WALKSPEED_TO_SELECTOR(ws)   (ws - 1)
#define WALKSPEED_FROM_SELECTOR(idx)    (idx + 1)

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
#define MAX_ROUTES  5

typedef struct {
    KKJ2 kkj;
    gboolean got_coords;
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
    KKJ2 kkj;
    time_t arrival;
    time_t departure;
    gchar *code;
    gchar *id;
    gchar *name;
} RoPoint;

typedef struct {
    guint time; /* in seconds */
    float distance;
} RoLength;

typedef struct {
    RoLength length;
    gint type;
    gint mobility;
    gchar *code;
    gchar *id;
    GArray *points;
} RoLine;

typedef struct {
    RoLength length;
    GList *lines;
} RoRoute;

typedef struct {
    gfloat lat;
    gfloat lon;
    gboolean error;
    RoElement elements[MAX_LEVELS];
    gint level;
    RoPoint point;
    RoRoute routes[MAX_ROUTES];
    gint n_routes;
    /* current elements */
    RoRoute *route;
    RoLine *line;
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

    g_free(line->code);
    g_free(line->id);

    for (i = 1; i < line->points->len; i++)
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
sax_data_free(SaxData *data)
{
    gint i;

    for (i = 0; i < MAX_ROUTES; i++)
        ro_route_free(&data->routes[i]);
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
    }

    if (got_x && got_y)
        a->got_coords = TRUE;

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

    if (el == EL_ROUTE)
    {
        if (data->n_routes >= MAX_ROUTES)
            data->error = TRUE;
        else
            data->route = data->routes + data->n_routes;
    }
    else if (EL_IS_SEGMENT(el))
    {
        if (!data->route)
        {
            data->error = TRUE;
            return;
        }
        data->line = ro_line_new();
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
            data->line->code = g_strdup(a.code);
            data->line->id = g_strdup(a.id);
            data->line->type = a.type;
            data->line->mobility = a.mobility;
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
        data->n_routes++;
        data->route = NULL;
    }

    data->level--;
}

static void
ro_point_to_path_point(const RoPoint *point, Point *p)
{
    kkj22unit(&point->kkj, &p->unit);
    p->time = point->departure;
    p->altitude = 0;
}

static void
ro_route_to_path(const RoRoute *route, Path *path)
{
    GList *list;

    for (list = route->lines; list != NULL; list = list->next)
    {
        RoLine *line = list->data;
        RoPoint *point;
        Point path_point;
        gint i;

        if (line->points->len < 2) continue;

        /* the first point is somehow special: handle it out of the for loop */
        point = &g_array_index(line->points, RoPoint, 0);

        memset(&path_point, 0, sizeof(path_point));
        ro_point_to_path_point(point, &path_point);
        if (path->head != path->tail &&
            path->tail->unit.x == path_point.unit.x &&
            path->tail->unit.y == path_point.unit.y)
        {
            /* if this point has the same coordinates of the previous one,
             * overwrite the previous: do not increment the path */
        }
        else
            MACRO_PATH_INCREMENT_TAIL(*path);
        *path->tail = path_point;

        /* the first point is always a waypoint */
        MACRO_PATH_INCREMENT_WTAIL(*path);
        path->wtail->point = path->tail;
        if (line->code)
        {
            path->wtail->desc =
                g_strdup_printf("%s\n%s %.4s", point->name,
                                transport_type(line->type),
                                line->code + 1);
        }
        else
            path->wtail->desc = g_strdup(point->name);

        for (i = 1; i < line->points->len; i++)
        {
            point = &g_array_index(line->points, RoPoint, i);

            MACRO_PATH_INCREMENT_TAIL(*path);
            ro_point_to_path_point(point, path->tail);
        }
    }

    /* Add a null point at the end of the route */
    MACRO_PATH_INCREMENT_TAIL(*path);
    *path->tail = _point_null;
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
download_route(MapReittiopas *self, Path *path,
               const KKJ2 *from, const KKJ2 *to, GError **error)
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

    if (!self->transport_allowed[RO_TRANSPORT_BUS])
        g_string_append(string, "&use_bus=0");
    if (!self->transport_allowed[RO_TRANSPORT_TRAIN])
        g_string_append(string, "&use_train=0");
    if (!self->transport_allowed[RO_TRANSPORT_FERRY])
        g_string_append(string, "&use_ferry=0");
    if (!self->transport_allowed[RO_TRANSPORT_METRO])
        g_string_append(string, "&use_metro=0");
    if (!self->transport_allowed[RO_TRANSPORT_TRAM])
        g_string_append(string, "&use_tram=0");

    g_string_append_printf(string, "&margin=%d&optimize=%d&walkspeed=%d",
                           self->margin, self->optimize, self->walkspeed);

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
    if (!parse_xml(&data, bytes, size) || data.n_routes == 0)
    {
        g_set_error(error, MAP_ERROR, MAP_ERROR_INVALID_ADDRESS,
                    _("Invalid source or destination."));
        sax_data_free(&data);
        goto finish;
    }

    ro_route_to_path(&data.routes[0], path);

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
                           MapRouterCalculateRouteCb callback,
                           gpointer user_data)
{
    KKJ2 to, from;
    Path path;
    GError *error = NULL;

    unit2kkj2(from_u, &from);
    unit2kkj2(to_u, &to);

    MACRO_PATH_INIT(path);
    download_route(MAP_REITTIOPAS(router), &path, &from, &to, &error);
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
    for (i = 0; i < RO_TRANSPORT_LAST; i++)
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

        for (i = 0; i < RO_TRANSPORT_LAST; i++)
            self->transport_allowed[i] = FALSE;
        selected_rows = hildon_touch_selector_get_selected_rows(transport, 0);
        for (list = selected_rows; list != NULL; list = list->next)
        {
            GtkTreePath *path = list->data;
            gint *p_i;

            p_i = gtk_tree_path_get_indices(path);
            if (*p_i < RO_TRANSPORT_LAST)
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
    iface->run_options_dialog = map_reittiopas_run_options_dialog;
    iface->calculate_route = map_reittiopas_calculate_route;
    iface->geocode = map_reittiopas_geocode;
}

static void
map_reittiopas_init(MapReittiopas *self)
{
    gint i;

    for (i = 0; i < RO_TRANSPORT_LAST; i++)
        self->transport_allowed[i] = TRUE;

    self->walkspeed = RO_WALKSPEED_NORMAL;
    self->margin = 3;
}

static void
map_reittiopas_class_init(MapReittiopasClass *klass)
{
}

