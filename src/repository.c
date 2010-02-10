#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <glib.h>

#include <hildon/hildon-button.h>
#include <hildon/hildon-entry.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-picker-dialog.h>
#include <hildon/hildon-touch-selector.h>

#include "controller.h"
#include "data.h"
#include "defines.h"
#include "display.h"
#include "menu.h"
#include "repository.h"
#include "settings.h"
#include "tile_source.h"
#include "types.h"
#include "util.h"

/* Tag names. Used in XML generator and parser. */
#define REPO_ROOT		"Repositories"
#define REPO_ENTRY		"Repository"
#define REPO_LAYERS_GROUP	"Layers"
#define REPO_LAYER_ENTRY	"Layer"
#define REPO_PRIMARY_ENTRY	"Primary"


static void
append_int(xmlNodePtr parent, const gchar *name, int val)
{
    gchar *p;

    p = g_strdup_printf("%d", val);
    xmlNewChild(parent, NULL, BAD_CAST name, p);
    g_free(p);
}


static void
fill_selector_with_repositories(HildonTouchSelector *selector, Repository *active)
{
    MapController *controller = map_controller_get_instance();
    GList *repo_list = map_controller_get_repo_list(controller);
    gint index = 0;
    Repository *repo;

    if (!active)
        active = map_controller_get_repository(controller);

    while (repo_list) {
        repo = (Repository*)repo_list->data;
        hildon_touch_selector_append_text(selector, repo->name);
        if (repo == active)
            hildon_touch_selector_set_active(selector, 0, index);
        repo_list = repo_list->next;
        index++;
    }
}


gchar*
repository_list_to_xml(GList *repositories)
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

        if (strcmp(n->name, "name") == 0)
            repo->name = g_strdup(s);
        if (strcmp(n->name, "min_zoom") == 0 && val_valid)
            repo->min_zoom = val;
        if (strcmp(n->name, "max_zoom") == 0 && val_valid)
            repo->max_zoom = val;
        if (strcmp(n->name, "zoom_step") == 0 && val_valid)
            repo->zoom_step = val;
        else if (strcmp(n->name, REPO_LAYERS_GROUP) == 0) {
            repo->layers = g_ptr_array_new();

            for (nn = n->children; nn; nn = nn->next) {
                ss = xmlNodeListGetString(doc, nn->children, 1);
                if (strcmp(nn->name, REPO_LAYER_ENTRY) == 0) {
                    ts = map_controller_lookup_tile_source(controller, ss);

                    if (ts)
                        g_ptr_array_add(repo->layers, ts);
                }
                else if (strcmp(nn->name, REPO_PRIMARY_ENTRY) == 0) {
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

    msg = g_strdup_printf(_("Do you really want to delete repository\n%s?"), repo->name);

    dialog = hildon_note_new_confirmation(parent, msg);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (ret == GTK_RESPONSE_OK)
        map_controller_delete_repository(map_controller_get_instance(), repo);
}


/*
 * Compare repositories using fields, which are not
 * supposed to be customized by user. Return TRUE if
 * they are equal.
 */
static gboolean
repository_compare(Repository *repo1, Repository *repo2)
{
    gint layer;
    if (strcmp(repo1->name, repo2->name) != 0)
        return FALSE;
    if (repo1->min_zoom != repo2->min_zoom ||
        repo1->max_zoom != repo2->max_zoom ||
        repo1->zoom_step != repo2->zoom_step)
    {
        return FALSE;
    }
    if (!repo1->primary || !repo2->primary || strcmp(repo1->primary->id, repo2->primary->id) != 0)
        return FALSE;
    if (repo1->layers || repo2->layers) {
        if (!repo1->layers || !repo2->layers)
            return FALSE;
        if (repo1->layers->len != repo2->layers->len)
            return FALSE;
        for (layer = 0; layer < repo1->layers->len; layer++)
            if (strcmp(((TileSource*)g_ptr_array_index(repo1->layers, layer))->id, 
                       ((TileSource*)g_ptr_array_index(repo2->layers, layer))->id) != 0)
                return FALSE;
    }
    return TRUE;
}


/*
 * Download XML data with repositories and tile sources and merge it into current configuration.
 * Don't really sure what is better: silently update user's repositories or ask about this?
 * At this moment, just update. Return TRUE if configuration changed by sync.
 */
static gboolean
repository_sync_handler(GtkWindow *parent)
{
    GnomeVFSResult res;
    gint size, i;
    gchar *data;
    GList *list, *list_head;
    TileSource *ts, *ts_old;
    Repository *repo, *repo_old;
    MapController *controller = map_controller_get_instance();
    gint ts_new, ts_mod, repo_new, repo_mod;

    ts_new = ts_mod = repo_new = repo_mod = 0;

    /* TileSources */
    res = gnome_vfs_read_entire_file("http://vcs.maemo.org/git/maemo-mapper/?p=maemo-mapper;"
                                     "a=blob_plain;f=tile_sources.xml;hb=refs/heads/configurations",
                                     &size, &data);
    if (res != GNOME_VFS_OK)
        return FALSE;

    if (data && size > 0) {
        list_head = list = tile_source_xml_to_list(data);
        while (list) {
            ts = (TileSource*)list->data;

            ts_old = map_controller_lookup_tile_source(controller, ts->id);
            if (ts_old) {
                if (!tile_source_compare(ts, ts_old)) {
                    /* Merge tile source */
                    if (strcmp(ts->name, ts_old->name) != 0) {
                        g_free(ts_old->name);
                        ts_old->name = g_strdup(ts->name);
                    }
                    if (strcmp(ts->url, ts_old->url) != 0) {
                        g_free(ts_old->url);
                        ts_old->url = g_strdup(ts->url);
                    }
                    ts_old->type = ts->type;
                    ts_mod++;
                }
                tile_source_free(ts);
            }
            else {
                /* We don't ask about new tile sources, because new repository may depend on them,
                 * so just silently add it */
                map_controller_append_tile_source(controller, ts);
                ts_new++;
            }

            list = list->next;
        }
    }

    /* Repositories */
    res = gnome_vfs_read_entire_file("http://vcs.maemo.org/git/maemo-mapper/?p=maemo-mapper;"
                                     "a=blob_plain;f=repositories.xml;hb=refs/heads/configurations",
                                     &size, &data);
    if (res != GNOME_VFS_OK)
        return ts_mod || ts_new;

    if (data && size > 0) {
        list_head = list = repository_xml_to_list(data);
        while (list) {
            repo = (Repository*)list->data;
            repo_old = map_controller_lookup_repository(controller, repo->name);
            if (repo_old) {
                if (!repository_compare(repo, repo_old)) {
                    /* Merge repositories */
                    if (strcmp(repo->name, repo_old->name) != 0) {
                        g_free(repo_old->name);
                        repo_old->name = g_strdup(repo->name);
                    }
                    repo_old->min_zoom = repo->min_zoom;
                    repo_old->max_zoom = repo->max_zoom;
                    repo_old->zoom_step = repo->zoom_step;
                    if (!repo_old->primary || strcmp(repo_old->primary->id, repo->primary->id) != 0)
                        repo_old->primary = map_controller_lookup_tile_source(controller, repo->primary->id);
                    if (repo_old->layers) {
                        g_ptr_array_free(repo_old->layers, TRUE);
                        repo_old->layers = g_ptr_array_new();
                        if (repo->layers)
                            for (i = 0; i < repo->layers->len; i++)
                                g_ptr_array_add(repo_old->layers, g_ptr_array_index(repo->layers, i));
                    }
                    repo_mod++;
                }
                repository_free(repo);
            }
            else {
                map_controller_append_repository(controller, repo);
                repo_new++;
            }

            list = list->next;
        }
    }

    /* Show user statistics about sync */
    {
        GtkWidget *dialog;
        gchar *msg;

        if (!ts_new && !ts_mod && !repo_new && !repo_mod)
            msg = g_strdup(_("No changes in repositories"));
        else
            msg = g_strdup_printf(_("Layers: %d new, %d updated\nRepositories: %d new, %d updated\n"),
                                  ts_new, ts_mod, repo_new, repo_mod);

        dialog = hildon_note_new_information(parent, msg);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(msg);
    }

    return ts_mod || ts_new || repo_new || repo_mod;
}


/*
 * Assign a HildonButton instance value of a comma-separated
 * list of layers' names.
 */
static void
update_layers_button_value(HildonButton *button, GPtrArray *layers)
{
    gint i;
    gchar **titles;
    gchar *title;

    if (layers && layers->len) {
        titles = g_malloc0(sizeof (gchar*) * (layers->len+1));

        for (i = 0; i < layers->len; i++)
            titles[i] = ((TileSource*)g_ptr_array_index(layers, i))->name;

        title = g_strjoinv(", ", titles);
        hildon_button_set_value(button, title);
        g_free(title);
        g_free(titles);
    }
    else
        hildon_button_set_value(button, NULL);
}


/*
 * Layers selector dialog context
 */
struct RepositoryLayersDialogContext {
    GPtrArray *layers;          /* List of TileSource references. Modified only on save clicked. */
    HildonButton *button;       /* Button with list of layer's names */
};


/*
 * Show dialog to select tile source from the list. Show only transprent layers.
 */
static TileSource*
select_tile_source_dialog(GtkWindow *parent, gboolean transparent)
{
    GtkWidget *dialog;
    HildonTouchSelector *selector;
    gint ret, index;

    selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    tile_source_fill_selector(selector, TRUE, transparent, NULL);

    dialog = hildon_picker_dialog_new(parent);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Select layer"));
    hildon_picker_dialog_set_selector(HILDON_PICKER_DIALOG(dialog), selector);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));

    if (ret == GTK_RESPONSE_DELETE_EVENT) {
        gtk_widget_destroy(dialog);
        return NULL;
    }

    index = hildon_touch_selector_get_active(selector, 0);
    gtk_widget_destroy(dialog);

    if (index >= 0) {
        /* Iterate over tile sources and find N'th */
        GList *ts_list = map_controller_get_tile_sources_list(map_controller_get_instance());
        TileSource *ts = NULL;

        while (ts_list) {
            if (((TileSource*)ts_list->data)->transparent == transparent) {
                if (!index)
                    ts = (TileSource*)ts_list->data;
                index--;
            }
            ts_list = ts_list->next;
        }

        return ts;
    }
    return NULL;
}


/*
 * Show dialog with list of layers
 */
static gboolean
select_layers_button_clicked(GtkWidget *widget, struct RepositoryLayersDialogContext *ctx)
{
    GtkWidget *dialog;
    HildonTouchSelector *layers_selector;
    TileSource *ts;
    gint resp, i;
    gboolean ret = FALSE;
    GPtrArray *layers = g_ptr_array_new();;
    enum {
        RESP_ADD,
        RESP_DELETE,
        RESP_SAVE,
    };

    /* Create copy of context's layers list. */
    if (ctx->layers && ctx->layers->len) {
        for (i = 0; i < ctx->layers->len; i++)
            g_ptr_array_add(layers, g_ptr_array_index(ctx->layers, i));
    }

    dialog = gtk_dialog_new_with_buttons(_("Repository's layers"), NULL, GTK_DIALOG_MODAL, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ADD, RESP_ADD);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_DELETE, RESP_DELETE);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_SAVE, RESP_SAVE);
    while (1) {
        layers_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());

        /* Populate selector with layers */
        for (i = 0; i < layers->len; i++) {
            ts = (TileSource*)g_ptr_array_index(layers, i);
            hildon_touch_selector_append_text(layers_selector, ts->name);
        }

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                           GTK_WIDGET(layers_selector), TRUE, TRUE, 0);
        gtk_widget_show_all(dialog);
        resp = gtk_dialog_run(GTK_DIALOG(dialog));

        if (resp == RESP_SAVE || resp == GTK_RESPONSE_DELETE_EVENT)
            break;

        i = hildon_touch_selector_get_active(layers_selector, 0);
        if (i < 0)
            ts = NULL;
        else
            ts = g_ptr_array_index(layers, i);

        switch (resp) {
        case RESP_ADD:
            ts = select_tile_source_dialog(GTK_WINDOW(dialog), TRUE);
            if (ts)
                g_ptr_array_add(layers, ts);
            break;
        case RESP_DELETE:
            g_ptr_array_remove(layers, ts);
            break;
        }
        gtk_widget_destroy(GTK_WIDGET(layers_selector));
     }

    gtk_widget_destroy(dialog);
    if (resp == RESP_SAVE) {
        update_layers_button_value(ctx->button, layers);
        if (ctx->layers)
            g_ptr_array_free(ctx->layers, TRUE);
        ctx->layers = layers;
        return TRUE;
    }
    else {
        return FALSE;
        g_ptr_array_free(layers, TRUE);
    }
}


/* Parse repositories XML */
GList* repository_xml_to_list(const gchar *data)
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
    if(!n || strcmp(n->name, REPO_ROOT) != 0) {
        xmlFreeDoc(doc);
        return res;
    }

    for (n = n->children; n; n = n->next) {
        if (n->type == XML_ELEMENT_NODE && strcmp(n->name, REPO_ENTRY) == 0) {
            Repository *repo = tree_to_repository(doc, n);
            if (repo)
                res = g_list_append(res, repo);
        }
    }

    xmlFreeDoc(doc);
    return res;
}


/*
 * Create default repositories and tile sources lists. Return default current repsoitory.
 */
Repository*
repository_create_default_lists(GList **tile_sources, GList **repositories)
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
    repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
    repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
    repo->zoom_step = 1;
    repo->primary = satellite;
    repo->layers = g_ptr_array_new();
    g_ptr_array_add(repo->layers, traffic);
    *repositories = g_list_append(*repositories, repo);

    repo = g_slice_new0(Repository);
    repo->name = g_strdup("Google");
    repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
    repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
    repo->zoom_step = 1;
    repo->primary = google;
    repo->layers = g_ptr_array_new();
    g_ptr_array_add(repo->layers, traffic);
    *repositories = g_list_append(*repositories, repo);

    return repo;
}


/*
 * Free repository structure
 */
void
repository_free(Repository *repo)
{
    if (repo->name)
        g_free(repo->name);
    if (repo->layers)
        g_ptr_array_free(repo->layers, TRUE);
    if (repo->menu_item)
        gtk_widget_destroy(repo->menu_item);
    g_slice_free(Repository, repo);
}


/* Show dialog with list of repositories */
void
repository_list_edit_dialog()
{
    GtkWidget *dialog, *edit_button, *delete_button;
    HildonTouchSelector *repos_selector;
    MapController *controller = map_controller_get_instance();
    GtkTreeModel *list_model;
    GtkListStore *list_store;
    enum {
        RESP_SYNC,
        RESP_ADD,
        RESP_EDIT,
        RESP_DELETE,
    };
    gint response;
    Repository *active_repo = NULL;
    gint active, rows_count;
    gboolean update_menus = FALSE, update_list = FALSE;

    dialog = gtk_dialog_new_with_buttons(_("Repositories"), GTK_WINDOW(_window),
                                         GTK_DIALOG_MODAL, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Sync"), RESP_SYNC);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ADD, RESP_ADD);
    edit_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_EDIT, RESP_EDIT);
    delete_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_DELETE, RESP_DELETE);
    repos_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    list_model = hildon_touch_selector_get_model(repos_selector, 0);
    list_store = GTK_LIST_STORE(list_model);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), GTK_WIDGET(repos_selector), TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    fill_selector_with_repositories(repos_selector, NULL);

    while (1) {
        rows_count = gtk_tree_model_iter_n_children(list_model, NULL);

        /* These two buttons have meaning only if we have something in a list */
        gtk_widget_set_sensitive(edit_button, rows_count > 0);
        gtk_widget_set_sensitive(delete_button, rows_count > 0);

        if ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_DELETE_EVENT)
            break;

        active = hildon_touch_selector_get_active(repos_selector, 0);
        if (active >= 0)
            active_repo = (Repository*)g_list_nth_data(map_controller_get_repo_list(controller), active);
        else
            active_repo = NULL;

        switch (response) {
        case RESP_SYNC:
            update_list = update_menus = repository_sync_handler(GTK_WINDOW(dialog));
            break;
        case RESP_ADD:
            active_repo = g_slice_new0(Repository);
            active_repo->name = g_strdup(_("New repository"));
            active_repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
            active_repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
            active_repo->zoom_step = 1;

            if (repository_edit_dialog(GTK_WINDOW(dialog), active_repo)) {
                map_controller_append_repository(controller, active_repo);
                update_list = update_menus = TRUE;
            }
            else
                repository_free(active_repo);
            active_repo = NULL;
            break;
        case RESP_EDIT:
            if (active_repo)
                if (repository_edit_dialog(GTK_WINDOW(dialog), active_repo)) {
                    if (active_repo == map_controller_get_repository(controller))
                        map_refresh_mark(TRUE);
                    update_list = update_menus = TRUE;
                }
            break;
        case RESP_DELETE:
            repository_delete_handler(GTK_WINDOW(dialog), active_repo);
            active_repo = NULL;
            update_list = TRUE;
            break;
        }

        /* We need to sync menu entries with repository list */
        if (update_menus)
            menu_maps_add_repos();

        if (update_list) {
            /* populate list with repositories */
            gtk_list_store_clear(list_store);
            fill_selector_with_repositories(repos_selector, active_repo);
        }
        update_menus = update_list = FALSE;
    }
    gtk_widget_destroy(dialog);
    settings_save();
}


/*
 * Show dialog to edit repository settings. Returns TRUE when
 * user pressed 'save', FALSE on cancel
 */
gboolean
repository_edit_dialog(GtkWindow *parent, Repository *repo)
{
    GtkWidget *dialog;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *name_entry;
    GtkWidget *min_zoom, *max_zoom, *zoom_step;
    HildonTouchSelector *min_zoom_selector, *max_zoom_selector;
    HildonTouchSelector *zoom_step_selector;
    GtkWidget *primary_layer;
    HildonTouchSelector *primary_selector;
    GtkWidget *layers;
    gint i;
    gchar buf[10];
    struct RepositoryLayersDialogContext layers_context;
    gboolean res = FALSE;
    MapController *controller = map_controller_get_instance();

    dialog = gtk_dialog_new_with_buttons(_("Repository"), parent, GTK_DIALOG_MODAL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    table = gtk_table_new(4, 3, TRUE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

    label = gtk_label_new(_("Name"));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    name_entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_table_attach(GTK_TABLE(table), name_entry, 1, 3, 0, 1, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Zoom selectors are used to otain zoom levels */
    min_zoom_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    max_zoom_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    zoom_step_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());

    for (i = MIN_ZOOM; i <= MAX_ZOOM; i++) {
        g_snprintf(buf, sizeof(buf), "%d", i);
        hildon_touch_selector_append_text(min_zoom_selector, buf);
        hildon_touch_selector_append_text(max_zoom_selector, buf);
    }

    for (i = 1; i <= 4; i++) {
        g_snprintf(buf, sizeof(buf), "%d", i);
        hildon_touch_selector_append_text(zoom_step_selector, buf);
    }

    min_zoom = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                            "size", HILDON_SIZE_FINGER_HEIGHT,
                            "title", _("Min zoom"),
                            "touch-selector", min_zoom_selector,
                            NULL);
    gtk_table_attach(GTK_TABLE(table), min_zoom, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    max_zoom = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                            "size", HILDON_SIZE_FINGER_HEIGHT,
                            "title", _("Max zoom"),
                            "touch-selector", max_zoom_selector,
                            NULL);
    gtk_table_attach(GTK_TABLE(table), max_zoom, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 2, 4);

    zoom_step = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                            "size", HILDON_SIZE_FINGER_HEIGHT,
                            "title", _("Zoom step"),
                            "touch-selector", zoom_step_selector,
                            NULL);
    gtk_table_attach(GTK_TABLE(table), zoom_step, 2, 3, 1, 2,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Primary layer */
    primary_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());

    /* In this selector, we allow to choose from non-transparent tile sources only */
    tile_source_fill_selector(primary_selector, TRUE, FALSE, repo->primary);

    primary_layer = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                            "size", HILDON_SIZE_FINGER_HEIGHT,
                            "title", _("Tiles"),
                            "touch-selector", primary_selector,
                            NULL);
    gtk_table_attach(GTK_TABLE(table), primary_layer, 0, 3, 2, 3,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Layers */
    layers = hildon_button_new_with_text(HILDON_SIZE_FINGER_HEIGHT,
                                         ((repo->layers) && (repo->layers->len > 3)) ?
                                         HILDON_BUTTON_ARRANGEMENT_VERTICAL :
                                         HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                                         _("Layers"), NULL);
    gtk_table_attach(GTK_TABLE(table), layers, 0, 3, 3, 4,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

    /* Fill with data */
    hildon_entry_set_text(HILDON_ENTRY(name_entry), repo->name);
    hildon_touch_selector_set_active(min_zoom_selector, 0, repo->min_zoom - MIN_ZOOM);
    hildon_touch_selector_set_active(max_zoom_selector, 0, repo->max_zoom - MIN_ZOOM);
    hildon_touch_selector_set_active(zoom_step_selector, 0, repo->zoom_step - 1);

    update_layers_button_value(HILDON_BUTTON(layers), repo->layers);

    layers_context.button = HILDON_BUTTON(layers);
    layers_context.layers = g_ptr_array_new();

    if (repo->layers && repo->layers->len) {
        for (i = 0; i < repo->layers->len; i++)
            g_ptr_array_add(layers_context.layers, g_ptr_array_index(repo->layers, i));
    }

    g_signal_connect(G_OBJECT(layers), "clicked", G_CALLBACK(select_layers_button_clicked), &layers_context);

    gtk_widget_show_all(dialog);
    res = FALSE;

    while (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gint index;
        TileSource *ts;

        /* Check for primary tile source. If not specified or not valid,
         * show notice and spin again. */
        ts = map_controller_lookup_tile_source_by_name(
            controller, hildon_button_get_value(HILDON_BUTTON(primary_layer)));

        if (ts)
            repo->primary = ts;
        else {
            popup_error(dialog, _("Tile source is required"));
            continue;
        }

        if (gtk_entry_get_text_length (GTK_ENTRY(name_entry)) &&
            strcmp(gtk_entry_get_text(GTK_ENTRY(name_entry)), repo->name) != 0)
        {
            g_free(repo->name);
            repo->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name_entry)));
        }

        index = hildon_touch_selector_get_active(min_zoom_selector, 0);
        if (index >= 0)
            repo->min_zoom = index + MIN_ZOOM;
        index = hildon_touch_selector_get_active(max_zoom_selector, 0);
        if (index >= 0)
            repo->max_zoom = index + MIN_ZOOM;
        index = hildon_touch_selector_get_active(zoom_step_selector, 0);
        if (index >= 0)
            repo->zoom_step = index + 1;

        /* Layers */
        if (repo->layers)
            g_ptr_array_free(repo->layers, TRUE);
        repo->layers = layers_context.layers;
        res = TRUE;
        break;
    }

    gtk_widget_destroy(dialog);

    if (!res)
        g_ptr_array_free(layers_context.layers, TRUE);

    return res;
}
