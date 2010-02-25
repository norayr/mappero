#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#define _GNU_SOURCE

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <glib.h>

#include "controller.h"
#include "data.h"
#include "defines.h"
#include "display.h"
#include "menu.h"
#include "tile_source.h"
#include "settings.h"
#include "types.h"
#include "util.h"

#include <hildon/hildon-check-button.h>
#include <hildon/hildon-entry.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-touch-selector.h>

/* Tag names. Used in XML generator and parser. */
#define TS_ROOT  "TileSources"
#define TS_ENTRY "TileSource"


static gint xyz_get_url(TileSource *source, gchar *buffer, gint len,
                        gint zoom, gint tilex, gint tiley);
static gint xyz_inv_get_url(TileSource *source, gchar *buffer, gint len,
                            gint zoom, gint tilex, gint tiley);
static gint xyz_signed_get_url(TileSource *source, gchar *buffer, gint len,
                               gint zoom, gint tilex, gint tiley);
static gint quad_qrst_get_url(TileSource *source, gchar *buffer, gint len,
                              gint zoom, gint tilex, gint tiley);
static gint quad_zero_get_url(TileSource *source, gchar *buffer, gint len,
                              gint zoom, gint tilex, gint tiley);


/* Supported repository types table */
static const TileSourceType tile_source_types[] = {
    { "XYZ", xyz_get_url, },
    { "XYZ_SIGNED", xyz_signed_get_url, },
    { "XYZ_INV", xyz_inv_get_url, },
    { "QUAD_QRST", quad_qrst_get_url, },
    { "QUAD_ZERO", quad_zero_get_url, },
    { NULL, }
};

typedef struct {
    const gchar *name;
    const gchar *ext;
    TileFormat format;
} TileFormatMapEntry;


static const TileFormatMapEntry tile_format_map[] = {
    { "PNG",  "png", FORMAT_PNG, },
    { "JPEG", "jpg", FORMAT_JPG, },
    { NULL, }
};


/**
 * Given the xyz coordinates of our map coordinate system, write the qrst
 * quadtree coordinates to buffer.
 */
static void
map_convert_coords_to_quadtree_string(gint x, gint y, gint zoomlevel,
                                      gchar *buffer, const gchar initial,
                                      const gchar *const quadrant)
{
    gchar *ptr = buffer;
    gint n;

    if (initial)
        *ptr++ = initial;

    for(n = MAX_ZOOM - zoomlevel; n >= 0; n--)
    {
        gint xbit = (x >> n) & 1;
        gint ybit = (y >> n) & 1;
        *ptr++ = quadrant[xbit + 2 * ybit];
    }
    *ptr++ = '\0';
}


static gint
xyz_get_url(TileSource *source, gchar *buffer, gint len,
            gint zoom, gint tilex, gint tiley)
{
    return g_snprintf(buffer, len, source->url,
                      tilex, tiley,  zoom - (MAX_ZOOM - 16));
}

static gint
xyz_inv_get_url(TileSource *source, gchar *buffer, gint len,
                gint zoom, gint tilex, gint tiley)
{
    return g_snprintf(buffer, len, source->url,
                      MAX_ZOOM + 1 - zoom, tilex, tiley);
}

static gint
xyz_signed_get_url(TileSource *source, gchar *buffer, gint len,
                   gint zoom, gint tilex, gint tiley)
{
    return g_snprintf(buffer, len, source->url,
                      tilex,
                      (1 << (MAX_ZOOM - zoom)) - tiley - 1,
                      zoom - (MAX_ZOOM - 17));
}

static gint
quad_qrst_get_url(TileSource *source, gchar *buffer, gint len,
                  gint zoom, gint tilex, gint tiley)
{
    gchar location[MAX_ZOOM + 2];
    map_convert_coords_to_quadtree_string(tilex, tiley, zoom,
                                          location, 't', "qrts");
    return g_snprintf(buffer, len, source->url, location);
}

static gint
quad_zero_get_url(TileSource *source, gchar *buffer, gint len,
                  gint zoom, gint tilex, gint tiley)
{
    gchar location[MAX_ZOOM + 2];
    map_convert_coords_to_quadtree_string(tilex, tiley, zoom,
                                          location, '\0', "0123");
    return g_snprintf(buffer, len, source->url, location);
}



static void
append_url(xmlDocPtr doc, xmlNodePtr parent, gchar *url)
{
    xmlNodePtr n, nn;

    n = xmlNewNode(NULL, BAD_CAST "url");
    nn = xmlNewCDataBlock(doc, BAD_CAST url, strlen(url));
    xmlAddChild(n, nn);
    xmlAddChild(parent, n);
}


static void
fill_selector_with_repotypes(HildonTouchSelector *selector,
                             const TileSourceType *active)
{
    gint i = 0;
    const TileSourceType *p = tile_source_types;

    while (p->name) {
        hildon_touch_selector_append_text(selector, p->name);

        if (p == active)
            hildon_touch_selector_set_active(selector, 0, i);
        p++;
        i++;
    }
}


static void
fill_selector_with_tile_formats(HildonTouchSelector *selector,
                                TileFormat active)
{
    gint i = 0;
    const TileFormatMapEntry *p = tile_format_map;

    while (p->name) {
        hildon_touch_selector_append_text(selector, p->name);
        if (p->format == active)
            hildon_touch_selector_set_active(selector, 0, i);
        p++;
        i++;
    }
}


/*
 * Check that given tile source is belong to active repository.
 */
static gboolean
tile_source_is_active(TileSource *ts)
{
    MapController *controller = map_controller_get_instance();
    Repository *active = map_controller_get_repository(controller);
    gint i;

    g_return_val_if_fail(ts != NULL, FALSE);

    if (active->primary == ts)
        return TRUE;

    if (!ts->transparent)
        return FALSE;

    for (i = 0; i < active->layers->len; i++)
        if (g_ptr_array_index(active->layers, i) == ts)
            return TRUE;
    return FALSE;
}


const TileSourceType*
tile_source_type_find_by_name(const gchar *name)
{
    const TileSourceType *type;

    g_return_val_if_fail(name != NULL, NULL);
    for (type = tile_source_types; type->name != NULL; type++)
        if (strcasecmp(name, type->name) == 0)
            return type;

    return NULL;
}


/*
 * Fill selector with names of tile sources, filtered by transparency (only if
 * filter is TRUE).  If active is non-null, this entry will be activated.
 */
void
tile_source_fill_selector(HildonTouchSelector *selector, gboolean filter,
                          gboolean transparent, TileSource *active)
{
    GList *ts_list = map_controller_get_tile_sources_list(map_controller_get_instance());
    TileSource *ts;
    gint index = 0;

    while (ts_list) {
        ts = (TileSource*)ts_list->data;
        if (!filter || ts->transparent == transparent) {
            hildon_touch_selector_append_text(selector, ts->name);
            if (ts == active)
                hildon_touch_selector_set_active(selector, 0, index);
            index++;
        }
        ts_list = ts_list->next;
    }
}


gchar*
tile_source_list_to_xml(GList *tile_sources)
{
    xmlDocPtr doc;
    xmlNodePtr n, nn;
    xmlChar *xmlbuf;
    int buf_size;
    gchar *res, *p;
    TileSource *ts;

    doc = xmlNewDoc(BAD_CAST "1.0");
    n = xmlNewNode(NULL, BAD_CAST TS_ROOT);
    xmlDocSetRootElement(doc, n);
    
    do {
        ts =(TileSource*)tile_sources->data;
        nn = xmlNewChild(n, NULL, BAD_CAST TS_ENTRY, NULL);

        if (ts->name)
            xmlNewChild(nn, NULL, BAD_CAST "name", BAD_CAST ts->name);
        if (ts->url)
            append_url(doc, nn, ts->url);
        if (ts->id)
            xmlNewChild(nn, NULL, BAD_CAST "id", BAD_CAST ts->id);
        if (ts->cache_dir)
            xmlNewChild(nn, NULL, BAD_CAST "cache_dir", BAD_CAST ts->cache_dir);
        xmlNewChild(nn, NULL, BAD_CAST "transparent",
                    BAD_CAST (ts->transparent ? "1" : "0"));
        xmlNewChild(nn, NULL, BAD_CAST "format",
                    BAD_CAST (tile_source_format_name(ts->format)));
        if (ts->type)
            xmlNewChild(nn, NULL, BAD_CAST "type", BAD_CAST ts->type->name);

        p = g_strdup_printf("%d", ts->refresh);
        xmlNewChild(nn, NULL, BAD_CAST "refresh", BAD_CAST p);
        g_free(p);
    } while ((tile_sources = g_list_next(tile_sources)));

    buf_size = 0;
    xmlDocDumpFormatMemory(doc, &xmlbuf, &buf_size, 1);
    if (buf_size > 0)
        res = g_strdup((gchar*)xmlbuf);
    else
        res = NULL;

    xmlFree(xmlbuf);
    xmlFreeDoc(doc);

    return res;
}



static TileSource *
tree_to_tile_source(xmlDocPtr doc, xmlNodePtr ts_node)
{
    TileSource* ts;
    xmlNodePtr n;
    gchar *s, *ss;
    gint val;
    gboolean val_valid;

    ts = g_slice_new0(TileSource);

    for (n = ts_node->children; n; n = n->next) {
        if (!n->children)
            continue;
        s = (gchar*)xmlNodeListGetString(doc, n->children, 1);
        val_valid = sscanf(s, "%d", &val) > 0;
        ss = (gchar*)n->name;

        if (strcmp(ss, "name") == 0)
            ts->name = g_strdup(s);
        else if (strcmp(ss, "id") == 0)
            ts->id = g_strdup(s);
        else if (strcmp(ss, "cache_dir") == 0)
            ts->cache_dir = g_strdup(s);
        else if (strcmp(ss, "type") == 0)
            ts->type = tile_source_type_find_by_name(s);
        else if (strcmp(ss, "url") == 0)
            ts->url = g_strdup(s);
        else if (strcmp(ss, "transparent") == 0 && val_valid)
            ts->transparent = val != 0;
        else if (strcmp(ss, "refresh") == 0 && val_valid)
            ts->refresh = val;
        else if (strcmp(ss, "format") == 0)
            ts->format = tile_source_format_by_name(s);
        xmlFree(s);
    }

    if (!ts->cache_dir && ts->id)
        ts->cache_dir = g_strdup(ts->id);

    return ts;
}


/* Ask about tile source deletion and remove it from all repositories it
 * referenced */
static void
tile_sources_delete_handler(GtkWindow* parent, TileSource* ts)
{
    GtkWidget *dialog;
    gchar *msg;
    gint ret;

    msg = g_strdup_printf(_("Do you really want to delete tile source\n%s?"),
                          ts->name);

    dialog = hildon_note_new_confirmation(parent, msg);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (ret == GTK_RESPONSE_OK)
        map_controller_delete_tile_source(map_controller_get_instance(), ts);
}


static TileSource *
tile_source_selector_get_activated(HildonTouchSelector *selector)
{
    GtkTreePath *path;
    gint *indices;
    GList *ts_list;

    ts_list = map_controller_get_tile_sources_list( map_controller_get_instance());

    path = hildon_touch_selector_get_last_activated_row(selector, 0);
    if (!path)
        return NULL;

    indices = gtk_tree_path_get_indices(path);
    if (!indices)
        return NULL;

    return (TileSource*)g_list_nth_data(ts_list, *indices);
}


static void
refresh_tile_source_list_selector(HildonTouchSelector *selector)
{
    GtkListStore *list_store = GTK_LIST_STORE(hildon_touch_selector_get_model(selector, 0));

    gtk_list_store_clear(list_store);
    tile_source_fill_selector(selector, FALSE, FALSE, NULL);
}


static void
select_tile_source_callback(HildonTouchSelector *selector,
                            gint column, gpointer data)
{
    TileSource *ts;

    ts = tile_source_selector_get_activated(selector);
    if (!ts)
        return;

    if (tile_source_edit_dialog(NULL, ts))
        refresh_tile_source_list_selector(selector);
}


/* Parse XML data */
GList* tile_source_xml_to_list(const gchar *data)
{
    xmlDocPtr doc;
    xmlNodePtr n;
    GList *res = NULL;

    if (!data)
        return NULL;

    doc = xmlParseMemory(data, strlen(data));

    if (!doc)
        return NULL;

    n = xmlDocGetRootElement(doc);
    if (!n || strcmp((gchar*)n->name, TS_ROOT) != 0) {
        xmlFreeDoc(doc);
        return res;
    }

    for (n = n->children; n; n = n->next) {
        if (n->type == XML_ELEMENT_NODE &&
            strcmp((gchar*)n->name, TS_ENTRY) == 0)
        {
            TileSource *ts = tree_to_tile_source(doc, n);
            if (ts)
                res = g_list_append(res, ts);
        }
    }

    xmlFreeDoc(doc);
    return res;
}


/*
 * Free tile source structure.
 * Warning: It's responsibility of caller that there are
 * no references to this structure
 */
void
tile_source_free(TileSource *ts)
{
    if (ts->name)
        g_free(ts->name);
    if (ts->id)
        g_free(ts->id);
    if (ts->cache_dir)
        g_free(ts->cache_dir);
    if (ts->url)
        g_free(ts->url);
    g_slice_free(TileSource, ts);
}


/*
 * Compare tile sources using fields, which are not
 * supposed to be customized by user. Return TRUE if
 * they are equal.
 */
gboolean
tile_source_compare(TileSource *ts1, TileSource *ts2)
{
    if (strcmp(ts1->name, ts2->name) != 0)
        return FALSE;
    if (strcmp(ts1->url, ts2->url) != 0)
        return FALSE;
    if (ts1->type != ts2->type)
        return FALSE;
    if (ts1->format != ts2->format)
        return FALSE;
    return TRUE;
}


/* Dialog with list of tile sources */
void
tile_source_list_edit_dialog()
{
    GtkWidget *dialog;
    HildonTouchSelector *ts_selector;
    MapController *controller = map_controller_get_instance();
    TileSource *ts;
    gint response;
    enum {
        RESP_NEW,
    };

    dialog = gtk_dialog_new_with_buttons(_("Tiles and layers"),
                  GTK_WINDOW(_window), GTK_DIALOG_MODAL, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_NEW, RESP_NEW);
    ts_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       GTK_WIDGET(ts_selector), TRUE, TRUE, 0);
    gtk_widget_set_size_request(GTK_WIDGET(ts_selector), -1, 300);
    hildon_touch_selector_set_hildon_ui_mode(ts_selector, HILDON_UI_MODE_NORMAL);
    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(ts_selector), "changed", G_CALLBACK(select_tile_source_callback), NULL);
    refresh_tile_source_list_selector(ts_selector);

    while (1) {
        if ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_DELETE_EVENT)
            break;

        switch (response) {
        case RESP_NEW:
            ts = g_slice_new0(TileSource);
            ts->name = g_strdup("New tile source");
            ts->id = g_strdup("New tile source ID");
            ts->type = NULL;
            if (tile_source_edit_dialog(GTK_WINDOW(dialog), ts)) {
                map_controller_append_tile_source(controller, ts);
                refresh_tile_source_list_selector(ts_selector);
            }
            else
                tile_source_free(ts);
            break;
        }
    }

    gtk_widget_destroy(dialog);
}


/*
 * Show dialog to edit tile source settings. Returns TRUE with
 * when user pressed 'save', FALSE on cancel
 */
gboolean
tile_source_edit_dialog(GtkWindow *parent, TileSource *ts)
{
    GtkWidget *dialog, *label;
    GtkWidget *name_entry, *id_entry, *cache_entry, *url_entry;
    GtkTable *table;
    GtkWidget *pannable;
    HildonTouchSelector *type_selector;
    GtkWidget *type_button;
    GtkWidget *transparent_button;
    HildonTouchSelector *refresh_selector;
    GtkWidget *refresh_button;
    HildonTouchSelector *format_selector;
    GtkWidget *format_button;
    gint i, response;
    gchar buf[10];
    MapController *controller = map_controller_get_instance();
    gboolean res = FALSE;
    enum {
        RESP_SAVE,
        RESP_DELETE,
    };

    if (!ts)
        return FALSE;

    dialog = gtk_dialog_new_with_buttons(
                 _("Tile source"), parent, GTK_DIALOG_MODAL, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_DELETE, RESP_DELETE);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_SAVE, RESP_SAVE);

    table = GTK_TABLE(gtk_table_new(6, 4, TRUE));
    pannable = hildon_pannable_area_new();
    gtk_widget_set_size_request(pannable, -1, 300);
    hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable),
                                           GTK_WIDGET(table));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), pannable, TRUE, TRUE, 0);

    /* Name */
    label = gtk_label_new(_("Name"));
    gtk_table_attach(table, label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    name_entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_table_attach(table, name_entry, 1, 4, 0, 1, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* ID */
    label = gtk_label_new(_("UniqID"));
    gtk_table_attach(table, label, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    id_entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_table_attach(table, id_entry, 1, 4, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Cache dir */
    label = gtk_label_new(_("Cache dir"));
    gtk_table_attach(table, label, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    cache_entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_table_attach(table, cache_entry, 1, 4, 2, 3, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* URL */
    label = gtk_label_new(_("URL"));
    gtk_table_attach(table, label, 0, 1, 3, 4, GTK_FILL | GTK_EXPAND, 0, 2, 4);
    url_entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_table_attach(table, url_entry, 1, 4, 3, 4, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Repository type */
    type_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    fill_selector_with_repotypes(type_selector, ts->type);
    type_button = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                               "size", HILDON_SIZE_FINGER_HEIGHT,
                               "title", _("Type"),
                               "touch-selector", type_selector,
                               NULL);
    gtk_table_attach(table, type_button, 0, 2, 4, 5, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* transparent */
    transparent_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label(GTK_BUTTON(transparent_button), _("Layer"));
    gtk_table_attach(table, transparent_button, 2, 4, 4, 5, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* refresh */
    refresh_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    for (i = 0; i <= 30; i++) {
        g_snprintf(buf, sizeof(buf), "%d", i);
        hildon_touch_selector_append_text(refresh_selector, buf);
    }
    refresh_button = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                                  "size", HILDON_SIZE_FINGER_HEIGHT,
                                  "title", _("Refresh, min"),
                                  "touch-selector", refresh_selector,
                                  NULL);
    gtk_table_attach(table, refresh_button, 0, 2, 5, 6, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* format */
    format_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    fill_selector_with_tile_formats(format_selector, ts->format);
    format_button = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                                 "size", HILDON_SIZE_FINGER_HEIGHT,
                                 "title", _("Format"),
                                 "touch-selector", format_selector,
                                 NULL);
    gtk_table_attach(table, format_button, 2, 4, 5, 6, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Fill with data */
    hildon_entry_set_text(HILDON_ENTRY(name_entry), ts->name);
    hildon_entry_set_text(HILDON_ENTRY(id_entry), ts->id);
    hildon_entry_set_text(HILDON_ENTRY(cache_entry), ts->cache_dir);
    hildon_entry_set_text(HILDON_ENTRY(url_entry), ts->url);
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(transparent_button), ts->transparent);
    hildon_touch_selector_set_active(refresh_selector, 0, ts->refresh);

    gtk_widget_show_all(dialog);

    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) != GTK_RESPONSE_DELETE_EVENT) {
        if (response == RESP_DELETE) {
            if (!tile_source_is_active(ts))
                tile_sources_delete_handler(GTK_WINDOW(dialog), ts);
            else
                popup_error(dialog, _("Selected layer is active, so, cannot be deleted."));
        }
        else {
            TileSource *ts_ref;
            const gchar *str;

            /* Check dialog data */
            if (!gtk_entry_get_text_length(GTK_ENTRY(name_entry))) {
                popup_error(dialog, _("Name is required"));
                continue;
            }
            if (!gtk_entry_get_text_length(GTK_ENTRY(id_entry))) {
                popup_error(dialog, _("ID is required"));
                continue;
            }
            str = gtk_entry_get_text(GTK_ENTRY(id_entry));
            ts_ref = map_controller_lookup_tile_source(controller, str);
            if (ts_ref && ts_ref != ts) {
                popup_error(dialog, _("ID is not unique"));
                continue;
            }
            if (!gtk_entry_get_text_length(GTK_ENTRY(cache_entry))) {
                popup_error(dialog, _("Cache dir is required"));
                continue;
            }
            if (!gtk_entry_get_text_length(GTK_ENTRY(url_entry))) {
                popup_error(dialog, _("URL is required"));
                continue;
            }

            /* All seems valid, update tile source record */
            str = gtk_entry_get_text(GTK_ENTRY(name_entry));
            if (g_strcmp0 (ts->name, str) != 0) {
                g_free(ts->name);
                ts->name = g_strdup(str);
            }
            str = gtk_entry_get_text(GTK_ENTRY(id_entry));
            if (g_strcmp0 (ts->id, str) != 0) {
                g_free(ts->id);
                ts->id = g_strdup(str);
            }
            str = gtk_entry_get_text(GTK_ENTRY(cache_entry));
            if (g_strcmp0 (ts->cache_dir, str) != 0) {
                g_free(ts->cache_dir);
                ts->cache_dir = g_strdup(str);
            }
            str = gtk_entry_get_text(GTK_ENTRY(url_entry));
            if (g_strcmp0 (ts->url, str) != 0) {
                g_free(ts->url);
                ts->url = g_strdup(str);
            }

            ts->type = &tile_source_types[hildon_touch_selector_get_active(type_selector, 0)];
            ts->transparent = hildon_check_button_get_active(HILDON_CHECK_BUTTON(transparent_button));
            ts->refresh = hildon_touch_selector_get_active(refresh_selector, 0);
            ts->format = tile_format_map[hildon_touch_selector_get_active(format_selector, 0)].format;
        }

        res = TRUE;
        break;
    }

    gtk_widget_destroy(dialog);

    return res;
}


/*
 * Return tile format file extention.
 */
const gchar *
tile_source_format_extention(TileFormat format)
{
    const TileFormatMapEntry *p = tile_format_map;

    while (p->name) {
        if (p->format == format)
            return p->ext;
        p++;
    }

    return "";
}


const gchar *
tile_source_format_name(TileFormat format)
{
    const TileFormatMapEntry *p = tile_format_map;

    while (p->name) {
        if (p->format == format)
            return p->name;
        p++;
    }

    return "";
}


TileFormat
tile_source_format_by_name(const gchar *name)
{
    const TileFormatMapEntry *p = tile_format_map;

    while (p->name) {
        if (strcmp(name, p->name) == 0)
            return p->format;
        p++;
    }

    return FORMAT_PNG;
}
