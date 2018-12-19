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
			gchar *getKey;
			
			// Get URL field, three cases managed
			g_sprintf(strTemp, "File%d", id);
			getKey = g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL);
			if (getKey == NULL)
			{
				g_sprintf(strTemp, "file%d", id);
				getKey = g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL);
				if (getKey == NULL)
				{
					g_sprintf(strTemp, "FILE%d", id);
					getKey = g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL);
				}
			}
			if (getKey == NULL)
				continue;
			playItem->url = g_string_new(getKey);
			
			// Get Title field, three cases managed
			g_sprintf(strTemp, "Title%d", id);
			getKey = g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL);
			if (getKey == NULL)
			{
				g_sprintf(strTemp, "title%d", id);
				getKey = g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL);
				if (getKey == NULL)
				{
					g_sprintf(strTemp, "TITLE%d", id);
					getKey = g_key_file_get_string(keyFilePls, "playlist", strTemp, NULL);
				}
			}
			if (getKey == NULL)
				playItem->title = playItem->url;
			else
				playItem->title = g_string_new(getKey);
			
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

GPtrArray *loadM3uGfile(GFile *m3uFile)
{
	GPtrArray *retVal = g_ptr_array_new();
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
			else
			{
				PlayItem *playItem = (PlayItem*)g_new(PlayItem, 1);
				playItem->title = g_string_new(readLine);
				playItem->url = g_string_new(readLine);
				g_ptr_array_add(retVal, (gpointer)playItem);
			}

			readLine = g_data_input_stream_read_line(dis, &numReadChar, NULL, NULL);
		}
		g_object_unref(fis);
	}
	g_ptr_array_set_free_func(retVal, deletePlsItem);
	
	g_object_unref(m3uFile);

	return retVal;

}


//StreamType checkStreamType(const gchar *streamUrl)
//{
//	StreamType retVal = E_OTHER;
//	
//	g_print("Analizzo: %s\n", streamUrl);
//	
//	if (g_str_has_prefix(streamUrl, "file:"))
//		retVal = E_FILE;
//	else if (g_str_has_prefix(streamUrl, "http")) // including https
//	{		
//		GFile *m3uUrl = g_file_new_for_uri(streamUrl);
//		GFileInputStream *fis = g_file_read(m3uUrl, NULL, NULL);
//		
//		if (fis != NULL)
//		{
//			GDataInputStream *dis = g_data_input_stream_new((GInputStream *)fis);
//			gsize numReadChar;
//			char *readLine = g_data_input_stream_read_line(dis, &numReadChar, NULL, NULL);
//			if (readLine != NULL)
//			{
//				if (g_str_has_prefix(readLine, "[Playlist]"))
//					retVal = E_PLS;
//				else if (g_str_has_prefix(readLine, "#EXTM3U"))
//					retVal = E_M3U;
//				else
//					g_print("* Unknown stream format: %s\n", readLine); // only for console debug
//			}
//			g_object_unref(fis);
//		}
//	}
//	else
//		g_print("* Unknown stream type: %s\n", streamUrl); // only for console debug
//
//	return retVal;
//}


GPtrArray *loadM3uFromFile(const gchar *plsDir, const gchar *plsName)
{
	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s", plsDir, plsName);
	GFile *m3uFile = g_file_new_for_path(strTemp);
	return loadM3uGfile(m3uFile);
}


GPtrArray *loadM3uFromHttp(const gchar *httpUrl)
{
	// playing http streams may issue the following error in console:
	// libsoup-CRITICAL **: soup_message_headers_append: assertion 'strpbrk (value, "\r\n") == NULL' failed
	GFile *m3uUrl = g_file_new_for_uri(httpUrl);
	return loadM3uGfile(m3uUrl);
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
			gint suffixLen = strlen(".m3u");
			if (g_str_has_suffix (filename, ".m3u8"))
				suffixLen = strlen(".m3u8");
				
			PlayList *playList = (PlayList*)g_new(PlayList, 1);
			playList->name = g_string_new_len(filename, strlen(filename)-suffixLen);
			playList->items = loadM3uFromFile(plsDir, filename);
//			playList->items = loadM3uFromHttp("http://pastebin.com/raw/8GpCCkhf");
			g_ptr_array_add(retVal, (gpointer)playList);
		}
		
	g_dir_close(dir);
	
	g_ptr_array_set_free_func(retVal, deletePlayListItem);
	return retVal;
}
