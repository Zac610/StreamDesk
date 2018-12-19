#include <gmodule.h>

/*typedef enum
{
	E_M3U,
	E_PLS,
	E_FILE,
	E_OTHER
} StreamType;*/

typedef struct
{
	GString *name;
	GPtrArray *items;
} PlayList;

typedef struct
{
	GString *title;
	GString *url;
} PlayItem;

//StreamType checkStreamType(const gchar *streamUrl);

GPtrArray *loadAllPlaylists(const gchar *plsDir);

GPtrArray *loadPls(const gchar *plsDir, const gchar *plsName);
void savePls(const gchar *plsName, GPtrArray *plsList);

GPtrArray *loadM3uFromHttp(const gchar *httpUrl);

// The array cannot be freed beacause its data is used in the sub-items callback
//void releasePlsData(GPtrArray *plsData);
