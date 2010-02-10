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
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif
#include "error.h"

#include <hildon/hildon-note.h>

void
map_error_show(GtkWindow *parent, const GError *error)
{
    GtkWidget *dialog;

    dialog = hildon_note_new_information(parent, error->message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void
map_error_show_and_clear(GtkWindow *parent, GError **error)
{
    if (G_LIKELY(error))
        map_error_show(parent, *error);
    g_clear_error(error);
}

GQuark
map_error_quark(void)
{
    static gsize quark = 0;

    if (g_once_init_enter(&quark))
    {
        GQuark domain = g_quark_from_static_string("map-error");

        g_assert(sizeof(GQuark) <= sizeof(gsize));

        g_once_init_leave(&quark, domain);
    }
    return (GQuark) quark;
}

