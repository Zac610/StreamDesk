// Application ID: com.github.zac610.streamdesk
// gcc StreamDesk.c -o StreamDesk `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-3.0 gstreamer-1.0`
//
// This app can play audio and video streams on the desktop.
// The only audio function makes this app very similiar to Radio Tray [http://radiotray.sourceforge.net/]
#include <string.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

#include <libappindicator/app-indicator.h>

#include "icon.xpm.h"
#include "PlsManager.h"
#include "global.h"

static const gchar CONFFILENAME[] = "settings.ini";

static const int RESIZEBORDER = 20;

enum DragState
{
	E_NONE,
	E_STARTING,
	E_DRAGGING,
	E_RESIZING
} dragging = E_NONE;

gboolean gIsPlaying; // TODO: check if can be used state of playbin
gchar gLastStream[255] = ""; // TODO: check if can be used uri of playbin
gint gxWinInitial, gyWinInitial;
gint gwWinInitial, ghWinInitial;
GtkWindow *gMainWindow;
GtkWidget *gContextualMenu;
GstElement *gPlaybin;
gint gNVideo = 0;
GPtrArray *gLocalPlayItemList;

struct MyData
{
	GtkWidget *dialog;
	GtkWidget *entry;
};

/* This function is called when the GUI toolkit creates the physical window that will hold the video.
 * At this point we can retrieve its handler (which has a different meaning depending on the windowing system)
 * and pass it to GStreamer through the VideoOverlay interface. */
static void realize_cb (GtkWidget *widget)
{
	GdkWindow *window = gtk_widget_get_window (widget);
	guintptr window_handle;

	if (!gdk_window_ensure_native (window))
		g_error ("Couldn't create native window needed for GstVideoOverlay!");

	// Retrieve window handler from GDK
#if defined (GDK_WINDOWING_WIN32)
	window_handle = (guintptr)GDK_WINDOW_HWND (window);
#elif defined (GDK_WINDOWING_QUARTZ)
	window_handle = gdk_quartz_window_get_nsview (window);
#elif defined (GDK_WINDOWING_X11)
	window_handle = GDK_WINDOW_XID (window);
#endif
	
	// Pass it to playbin, which implements VideoOverlay and will forward it to the video sink
	gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (gPlaybin), window_handle);
}


static void playPlaybin()
{
	GstStateChangeReturn ret = gst_element_set_state (gPlaybin, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		gchar *strval;
		g_object_get(gPlaybin, "uri", &strval, NULL);
		
		GtkWidget* dialog = gtk_message_dialog_new (gMainWindow, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Stream not found:\n%s", strval);
		gtk_dialog_run(GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}


static void setCursor(GtkWidget *widget, const char *cursorName)
{
	GdkDisplay *display = gdk_display_get_default ();
	g_autoptr(GdkCursor) cursor = gdk_cursor_new_from_name (display, cursorName);
	GdkWindow *window = gtk_widget_get_window (widget);

	gdk_window_set_cursor(window, cursor);
}


// This function is called when the main window is closed
static void cbCloseApp (GtkWidget *widget, GdkEvent *event)
{
	// Save app state
	g_autoptr(GKeyFile) keyFileIni = g_key_file_new ();
	g_key_file_set_boolean(keyFileIni, "Global", "autostart", gIsPlaying);

	g_key_file_set_string(keyFileIni, "Global", "lastStream", gLastStream);

	GdkWindow *window = gtk_widget_get_window ((GtkWidget *)gMainWindow);
	gdk_window_get_origin(window, &gxWinInitial, &gyWinInitial);
	g_key_file_set_integer(keyFileIni, "Global", "initialX", gxWinInitial);
	g_key_file_set_integer(keyFileIni, "Global", "initialY", gyWinInitial);

	gwWinInitial = gdk_window_get_width(window);
	ghWinInitial = gdk_window_get_height(window);
	g_key_file_set_integer(keyFileIni, "Global", "initialW", gwWinInitial);
	g_key_file_set_integer(keyFileIni, "Global", "initialH", ghWinInitial);

	gchar confFile[255];
	sprintf(confFile, "%s/%s/%s", g_get_user_config_dir(), APPNAME, CONFFILENAME);
	g_key_file_save_to_file (keyFileIni, confFile, NULL);

	// save local playlist
	savePls("Local", gLocalPlayItemList);

	gst_element_set_state (gPlaybin, GST_STATE_READY);
	gtk_main_quit ();
}


void cbBrowseButtonClicked (GtkButton *button, gpointer parent_window)
{
	struct MyData *data = (struct MyData *)parent_window;
	
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Open File",
                                      GTK_WINDOW(data->dialog),
                                      action,
                                      ("_Cancel"),
                                      GTK_RESPONSE_CANCEL,
                                      ("_Open"),
                                      GTK_RESPONSE_ACCEPT,
                                      NULL);

	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		sprintf(gLastStream, "file:%s", gtk_file_chooser_get_filename (chooser));
		gtk_entry_set_text(GTK_ENTRY(data->entry), gLastStream);
	}

	gtk_widget_destroy (dialog);
}


void playLastStream()
{
	if (gIsPlaying)
		gst_element_set_state (gPlaybin, GST_STATE_READY);
	g_object_set (gPlaybin, "uri", gLastStream, NULL);
	playPlaybin();
}


void cbOpenUrl(GtkMenuItem *menuitem)
{  
	GtkWidget * dialog = gtk_dialog_new_with_buttons("Open URL", gMainWindow, GTK_DIALOG_MODAL,
		"Open", GTK_RESPONSE_ACCEPT, "Cancel", GTK_RESPONSE_CANCEL, NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	
	GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	
	GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_container_add(GTK_CONTAINER (content_area), layout);
	GtkWidget *entry = gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(entry), gLastStream);
	gtk_container_add (GTK_CONTAINER (layout), entry);
	GtkWidget *browseButton = gtk_button_new_with_label("...");
	struct MyData data = {dialog, entry};
	g_signal_connect(G_OBJECT (browseButton), "clicked", G_CALLBACK(cbBrowseButtonClicked), &data);

	gtk_container_add (GTK_CONTAINER (layout), browseButton);
	gtk_widget_show_all(dialog);
		
	GtkResponseType response = gtk_dialog_run(GTK_DIALOG (dialog));
	if (response == GTK_RESPONSE_ACCEPT)
	{
		strcpy(gLastStream, gtk_entry_get_text(GTK_ENTRY(entry)));
		playLastStream();
		
		// add stream in local playlist
		PlayItem *playItem = (PlayItem*)g_new(PlayItem, 1);
		playItem->title = g_string_new(gLastStream);
		playItem->url = g_string_new(gLastStream);
		g_ptr_array_insert(gLocalPlayItemList, 0, playItem);
	}
	gtk_widget_destroy (entry);
	gtk_widget_destroy (dialog);
}				


void cbPlayUrl(GtkMenuItem *menuitem, gpointer data)
{
	if (data)
	{
		strcpy(gLastStream, data);
		playLastStream();		
	}
}		 


void cbAbout(GtkMenuItem *menuitem)
{
	gchar* authors[] = {"Sergio Lo Cascio", NULL};
	gchar comments[255];
	sprintf(comments, "Play audio/video data on your desktop.\n\nPlaylist directory is in %s/%s", g_get_user_config_dir(), APPNAME);
	GdkPixbuf *logo = gdk_pixbuf_new_from_xpm_data (icon);
	gtk_show_about_dialog (gMainWindow,
												"program-name", "StreamDesk",
												"logo", logo,
												"title", "Program Information",
												"authors", authors, 
												"comments", comments, 
												"website", "https://github.com/Zac610/StreamDesk", 
												"license-type", GTK_LICENSE_GPL_3_0,
												NULL);
}


static void cbButtonPressEvent(GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *ev = (GdkEventButton *)event;
	if (ev->button == 1) // left mouse button
		dragging = E_STARTING;
	else if (ev->button == 3) // right mouse button
		gtk_menu_popup((GtkMenu *)gContextualMenu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


static void cbButtonReleaseEvent(GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *ev = (GdkEventButton *)event;
	if (ev->button == 1) // left mouse button
	{
		dragging = E_NONE;
		setCursor(widget, "default");
	}
}


static void cbMotionEvent(GtkWidget *widget, GdkEvent *event)
{
	GdkEventMotion *ev = (GdkEventMotion *)event;

	if (dragging == E_STARTING)
	{
		GdkWindow *window = gtk_widget_get_window (widget);
		gdk_window_get_origin(window, &gxWinInitial, &gyWinInitial);
		gwWinInitial = gdk_window_get_width(window);
		ghWinInitial = gdk_window_get_height(window);

		gint xinitial, yinitial;
		xinitial = (int)ev->x_root;
		yinitial = (int)ev->y_root;

		char cursorName[16] = "move";
		dragging = E_DRAGGING;
		if (xinitial > gxWinInitial+gwWinInitial-RESIZEBORDER)
		{
			strcpy(cursorName, "e-resize");
			dragging = E_RESIZING;
		}
		if (yinitial > gyWinInitial+ghWinInitial-RESIZEBORDER)
		{
			if (dragging == E_RESIZING)
				strcpy(cursorName, "se-resize");
			else
			{
				strcpy(cursorName, "s-resize");
				dragging = E_RESIZING;
			}
		}

		setCursor(widget, cursorName);
		
		// optimization
		gxWinInitial-=xinitial;
		gyWinInitial-=yinitial;
		gwWinInitial-=xinitial;
		ghWinInitial-=yinitial;
	}

	if (dragging == E_DRAGGING)
	{
		GdkWindow *window = gtk_widget_get_window (widget);
		gdk_window_move(window, gxWinInitial+(int)ev->x_root, gyWinInitial+(int)ev->y_root);
	}
	else if (dragging == E_RESIZING)
	{
		GdkWindow *window = gtk_widget_get_window (widget);
		gdk_window_resize(window, gwWinInitial+(int)ev->x_root, ghWinInitial+(int)ev->y_root);
	}
}


static void cbKeyPressEvent (GtkWidget *widget, GdkEvent *event)
{
	GdkEventKey *ev = (GdkEventKey *)event;

	switch (ev->keyval)
	{
		case GDK_KEY_Escape:
			cbCloseApp(widget, event);
			break;
		case GDK_KEY_space:
			if (gIsPlaying)
			{
				// pause stream
				gIsPlaying = FALSE;
				gst_element_set_state (gPlaybin, GST_STATE_PAUSED);
			}
			else
			{
				playPlaybin();
				gIsPlaying = TRUE;
			}
			break;
	}
}


GtkWidget *addMenuItem(const gchar *label, GtkWidget *menu_shell, GCallback c_handler, gpointer data)
{
	GtkWidget *menuItem = gtk_menu_item_new_with_label(label);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_shell), menuItem);
	if (c_handler)
		g_signal_connect(G_OBJECT (menuItem), "activate", c_handler, data);
	return menuItem;
}


void playItemAdd(PlayItem *playItem, gpointer user_data)
{
	addMenuItem(playItem->title->str, user_data, G_CALLBACK (cbPlayUrl), playItem->url->str);
}


void addPlsSubMenu(const gchar* plsName, GtkWidget *streamsSubMenu)
{
	GPtrArray *playItemList = loadPls(plsName);
	GtkWidget *localSubMenu = gtk_menu_new();
	GtkWidget *localMenuItem = addMenuItem(plsName, streamsSubMenu, NULL, NULL);
	gtk_menu_item_set_submenu( GTK_MENU_ITEM(localMenuItem), localSubMenu);
	g_ptr_array_foreach(playItemList, (GFunc)playItemAdd, localSubMenu);
}


void playListAdd(GString *plsName, gpointer user_data)
{
	addPlsSubMenu(plsName->str, (GtkWidget *)user_data);
}


/* This function is called everytime the video window needs to be redrawn (due to damage/exposure,
 * rescaling, etc). GStreamer takes care of this in the PAUSED and PLAYING states, otherwise,
 * we simply draw a black rectangle to avoid garbage showing up. */
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr)
{
	if (gNVideo)
		return FALSE; // redrawn not needed
		
	GtkAllocation allocation;

	/* Cairo is a 2D graphics library which we use here to clean the video window.
	 * It is used by GStreamer for other reasons, so it will always be available to us. */
	gtk_widget_get_allocation (widget, &allocation);
	cairo_set_source_rgb (cr, 77./255., 61./255., 77./255.);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);

	GdkPixbuf *iconBuf = gdk_pixbuf_new_from_xpm_data (icon);
	GdkWindow *window = gtk_widget_get_window (widget);
	cairo_surface_t *imageSurface = gdk_cairo_surface_create_from_pixbuf(iconBuf, 0, window);
	int xPos = (allocation.width  - gdk_pixbuf_get_width(iconBuf))/2;
	int yPos = (allocation.height - gdk_pixbuf_get_height(iconBuf))/2;
	cairo_set_source_surface(cr, imageSurface, xPos, yPos);
	cairo_paint(cr);

  return FALSE;
}


static void create_ui()
{
	// This creates all the GTK+ widgets that compose our application, and registers the callbacks
	gMainWindow = (GtkWindow *)gtk_window_new (GTK_WINDOW_TOPLEVEL); //GTK_WINDOW_POPUP
	g_signal_connect (G_OBJECT (gMainWindow), "delete-event", G_CALLBACK (cbCloseApp), NULL);
	g_signal_connect (G_OBJECT (gMainWindow), "button-press-event", G_CALLBACK (cbButtonPressEvent), NULL);
	g_signal_connect (G_OBJECT (gMainWindow), "button-release-event", G_CALLBACK (cbButtonReleaseEvent), NULL);
	g_signal_connect (G_OBJECT (gMainWindow), "motion-notify-event", G_CALLBACK (cbMotionEvent), NULL);
	g_signal_connect (G_OBJECT (gMainWindow), "key-press-event", G_CALLBACK (cbKeyPressEvent), NULL);

	GtkWidget *video_window; // The drawing area where the video will be shown
	video_window = gtk_drawing_area_new ();
	g_signal_connect (video_window, "realize", G_CALLBACK (realize_cb), NULL);
	g_signal_connect (video_window, "draw", G_CALLBACK (draw_cb), NULL);

	gtk_container_add (GTK_CONTAINER (gMainWindow), video_window);

	gtk_window_set_default_size (GTK_WINDOW (gMainWindow), gwWinInitial, ghWinInitial);

	gtk_window_set_decorated(gMainWindow, FALSE);
	gtk_widget_show_all ((GtkWidget *)gMainWindow);

	gtk_window_move(gMainWindow, gxWinInitial, gyWinInitial);

	// Contextual menu
	gContextualMenu = gtk_menu_new();

	addMenuItem("Open URL...", gContextualMenu, G_CALLBACK (cbOpenUrl), NULL);

	GtkWidget *streamsMenuItem = addMenuItem("Streams", gContextualMenu, NULL, NULL);
	GtkWidget *streamsSubMenu = gtk_menu_new();
	gtk_menu_item_set_submenu( GTK_MENU_ITEM(streamsMenuItem), streamsSubMenu);
	
	// Fills the local submenu
	addPlsSubMenu("Local", streamsSubMenu);

	GPtrArray *plsList = listPls();
	g_ptr_array_foreach(plsList, (GFunc)playListAdd, streamsSubMenu);
	g_ptr_array_free(plsList, TRUE);

	addMenuItem("Info", gContextualMenu, G_CALLBACK (cbAbout), NULL);	
	addMenuItem("Quit", gContextualMenu, G_CALLBACK (cbCloseApp), NULL);	

	gtk_widget_show_all(gContextualMenu);
 
	gLocalPlayItemList = loadPls("Local");
	
	// Icon on tray bar
	gchar strTemp[255];
	g_sprintf(strTemp, "%s/%s/", g_get_user_config_dir(), APPNAME);

	AppIndicator *indicator = app_indicator_new_with_path("sd", "icon", APP_INDICATOR_CATEGORY_APPLICATION_STATUS, strTemp);
	app_indicator_set_status (indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_attention_icon (indicator, "indicator-messages-new");
	app_indicator_set_menu (indicator, GTK_MENU (gContextualMenu)); // this instruction issues the following error:
	// Gdk-CRITICAL **: gdk_window_thaw_toplevel_updates: assertion 'window->update_and_descendants_freeze_count > 0' failed
	
	// no presence in task bar
	gtk_window_set_skip_taskbar_hint(gMainWindow, TRUE);
}


// This function is called when an error message is posted on the bus
static void error_cb (GstBus *bus, GstMessage *msg)
{
	GError *err;
	gchar *debug_info;

	// Print error details on the screen
	gst_message_parse_error (msg, &err, &debug_info);
	g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	g_clear_error (&err);
	g_free (debug_info);

	// Set the pipeline to READY (which stops playback)
	gst_element_set_state (gPlaybin, GST_STATE_READY);
}


/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg)
{
	g_print ("End-Of-Stream reached.\n");
	gst_element_set_state (gPlaybin, GST_STATE_READY);
}


// This function is called when new metadata is discovered in the stream
static void tags_cb (GstElement *playbin, gint stream)
{
	gint oldVideo = gNVideo;
	g_object_get (gPlaybin, "n-video", &gNVideo, NULL);
	if (oldVideo != gNVideo)
		gtk_widget_queue_draw((GtkWidget *)gMainWindow);
}


int main(int argc, char *argv[])
{
	GstBus *bus;

	/* Initialize GTK */
	gtk_init (&argc, &argv);

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	/* Create the elements */
	gPlaybin = gst_element_factory_make ("playbin", "playbin");

	if (!gPlaybin)
	{
		g_printerr ("Not all elements could be created.\n");
		return -1;
	}

	// Access the configuration file
	GKeyFile *keyFileIni = g_key_file_new ();
	gchar confFile[255];
	sprintf(confFile, "%s/%s/%s", g_get_user_config_dir(), APPNAME, CONFFILENAME);
	gboolean keyFileFound = g_key_file_load_from_file(keyFileIni, confFile, G_KEY_FILE_NONE, NULL);
	if (keyFileFound)
	{
		gIsPlaying = g_key_file_get_boolean(keyFileIni, "Global", "autostart", NULL);
		gchar *ret = g_key_file_get_string(keyFileIni, "Global", "lastStream", NULL);
		if (ret != NULL)
			strcpy(gLastStream, ret);
		gxWinInitial = g_key_file_get_integer(keyFileIni, "Global", "initialX", NULL);
		gyWinInitial = g_key_file_get_integer(keyFileIni, "Global", "initialY", NULL);
		gwWinInitial = g_key_file_get_integer(keyFileIni, "Global", "initialW", NULL);
		ghWinInitial = g_key_file_get_integer(keyFileIni, "Global", "initialH", NULL);
	}
	else
	{
		sprintf(confFile, "%s/%s", g_get_user_config_dir(), APPNAME);
		g_autoptr(GFile) gFile;
		gFile = g_file_new_for_path (confFile);
		g_file_make_directory (gFile, NULL, NULL);

		// Default values
		gIsPlaying = TRUE;
		gxWinInitial = 0;
		gyWinInitial = 0;
		gwWinInitial = 320;
		ghWinInitial = 200;
		strcpy(gLastStream, "http://ubuntu.hbr1.com:19800/trance.ogg");
	}
	g_key_file_free(keyFileIni);

  g_signal_connect (G_OBJECT (gPlaybin), "video-tags-changed", (GCallback) tags_cb, NULL);
  g_signal_connect (G_OBJECT (gPlaybin), "audio-tags-changed", (GCallback) tags_cb, NULL);

	// Create the GUI
	create_ui();

	// Instruct the bus to emit signals for each received message, and connect to the interesting signals
	bus = gst_element_get_bus (gPlaybin);
	gst_bus_add_signal_watch (bus);
	g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, NULL);
	g_signal_connect (G_OBJECT (bus), "message::eos", (GCallback)eos_cb, NULL);
	gst_object_unref (bus);

	g_object_set (gPlaybin, "uri", gLastStream, NULL);
	if (gIsPlaying)
		playPlaybin();

	gtk_main ();

	// Free resources
	gst_element_set_state (gPlaybin, GST_STATE_NULL);
	gst_object_unref (gPlaybin);
	return 0;
}
