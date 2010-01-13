#ifndef __REPO_H__
#define __REPO_H__

gchar* tile_sources_to_xml (GList *tile_sources);
gchar* repositories_to_xml (GList *repositories);

GList* xml_to_tile_sources (gchar *data);
GList* xml_to_repositories (gchar *data);

#endif /* __REPO_H__ */
