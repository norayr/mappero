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

#ifndef MAP_ADDRESS_PICKER_H
#define MAP_ADDRESS_PICKER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <mappero/router.h>
 
G_BEGIN_DECLS

#define MAP_TYPE_ADDRESS_PICKER         (map_address_picker_get_type ())
#define MAP_ADDRESS_PICKER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MAP_TYPE_ADDRESS_PICKER, MapAddressPicker))
#define MAP_ADDRESS_PICKER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MAP_TYPE_ADDRESS_PICKER, MapAddressPickerClass))
#define MAP_IS_ADDRESS_PICKER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MAP_TYPE_ADDRESS_PICKER))
#define MAP_IS_ADDRESS_PICKER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MAP_TYPE_ADDRESS_PICKER))
#define MAP_ADDRESS_PICKER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MAP_TYPE_ADDRESS_PICKER, MapAddressPickerClass))

typedef struct _MapAddressPicker MapAddressPicker;
typedef struct _MapAddressPickerPrivate MapAddressPickerPrivate;
typedef struct _MapAddressPickerClass MapAddressPickerClass;


struct _MapAddressPicker
{
    GtkDialog parent;

    MapAddressPickerPrivate *priv;
};

struct _MapAddressPickerClass
{
    GtkDialogClass parent_class;
};

GType map_address_picker_get_type (void);

GtkWidget *map_address_picker_new(const gchar *title, GtkWindow *parent,
                                  MapRouter *router,
                                  const MapLocation *location);
gboolean map_address_picker_run(const gchar *title, GtkWindow *parent,
                                MapRouter *router, MapLocation *location);

G_END_DECLS
#endif /* MAP_ADDRESS_PICKER_H */
