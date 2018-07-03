#include "PlsManager.h"
#include "global.h"
#include <glib/gprintf.h>
#include <string.h>
#include <gio/gio.h>

#define MAX_LOCAL_ENTRIES 10

gint gSavePlsCounter;

void deletePlsItem(gpointer data)
{
	PlayItem *playItem = (PlayItem *)data;
	if (playItem->title->len)
		g_string_free(playItem->title, TRUE);
	if (playItem->url->len)
	g_string_free(playItem->url, TRUE);
}


GPtrArray *loadPls(const gchar *plsDir, const gchar *plsName)
{
	GPtrArray *retVal = g_ptr_array_new();

	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s", plsDir, plsName);

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
			g_ptr_array_add(retVal, (gpointer)playItem);
			id++;
		}
	}
	g_ptr_array_set_free_func(retVal, deletePlsItem);
	g_key_file_free(keyFilePls);

	return retVal;
}


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


GPtrArray *loadM3u(const gchar *plsDir, const gchar *plsName)
{
	GPtrArray *retVal = g_ptr_array_new();
	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s", plsDir, plsName);
	GFile *m3uFile = g_file_new_for_path(strTemp);
	GFileInputStream *fis = g_file_read(m3uFile, NULL, NULL);
	
	if (fis != NULL)
	{
		GDataInputStream *dis = g_data_input_stream_new((GInputStream *)fis);
		gsize numReadChar;
		gchar itemTitle[128];
		char *readLine = g_data_input_stream_read_line(dis, &numReadChar, NULL, NULL);
		while (readLine != NULL)
		{
			if (g_str_has_prefix(readLine, "#EXTINF"))
			{
				sscanf(readLine, "%*[^,],%[^\n]", itemTitle);
				readLine = g_data_input_stream_read_line(dis, &numReadChar, NULL, NULL);
				if (readLine != NULL)
				{
					PlayItem *playItem = (PlayItem*)g_new(PlayItem, 1);
					playItem->title = g_string_new(itemTitle);
					playItem->url = g_string_new(readLine);
					g_ptr_array_add(retVal, (gpointer)playItem);
				}
			}

			readLine = g_data_input_stream_read_line(dis, &numReadChar, NULL, NULL);
		}
		g_object_unref(fis);
	}
	g_ptr_array_set_free_func(retVal, deletePlsItem);
	
	g_object_unref(m3uFile);

	return retVal;
}


void deletePlayListItem(gpointer data)
{
	PlayList *item = (PlayList *)data;
	if (item->name->len)
		g_string_free(item->name, TRUE);
	g_ptr_array_free(item->items, TRUE);
}


GPtrArray *loadAllPlaylists(const gchar *plsDir)
{
	GPtrArray *retVal = g_ptr_array_new();

	GDir *dir;
	GError *error;
	const gchar *filename;

	dir = g_dir_open(plsDir, 0, &error);
	while ((filename = g_dir_read_name(dir)))
		if (g_str_has_suffix (filename, ".pls"))
		{
			if (g_strcmp0(filename, "Local.pls") == 0)
				continue;
			PlayList *playList = (PlayList*)g_new(PlayList, 1);
			playList->name = g_string_new_len(filename, strlen(filename)-strlen(".pls"));
			playList->items = loadPls(plsDir, filename);
			g_ptr_array_add(retVal, (gpointer)playList);
		}
		else if (g_str_has_suffix (filename, ".m3u") || g_str_has_suffix (filename, ".m3u8")) // Case sensitive?
		{
			gint suffixLen = 3;
			if (g_str_has_suffix (filename, ".m3u8"))
				suffixLen = 4;
				
			PlayList *playList = (PlayList*)g_new(PlayList, 1);
			playList->name = g_string_new_len(filename, strlen(filename)-suffixLen);
			playList->items = loadM3u(plsDir, filename);
			g_ptr_array_add(retVal, (gpointer)playList);
		}
		
	g_dir_close(dir);
	
	g_ptr_array_set_free_func(retVal, deletePlayListItem);
	return retVal;
}
