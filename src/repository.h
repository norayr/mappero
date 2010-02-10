#ifndef __REPOSITORY_H__
#define __REPOSITORY_H__

#include "types.h"

gchar* repository_list_to_xml(GList *repositories);
GList* repository_xml_to_list(const gchar *data);

Repository* repository_create_default_lists(GList **tile_sources,
                                            GList **repositories);
void repository_free(Repository *repo);
void repository_list_edit_dialog();
gboolean repository_edit_dialog(GtkWindow *parent, Repository *repo);

#endif /* __REPOSITORY_H__ */
