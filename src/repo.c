#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <glib.h>

#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-touch-selector.h>
#include <hildon/hildon-note.h>


#include "data.h"
#include "types.h"
#include "defines.h"
#include "controller.h"
#include "repo.h"

/* Tag names. Used in XML generator and parser. */
#define TS_ROOT  "TileSources"
#define TS_ENTRY "TileSource"

#define REPO_ROOT		"Repositories"
#define REPO_ENTRY		"Repository"
#define REPO_LAYERS_GROUP	"Layers"
#define REPO_LAYER_ENTRY	"Layer"
#define REPO_PRIMARY_ENTRY	"Primary"


/* Map between repotype and it's string reporesentation */
const static
struct {
    const char* s;
    RepoType type;
} repotype_map[] = {
    { "NONE",		REPOTYPE_NONE },
    { "XYZ",		REPOTYPE_XYZ },
    { "XYZ_SIGNED",	REPOTYPE_XYZ_SIGNED },
    { "XYZ_INV",	REPOTYPE_XYZ_INV },
    { "QUAD_QRST",	REPOTYPE_QUAD_QRST },
    { "QUAD_ZERO",	REPOTYPE_QUAD_ZERO },
    { "WMS",		REPOTYPE_WMS },
    { NULL,		REPOTYPE_NONE }
};


static void
append_url(xmlDocPtr doc, xmlNodePtr parent, gchar *url)
{
    xmlNodePtr n, nn;

    n = xmlNewNode(NULL, BAD_CAST "url");
    nn = xmlNewCDataBlock(doc, url, strlen(url));
    xmlAddChild(n, nn);
    xmlAddChild(parent, n);
}

static void
append_int(xmlNodePtr parent, const gchar *name, int val)
{
    gchar *p;

    p = g_strdup_printf("%d", val);
    xmlNewChild(parent, NULL, BAD_CAST name, p);
    g_free(p);
}


static const gchar *
repotype_to_str(RepoType type)
{
    int i = 0;

    while (repotype_map[i].s) {
        if (type == repotype_map[i].type)
            return repotype_map[i].s;
        i++;
    }

    return repotype_map[i].s;
}


static RepoType
str_to_repotype(const char* s)
{
    int i = 0;

    while (repotype_map[i].s) {
        if (!strcmp(repotype_map[i].s, s))
            return repotype_map[i].type;
        i++;
    }

    return repotype_map[i].type;
}


gchar*
tile_sources_to_xml(GList *tile_sources)
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
            xmlNewChild(nn, NULL, BAD_CAST "name", ts->name);
        if (ts->url)
            append_url(doc, nn, ts->url);
        if (ts->id)
            xmlNewChild(nn, NULL, BAD_CAST "id", ts->id);
        if (ts->cache_dir)
            xmlNewChild(nn, NULL, BAD_CAST "cache_dir", ts->cache_dir);
        xmlNewChild(nn, NULL, BAD_CAST "visible", ts->visible ? "1" : "0");
        xmlNewChild(nn, NULL, BAD_CAST "transparent", ts->transparent ? "1" : "0");
        xmlNewChild(nn, NULL, BAD_CAST "type", repotype_to_str(ts->type));

        p = g_strdup_printf("%d", ts->refresh);
        xmlNewChild(nn, NULL, BAD_CAST "refresh", p);
        g_free(p);
    } while (tile_sources = g_list_next(tile_sources));

    buf_size = 0;
    xmlDocDumpFormatMemory(doc, &xmlbuf, &buf_size, 1);
    if (buf_size > 0)
        res = g_strdup(xmlbuf);
    else
        res = NULL;

    xmlFree(xmlbuf);
    xmlFreeDoc(doc);

    return res;
}


gchar*
repositories_to_xml(GList *repositories)
{
    xmlDocPtr doc;
    xmlNodePtr n, nn, nl, nll;
    Repository *repo;
    xmlChar *xmlbuf;
    int i;
    int buf_size;
    gchar *res;
    TileSource *ts;

    doc = xmlNewDoc(BAD_CAST "1.0");
    n = xmlNewNode(NULL, BAD_CAST REPO_ROOT);
    xmlDocSetRootElement(doc, n);

    while (repositories)
    {
        repo = (Repository*)repositories->data;
        nn = xmlNewChild(n, NULL, BAD_CAST REPO_ENTRY, NULL);
        
        if (repo->name)
            xmlNewChild(nn, NULL, BAD_CAST "name", repo->name);
        append_int(nn, "min_zoom", repo->min_zoom);
        append_int(nn, "max_zoom", repo->max_zoom);
        append_int(nn, "zoom_step", repo->zoom_step);

        nl = xmlNewNode(NULL, BAD_CAST REPO_LAYERS_GROUP);
        xmlAddChild(nn, nl);

        if (repo->primary && repo->primary->id)
            xmlNewChild(nl, NULL, BAD_CAST REPO_PRIMARY_ENTRY, repo->primary->id);

        if (repo->layers) {
            for (i = 0; i < repo->layers->len; i++) {
                ts = g_ptr_array_index (repo->layers, i);
                if (ts && ts->name)
                    nll = xmlNewChild(nl, NULL, BAD_CAST REPO_LAYER_ENTRY, ts->id ? ts->id : "");
            }
        }
        repositories = g_list_next(repositories);
    }

    buf_size = 0;
    xmlDocDumpFormatMemory(doc, &xmlbuf, &buf_size, 1);
    if (buf_size > 0)
        res = g_strdup(xmlbuf);
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
    xmlChar *s;
    gint val;
    gboolean val_valid;

    ts = g_slice_new0(TileSource);
    if (!ts)
        return NULL;

    for (n = ts_node->children; n; n = n->next) {
        if (!n->children)
            continue;
        s = xmlNodeListGetString(doc, n->children, 1);
        val_valid = sscanf(s, "%d", &val) > 0;

        if (!strcmp(n->name, "name"))
            ts->name = g_strdup(s);
        else if (!strcmp(n->name, "id"))
            ts->id = g_strdup(s);
        else if (!strcmp(n->name, "cache_dir"))
            ts->cache_dir = g_strdup(s);
        else if (!strcmp(n->name, "type"))
            ts->type = str_to_repotype(s);
        else if (!strcmp(n->name, "url"))
            ts->url = g_strdup(s);
        else if (!strcmp(n->name, "visible") && val_valid)
            ts->visible = val != 0;
        else if (!strcmp(n->name, "transparent") && val_valid)
            ts->transparent = val != 0;
        else if (!strcmp(n->name, "refresh") && val_valid)
            ts->refresh = val;
        xmlFree(s);
    }

    return ts;
}


static Repository *
tree_to_repository(xmlDocPtr doc, xmlNodePtr repo_node)
{
    Repository* repo;
    xmlNodePtr n, nn;
    xmlChar *s, *ss;
    gint val, count;
    gboolean val_valid;
    TileSource *ts;
    MapController *controller = map_controller_get_instance();

    repo = g_slice_new0(Repository);
    if (!repo)
        return NULL;

    for (n = repo_node->children; n; n = n->next) {
        if (!n->children)
            continue;
        s = xmlNodeListGetString(doc, n->children, 1);
        val_valid = sscanf(s, "%d", &val) > 0;

        if (!strcmp(n->name, "name"))
            repo->name = g_strdup(s);
        if (!strcmp(n->name, "min_zoom") && val_valid)
            repo->min_zoom = val;
        if (!strcmp(n->name, "max_zoom") && val_valid)
            repo->max_zoom = val;
        if (!strcmp(n->name, "zoom_step") && val_valid)
            repo->zoom_step = val;
        else if (!strcmp(n->name, REPO_LAYERS_GROUP)) {
            repo->layers = g_ptr_array_new();

            for (nn = n->children; nn; nn = nn->next) {
                ss = xmlNodeListGetString(doc, nn->children, 1);
                if (!strcmp(nn->name, REPO_LAYER_ENTRY)) {
                    ts = map_controller_lookup_tile_source(controller, ss);

                    if (ts)
                        g_ptr_array_add(repo->layers, ts);
                }
                else if (!strcmp(nn->name, REPO_PRIMARY_ENTRY)) {
                    ts = map_controller_lookup_tile_source(controller, ss);
                    if (ts)
                        repo->primary = ts;
                }
            }
        }
        xmlFree(s);
    }

    return repo;
}


/* Ask about repository deletion and removes it, also removing menu entry */
static void
repository_delete_handler(GtkWindow* parent, Repository* repo)
{
    GtkWidget *dialog;
    gchar *msg;
    gint ret;

    msg = g_strdup_printf(_("Do ou really want to delete repository\n%s?"), repo->name);

    dialog = hildon_note_new_confirmation(parent, msg);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (ret == GTK_RESPONSE_OK)
        map_controller_delete_repository(map_controller_get_instance(), repo);
}


/* Parse XML data */
GList* xml_to_tile_sources(const gchar *data)
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
    if (!n || strcmp(n->name, TS_ROOT)) {
        xmlFreeDoc(doc);
        return res;
    }

    for (n = n->children; n; n = n->next) {
        if (n->type == XML_ELEMENT_NODE && !strcmp(n->name, TS_ENTRY)) {
            TileSource *ts = tree_to_tile_source(doc, n);
            if (ts)
                res = g_list_append(res, ts);
        }
    }

    xmlFreeDoc(doc);
    return res;
}



/* Parse repositories XML */
GList* xml_to_repositories(const gchar *data)
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
    if(!n || strcmp(n->name, REPO_ROOT)) {
        xmlFreeDoc(doc);
        return res;
    }

    for (n = n->children; n; n = n->next) {
        if (n->type == XML_ELEMENT_NODE && !strcmp(n->name, REPO_ENTRY)) {
            Repository *repo = tree_to_repository(doc, n);
            if (repo)
                res = g_list_append(res, repo);
        }
    }

    xmlFreeDoc(doc);
    return res;
}


/*
 * Create default repositories
 */
Repository*
create_default_repo_lists(GList **tile_sources, GList **repositories)
{
    TileSource *osm, *google, *satellite, *traffic;
    Repository *repo;

    *tile_sources = NULL;
    *repositories = NULL;

    osm = g_slice_new0(TileSource);
    osm->name = g_strdup("OpenStreet");
    osm->id = g_strdup("OpenStreet");
    osm->cache_dir = g_strdup(osm->id);
    osm->url = g_strdup(REPO_DEFAULT_MAP_URI);
    osm->type = REPOTYPE_XYZ_INV;
    osm->visible = TRUE;
    *tile_sources = g_list_append(*tile_sources, osm);

    google = g_slice_new0(TileSource);
    google->name = g_strdup("Google Vector");
    google->id = g_strdup("GoogleVector");
    google->cache_dir = g_strdup(google->id);
    google->url = g_strdup("http://mt.google.com/vt?z=%d&x=%d&y=%0d");
    google->type = REPOTYPE_XYZ_INV;
    google->visible = TRUE;
    *tile_sources = g_list_append(*tile_sources, google);

    satellite = g_slice_new0(TileSource);
    satellite->name = g_strdup("Google Satellite");
    satellite->id = g_strdup("GoogleSatellite");
    satellite->cache_dir = g_strdup(satellite->id);
    satellite->url = g_strdup("http://khm.google.com/kh/v=51&z=%d&x=%d&y=%0d");
    satellite->type = REPOTYPE_XYZ_INV;
    satellite->visible = TRUE;
    *tile_sources = g_list_append(*tile_sources, satellite);

    /* layers */
    traffic = g_slice_new0(TileSource);
    traffic->name = g_strdup("Google Traffic");
    traffic->id = g_strdup("GoogleTraffic");
    traffic->cache_dir = g_strdup(traffic->id);
    traffic->url = g_strdup("http://mt0.google.com/vt?lyrs=m@115,traffic&"
                            "z=%d&x=%d&y=%0d&opts=T");
    traffic->type = REPOTYPE_XYZ_INV;
    traffic->visible = TRUE;
    traffic->transparent = TRUE;
    *tile_sources = g_list_append(*tile_sources, traffic);

    /* repositories */
    repo = g_slice_new0(Repository);
    repo->name = g_strdup("OpenStreet");
    repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
    repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
    repo->zoom_step = 1;
    repo->primary = osm;
    *repositories = g_list_append(*repositories, repo);

    repo = g_slice_new0(Repository);
    repo->name = g_strdup("Google Satellite");
    repo->min_zoom = 4;
    repo->max_zoom = 24;
    repo->zoom_step = 1;
    repo->primary = satellite;
    repo->layers = g_ptr_array_new();
    g_ptr_array_add(repo->layers, traffic);
    *repositories = g_list_append(*repositories, repo);

    repo = g_slice_new0(Repository);
    repo->name = g_strdup("Google");
    repo->min_zoom = 4;
    repo->max_zoom = 24;
    repo->zoom_step = 1;
    repo->primary = google;
    repo->layers = g_ptr_array_new();
    g_ptr_array_add(repo->layers, traffic);
    *repositories = g_list_append(*repositories, repo);

    return repo;
}


/* Show dialog with list of repositories */
void
repositories_dialog()
{
    GtkWidget *dialog, *edit_button, *delete_button;
    HildonTouchSelector *repos_selector;
    MapController *controller = map_controller_get_instance();
    enum {
        RESP_SYNC,
        RESP_ADD,
        RESP_EDIT,
        RESP_DELETE,
    };
    gint response;
    GList *repo_list;
    Repository *repo, *active_repo = NULL;
    gint active;

    dialog = gtk_dialog_new_with_buttons(_("Repositories"), GTK_WINDOW(_window),
                                         GTK_DIALOG_MODAL, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Sync"), RESP_SYNC);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ADD, RESP_ADD);
    edit_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_EDIT, RESP_EDIT);
    delete_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_DELETE, RESP_DELETE);

    /* There is no way to edit/delete items in touch selector widget (Shame on you, Nokia!),
       so, we must spin there to maintain our list in sync */
    while (1) {
        repos_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());

        /* Populate selector with repositories */
        repo_list = map_controller_get_repo_list(controller);
        if (!active_repo)
            active_repo = map_controller_get_repository(controller);
        active = 0;
        while (repo_list) {
            repo = (Repository*)repo_list->data;
            hildon_touch_selector_append_text(repos_selector, repo->name);
            if (repo == active_repo)
                hildon_touch_selector_set_active(repos_selector, 0, active);
            repo_list = repo_list->next;
            active++;
        }

        /* These two buttons have meaning only if we have something in a list */
        gtk_widget_set_sensitive(edit_button, active > 0);
        gtk_widget_set_sensitive(delete_button, active > 0);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), GTK_WIDGET(repos_selector), TRUE, TRUE, 0);

        gtk_widget_show_all(dialog);
        if ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_DELETE_EVENT)
            break;

        active = hildon_touch_selector_get_active(repos_selector, 0);
        if (active >= 0)
            active_repo = (Repository*)g_list_nth_data(map_controller_get_repo_list(controller), active);
        else
            active_repo = NULL;

        switch (response) {
        case RESP_SYNC:
            printf ("Sync not implemented\n");
            break;
        case RESP_ADD:
            printf ("Add not implemented\n");
            break;
        case RESP_EDIT:
            printf ("Edit not implemented\n");
            break;
        case RESP_DELETE:
            repository_delete_handler(GTK_WINDOW(dialog), active_repo);
            active_repo = NULL;
            break;
        }
        gtk_widget_destroy(GTK_WIDGET(repos_selector));
    }
    gtk_widget_destroy(dialog);
}


/*
 * Show dialog to edit repository settings. Returns TRUE when
 * user pressed 'save', FALSE on cancel
 */
gboolean
repository_edit_dialog(Repository *repo)
{
    return FALSE;
}


/*
 * Show dialog to edit tile source settings. Returns TRUE with
 * when user pressed 'save', FALSE on cancel
 */
gboolean
tile_source_edit_dialog(TileSource *ts)
{
    return FALSE;
}
