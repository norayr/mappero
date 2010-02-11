#ifndef __TILE_SOURCE_H__
#define __TILE_SOURCE_H__

#include "types.h"

#include <hildon/hildon-touch-selector.h>

const gchar *tile_source_format_extention(TileFormat format);
const gchar *tile_source_format_name(TileFormat format);
TileFormat   tile_source_format_by_name(const gchar *name);

const TileSourceType* tile_source_type_find_by_name(const gchar *name);

gchar* tile_source_list_to_xml(GList *tile_sources);
GList* tile_source_xml_to_list(const gchar *data);

gboolean tile_source_compare(TileSource *ts1, TileSource *ts2);

void tile_source_free(TileSource *ts);

void tile_source_list_edit_dialog();
gboolean tile_source_edit_dialog(GtkWindow *parent, TileSource *ts);

void tile_source_fill_selector(HildonTouchSelector *selector, gboolean filter, gboolean transparent, TileSource *active);

#endif /* __TILE_SOURCE_H__ */