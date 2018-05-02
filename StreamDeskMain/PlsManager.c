#include "PlsManager.h"
#include "global.h"
#include <glib/gprintf.h>


//void deletePlsItem(gpointer data)
//{
//	PlayItem *playItem = (PlayItem *)data;
//	if (playItem->title->len)
//		g_string_free(playItem->title, TRUE);
//	if (playItem->url->len)
//	g_string_free(playItem->url, TRUE);
//}


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
		
		PlayItem *playItem = (PlayItem*)g_new(PlayItem, 1);
		while (id <= numberofentries)
		{
			g_sprintf(strTemp, "Title%d", id);
			playItem->title = g_string_new(g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL));
			g_sprintf(strTemp, "File%d", id);
			playItem->url = g_string_new(g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL));
			g_ptr_array_add(retVal, (gpointer)playItem);
			id++;
		}
		
//		g_ptr_array_set_free_func(retVal, deletePlsItem);
	}

	g_key_file_free(keyFilePls);

	return retVal;
}

void deleteStringItem(gpointer data)
{
	GString *stringItem = (GString *)data;
	if (stringItem->len)
		g_string_free(stringItem, TRUE);
}


GPtrArray *listPls(void)
{
	GPtrArray *retVal = NULL;
	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s/", g_get_user_config_dir(), APPNAME);

	GDir *dir;
	GError *error;
	const gchar *filename;

	dir = g_dir_open(strTemp, 0, &error);
	while ((filename = g_dir_read_name(dir)))
    printf("%s\n", filename);
		
	g_dir_close(dir);
	
	g_ptr_array_set_free_func(retVal, deleteStringItem);
	return retVal;
}


//void releasePlsData(GPtrArray *plsData)
//{
//	g_ptr_array_free(plsData, TRUE);
//}