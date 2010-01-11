#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include "types.h"
#include "repo.h"

#include <libxml/parser.h>
#include <string.h>



inline void
append_url (xmlDocPtr doc, xmlNodePtr parent, gchar *url)
{
    xmlNodePtr n, nn;

    n = xmlNewNode(NULL, BAD_CAST "url");
    nn = xmlNewCDataBlock(doc, url, strlen(url));
    xmlAddChild(n, nn);
    xmlAddChild(parent, n);
}


inline void
append_int (xmlNodePtr parent, const gchar *name, int val)
{
    gchar *p;

    p = g_strdup_printf("%d", val);
    xmlNewChild(parent, NULL, BAD_CAST name, p);
    g_free(p);
}



inline const gchar *
repotype_to_str (RepoType type)
{
    switch (type) {
    case REPOTYPE_NONE:
        return "NONE";
    case REPOTYPE_XYZ:
        return "XYZ";
    case REPOTYPE_XYZ_SIGNED:
        return "XYZ_SIGNED";
    case REPOTYPE_XYZ_INV:
        return "XYZ_INV";
    case REPOTYPE_QUAD_QRST:
        return "QUAD_QRST";
    case REPOTYPE_QUAD_ZERO:
        return "QUAD_ZERO";
    case REPOTYPE_WMS:
        return "WMS";
    default:
        return "UNKNOWN";
    }
}


gchar*
tile_sources_to_xml (GList *tile_sources)
{
    xmlDocPtr doc;
    xmlNodePtr n, nn;
    xmlChar *xmlbuf;
    int buf_size;
    gchar *res, *p;
    TileSource *ts;

    doc = xmlNewDoc(BAD_CAST "1.0");
    n = xmlNewNode(NULL, BAD_CAST "TileSources");
    xmlDocSetRootElement(doc, n);
    
    do {
        ts =(TileSource*)tile_sources->data;
        nn = xmlNewChild(n, NULL, BAD_CAST "TileSource", NULL);

        xmlNewChild(nn, NULL, BAD_CAST "name", ts->name ? ts->name : "");
        append_url(doc, nn, ts->url ? ts->url : "");
        xmlNewChild(nn, NULL, BAD_CAST "id", ts->id ? ts->id : "");
        xmlNewChild(nn, NULL, BAD_CAST "type", repotype_to_str(ts->type));
    } while (tile_sources = g_list_next(tile_sources));

    buf_size = 0;
    xmlDocDumpFormatMemory(doc, &xmlbuf, &buf_size, 1);
    if (buf_size > 0)
        res = g_strdup(xmlbuf);
    else
        res = NULL;

    xmlFree(xmlbuf);
    xmlFreeDoc(doc);

    return res;
}


gchar*
repositories_to_xml (GList *repositories)
{
    xmlDocPtr doc;
    xmlNodePtr n, nn, nl, nll;
    Repository *repo;
    xmlChar *xmlbuf;
    int i;
    int buf_size;
    gchar *res;

    doc = xmlNewDoc(BAD_CAST "1.0");
    n = xmlNewNode(NULL, BAD_CAST "Repositories");
    xmlDocSetRootElement(doc, n);

    do {
        repo = (Repository*)repositories->data;
        nn = xmlNewChild(n, NULL, BAD_CAST "Repository", NULL);
        
        xmlNewChild(nn, NULL, BAD_CAST "name", repo->name ? repo->name : "");
        append_int(nn, "min_zoom", repo->min_zoom);
        append_int(nn, "max_zoom", repo->max_zoom);
        append_int(nn, "zoom_step", repo->zoom_step);

        nl = xmlNewNode(NULL, BAD_CAST "Layers");
        xmlAddChild(nn, nl);

        xmlNewChild(nl, NULL, BAD_CAST "Primary",(repo->primary && repo->primary->id) ? repo->primary->id : "");
        
        for (i = 0; i < repo->layers_count; i++)
            if (repo->layers[i] && repo->layers[i]->name) {
                gchar *order;
                nll = xmlNewChild(nl, NULL, BAD_CAST "Layer", repo->layers[i]->id ? repo->layers[i]->id : "");
                order = g_strdup_printf("%d", i);
                xmlNewProp(nll, BAD_CAST "order", BAD_CAST order);
                g_free(order);
            }
    } while (repositories = g_list_next(repositories));

    buf_size = 0;
    xmlDocDumpFormatMemory(doc, &xmlbuf, &buf_size, 1);
    if (buf_size > 0)
        res = g_strdup(xmlbuf);
    else
        res = NULL;

    xmlFree(xmlbuf);
    xmlFreeDoc(doc);

    return res;
}

