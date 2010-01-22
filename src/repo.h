#ifndef __REPO_H__
#define __REPO_H__

gchar* tile_sources_to_xml(GList *tile_sources);
gchar* repositories_to_xml(GList *repositories);

GList* xml_to_tile_sources(const gchar *data);
GList* xml_to_repositories(const gchar *data);

Repository* create_default_repo_lists(GList **tile_sources,
                                      GList **repositories);

void delete_repository(Repository *repo);

void repositories_dialog();
gboolean repository_edit_dialog(Repository *repo);
gboolean tile_source_dialog(TileSource *ts);

#endif /* __REPO_H__ */
