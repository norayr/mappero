#ifndef __REPO_H__
#define __REPO_H__

gchar* tile_sources_to_xml(GList *tile_sources);
gchar* repositories_to_xml(GList *repositories);

GList* xml_to_tile_sources(const gchar *data);
GList* xml_to_repositories(const gchar *data);

Repository* create_default_repo_lists(GList **tile_sources,
                                      GList **repositories);

gboolean compare_tile_sources(TileSource *ts1, TileSource *ts2);
gboolean compare_repositories(Repository *repo1, Repository *repo2);

void free_tile_source(TileSource *ts);
void free_repository(Repository *repo);

void delete_repository(Repository *repo);

void repositories_dialog();
gboolean repository_edit_dialog(GtkWindow *parent, Repository *repo);
gboolean tile_source_dialog(TileSource *ts);

#endif /* __REPO_H__ */
