#include "PlsManager.h"
#include "global.h"
#include <glib/gprintf.h>

GPtrArray *loadPls(const gchar *plsName)
{
	GPtrArray *retVal = NULL;
	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s/%s.pls", g_get_user_config_dir(), APPNAME, plsName);

	GKeyFile *keyFilePls = g_key_file_new ();

	gboolean keyFileFound = g_key_file_load_from_file(keyFilePls, strTemp, G_KEY_FILE_NONE, NULL);
	if (keyFileFound)
	{
		retVal = g_ptr_array_new();
		int numberofentries = g_key_file_get_integer(keyFilePls, "playlist", "numberofentries", NULL);
		int id = 1;
		PlayItem playItem;
		while (id <= numberofentries)
		{
			g_sprintf(strTemp, "Title%d", id);
			playItem.title = g_string_new(g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL));
			g_sprintf(strTemp, "File%d", id);
			playItem.url = g_string_new(g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL));
			g_ptr_array_add(retVal, (gpointer)&playItem);
			id++;
		}
	}

	g_key_file_free(keyFilePls);

	return retVal;
}