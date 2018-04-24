#include <gmodule.h>

typedef struct
{
	GString *title;
	GString *url;
} PlayItem;


GPtrArray *loadPls(const gchar *plsName);
