/*
 * Copyright (C) 2006, 2007 John Costigan.
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "types.h"

#include <hildon/hildon-note.h>
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-sound.h>

#include "types.h"
#include "data.h"
#include "defines.h"

#include "dbus-ifc.h"
#include "display.h"
#include "path.h"
#include "poi.h"
#include "screen.h"
#include "settings.h"
#include "util.h"

#define VELVEC_SIZE_FACTOR (4)

static GtkWidget *_sat_details_panel = NULL;
static GtkWidget *_sdi_lat = NULL;
static GtkWidget *_sdi_lon = NULL;
static GtkWidget *_sdi_spd = NULL;
static GtkWidget *_sdi_alt = NULL;
static GtkWidget *_sdi_hea = NULL;
static GtkWidget *_sdi_tim = NULL;
static GtkWidget *_sdi_vie = NULL;
static GtkWidget *_sdi_use = NULL;
static GtkWidget *_sdi_fix = NULL;
static GtkWidget *_sdi_fqu = NULL;
static GtkWidget *_sdi_msp = NULL;

static gint _dl_errors = 0;

static volatile gint _pending_replaces = 0;

/** Pango stuff. */
GdkRectangle _comprose_rect = { 0, 0, 0, 0};
PangoLayout *_scale_layout = NULL;
PangoLayout *_zoom_layout = NULL;
PangoLayout *_comprose_layout = NULL;
GdkGC *_speed_limit_gc1 = NULL;
GdkGC *_speed_limit_gc2 = NULL;
PangoLayout *_speed_limit_layout = NULL;
PangoLayout *_sat_panel_layout = NULL;
PangoLayout *_heading_panel_layout = NULL;
PangoFontDescription *_heading_panel_fontdesc = NULL;
GdkGC *_sat_info_gc1 = NULL;
GdkGC *_sat_info_gc2 = NULL;
PangoLayout *_sat_info_layout = NULL;
PangoLayout *_sat_details_layout = NULL;
PangoLayout *_sat_details_expose_layout = NULL;

gboolean
gps_display_details(void)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gchar *buffer, litbuf[LL_FMT_LEN], buffer2[LL_FMT_LEN];

    if(gps->fix < 2)
    {
        /* no fix no fun */
        gtk_label_set_label(GTK_LABEL(_sdi_lat), " --- ");
        gtk_label_set_label(GTK_LABEL(_sdi_lon), " --- ");
        gtk_label_set_label(GTK_LABEL(_sdi_spd), " --- ");
        gtk_label_set_label(GTK_LABEL(_sdi_alt), " --- ");
        gtk_label_set_label(GTK_LABEL(_sdi_hea), " --- ");
        gtk_label_set_label(GTK_LABEL(_sdi_tim), " --:--:-- ");
    }
    else
    {
        gfloat speed = gps->speed * UNITS_CONVERT[_units];

        format_lat_lon(gps->lat, gps->lon, litbuf, buffer2);
        
        /* latitude */
        gtk_label_set_label(GTK_LABEL(_sdi_lat), litbuf);

        /* longitude */
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        	gtk_label_set_label(GTK_LABEL(_sdi_lon), buffer2);

        /* speed */
        switch(_units)
        {
            case UNITS_MI:
                buffer = g_strdup_printf("%.1f mph", speed);
                break;
            case UNITS_NM:
                buffer = g_strdup_printf("%.1f kn", speed);
                break;
            default:
                buffer = g_strdup_printf("%.1f km/h", speed);
                break;
        }
        gtk_label_set_label(GTK_LABEL(_sdi_spd), buffer);
        g_free(buffer);

        /* altitude */
        switch(_units)
        {
            case UNITS_MI:
            case UNITS_NM:
                buffer = g_strdup_printf("%d ft",
                        (gint)(_pos.altitude * 3.2808399f));
                break;
            default:
                buffer = g_strdup_printf("%d m", _pos.altitude);
                break;
        }
        gtk_label_set_label(GTK_LABEL(_sdi_alt), buffer);
        g_free(buffer);

        /* heading */
        buffer = g_strdup_printf("%0.0f°", gps->heading);
        gtk_label_set_label(GTK_LABEL(_sdi_hea), buffer);
        g_free(buffer);

        /* local time */
        strftime(litbuf, 15, "%X", localtime(&_pos.time));
        gtk_label_set_label(GTK_LABEL(_sdi_tim), litbuf);
    }

    /* Sat in view */
    buffer = g_strdup_printf("%d", gps->satinview);
    gtk_label_set_label(GTK_LABEL(_sdi_vie), buffer);
    g_free(buffer);

    /* Sat in use */
    buffer = g_strdup_printf("%d", gps->satinuse);
    gtk_label_set_label(GTK_LABEL(_sdi_use), buffer);
    g_free(buffer);

    /* fix */
    switch(gps->fix)
    {
        case 2:
        case 3: buffer = g_strdup_printf("%dD fix", gps->fix); break;
        default: buffer = g_strdup_printf("nofix"); break;
    }
    gtk_label_set_label(GTK_LABEL(_sdi_fix), buffer);
    g_free(buffer);

    if(gps->fix == 1)
        buffer = g_strdup("none");
    else
    {
        switch (gps->fixquality)
        {
            case 1 : buffer = g_strdup_printf(_("SPS")); break;
            case 2 : buffer = g_strdup_printf(_("DGPS")); break;
            case 3 : buffer = g_strdup_printf(_("PPS")); break;
            case 4 : buffer = g_strdup_printf(_("Real Time Kinematic")); break;
            case 5 : buffer = g_strdup_printf(_("Float RTK")); break;
            case 6 : buffer = g_strdup_printf(_("Estimated")); break;
            case 7 : buffer = g_strdup_printf(_("Manual")); break;
            case 8 : buffer = g_strdup_printf(_("Simulation")); break;
            default : buffer = g_strdup_printf(_("none")); break;
        }
    }
    gtk_label_set_label(GTK_LABEL(_sdi_fqu), buffer);
    g_free(buffer);

    /* max speed */
    {
        gfloat maxspeed = gps->maxspeed * UNITS_CONVERT[_units];

        /* speed */
        switch(_units)
        {
            case UNITS_MI:
                buffer = g_strdup_printf("%.1f mph", maxspeed);
                break;
            case UNITS_NM:
                buffer = g_strdup_printf("%.1f kn", maxspeed);
                break;
            default:
                buffer = g_strdup_printf("%.1f km/h", maxspeed);
                break;
        }
        gtk_label_set_label(GTK_LABEL(_sdi_msp), buffer);
        g_free(buffer);
    }

    /* refresh sat panel */
    gtk_widget_queue_draw_area(GTK_WIDGET(_sat_details_panel),
        0, 0,
        _sat_details_panel->allocation.width,
        _sat_details_panel->allocation.height);

    return TRUE;
}

void
gps_display_data(void)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gchar *buffer, litbuf[LL_FMT_LEN], buffer2[LL_FMT_LEN];

    if(gps->fix < 2)
    {
        /* no fix no fun */
        gtk_label_set_label(GTK_LABEL(_text_lat), " --- ");
        gtk_label_set_label(GTK_LABEL(_text_lon), " --- ");
        gtk_label_set_label(GTK_LABEL(_text_speed), " --- ");
        gtk_label_set_label(GTK_LABEL(_text_alt), " --- ");
        gtk_label_set_label(GTK_LABEL(_text_time), " --:--:-- ");
    }
    else
    {
        gfloat speed = gps->speed * UNITS_CONVERT[_units];

        format_lat_lon(gps->lat, gps->lon, litbuf, buffer2);
        
        /* latitude */
        //lat_format(gps->lat, litbuf);
        gtk_label_set_label(GTK_LABEL(_text_lat), litbuf);

        /* longitude */
        //lon_format(gps->lon, litbuf);
        if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
        	gtk_label_set_label(GTK_LABEL(_text_lon), buffer2);	

        /* speed */
        switch(_units)
        {
            case UNITS_MI:
                buffer = g_strdup_printf("Spd: %.1f mph", speed);
                break;
            case UNITS_NM:
                buffer = g_strdup_printf("Spd: %.1f kn", speed);
                break;
            default:
                buffer = g_strdup_printf("Spd: %.1f km/h", speed);
                break;
        }
        gtk_label_set_label(GTK_LABEL(_text_speed), buffer);
        g_free(buffer);

        /* altitude */
        switch(_units)
        {
            case UNITS_MI:
            case UNITS_NM:
                buffer = g_strdup_printf("Alt: %d ft",
                        (gint)(_pos.altitude * 3.2808399f));
                break;
            default:
                buffer = g_strdup_printf("Alt: %d m", _pos.altitude);
        }
        gtk_label_set_label(GTK_LABEL(_text_alt), buffer);
        g_free(buffer);

        /* local time */
        strftime(litbuf, 15, "%X", localtime(&_pos.time));
        gtk_label_set_label(GTK_LABEL(_text_time), litbuf);
    }

    /* refresh sat panel */
    gtk_widget_queue_draw_area(GTK_WIDGET(_sat_panel),
        0, 0,
        _sat_panel->allocation.width,
        _sat_panel->allocation.height);

    /* refresh heading panel*/
    gtk_widget_queue_draw_area(GTK_WIDGET(_heading_panel),
        0, 0,
        _heading_panel->allocation.width,
        _heading_panel->allocation.height);

    return;
}

void
gps_show_info(void)
{
    if(_gps_info && _enable_gps)
        gtk_widget_show_all(GTK_WIDGET(_gps_widget));
    else
        gtk_widget_hide(GTK_WIDGET(_gps_widget));
}

static void
draw_sat_info(GtkWidget *widget, gint x0, gint y0,
        gint width, gint height, gboolean showsnr)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    GdkGC *gc;
    gint step, i, j, snr_height, bymargin, xoffset, yoffset;
    gint x, y, x1, y1;
    gchar *tmp = NULL;

    xoffset = x0;
    yoffset = y0;
    /* Bootom margin - 12% */
    bymargin = height * 0.88f;

    /* Bottom line */
    gdk_draw_line(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + 5, yoffset + bymargin,
        xoffset + width - 10 - 2, yoffset + bymargin);
    gdk_draw_line(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + 5, yoffset + bymargin - 1,
        xoffset + width - 10 - 2, yoffset + bymargin - 1);

    if(gps->satinview > 0 )
    {
        /* Left margin - 5pix, Right margin - 5pix */
        step = (width - 10) / gps->satinview;

        for(i = 0; i < gps->satinview; i++)
        {
            /* Sat used or not */
            gc = _sat_info_gc1;
            for(j = 0; j < gps->satinuse ; j++)
            {
                if(gps->satforfix[j] == _gps_sat[i].prn)
                {
                    gc = _sat_info_gc2;
                    break;
                }
            }

            x = 5 + i * step;
            snr_height = _gps_sat[i].snr * height * 0.78f / 100;
            y = height * 0.1f + (height * 0.78f - snr_height);

            /* draw sat rectangle... */
            gdk_draw_rectangle(widget->window,
                    gc,
                    TRUE,
                    xoffset + x,
                    yoffset + y,
                    step - 2,
                    snr_height);

            if(showsnr && _gps_sat[i].snr > 0)
            {
                /* ...snr.. */
                tmp = g_strdup_printf("%02d", _gps_sat[i].snr);
                pango_layout_set_text(_sat_info_layout, tmp, 2);
                pango_layout_get_pixel_size(_sat_info_layout, &x1, &y1);
                gdk_draw_layout(widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                    xoffset + x + ((step - 2) - x1)/2,
                    yoffset + y - 15,
                    _sat_info_layout);
                g_free(tmp);
            }

            /* ...and sat number */
            tmp = g_strdup_printf("%02d", _gps_sat[i].prn);
            pango_layout_set_text(_sat_info_layout, tmp, 2);
            pango_layout_get_pixel_size(_sat_info_layout, &x1, &y1);
            gdk_draw_layout(widget->window,
                widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                xoffset + x + ((step - 2) - x1)/2 ,
                yoffset + bymargin + 1,
                _sat_info_layout);
            g_free(tmp);
        }
    }

    return;
}

static void
draw_sat_details(GtkWidget *widget, gint x0, gint y0,
        gint width, gint height)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gint i, j, x, y, size, halfsize, xoffset, yoffset;
    gint x1, y1;
    gfloat tmp;
    GdkColor color;
    GdkGC *gc1, *gc2, *gc3, *gc;
    gchar *buffer = NULL;

    size = MIN(width, height);
    halfsize = size/2;
    if(width > height)
    {
        xoffset = x0 + (width - height - 10) / 2;
        yoffset = y0 + 5;
    }
    else
    {
        xoffset = x0 + 5;
        yoffset = y0 + (height - width - 10) / 2;
    }

    /* 90 */
    gdk_draw_arc(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            FALSE,
            xoffset + 2, yoffset + 2, size - 4, size - 4,
            0, 64 * 360);

    /* 60 */
    gdk_draw_arc(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            FALSE,
            xoffset + size/6, yoffset + size/6,
            size/6*4, size/6*4,
            0, 64 * 360);

    /* 30 */
    gdk_draw_arc(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            FALSE,
            xoffset + size/6*2, yoffset + size/6*2,
            size/6*2, size/6*2,
            0, 64 * 360);

    gint line[12] = {0,30,60,90,120,150,180,210,240,270,300,330};

    for(i = 0; i < 6; i++)
    {
        /* line */
        tmp = deg2rad(line[i]);
        gdk_draw_line(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            xoffset + halfsize + (halfsize -2) * sinf(tmp),
            yoffset + halfsize - (halfsize -2) * cosf(tmp),
            xoffset + halfsize - (halfsize -2) * sinf(tmp),
            yoffset + halfsize + (halfsize -2) * cosf(tmp));
    }

    for(i = 0; i < 12; i++)
    {
        tmp = deg2rad(line[i]);
        /* azimuth */
        if(line[i] == 0)
            buffer = g_strdup_printf("N");
        else
            buffer = g_strdup_printf("%d°", line[i]);
        pango_layout_set_text(_sat_details_layout, buffer, -1);
        pango_layout_get_pixel_size(_sat_details_layout, &x, &y);
        gdk_draw_layout(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            (xoffset + halfsize + (halfsize - size/12) * sinf(tmp)) - x/2,
            (yoffset + halfsize - (halfsize - size/12) * cosf(tmp)) - y/2,
            _sat_details_layout);
        g_free(buffer);
    }

    /* elevation 30 */
    tmp = deg2rad(30);
    buffer = g_strdup_printf("30°");
    pango_layout_set_text(_sat_details_layout, buffer, -1);
    pango_layout_get_pixel_size(_sat_details_layout, &x, &y);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        (xoffset + halfsize + size/6*2 * sinf(tmp)) - x/2,
        (yoffset + halfsize - size/6*2 * cosf(tmp)) - y/2,
        _sat_details_layout);
    g_free(buffer);

    /* elevation 60 */
    tmp = deg2rad(30);
    buffer = g_strdup_printf("60°");
    pango_layout_set_text(_sat_details_layout, buffer, -1);
    pango_layout_get_pixel_size(_sat_details_layout, &x, &y);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        (xoffset + halfsize + size/6 * sinf(tmp)) - x/2,
        (yoffset + halfsize - size/6 * cosf(tmp)) - y/2,
        _sat_details_layout);
    g_free(buffer);

    color.red = 0xffff;
    color.green = 0;
    color.blue = 0;
    gc1 = gdk_gc_new (widget->window);
    gdk_gc_set_rgb_fg_color (gc1, &color);

    color.red = 0;
    color.green = 0;
    color.blue = 0xffff;
    gc2 = gdk_gc_new (widget->window);
    gdk_gc_set_rgb_fg_color (gc2, &color);

    color.red = 0xffff;
    color.green = 0xffff;
    color.blue = 0xffff;
    gc3 = gdk_gc_new (widget->window);
    gdk_gc_set_rgb_fg_color (gc3, &color);

    for(i = 0; i < gps->satinview; i++)
    {
        /* Sat used or not */
        gc = gc1;
        for(j = 0; j < gps->satinuse ; j++)
        {
            if(gps->satforfix[j] == _gps_sat[i].prn)
            {
                gc = gc2;
                break;
            }
        }

        tmp = deg2rad(_gps_sat[i].azimuth);
        x = xoffset + halfsize
            + (90 - _gps_sat[i].elevation)*halfsize/90 * sinf(tmp);
        y = yoffset + halfsize
            - (90 - _gps_sat[i].elevation)*halfsize/90 * cosf(tmp);

        gdk_draw_arc (widget->window,
            gc, TRUE,
            x - 10, y - 10, 20, 20,
            0, 64 * 360);

        buffer = g_strdup_printf("%02d", _gps_sat[i].prn);
        pango_layout_set_text(_sat_details_layout, buffer, -1);
        pango_layout_get_pixel_size(_sat_details_layout, &x1, &y1);
        gdk_draw_layout(widget->window,
            gc3,
            x - x1/2,
            y - y1/2,
            _sat_details_layout);
        g_free(buffer);
    }
    g_object_unref (gc1);
    g_object_unref (gc2);
    g_object_unref (gc3);

    return;
}


static gboolean
sat_details_panel_expose(GtkWidget *widget, GdkEventExpose *event)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gint width, height, x, y;
    gchar *buffer = NULL;

    width = widget->allocation.width;
    height = widget->allocation.height * 0.9;

    draw_sat_info(widget, 0, 0, width/2, height, TRUE);
    draw_sat_details(widget, width/2, 0, width/2, height);

    buffer = g_strdup_printf(
        "%s: %d; %s: %d",
        _("Satellites in view"), gps->satinview,
        _("in use"), gps->satinuse);
    pango_layout_set_text(_sat_details_expose_layout, buffer, -1);
    pango_layout_get_pixel_size(_sat_details_expose_layout, &x, &y);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        10,
        height*0.9 + 10,
        _sat_details_expose_layout);
    g_free(buffer);

    buffer = g_strdup_printf("HDOP: %d", gps->hdop);
    pango_layout_set_text(_sat_details_expose_layout, buffer, -1);
    pango_layout_get_pixel_size(_sat_details_expose_layout, &x, &y);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        (width/8) - x/2,
        (height/6) - y/2,
        _sat_details_expose_layout);
    g_free(buffer);
    buffer = g_strdup_printf("VDOP: %.01f", gps->vdop);
    pango_layout_set_text(_sat_details_expose_layout, buffer, -1);
    pango_layout_get_pixel_size(_sat_details_expose_layout, &x, &y);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        (width/8) - x/2,
        (height/6) - y/2 + 20,
        _sat_details_expose_layout);
    g_free(buffer);

    return TRUE;
}

void
gps_details(void)
{
    static GtkWidget *dialog = NULL;
    static GtkWidget *table = NULL;
    static GtkWidget *label = NULL;
    static GtkWidget *notebook = NULL;

    if(dialog == NULL)
    {
        dialog = gtk_dialog_new_with_buttons(_("GPS Details"),
                GTK_WINDOW(_window), GTK_DIALOG_MODAL,
                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                NULL);

        gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 300);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                notebook = gtk_notebook_new(), TRUE, TRUE, 0);

        /* textual info */
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                table = gtk_table_new(4, 6, FALSE),
                label = gtk_label_new(_("GPS Information")));

        _sat_details_panel = gtk_drawing_area_new ();
        gtk_widget_set_size_request (_sat_details_panel, 300, 300);
        /* sat details info */
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                _sat_details_panel,
                label = gtk_label_new(_("Satellites details")));
        g_signal_connect (G_OBJECT (_sat_details_panel), "expose_event",
                            G_CALLBACK (sat_details_panel_expose), NULL);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Latitude")),
                0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_lat = gtk_label_new(" --- "),
                1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_lat), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Longitude")),
                0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_lon = gtk_label_new(" --- "),
                1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_lon), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Speed")),
                0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_spd = gtk_label_new(" --- "),
                1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_spd), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Altitude")),
                0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_alt = gtk_label_new(" --- "),
                1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_alt), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Heading")),
                0, 1, 4, 5, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_hea = gtk_label_new(" --- "),
                1, 2, 4, 5, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_hea), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Local time")),
                0, 1, 5, 6, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_tim = gtk_label_new(" --:--:-- "),
                1, 2, 5, 6, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_tim), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Sat in view")),
                2, 3, 0, 1, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_vie = gtk_label_new("0"),
                3, 4, 0, 1, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_vie), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Sat in use")),
                2, 3, 1, 2, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_use = gtk_label_new("0"),
                3, 4, 1, 2, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_use), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Fix")),
                2, 3, 2, 3, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_fix = gtk_label_new(_("nofix")),
                3, 4, 2, 3, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_fix), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Fix Quality")),
                2, 3, 3, 4, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_fqu = gtk_label_new(_("none")),
                3, 4, 3, 4, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_fqu), 0.f, 0.5f);

        gtk_table_attach(GTK_TABLE(table),
                label = gtk_label_new(_("Max speed")),
                2, 3, 5, 6, GTK_EXPAND | GTK_FILL, 0, 20, 4);
        gtk_misc_set_alignment(GTK_MISC(label), 1.f, 0.5f);
        gtk_table_attach(GTK_TABLE(table),
                _sdi_msp = gtk_label_new(" --- "),
                3, 4, 5, 6, GTK_EXPAND | GTK_FILL, 0, 2, 4);
        gtk_misc_set_alignment(GTK_MISC(_sdi_msp), 0.f, 0.5f);
    }

    gtk_widget_show_all(dialog);
    _satdetails_on = TRUE;
    gps_display_details();
    while(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        _satdetails_on = FALSE;
        break;
    }
    gtk_widget_hide(dialog);
}

/**
 * Force a redraw of the entire _map_pixmap, including fetching the
 * background maps from disk and redrawing the tracks on top of them.
 */
void
map_force_redraw()
{
}

MapPoint
map_calc_new_center(gint zoom)
{
    MapController *controller;
    MapPoint new_center;

    /* TODO: remove this function */
    controller = map_controller_get_instance();
    map_controller_calc_best_center(controller, &new_center);
    return new_center;
}

/**
 * Center the view on the given unitx/unity.
 */
void
map_center_unit_full(MapPoint new_center,
        gint zoom, gint rotate_angle)
{
}

void
map_rotate(gint rotate_angle)
{
    MapController *controller = map_controller_get_instance();

    map_controller_rotate(controller, rotate_angle);
}

/**
 * Initiate a move of the mark from the old location to the current
 * location.  This function queues the draw area of the old mark (to force
 * drawing of the background map), then updates the mark, then queus the
 * draw area of the new mark.
 */
void
map_move_mark()
{
    map_screen_update_mark(MAP_SCREEN(_w_map));
}

/**
 * Make sure the mark is up-to-date.  This function triggers a panning of
 * the view if the mark is appropriately close to the edge of the view.
 */
void
map_refresh_mark(gboolean force_redraw)
{
    /* TODO: remove this function, when the related menu items are removed. */
}

void
map_download_refresh_idle(MapTileSpec *tile, GdkPixbuf *pixbuf,
                          const GError *error, gpointer user_data)
{
    MapUpdateType update_type;

    printf("%s(%p, %d, %d, %d)", G_STRFUNC, tile,
            tile->zoom, tile->tilex, tile->tiley);

    update_type = GPOINTER_TO_INT(user_data);

    if (error != NULL)
    {
        _dl_errors++;
    }

    if(++_curr_download == _num_downloads)
    {
        _num_downloads = _curr_download = 0;

        if(_dl_errors)
        {
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%d %s", _dl_errors,
                     _("maps failed to download."));
            MACRO_BANNER_SHOW_INFO(_window, buffer);
            _dl_errors = 0;
        }
    }
    else if(_download_banner)
    {
        hildon_banner_set_fraction(HILDON_BANNER(_download_banner),
                _curr_download / (double)_num_downloads);
    }
}

gboolean
sat_panel_expose(GtkWidget *widget, GdkEventExpose *event)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gchar *tmp = NULL;
    gint x, y;

    g_return_val_if_fail(gps != NULL, FALSE);

    draw_sat_info(widget,
        0, 0,
        widget->allocation.width,
        widget->allocation.height,
        FALSE);

    /* Sat View/In Use */
    tmp = g_strdup_printf("%d/%d %dm", gps->satinuse, gps->satinview,
                          gps->hdop);
    pango_layout_set_text(_sat_panel_layout, tmp, -1);
    pango_layout_set_alignment(_sat_panel_layout, PANGO_ALIGN_LEFT);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        20, 2,
        _sat_panel_layout);
    g_free(tmp);

    switch(gps->fix)
    {
        case 2:
        case 3: tmp = g_strdup_printf("%dD fix", gps->fix); break;
        default: tmp = g_strdup_printf("nofix"); break;
    }
    pango_layout_set_text(_sat_panel_layout, tmp, -1);
    pango_layout_set_alignment(_sat_panel_layout, PANGO_ALIGN_RIGHT);
    pango_layout_get_pixel_size(_sat_panel_layout, &x, &y);
    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        widget->allocation.width - 20 - x, 2,
        _sat_panel_layout);
    g_free(tmp);

    return TRUE;
}

gboolean
heading_panel_expose(GtkWidget *widget, GdkEventExpose *event)
{
    MapController *controller = map_controller_get_instance();
    const MapGpsData *gps = map_controller_get_gps_data(controller);
    gint size, xoffset, yoffset, i, x, y;
    gint dir;
    gfloat tmp;
    gchar *text;

    size = MIN(widget->allocation.width, widget->allocation.height);
    if(widget->allocation.width > widget->allocation.height)
    {
        xoffset = (widget->allocation.width - widget->allocation.height) / 2;
        yoffset = 0;
    }
    else
    {
        xoffset = 0;
        yoffset = (widget->allocation.height - widget->allocation.width) / 2;
    }
    pango_font_description_set_size(_heading_panel_fontdesc,12*PANGO_SCALE);
    pango_layout_set_font_description(_heading_panel_layout,
            _heading_panel_fontdesc);
    pango_layout_set_alignment(_heading_panel_layout, PANGO_ALIGN_CENTER);

    text = g_strdup_printf("%3.0f°", gps->heading);
    pango_layout_set_text(_heading_panel_layout, text, -1);
    pango_layout_get_pixel_size(_heading_panel_layout, &x, &y);

    gdk_draw_layout(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + size/2 - x/2,
        yoffset + size - y - 2, _heading_panel_layout);
    g_free(text);

    gdk_draw_arc (widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        FALSE,
        xoffset, yoffset + size/2,
        size, size,
        0, 64 * 180);

    /* Simple arrow for heading*/
    gdk_draw_line(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + size/2 + 3,
        yoffset + size - y - 5,
        xoffset + size/2,
        yoffset + size/2 + 5);

    gdk_draw_line(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + size/2 - 3,
        yoffset + size - y - 5,
        xoffset + size/2,
        yoffset + size/2 + 5);

    gdk_draw_line(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + size/2 - 3,
        yoffset + size - y - 5,
        xoffset + size/2,
        yoffset + size - y - 8);

    gdk_draw_line(widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
        xoffset + size/2 + 3,
        yoffset + size - y - 5,
        xoffset + size/2,
        yoffset + size - y - 8);

    gint angle[5] = {-90,-45,0,45,90};
    gint fsize[5] = {0,4,10,4,0};
    for(i = 0; i < 5; i++)
    {
        dir = (gint)(gps->heading/45)*45 + angle[i];

        switch(dir)
        {
            case   0:
            case 360: text = g_strdup("N"); break;
            case  45:
            case 405:
                text = g_strdup("NE"); break;
            case  90:
                text = g_strdup("E"); break;
            case 135:
                text = g_strdup("SE"); break;
            case 180:
                text = g_strdup("S"); break;
            case 225:
                text = g_strdup("SW"); break;
            case 270:
            case -90:
                text = g_strdup("W"); break;
            case 315:
            case -45:
                text = g_strdup("NW"); break;
            default :
                text = g_strdup("??");
                break;
        }

        tmp = deg2rad(dir - gps->heading);
        gdk_draw_line(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            xoffset + size/2 + ((size/2 - 5) * sinf(tmp)),
            yoffset + size - ((size/2 - 5) * cosf(tmp)),
            xoffset + size/2 + ((size/2 + 5) * sinf(tmp)),
            yoffset + size - ((size/2 + 5) * cosf(tmp)));

        x = fsize[i];
        if(abs((gint)(gps->heading/45)*45 - gps->heading)
                > abs((gint)(gps->heading/45)*45 + 45 - gps->heading)
                && (i > 0))
            x = fsize[i - 1];

        pango_font_description_set_size(_heading_panel_fontdesc,
                (10 + x)*PANGO_SCALE);
        pango_layout_set_font_description(_heading_panel_layout,
                _heading_panel_fontdesc);
        pango_layout_set_text(_heading_panel_layout, text, -1);
        pango_layout_get_pixel_size(_heading_panel_layout, &x, &y);
        x = xoffset + size/2 + ((size/2 + 15) * sinf(tmp)) - x/2,
        y = yoffset + size - ((size/2 + 15) * cosf(tmp)) - y/2,

        gdk_draw_layout(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            x, y, _heading_panel_layout);
        g_free(text);
    }

    return TRUE;
}

static void
latlon_cb_copy_clicked(GtkWidget *widget, LatlonDialog *lld) {
    gchar buffer[42];

    snprintf(buffer, sizeof(buffer),
            "%s %s",
            gtk_label_get_text(GTK_LABEL(lld->lat)),
            gtk_label_get_text(GTK_LABEL(lld->lon)));

    gtk_clipboard_set_text(
            gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), buffer, -1);
}

static void
latlon_cb_fmt_changed(HildonPickerButton *button, LatlonDialog *lld) {
  DegFormat fmt;

  fmt = hildon_picker_button_get_active(button);

  {
    gint old = _degformat; /* augh... */
    gchar buffer[LL_FMT_LEN];
    gchar buffer2[LL_FMT_LEN];

    _degformat = fmt;
    
    format_lat_lon(lld->glat, lld->glon, buffer, buffer2);
    
    //lat_format(lld->glat, buffer);
    gtk_label_set_label(GTK_LABEL(lld->lat), buffer);
    //lon_format(lld->glon, buffer);
    
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	gtk_label_set_label(GTK_LABEL(lld->lon), buffer2);
    else
    	gtk_label_set_label(GTK_LABEL(lld->lon), g_strdup(""));
    
    
    // And set the titles
    gtk_label_set_label(GTK_LABEL(lld->lat_title), 
    		DEG_FORMAT_ENUM_TEXT[_degformat].short_field_1 );
    
    if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	gtk_label_set_label(GTK_LABEL(lld->lon_title), 
        		DEG_FORMAT_ENUM_TEXT[_degformat].short_field_2 );
    else
    	gtk_label_set_label(GTK_LABEL(lld->lon_title), g_strdup(""));
    
    
    _degformat = old;
  }
}

gboolean
latlon_dialog(gdouble lat, gdouble lon)
{
    LatlonDialog lld;
    GtkWidget *dialog;
    GtkWidget *table;
    GtkBox *box;
    GtkWidget *lbl_lon_title;
    GtkWidget *lbl_lat_title;
    GtkWidget *txt_lat;
    GtkWidget *txt_lon;
    GtkWidget *format;
    HildonTouchSelector *selector;
    GtkWidget *btn_copy = NULL;
    gint prev_degformat = _degformat;
    gint fallback_deg_format = _degformat;
    gint i;
    
    // Check that the current coord system supports the select position
    if(!coord_system_check_lat_lon (lat, lon, &fallback_deg_format))
    {
    	_degformat = fallback_deg_format;
    }
     
    
    dialog = gtk_dialog_new_with_buttons(_("Show Position"),
            GTK_WINDOW(_window), GTK_DIALOG_MODAL,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
            NULL);

    /* Set the lat/lon strings. */
    box = GTK_BOX(GTK_DIALOG(dialog)->vbox);
    gtk_box_pack_start(box,
                       table = gtk_table_new(2, 2, TRUE), TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(table),
    		lbl_lat_title = gtk_label_new(/*_("Lat")*/ DEG_FORMAT_ENUM_TEXT[_degformat].short_field_1 ),
            0, 1, 0, 1, GTK_FILL, 0, HILDON_MARGIN_TRIPLE, 0);
    gtk_misc_set_alignment(GTK_MISC(lbl_lat_title), 1.f, 0.5f);
    gtk_table_attach(GTK_TABLE(table),
            txt_lat = gtk_label_new(""),
            1, 2, 0, 1, GTK_FILL|GTK_EXPAND, 0, 0, 0);
    gtk_misc_set_alignment(GTK_MISC(txt_lat), 0.0, 0.5f);

    
    gtk_table_attach(GTK_TABLE(table),
            lbl_lon_title = gtk_label_new(/*_("Lon")*/ DEG_FORMAT_ENUM_TEXT[_degformat].short_field_2 ),
            0, 1, 1, 2, GTK_FILL, 0, HILDON_MARGIN_TRIPLE, 0);
    gtk_misc_set_alignment(GTK_MISC(lbl_lon_title), 1.f, 0.5f);
    
    gtk_table_attach(GTK_TABLE(table),
            txt_lon = gtk_label_new(""),
            1, 2, 1, 2, GTK_FILL|GTK_EXPAND, 0, 0, 0);
    gtk_misc_set_alignment(GTK_MISC(txt_lon), 0.0, 0.5f);

    
    selector = HILDON_TOUCH_SELECTOR (hildon_touch_selector_new_text());
    for (i = 0; i < DEG_FORMAT_ENUM_COUNT; i++)
        hildon_touch_selector_append_text(selector,
                                          DEG_FORMAT_ENUM_TEXT[i].name);
    format =
        g_object_new(HILDON_TYPE_PICKER_BUTTON,
                     "arrangement", HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                     "size", HILDON_SIZE_FINGER_HEIGHT,
                     "title", _("Format"),
                     "touch-selector", selector,
                     "xalign", 0.0,
                     NULL);
    hildon_picker_button_set_active(HILDON_PICKER_BUTTON(format), _degformat);
    gtk_box_pack_start(box, format, FALSE, FALSE, 0);

    btn_copy = gtk_button_new_with_label(_("Copy"));
    hildon_gtk_widget_set_theme_size (btn_copy, HILDON_SIZE_FINGER_HEIGHT);
    gtk_box_pack_start(box, btn_copy, FALSE, FALSE, 0);

    /* Lat/Lon */
    {
      gchar buffer[LL_FMT_LEN];
      gchar buffer2[LL_FMT_LEN];

      format_lat_lon(lat, lon, buffer, buffer2);
      
      //lat_format(lat, buffer);
      gtk_label_set_label(GTK_LABEL(txt_lat), buffer);
      //lat_format(lon, buffer);
      if(DEG_FORMAT_ENUM_TEXT[_degformat].field_2_in_use)
    	  gtk_label_set_label(GTK_LABEL(txt_lon), buffer2);
      else
    	  gtk_label_set_label(GTK_LABEL(txt_lon), g_strdup(""));
    }

    /* setup cb context */
    lld.glat = lat;
    lld.glon = lon;
    lld.lat = txt_lat;
    lld.lon = txt_lon;

    lld.lat_title = lbl_lat_title;
    lld.lon_title = lbl_lon_title;
    
    /* Connect Signals */
    g_signal_connect(format, "value-changed",
                     G_CALLBACK(latlon_cb_fmt_changed), &lld);
    g_signal_connect(G_OBJECT(btn_copy), "clicked",
                    G_CALLBACK(latlon_cb_copy_clicked), &lld);

    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    
    _degformat = prev_degformat; // Put back incase it had to be auto changed
    
    gtk_widget_hide(dialog);

    return TRUE;
}


/**
 * This is a multi-purpose function for allowing the user to select a file
 * for either reading or writing.  If chooser_action is
 * GTK_FILE_CHOOSER_ACTION_OPEN, then bytes_out and size_out must be
 * non-NULL.  If chooser_action is GTK_FILE_CHOOSER_ACTION_SAVE, then
 * handle_out must be non-NULL.  Either dir or file (or both) can be NULL.
 * This function returns TRUE if a file was successfully opened.
 */
gboolean
display_open_file(GtkWindow *parent, gchar **bytes_out,
        GnomeVFSHandle **handle_out, gint *size_out,
        gchar **dir, gchar **file, GtkFileChooserAction chooser_action)
{
    GtkWidget *dialog;
    gboolean success = FALSE;

    dialog = hildon_file_chooser_dialog_new(parent, chooser_action);

    if(dir && *dir)
        gtk_file_chooser_set_current_folder_uri(
                GTK_FILE_CHOOSER(dialog), *dir);
    if(file && *file)
    {
        gtk_file_chooser_set_uri(
                GTK_FILE_CHOOSER(dialog), *file);
        if(chooser_action == GTK_FILE_CHOOSER_ACTION_SAVE)
        {
            /* Work around a bug in HildonFileChooserDialog. */
            gchar *basename = g_path_get_basename(*file);
            g_object_set(G_OBJECT(dialog), "autonaming", FALSE, NULL);
            gtk_file_chooser_set_current_name(
                    GTK_FILE_CHOOSER(dialog), basename);
            g_free(basename);
        }
    }

    gtk_widget_show_all(dialog);

    while(!success && gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_OK)
    {
        gchar *file_uri_str;
        GnomeVFSResult vfs_result;

        /* Get the selected filename. */
        file_uri_str = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));

        if((chooser_action == GTK_FILE_CHOOSER_ACTION_OPEN
                && (GNOME_VFS_OK != (vfs_result = gnome_vfs_read_entire_file(
                        file_uri_str, size_out, bytes_out))))
                || (chooser_action == GTK_FILE_CHOOSER_ACTION_SAVE
                    && GNOME_VFS_OK != (vfs_result = gnome_vfs_create(
                            handle_out, file_uri_str,
                            GNOME_VFS_OPEN_WRITE, FALSE, 0664))))
        {
            gchar buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer),
                    "%s:\n%s", chooser_action == GTK_FILE_CHOOSER_ACTION_OPEN
                                ? _("Failed to open file for reading")
                                : _("Failed to open file for writing"),
                    gnome_vfs_result_to_string(vfs_result));
            popup_error(dialog, buffer);
        }
        else
            success = TRUE;

        g_free(file_uri_str);
    }

    if(success)
    {
        /* Success!. */
        if(dir)
        {
            g_free(*dir);
            *dir = gtk_file_chooser_get_current_folder_uri(
                    GTK_FILE_CHOOSER(dialog));
        }

        /* If desired, save the file for later. */
        if(file)
        {
            g_free(*file);
            *file = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        }
    }

    gtk_widget_destroy(dialog);

    return success;
}

void
display_init()
{
    PangoContext *pango_context;
    PangoFontDescription *pango_font;
    GdkColor color;

    /* draw_sat_info() */
    _sat_info_gc1 = gdk_gc_new(_window->window);
    color.red = 0xffff;
    color.green = 0;
    color.blue = 0;
    gdk_gc_set_rgb_fg_color(_sat_info_gc1, &color);
    color.red = 0;
    color.green = 0;
    color.blue = 0xffff;
    _sat_info_gc2 = gdk_gc_new(_window->window);
    gdk_gc_set_rgb_fg_color(_sat_info_gc2, &color);
    pango_context = gtk_widget_get_pango_context(_window);
    _sat_info_layout = pango_layout_new(pango_context);
    pango_font = pango_font_description_new();
    pango_font_description_set_family(pango_font,"Sans Serif");
    pango_font_description_set_size(pango_font, 8*PANGO_SCALE);
    pango_layout_set_font_description(_sat_info_layout, pango_font);
    pango_layout_set_alignment(_sat_info_layout, PANGO_ALIGN_CENTER);

    /* sat_panel_expose() */
    pango_context = gtk_widget_get_pango_context(_window);
    _sat_panel_layout = pango_layout_new(pango_context);
    pango_font = pango_font_description_new();
    pango_font_description_set_family(pango_font,"Sans Serif");
    pango_font_description_set_size(pango_font, 14*PANGO_SCALE);
    pango_layout_set_font_description (_sat_panel_layout, pango_font);

    /* heading_panel_expose() */
    pango_context = gtk_widget_get_pango_context(_window);
    _heading_panel_layout = pango_layout_new(pango_context);
    _heading_panel_fontdesc =  pango_font_description_new();
    pango_font_description_set_family(_heading_panel_fontdesc, "Sans Serif");

    /* draw_sat_details() */
    pango_context = gtk_widget_get_pango_context(_window);
    _sat_details_layout = pango_layout_new(pango_context);
    pango_font = pango_font_description_new();
    pango_font_description_set_family(pango_font,"Sans Serif");
    pango_font_description_set_size(pango_font, 10*PANGO_SCALE);
    pango_layout_set_font_description(_sat_details_layout,
            pango_font);
    pango_layout_set_alignment(_sat_details_layout, PANGO_ALIGN_CENTER);

    /* sat_details_panel_expose() */
    pango_context = gtk_widget_get_pango_context(_window);
    _sat_details_expose_layout = pango_layout_new(pango_context);
    pango_font = pango_font_description_new();
    pango_font_description_set_family(
            pango_font,"Sans Serif");
    pango_layout_set_alignment(_sat_details_expose_layout,
            PANGO_ALIGN_CENTER);
    pango_font_description_set_size(pango_font,
            14*PANGO_SCALE);
    pango_layout_set_font_description(_sat_details_expose_layout,
            pango_font);

    /* Load the _redraw_wait_icon. */
    {
        GError *error = NULL;
        gchar *icon_path = "/usr/share/icons/hicolor/scalable/hildon"
                           "/maemo-mapper-wait.png";
        _redraw_wait_bounds.x = 0;
        _redraw_wait_bounds.y = 0;
        _redraw_wait_icon = gdk_pixbuf_new_from_file(icon_path, &error);
        if(!_redraw_wait_icon || error)
        {
            g_printerr("Error parsing pixbuf: %s\n",
                    error ? error->message : icon_path);
            _redraw_wait_bounds.width = 0;
            _redraw_wait_bounds.height = 0;
            _redraw_wait_icon = NULL;
        }
        else
        {
            _redraw_wait_bounds.width
                = gdk_pixbuf_get_width(_redraw_wait_icon);
            _redraw_wait_bounds.height
                = gdk_pixbuf_get_height(_redraw_wait_icon);
        }
    }

    g_signal_connect(G_OBJECT(_sat_panel), "expose_event",
            G_CALLBACK(sat_panel_expose), NULL);
    g_signal_connect(G_OBJECT(_heading_panel), "expose_event",
            G_CALLBACK(heading_panel_expose), NULL);
}
