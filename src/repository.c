/*
 * Copyright (C) 2010 Max Lapan
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

#include <mappero/debug.h>

/* Tag names. Used in XML generator and parser. */
#define REPO_ROOT		"Repositories"
#define REPO_ENTRY		"Repository"
#define REPO_LAYERS_GROUP	"Layers"
#define REPO_LAYER_ENTRY	"Layer"
#define REPO_PRIMARY_ENTRY	"Primary"


static gboolean
repository_is_valid(Repository *repo)
{
    g_return_val_if_fail(repo != NULL, FALSE);

    return repo->name != NULL &&
        repo->max_zoom >= repo->min_zoom &&
        repo->primary != NULL;
}

static void
append_int(xmlNodePtr parent, const gchar *name, int val)
{
    gchar *p;

    p = g_strdup_printf("%d", val);
    xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST p);
    g_free(p);
}


static void
fill_selector_with_repositories(HildonTouchSelector *selector)
{
    MapController *controller = map_controller_get_instance();
    GList *repo_list = map_controller_get_repo_list(controller);
    Repository *repo;

    while (repo_list) {
        repo = (Repository*)repo_list->data;
        hildon_touch_selector_append_text(selector, repo->name);
        repo_list = repo_list->next;
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
    RepositoryLayer *repo_layer;

    doc = xmlNewDoc(BAD_CAST "1.0");
    n = xmlNewNode(NULL, BAD_CAST REPO_ROOT);
    xmlDocSetRootElement(doc, n);

    while (repositories)
    {
        repo = (Repository*)repositories->data;
        nn = xmlNewChild(n, NULL, BAD_CAST REPO_ENTRY, NULL);
        
        if (repo->name)
            xmlNewChild(nn, NULL, BAD_CAST "name", BAD_CAST repo->name);
        append_int(nn, "min_zoom", repo->min_zoom);
        append_int(nn, "max_zoom", repo->max_zoom);
        append_int(nn, "zoom_step", repo->zoom_step);

        nl = xmlNewNode(NULL, BAD_CAST REPO_LAYERS_GROUP);
        xmlAddChild(nn, nl);

        if (repo->primary && repo->primary->id)
            xmlNewChild(nl, NULL, BAD_CAST REPO_PRIMARY_ENTRY,
                        BAD_CAST repo->primary->id);

        if (repo->layers) {
            for (i = 0; i < repo->layers->len; i++) {
                repo_layer = g_ptr_array_index (repo->layers, i);
                if (repo_layer && repo_layer->ts->name) {
                    nll = xmlNewChild(nl, NULL, BAD_CAST REPO_LAYER_ENTRY,
                                      BAD_CAST (repo_layer->ts->id ? repo_layer->ts->id : ""));
                    xmlSetProp(nll, BAD_CAST "visible", BAD_CAST (repo_layer->visible ? "1" : "0"));
                }
            }
        }
        repositories = g_list_next(repositories);
    }

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


static Repository *
tree_to_repository(xmlDocPtr doc, xmlNodePtr repo_node)
{
    Repository* repo;
    xmlNodePtr n, nn;
    gchar *s, *ss;
    gint val;
    gboolean val_valid;
    TileSource *ts;
    MapController *controller = map_controller_get_instance();

    repo = g_slice_new0(Repository);

    for (n = repo_node->children; n; n = n->next) {
        if (!n->children)
            continue;
        s = (gchar*)xmlNodeListGetString(doc, n->children, 1);
        val_valid = sscanf(s, "%d", &val) > 0;
        ss = (gchar*)n->name;

        if (strcmp(ss, "name") == 0)
            repo->name = g_strdup(s);
        if (strcmp(ss, "min_zoom") == 0 && val_valid)
            repo->min_zoom = val;
        if (strcmp(ss, "max_zoom") == 0 && val_valid)
            repo->max_zoom = val;
        if (strcmp(ss, "zoom_step") == 0 && val_valid)
            repo->zoom_step = val;
        else if (strcmp(ss, REPO_LAYERS_GROUP) == 0) {
            repo->layers = g_ptr_array_new();

            for (nn = n->children; nn; nn = nn->next) {
                ss = (gchar*)xmlNodeListGetString(doc, nn->children, 1);
                if (strcmp((gchar*)nn->name, REPO_LAYER_ENTRY) == 0) {
                    ts = map_controller_lookup_tile_source(controller, ss);

                    if (ts) {
                        RepositoryLayer *repo_layer = g_slice_new0(RepositoryLayer);
                        const xmlChar *visible = xmlGetProp(nn, BAD_CAST "visible");
                        repo_layer->ts = ts;
                        repo_layer->visible = (!visible || *visible == '1') ? TRUE : FALSE;
                        g_ptr_array_add(repo->layers, repo_layer);
                    }
                }
                else if (strcmp((gchar*)nn->name, REPO_PRIMARY_ENTRY) == 0) {
                    ts = map_controller_lookup_tile_source(controller, ss);
                    if (ts)
                        repo->primary = ts;
                }
                xmlFree(ss);
            }
        }
        xmlFree(s);
    }

    if (!repository_is_valid(repo))
    {
        DEBUG("Invalid repository %s", repo->name);
        repository_free(repo);
        repo = NULL;
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

    msg = g_strdup_printf(_("Do you really want to delete repository\n%s?"),
                          repo->name);

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
    if (!repo1->primary || !repo2->primary ||
        strcmp(repo1->primary->id, repo2->primary->id) != 0)
        return FALSE;
    if (repo1->layers || repo2->layers) {
        if (!repo1->layers || !repo2->layers)
            return FALSE;
        if (repo1->layers->len != repo2->layers->len)
            return FALSE;
        for (layer = 0; layer < repo1->layers->len; layer++)
            if (strcmp(((RepositoryLayer*)g_ptr_array_index(repo1->layers, layer))->ts->id,
                       ((RepositoryLayer*)g_ptr_array_index(repo2->layers, layer))->ts->id) != 0)
                return FALSE;
    }
    return TRUE;
}


/*
 * Download XML data with repositories and tile sources and merge it into
 * current configuration.  Don't really sure what is better: silently update
 * user's repositories or ask about this?  At this moment, just update. Return
 * TRUE if configuration changed by sync.
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
    res = gnome_vfs_read_entire_file(
            "http://vcs.maemo.org/git/maemo-mapper/?p=maemo-mapper;"
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
                    ts_old->format = ts->format;
                    ts_mod++;
                }
                tile_source_free(ts);
            }
            else {
                /* We don't ask about new tile sources, because new repository
                 * may depend on them, so just silently add it */
                map_controller_append_tile_source(controller, ts);
                ts_new++;
            }

            list = list->next;
        }
    }

    /* Repositories */
    res = gnome_vfs_read_entire_file(
               "http://vcs.maemo.org/git/maemo-mapper/?p=maemo-mapper;"
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
                        repo_old->primary =
                            map_controller_lookup_tile_source(controller, repo->primary->id);
                    if (repo_old->layers) {
                        for (i = 0; i < repo_old->layers->len; i++)
                            g_slice_free(RepositoryLayer, g_ptr_array_index(repo_old->layers, i));
                        g_ptr_array_free(repo_old->layers, TRUE);
                        repo_old->layers = g_ptr_array_new();
                        if (repo->layers) {
                            for (i = 0; i < repo->layers->len; i++) {
                                RepositoryLayer *repo_layer = g_ptr_array_index(repo->layers, i);
                                g_ptr_array_add(repo_old->layers, repo_layer);
                            }
                            g_ptr_array_free(repo->layers, TRUE);
                            repo->layers = NULL;
                        }
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
        gchar *msg;

        if (!ts_new && !ts_mod && !repo_new && !repo_mod)
            msg = g_strdup(_("No changes in repositories"));
        else
            msg = g_strdup_printf(_("Layers: %d new, %d updated\n"
                                    "Repositories: %d new, %d updated\n"),
                                  ts_new, ts_mod, repo_new, repo_mod);

        popup_error(GTK_WIDGET(parent), msg);
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
    GPtrArray *layers;          /* List of TileSource references. Modified only
                                 * on save clicked. */
    HildonButton *button;       /* Button with list of layer's names */
};


static gboolean
lookup_ptr_array(GPtrArray *arr, gpointer ptr)
{
    gint i;

    for (i = 0; i < arr->len; i++)
        if (g_ptr_array_index(arr, i) == ptr)
            return TRUE;
    return FALSE;
}


static void
toggle_layer_callback(GtkCellRendererToggle *toggle,
                      gchar *path, GtkListStore *store)
{
    GtkTreePath *tree_path = gtk_tree_path_new_from_string(path);
    GtkTreeIter iter;
    gboolean val;

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, tree_path))
        return;

    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &val, -1);
    gtk_list_store_set(store, &iter, 0, !val, -1);
    gtk_tree_path_free(tree_path);
}


/*
 * Show dialog with list of layers
 */
static gboolean
select_layers_button_clicked(GtkWidget *widget,
                             struct RepositoryLayersDialogContext *ctx)
{
    MapController *controller = map_controller_get_instance();
    GList *ts_list;
    GtkWidget *dialog;
    HildonTouchSelector *layers_selector;
    TileSource *ts;
    gint resp;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    HildonTouchSelectorColumn *column;
    enum {
        RESP_SAVE,
    };

    dialog = gtk_dialog_new_with_buttons(_("Repository's layers"), NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_SAVE, RESP_SAVE, NULL);
    layers_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new());

    /* Fill store with list of transparent tile source to let user select from */
    store = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);
    ts_list = map_controller_get_tile_sources_list(controller);

    while (ts_list) {
        ts = (TileSource*)ts_list->data;
        if (ts->transparent) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, lookup_ptr_array(ctx->layers, ts),
                               1, ts->name,
                               -1);
        }
        ts_list = ts_list->next;
    }

    /* make column with text and checkbox */
    column = hildon_touch_selector_append_column(layers_selector, GTK_TREE_MODEL(store), NULL, NULL);
    hildon_touch_selector_column_set_text_column(column, 1);

    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_set_fixed_size (renderer, 50, 50);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer, "active", 0, NULL);
    g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(toggle_layer_callback), store);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer, "text", 1, NULL);

    hildon_touch_selector_set_hildon_ui_mode(layers_selector, HILDON_UI_MODE_NORMAL);
    gtk_widget_set_size_request(GTK_WIDGET(layers_selector), -1, 300);

    /* We finished with selector, pack it */
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       GTK_WIDGET(layers_selector), TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    resp = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
    if (resp == RESP_SAVE) {
        /* iterate list store and update context's list*/
        if (ctx->layers) {
            g_ptr_array_free(ctx->layers, TRUE);
            ctx->layers = NULL;
        }
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
            gboolean val;
            const gchar *name;

            ctx->layers = g_ptr_array_new();

            while (1) {
                gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &val, 1, &name, -1);
                if (val) {
                    ts = map_controller_lookup_tile_source_by_name(controller, name);
                    if (ts)
                        g_ptr_array_add(ctx->layers, ts);
                }
                if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
                    break;
            }
        }
        update_layers_button_value(ctx->button, ctx->layers);
        return TRUE;
    }
    else
        return FALSE;
}



/*
 * Returns last activated item in repository selector
 */
static Repository *
repository_selector_get_activated(HildonTouchSelector *selector)
{
    GList *repo_list = map_controller_get_repo_list(map_controller_get_instance());
    GtkTreePath *path;
    gint *indices;

    path = hildon_touch_selector_get_last_activated_row(selector, 0);
    if (!path)
        return NULL;

    indices = gtk_tree_path_get_indices(path);
    if (!indices)
        return NULL;

    return (Repository*)g_list_nth_data(repo_list, *indices);
}


/*
 * Routine erases passed selector and fill it with actual
 * list of repositories' names.
 */
static void
refresh_repository_list_selector(HildonTouchSelector *selector)
{
    GtkListStore *list_store = GTK_LIST_STORE(hildon_touch_selector_get_model(selector, 0));

    /* clear current items */
    gtk_list_store_clear(list_store);

    /* fill selector with repositories */
    fill_selector_with_repositories(selector);
}


/*
 * Show dialog to edit repository settings. Returns TRUE when
 * user pressed 'save' or 'delete', FALSE on cancel
 */
static gboolean
repository_edit_dialog(GtkWindow *parent, Repository *repo, gboolean allow_delete)
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
    gint i, response;
    gchar buf[10];
    struct RepositoryLayersDialogContext layers_context;
    gboolean res = FALSE;
    MapController *controller = map_controller_get_instance();
    enum {
        RESP_DELETE,
        RESP_SAVE,
    };

    dialog = gtk_dialog_new_with_buttons(_("Repository"), parent, GTK_DIALOG_MODAL,
                                         NULL);
    gtk_widget_set_sensitive(gtk_dialog_add_button(GTK_DIALOG(dialog),
                                                   GTK_STOCK_DELETE, RESP_DELETE),
                             allow_delete);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_SAVE, RESP_SAVE);
    table = gtk_table_new(4, 3, TRUE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

    label = gtk_label_new(_("Name"));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

    name_entry = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_table_attach(GTK_TABLE(table), name_entry, 1, 3, 0, 1,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

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
    gtk_table_attach(GTK_TABLE(table), min_zoom, 0, 1, 1, 2,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

    max_zoom = g_object_new(HILDON_TYPE_PICKER_BUTTON,
                            "size", HILDON_SIZE_FINGER_HEIGHT,
                            "title", _("Max zoom"),
                            "touch-selector", max_zoom_selector,
                            NULL);
    gtk_table_attach(GTK_TABLE(table), max_zoom, 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, 0, 2, 4);

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
    hildon_touch_selector_set_active(min_zoom_selector, 0,
                                     repo->min_zoom - MIN_ZOOM);
    hildon_touch_selector_set_active(max_zoom_selector, 0,
                                     repo->max_zoom - MIN_ZOOM);
    hildon_touch_selector_set_active(zoom_step_selector, 0,
                                     repo->zoom_step - 1);

    layers_context.button = HILDON_BUTTON(layers);
    layers_context.layers = g_ptr_array_new();

    if (repo->layers && repo->layers->len) {
        RepositoryLayer *repo_layer;
        for (i = 0; i < repo->layers->len; i++) {
            repo_layer = (RepositoryLayer*)g_ptr_array_index(repo->layers, i);
            g_ptr_array_add(layers_context.layers, repo_layer->ts);
        }
    }

    update_layers_button_value(HILDON_BUTTON(layers), layers_context.layers);
    g_signal_connect(G_OBJECT(layers), "clicked",
                     G_CALLBACK(select_layers_button_clicked),
                     &layers_context);

    gtk_widget_show_all(dialog);
    res = FALSE;

    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) != GTK_RESPONSE_DELETE_EVENT) {
        gint index;
        TileSource *ts;

        if (response == RESP_DELETE) {
            /* User wants to delete repository completely */
            repository_delete_handler(GTK_WINDOW(dialog), repo);
            repo = NULL;
        }
        else {
            /* User wants to save repository */
            /* Check for primary tile source. If not specified or not valid,
             * show notice and spin again. */
            ts = map_controller_lookup_tile_source_by_name(controller,
                       hildon_button_get_value(HILDON_BUTTON(primary_layer)));

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
            if (repo->layers) {
                for (i = 0; i < repo->layers->len; i++)
                    g_slice_free(RepositoryLayer, g_ptr_array_index(repo->layers, i));
                g_ptr_array_free(repo->layers, TRUE);
            }

            repo->layers = g_ptr_array_new();
            for (i = 0; i < layers_context.layers->len; i++) {
                RepositoryLayer *repo_layer = g_slice_new0(RepositoryLayer);
                repo_layer->visible = TRUE;
                repo_layer->ts = g_ptr_array_index(layers_context.layers, i);
                g_ptr_array_add(repo->layers, repo_layer);
            }
        }
        res = TRUE;
        break;
    }

    gtk_widget_destroy(dialog);
    g_ptr_array_free(layers_context.layers, TRUE);

    return res;
}



/*
 * Routine called when user activates item in touch selector
 */
static void
select_repository_callback(HildonTouchSelector *selector,
                           gint column, gpointer data)
{
    MapController *controller = map_controller_get_instance();
    Repository *repo;
    gboolean repo_is_active;

    repo = repository_selector_get_activated(selector);
    if (!repo)
        return;

    repo_is_active = repo == map_controller_get_repository(controller);

    if (repository_edit_dialog(NULL, repo, !repo_is_active))
    {
        if (repo_is_active)
            map_refresh_mark(TRUE);
        /* Refresh repostiory list */
        refresh_repository_list_selector(selector);
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
    if(!n || strcmp((gchar*)n->name, REPO_ROOT) != 0) {
        xmlFreeDoc(doc);
        return res;
    }

    for (n = n->children; n; n = n->next) {
        if (n->type == XML_ELEMENT_NODE && 
            strcmp((gchar*)n->name, REPO_ENTRY) == 0)
        {
            Repository *repo = tree_to_repository(doc, n);
            if (repo)
                res = g_list_append(res, repo);
        }
    }

    xmlFreeDoc(doc);
    return res;
}


/*
 * Create default repositories and tile sources lists. Return default current
 * repsoitory.
 */
Repository*
repository_create_default_lists(GList **tile_sources, GList **repositories)
{
    TileSource *osm, *google, *satellite, *traffic;
    Repository *repo;
    const TileSourceType *type = tile_source_type_find_by_name("XYZ_INV");
    RepositoryLayer *repo_layer;

    *tile_sources = NULL;
    *repositories = NULL;

    osm = g_slice_new0(TileSource);
    osm->name = g_strdup("OpenStreet");
    osm->id = g_strdup("OpenStreetMap I");
    osm->cache_dir = g_strdup(osm->id);
    osm->url = g_strdup(REPO_DEFAULT_MAP_URI);
    osm->type = type;
    osm->format = FORMAT_PNG;
    *tile_sources = g_list_append(*tile_sources, osm);

    google = g_slice_new0(TileSource);
    google->name = g_strdup("Google Vector");
    google->id = g_strdup("GoogleVector");
    google->cache_dir = g_strdup(google->id);
    google->url = g_strdup("http://mt.google.com/vt?z=%d&x=%d&y=%0d");
    google->type = type;
    google->format = FORMAT_PNG;
    *tile_sources = g_list_append(*tile_sources, google);

    satellite = g_slice_new0(TileSource);
    satellite->name = g_strdup("Google Satellite");
    satellite->id = g_strdup("GoogleSatellite");
    satellite->cache_dir = g_strdup(satellite->id);
    satellite->url = g_strdup("http://khm.google.com/kh/v=51&z=%d&x=%d&y=%0d");
    satellite->type = type;
    satellite->format = FORMAT_JPG;
    *tile_sources = g_list_append(*tile_sources, satellite);

    /* layers */
    traffic = g_slice_new0(TileSource);
    traffic->name = g_strdup("Google Traffic");
    traffic->id = g_strdup("GoogleTraffic");
    traffic->cache_dir = g_strdup(traffic->id);
    traffic->url = g_strdup("http://mt0.google.com/vt?lyrs=m@115,traffic&"
                            "z=%d&x=%d&y=%0d&opts=T");
    traffic->type = type;
    traffic->transparent = TRUE;
    *tile_sources = g_list_append(*tile_sources, traffic);

    /* repositories */
    repo = g_slice_new0(Repository);
    repo->name = g_strdup("Google Satellite");
    repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
    repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
    repo->zoom_step = 1;
    repo->primary = satellite;
    repo->layers = g_ptr_array_new();
    repo_layer = g_slice_new0(RepositoryLayer);
    repo_layer->ts = traffic;
    g_ptr_array_add(repo->layers, repo_layer);
    *repositories = g_list_append(*repositories, repo);

    repo = g_slice_new0(Repository);
    repo->name = g_strdup("Google");
    repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
    repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
    repo->zoom_step = 1;
    repo->primary = google;
    repo->layers = g_ptr_array_new();
    repo_layer = g_slice_new0(RepositoryLayer);
    repo_layer->ts = traffic;
    g_ptr_array_add(repo->layers, repo_layer);
    *repositories = g_list_append(*repositories, repo);

    repo = g_slice_new0(Repository);
    repo->name = g_strdup("OpenStreet");
    repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
    repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
    repo->zoom_step = 1;
    repo->primary = osm;
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
    if (repo->layers) {
        gint i;
        RepositoryLayer *repo_layer;
        for (i = 0; i < repo->layers->len; i++) {
            repo_layer = (RepositoryLayer*)g_ptr_array_index(repo->layers, i);
            g_slice_free(RepositoryLayer, repo_layer);
        }
        g_ptr_array_free(repo->layers, TRUE);
    }
    if (repo->menu_item)
        gtk_widget_destroy(repo->menu_item);
    g_slice_free(Repository, repo);
}


/* Show dialog with list of repositories */
void
repository_list_edit_dialog()
{
    GtkWidget *dialog;
    HildonTouchSelector *repos_selector;
    MapController *controller = map_controller_get_instance();
    enum {
        RESP_SYNC,
        RESP_NEW
    };
    gint response;
    Repository *repo;
    gboolean update_list = FALSE;

    dialog = gtk_dialog_new_with_buttons(_("Repositories"), GTK_WINDOW(_window),
                                         GTK_DIALOG_MODAL, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Sync"), RESP_SYNC);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_NEW, RESP_NEW);
    repos_selector = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
    gtk_widget_set_size_request(GTK_WIDGET(repos_selector), -1, 300);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       GTK_WIDGET(repos_selector), TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    hildon_touch_selector_set_hildon_ui_mode(repos_selector, HILDON_UI_MODE_NORMAL);
    g_signal_connect(G_OBJECT(repos_selector), "changed", G_CALLBACK(select_repository_callback), NULL);
    refresh_repository_list_selector(repos_selector);

    while (1) {
        if ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_DELETE_EVENT)
            break;

        switch (response) {
        case RESP_SYNC:
            update_list = repository_sync_handler(GTK_WINDOW(dialog));
            break;
        case RESP_NEW:
            repo = g_slice_new0(Repository);
            repo->name = g_strdup(_("New repository"));
            repo->min_zoom = REPO_DEFAULT_MIN_ZOOM;
            repo->max_zoom = REPO_DEFAULT_MAX_ZOOM;
            repo->zoom_step = 1;

            if (repository_edit_dialog(GTK_WINDOW(dialog), repo, FALSE)) {
                map_controller_append_repository(controller, repo);
                update_list = TRUE;
            }
            else
                repository_free(repo);
            break;
        }

        if (update_list) {
            refresh_repository_list_selector(repos_selector);
            update_list = FALSE;
        }
    }
    gtk_widget_destroy(dialog);
    settings_save();
}

gboolean
repository_tile_sources_can_expire(Repository *repository)
{
    TileSource *ts;
    gint i;

    if (G_UNLIKELY(!repository)) return FALSE;

    ts = repository->primary;
    if (ts && ts->refresh)
        return TRUE;

    if (repository->layers)
    {
        /* Iterate over the active tile sources and check if they have refresh
         * turned on */
        for (i = 0; i < repository->layers->len; i++) {
            ts = g_ptr_array_index(repository->layers, i);
            if (ts->refresh)
                return TRUE;
        }
    }
    return FALSE;
}

gboolean
repository_tile_sources_expired(Repository *repository)
{
    TileSource *ts;
    gboolean expired = FALSE;
    gint i;

    /* decrement the refresh counter for the primary tile source */
    ts = repository->primary;
    if (ts && ts->refresh)
    {
        ts->countdown--;
        if (ts->countdown < 0)
            expired = TRUE;
    }

    if (repository->layers)
    {
        /* Iterate over the active tile sources and if they have refresh turned
         * on, decrement coundown */
        for (i = 0; i < repository->layers->len; i++) {
            ts = g_ptr_array_index(repository->layers, i);
            if (ts->refresh) {
                ts->countdown--;
                if (ts->countdown < 0)
                    expired = TRUE;
            }
        }
    }
    return expired;
}

