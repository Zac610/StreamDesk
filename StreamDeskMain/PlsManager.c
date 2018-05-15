#include "PlsManager.h"
#include "global.h"
#include <glib/gprintf.h>
#include <string.h>

#define MAX_LOCAL_ENTRIES 10
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
	GPtrArray *retVal = g_ptr_array_new();

	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s/%s.pls", g_get_user_config_dir(), APPNAME, plsName);

	GKeyFile *keyFilePls = g_key_file_new ();

	gboolean keyFileFound = g_key_file_load_from_file(keyFilePls, strTemp, G_KEY_FILE_NONE, NULL);
	if (keyFileFound)
	{
		int numberofentries = g_key_file_get_integer(keyFilePls, "playlist", "numberofentries", NULL);
		int id = 1;
		
		while (id <= numberofentries)
		{
			PlayItem *playItem = (PlayItem*)g_new(PlayItem, 1);
			
			g_sprintf(strTemp, "Title%d", id);
			playItem->title = g_string_new(g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL));
			g_sprintf(strTemp, "File%d", id);
			playItem->url = g_string_new(g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL));
//			if (playItem->url->len)
			g_ptr_array_add(retVal, (gpointer)playItem);
			id++;
		}
	}
//	else
//	{
//		PlayItem *playItem = (PlayItem*)g_new(PlayItem, 1);
//		playItem->title = g_string_new("<empty>");
//		playItem->url = g_string_new("");
//		g_ptr_array_add(retVal, (gpointer)playItem);
//	}

	g_key_file_free(keyFilePls);

//		g_ptr_array_set_free_func(retVal, deletePlsItem);
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
	GPtrArray *retVal = g_ptr_array_new();
	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s/", g_get_user_config_dir(), APPNAME);

	GDir *dir;
	GError *error;
	const gchar *filename;

	dir = g_dir_open(strTemp, 0, &error);
	while ((filename = g_dir_read_name(dir)))
		if (g_str_has_suffix (filename, ".pls"))
		{
			if (g_strcmp0(filename, "Local.pls") == 0)
				continue;
			GString *item = g_string_new_len(filename, strlen(filename)-strlen(".pls"));
			g_ptr_array_add(retVal, (gpointer)item);
		}
		
	g_dir_close(dir);
	
	g_ptr_array_set_free_func(retVal, deleteStringItem);
	return retVal;
}

int gSavePlsCounter;
void savePlayItem(PlayItem *playItem, gpointer user_data)
{
	GKeyFile *keyFileIni = (GKeyFile *)user_data;
	
	gchar strTemp[16];
	g_sprintf(strTemp, "File%d", gSavePlsCounter);
	g_key_file_set_string(keyFileIni, "playlist", strTemp, playItem->url->str);

	g_sprintf(strTemp, "Title%d", gSavePlsCounter);
	g_key_file_set_string(keyFileIni, "playlist", strTemp, playItem->title->str);

	gSavePlsCounter++;
}


void savePls(const gchar *plsName, GPtrArray *plsList)
{
	gchar plsFile[255];
	g_sprintf(plsFile, "%s/%s/%s.pls", g_get_user_config_dir(), APPNAME, plsName);

	g_autoptr(GKeyFile) keyFileIni = g_key_file_new ();

	guint numEntries = plsList->len;
	if (numEntries > MAX_LOCAL_ENTRIES)
		numEntries = MAX_LOCAL_ENTRIES;

	g_key_file_set_integer(keyFileIni, "playlist", "numberofentries", numEntries);

	gSavePlsCounter = 1;
	g_ptr_array_foreach(plsList, (GFunc)savePlayItem, keyFileIni);

	g_key_file_set_integer(keyFileIni, "playlist", "Version", 2);

	g_key_file_save_to_file (keyFileIni, plsFile, NULL);

}

//void releasePlsData(GPtrArray *plsData)
//{
//	g_ptr_array_free(plsData, TRUE);
//}