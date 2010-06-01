/*
 * Copyright (C) 2010 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Mappero.
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif

#include "debug.h"
#include "loader.h"

#include <gmodule.h>
#include <string.h>

#define MAP_PLUGIN_LIST_OBJECTS_S "map_plugin_list_objects"
#define MAP_PLUGIN_GET_OBJECT_S "map_plugin_get_object"

typedef const gchar * const * (*MapPluginListObjects)(void);
typedef GObject * (*MapPluginGetObject)(const gchar *);

static GList *all_objects = NULL;

static gboolean
map_loader_get_plugin_symbols(GModule *module,
                              MapPluginListObjects *list_objects,
                              MapPluginGetObject *get_object)
{
    gpointer symbol;

    if (!g_module_symbol(module, MAP_PLUGIN_LIST_OBJECTS_S, &symbol)) {
        g_warning("%s: missing symbol %s", g_module_name(module),
                  MAP_PLUGIN_LIST_OBJECTS_S);
        return FALSE;
    }
    
    *list_objects = symbol;
    
    if (!g_module_symbol(module, MAP_PLUGIN_GET_OBJECT_S, &symbol)) {
        g_warning("%s: missing symbol %s", g_module_name(module),
                  MAP_PLUGIN_GET_OBJECT_S);
        return FALSE;
    }
    
    *get_object = symbol;

    return TRUE;
}

void
map_loader_read_dir(const gchar *path)
{
    GError *error = NULL;
    gchar filename[1024], *ptr;
    const gchar *entry;
    gint path_len;
    GDir *dir;

    dir = g_dir_open(path, 0, &error);
    if (G_UNLIKELY(!dir)) {
        g_warning("%s: could not open plugin directory %s: %s",
                  G_STRFUNC, path, error->message);
        g_error_free(error);
        return;
    }

    path_len = g_snprintf(filename, sizeof(filename),
                          "%s" G_DIR_SEPARATOR_S, path);
    ptr = filename + path_len;
    while ((entry = g_dir_read_name(dir)) != NULL)
    {
        GModule *module;
        MapPluginListObjects list_objects;
        MapPluginGetObject get_object;

        if (entry[0] == '.' || !g_str_has_suffix(entry, "." G_MODULE_SUFFIX))
            continue;

        strncpy(ptr, entry, sizeof(filename) - path_len);
        module = g_module_open(filename, G_MODULE_BIND_LOCAL);
        if (G_UNLIKELY(!module))
        {
            g_warning("%s: %s", entry, g_module_error());
            continue;
        }

        if (map_loader_get_plugin_symbols(module,
                                          &list_objects,
                                          &get_object))
        {
            const gchar * const *object_ids;
            gint i;

            g_module_make_resident(module);

            object_ids = list_objects();
            for (i = 0; object_ids[i] != NULL; i++)
            {
                DEBUG("%s: load object %s", entry, object_ids[i]);
                all_objects = g_list_prepend(all_objects,
                                             get_object(object_ids[i]));
            }
        }
        else
        {
            g_module_close(module);
        }
    }
}

GList *map_loader_get_objects()
{
    return all_objects;
}

