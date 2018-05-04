#include <gmodule.h>

typedef struct
{
	GString *title;
	GString *url;
} PlayItem;

GPtrArray *listPls(void);

GPtrArray *loadPls(const gchar *plsName);
void savePls(const gchar *plsName, GPtrArray *plsList);

// The array cannot be freed beacause its data is used in the sub-items callback
//void releasePlsData(GPtrArray *plsData);
