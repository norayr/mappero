/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) 2009-2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Mappero.
 *
 * Mappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mappero.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "address-picker.h"

#include "data.h"
#include "defines.h"

#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-entry.h>
#include <mappero/debug.h>
#include <string.h>

enum
{
    PROP_0,
    PROP_ROUTER,
    PROP_LOCATION,
};

struct _MapAddressPickerPrivate
{
    MapRouter *router; /* unused, for the time being */
    GtkWidget *w_address;
    GtkTreeModel *model;

    gboolean disposed;
};

G_DEFINE_TYPE(MapAddressPicker, map_address_picker, GTK_TYPE_DIALOG);

#define MAP_ADDRESS_PICKER_PRIV(address_picker) (MAP_ADDRESS_PICKER(address_picker)->priv)

static gboolean
map_address_picker_dup_location(MapAddressPicker *self, MapLocation *location)
{
    const gchar *address;
    g_return_val_if_fail(location != NULL, FALSE);

    g_free(location->address);
    address = gtk_entry_get_text(GTK_ENTRY(self->priv->w_address));
    if (address)
        location->address = g_strstrip(g_strdup(address));
    return address != NULL;
}

static gboolean
address_filter(GtkTreeModel *model, GtkTreeIter *iter, MapAddressPicker *self)
{
    const gchar *entered;
    gchar *address;
    gboolean visible = TRUE;

    entered = gtk_entry_get_text(GTK_ENTRY(self->priv->w_address));

    gtk_tree_model_get(model, iter, 0, &address, -1);
    if (entered && address)
    {
        gint len = strlen(entered);
        visible = g_strncasecmp(entered, address, len) == 0;
    }

    g_free(address);

    return visible;
}

static void
on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                 GtkTreeViewColumn *column, MapAddressPicker *self)
{
    MapAddressPickerPrivate *priv = self->priv;
    GtkTreeIter iter;
    gchar *address;

    if (!gtk_tree_model_get_iter(priv->model, &iter, path))
        return;

    gtk_tree_model_get(priv->model, &iter, 0, &address, -1);
    if (G_UNLIKELY(!address)) return;

    gtk_entry_set_text(GTK_ENTRY(priv->w_address), address);
    g_free(address);

    gtk_dialog_response(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT);
}

static void
on_address_changed(GtkEditable *editable, MapAddressPicker *self)
{
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(self->priv->model));
}

static void
map_address_picker_set_property(GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec)
{
    MapAddressPickerPrivate *priv = MAP_ADDRESS_PICKER_PRIV(object);

    switch (property_id)
    {
    case PROP_ROUTER:
        priv->router = g_value_dup_object(value);
        break;
    case PROP_LOCATION:
        {
            const MapLocation *location = g_value_get_pointer(value);
            if (location && location->address)
                gtk_entry_set_text(GTK_ENTRY(priv->w_address),
                                   location->address);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
map_address_picker_dispose(GObject *object)
{
    MapAddressPickerPrivate *priv = MAP_ADDRESS_PICKER_PRIV(object);

    if (priv->disposed)
        return;

    priv->disposed = TRUE;

    if (priv->router)
    {
        g_object_unref(priv->router);
        priv->router = NULL;
    }

    if (priv->model)
    {
        g_object_unref(priv->model);
        priv->model = NULL;
    }

    G_OBJECT_CLASS(map_address_picker_parent_class)->dispose(object);
}

static void
map_address_picker_init(MapAddressPicker *self)
{
    MapAddressPickerPrivate *priv;
    GtkWidget *pannable, *tree_view, *done;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(self, MAP_TYPE_ADDRESS_PICKER,
                                       MapAddressPickerPrivate);
    self->priv = priv;

    priv->model = gtk_tree_model_filter_new(GTK_TREE_MODEL(_loc_model), NULL);
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(priv->model),
        (GtkTreeModelFilterVisibleFunc)address_filter, self,
        NULL);

    done = gtk_dialog_add_button(GTK_DIALOG(self),
                                 H_("wdgt_bd_done"), GTK_RESPONSE_ACCEPT);
    gtk_window_set_default(GTK_WINDOW(self), done);

    /* Address */
    priv->w_address = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_entry_set_activates_default(GTK_ENTRY(priv->w_address), TRUE);
    g_signal_connect(priv->w_address, "changed",
                     G_CALLBACK(on_address_changed), self);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(self)->vbox), priv->w_address,
                       FALSE, FALSE, 0);

    /* suggestion box */
    tree_view = hildon_gtk_tree_view_new_with_model(HILDON_UI_MODE_NORMAL,
                                                    priv->model);
    column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
                          NULL);
    renderer = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
                            "xalign", 0.0,
                            NULL);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    g_signal_connect(tree_view, "row-activated",
                     G_CALLBACK(on_row_activated), self);

    pannable = hildon_pannable_area_new();
    hildon_pannable_area_set_size_request_policy(HILDON_PANNABLE_AREA(pannable),
                                                 HILDON_SIZE_REQUEST_CHILDREN);
    gtk_container_add(GTK_CONTAINER(pannable), tree_view);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(self)->vbox), pannable,
                       TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(self));
}

static void
map_address_picker_class_init(MapAddressPickerClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(object_class, sizeof(MapAddressPickerPrivate));

    object_class->set_property = map_address_picker_set_property;
    object_class->dispose = map_address_picker_dispose;

    g_object_class_install_property(object_class, PROP_ROUTER,
        g_param_spec_object("router", "router", "router",
                            MAP_TYPE_ROUTER,
                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
                            G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_LOCATION,
        g_param_spec_pointer("location", "location", "location",
                             G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS));
}

GtkWidget *
map_address_picker_new(const gchar *title, GtkWindow *parent,
                       MapRouter *router, const MapLocation *location)
{
    return g_object_new(MAP_TYPE_ADDRESS_PICKER,
                        "title", title,
                        "modal", TRUE,
                        "has-separator", FALSE,
                        "transient-for", parent,
                        "destroy-with-parent", TRUE,
                        "router", router,
                        "location", location,
                        NULL);
}

gboolean
map_address_picker_run(const gchar *title, GtkWindow *parent,
                       MapRouter *router, MapLocation *location)
{
    GtkWidget *picker;
    gint response;
    gboolean location_set = FALSE;

    picker = map_address_picker_new(title, parent, router, location);
    response = gtk_dialog_run(GTK_DIALOG(picker));
    if (response == GTK_RESPONSE_ACCEPT)
    {
        if (map_address_picker_dup_location(MAP_ADDRESS_PICKER(picker),
                                            location))
            location_set = TRUE;
    }

    gtk_widget_destroy(picker);
    return location_set;
}

