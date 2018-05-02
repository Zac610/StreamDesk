#include <gmodule.h>

typedef struct
{
	GString *title;
	GString *url;
} PlayItem;


GPtrArray *loadPls(const gchar *plsName);

GPtrArray *listPls(void);


// The array cannot be freed beacause its data is used in the sub-items callback
//void releasePlsData(GPtrArray *plsData);