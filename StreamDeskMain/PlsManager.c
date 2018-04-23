#include "PlsManager.h"
#include "global.h"

GPtrArray *loadPls(const gchar *plsName)
{
	gchar plsFile[255];
	g_sprintf(plsFile, "%s/%s/%s", g_get_user_config_dir(), APPNAME, plsName);

	GKeyFile *keyFilePls = g_key_file_new ();

	gboolean keyFileFound = g_key_file_load_from_file(keyFilePls, plsFile, G_KEY_FILE_NONE, NULL);
	if (keyFileFound)


	g_key_file_free(keyFilePls);

	return NULL;
}