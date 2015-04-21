/*
 *    druid.c  --  First install configuration wizard.
 *
 *    Copyright (C) 2001, 2002, 2003
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This file is part of gMFSK.
 *
 *    gMFSK is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    gMFSK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with gMFSK; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>

#include "interface.h"
#include "support.h"

#include "druid.h"
#include "conf.h"
#include "snd.h"

/* ---------------------------------------------------------------------- */

static GtkWidget *druid;

static gboolean druid_delete_cb(GtkWidget *w, GdkEvent *e, gpointer p);
static void gmfskdruid_cancel_cb(GnomeDruid *g, gpointer data);
static void gmfskdruid_finish_cb(GnomeDruidPage *g, GtkWidget *w, gpointer p);

static void druid_fileentry_changed_cb(GtkEditable *e, gpointer p);
static void druid_create_files_cb(GtkButton *b, gpointer p);

static void druid_test_sound_cb(GtkButton *b, gpointer p);

static gboolean druid_test_ptt_pressed_cb(GtkWidget *w, GdkEventButton *e, 
					  gpointer p);
static gboolean druid_test_ptt_released_cb(GtkWidget *w, GdkEventButton *e, 
					   gpointer p);

static gboolean druid_create_file(gchar *filename);
static gboolean druid_create_dir(gchar *dirname);

static void druid_test_sound(const gchar *path, gboolean eb, gboolean st);

static void druid_init_ptt(const gchar *path, gint inv, gint mode);
static void druid_set_ptt(gint ptt);
static void druid_close_ptt(void);

static void druid_errmsg(const gchar *fmt, ...);

/* ---------------------------------------------------------------------- */

typedef enum {
	T_ENTRY,
	T_TOGGLEBUTTON,
	T_RADIOBUTTON,
} conf_type_t;

#define	CONFITEM(c)	((ConfItem *) (c))

typedef struct _ConfItem ConfItem;

struct _ConfItem {
	const gchar	*path;
	conf_type_t	type;
	const gchar	*widgetname;
	gint		radiodata;
};

static ConfItem confitems[] = {
	{ "info/mycall",	T_ENTRY,	"druid_callentry"    },
	{ "info/myname",	T_ENTRY,	"druid_nameentry"    },
	{ "info/myqth",		T_ENTRY,	"druid_qthentry"     },
	{ "info/myloc",		T_ENTRY,	"druid_locentry"     },
	{ "info/myemail",	T_ENTRY,	"druid_emailentry"   },
	{ "misc/logfile",	T_ENTRY,	"druid_logfileentry" },
	{ "misc/pictxdir",	T_ENTRY,	"druid_txpicentry"   },
	{ "misc/picrxdir",	T_ENTRY,	"druid_rxpicentry"   },
	{ "info/myemail",	T_ENTRY,	"druid_emailentry"   },
	{ "sound/dev",		T_ENTRY,	"druid_soundentry"   },
	{ "ptt/dev",		T_ENTRY,	"druid_pttentry"     },

	{"sound/8bit",		T_TOGGLEBUTTON,	"druid_eightbitcheckbutton"},
	{"sound/stereo",	T_TOGGLEBUTTON,	"druid_stereocheckbutton"},

	{"ptt/inverted",	T_TOGGLEBUTTON,	"druid_pttinvcheckbutton"},
	{"ptt/mode",		T_RADIOBUTTON,	"druid_pttrtsradiobutton",  0},
	{"ptt/mode",		T_RADIOBUTTON,	"druid_pttdtrradiobutton",  1},
	{"ptt/mode",		T_RADIOBUTTON,	"druid_pttbothradiobutton", 2},

	{ NULL }
};

struct druidsignal {
	const gchar	*widgetname;
	const gchar	*signalname;
	GCallback	callback;
};

static struct druidsignal signals[] = {
	{ "gmfskdruid", 
	  "cancel", 
	  G_CALLBACK(gmfskdruid_cancel_cb) },

	{ "gmfskdruidpagefinish",
	  "finish", 
	  G_CALLBACK(gmfskdruid_finish_cb) },

	{ "druid_filesbutton",
	  "clicked", 
	  G_CALLBACK(druid_create_files_cb) },

	{ "druid_soundtestbutton", 
	  "clicked", 
	  G_CALLBACK(druid_test_sound_cb) },

	{ "druid_ptttestbutton", 
	  "pressed", 
	  G_CALLBACK(druid_test_ptt_pressed_cb) },

	{ "druid_ptttestbutton", 
	  "released", 
	  G_CALLBACK(druid_test_ptt_released_cb) },

	{ "druid_logfileentry",
	  "changed",
	  G_CALLBACK(druid_fileentry_changed_cb) },

	{ "druid_txpicentry",
	  "changed",
	  G_CALLBACK(druid_fileentry_changed_cb) },

	{ "druid_rxpicentry",
	  "changed",
	  G_CALLBACK(druid_fileentry_changed_cb) },

	{ NULL }
};

gboolean druid_run(void)
{
	struct druidsignal *sig;
	ConfItem *item;
	GtkWidget *w;
	gchar *s;
	gboolean b;

	druid = create_druidwindow();

	g_signal_connect((gpointer) druid, "delete_event",
			 G_CALLBACK(druid_delete_cb),
			 NULL);

	for (sig = signals; sig->widgetname; sig++) {
		w = lookup_widget(druid, sig->widgetname);
		g_signal_connect((gpointer) w, 
				 sig->signalname,
				 sig->callback,
				 NULL);
	}

	for (item = confitems; item->path; item++) {
		w = lookup_widget(druid, item->widgetname);

		switch (item->type) {
		case T_ENTRY:
			s = conf_get_string(item->path);
			gtk_entry_set_text(GTK_ENTRY(w), s);
			g_free(s);
			break;
		case T_TOGGLEBUTTON:
			b = conf_get_bool(item->path);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), b);
			break;
		case T_RADIOBUTTON:
			if (item->radiodata == conf_get_int(item->path))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
			break;
		default:
			break;
		}
	}

	gtk_widget_show(druid);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	sound_set_conf(NULL);
	sound_set_flags(0);

	druid_close_ptt();

	return TRUE;
}

/* ---------------------------------------------------------------------- */

gboolean druid_delete_cb(GtkWidget *w, GdkEvent *e, gpointer p)
{
	gtk_widget_destroy(druid);
	gtk_main_quit();
	return TRUE;
}

void gmfskdruid_cancel_cb(GnomeDruid *g, gpointer data)
{
	gtk_widget_destroy(druid);
	gtk_main_quit();
}

void gmfskdruid_finish_cb(GnomeDruidPage *g, GtkWidget *wid, gpointer p)
{
	ConfItem *item = confitems;
	GtkWidget *w;
	gchar *s;
	gboolean b;

	for (item = confitems; item->path; item++) {
		w = lookup_widget(druid, item->widgetname);

		switch (item->type) {
		case T_ENTRY:
			s = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
			conf_set_string(item->path, s);
			g_free(s);
			break;
		case T_TOGGLEBUTTON:
			b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
			conf_set_bool(item->path, b);
			break;
		case T_RADIOBUTTON:
			b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
			if (b == TRUE)
				conf_set_int(item->path, item->radiodata);
			break;
		default:
			break;
		}
	}

	conf_set_int("misc/druidlevel", DRUID_LEVEL);

	gtk_widget_destroy(druid);
	gtk_main_quit();
}

static void druid_fileentry_changed_cb(GtkEditable *e, gpointer p)
{
	GtkWidget *w;

	w = lookup_widget(druid, "druid_filesbutton");
	gtk_widget_set_sensitive(w, TRUE);
}

static gchar *druid_get_filename(const char *widgetname)
{
	GtkWidget *w;
	GError *error = NULL;
	gchar *s, *name;

	w = lookup_widget(druid, widgetname);
	s = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);

	name = g_filename_from_utf8(s, -1, NULL, NULL, &error);

	if (name == NULL) {
		if (error != NULL) {
			druid_errmsg(_("Error converting filename (%s=%s): %s"),
				     widgetname, s, error->message);
			g_error_free(error);
		}
		return NULL;
	}

	g_free(s);

	if (name[0] == '~' && name[1] == G_DIR_SEPARATOR) {
		gchar *tmp;
 
		tmp = g_strdup_printf("%s%c%s", g_get_home_dir(),
				      G_DIR_SEPARATOR, name + 2);
 
		g_free(name);
		name = tmp;
	}
 
        return name;
}

static void druid_create_files_cb(GtkButton *b, gpointer p)
{
	gchar *s;

	s = druid_get_filename("druid_logfileentry");
	if (druid_create_file(s) == FALSE) {
		g_free(s);
		return;
	}
	g_free(s);

	s = druid_get_filename("druid_txpicentry");
	if (druid_create_dir(s) == FALSE) {
		g_free(s);
		return;
	}
	g_free(s);

	s = druid_get_filename("druid_rxpicentry");
	if (druid_create_dir(s) == FALSE) {
		g_free(s);
		return;
	}
	g_free(s);

	gtk_widget_set_sensitive(GTK_WIDGET(b), FALSE);
}

static void druid_test_sound_cb(GtkButton *b, gpointer p)
{
	GtkWidget *w;
	gboolean b1, b2;
	gchar *s;

	s = druid_get_filename("druid_soundentry");

	w = lookup_widget(druid, "druid_eightbitcheckbutton");
	b1 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

	w = lookup_widget(druid, "druid_stereocheckbutton");
	b2 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

	druid_test_sound(s, b1, b2);

	g_free(s);
}

static gboolean druid_test_ptt_pressed_cb(GtkWidget *wid, GdkEventButton *e, 
					  gpointer p)
{
	GtkWidget *w;
	gboolean b;
	gchar *s;
	gint i = 0;

	s = druid_get_filename("druid_pttentry");

	w = lookup_widget(druid, "druid_pttinvcheckbutton");
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

	w = lookup_widget(druid, "druid_pttrtsradiobutton");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) == TRUE)
		i = 0;

	w = lookup_widget(druid, "druid_pttdtrradiobutton");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) == TRUE)
		i = 1;

	w = lookup_widget(druid, "druid_pttbothradiobutton");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) == TRUE)
		i = 2;

	druid_init_ptt(s, b, i);
	druid_set_ptt(1);

	return FALSE;
}

static gboolean druid_test_ptt_released_cb(GtkWidget *w, GdkEventButton *e, 
					   gpointer p)
{
	druid_set_ptt(0);

	return FALSE;
}

/* ---------------------------------------------------------------------- */

static gboolean druid_mkdir(const gchar *name)
{
	/* don't try to create "/" */
	if (*name == 0)
		return TRUE;

	if (g_file_test(name, G_FILE_TEST_EXISTS) == FALSE) {
		if (mkdir(name, 0755) < 0) {
			druid_errmsg(_("Error creating directory: %s: %m"), name);
			return FALSE;
		}
		return TRUE;
	}

	if (g_file_test(name, G_FILE_TEST_IS_DIR) == FALSE) {
		druid_errmsg(_("Error creating directory: %s exists but is not a directory"), name);
		return FALSE;
	}

	return TRUE;
}

static gboolean druid_create_file(gchar *filename)
{
	gchar *dirname = g_path_get_dirname(filename);

	if (druid_create_dir(dirname) == FALSE) {
		g_free(dirname);
		return FALSE;
	}
	g_free(dirname);

	if (g_file_test(filename, G_FILE_TEST_EXISTS) == FALSE) {
		if (creat(filename, 0644) < 0) {
			druid_errmsg(_("Error creating file: %s: %m"), filename);
			return FALSE;
		}
		return TRUE;
	}

	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR) == FALSE) {
		druid_errmsg(_("Error creating file: %s exists but is not a regular file"), filename);
		return FALSE;
	}

	if (access(filename, R_OK | W_OK) < 0) {
		druid_errmsg(_("Error: %s is not writable: %m"), filename);
		return FALSE;
	}

	return TRUE;
}

static gboolean druid_create_dir(gchar *dirname)
{
	gchar *p;

	p = strchr(dirname, G_DIR_SEPARATOR);

	while (p) {
		*p = 0;

		if (druid_mkdir(dirname) == FALSE)
			return FALSE;

		*p = G_DIR_SEPARATOR;
		p = strchr(++p, G_DIR_SEPARATOR);
	}

	if (druid_mkdir(dirname) == FALSE)
		return FALSE;

	return TRUE;
}

/* ---------------------------------------------------------------------- */

#define	SAMPLERATE		8000
#define	BUFSIZE			8000

static void druid_test_sound(const gchar *path, gboolean eb, gboolean st)
{
	snd_config_t cfg;
	guint flags = 0;
	gfloat buf[BUFSIZE], *p;
	gdouble f, phi = 0.0;
	gint i;

	cfg.device = path;
	cfg.samplerate = SAMPLERATE;
	cfg.txoffset = 0;
	cfg.rxoffset = 0;

	if (eb)
		flags |= SND_FLAG_8BIT;
	if (st)
		flags |= SND_FLAG_STEREO;

	sound_set_conf(&cfg);
	sound_set_flags(flags);

	g_printerr("Testing read...\n");

	if (sound_open_for_read(SAMPLERATE) < 0) {
		druid_errmsg(_("Sound card open for read failed: %s"), 
			     sound_error());
		return;
	}

	i = BUFSIZE / 4;
	if (sound_read(&p, &i) < 0) {
		druid_errmsg(_("Reading from sound card failed: %s"),
			     sound_error());
		sound_close();
		return;
	}

	sound_close();

	g_printerr("Testing write...\n");

	if (sound_open_for_write(SAMPLERATE) < 0) {
		druid_errmsg(_("Sound card open for write failed: %s"), 
			     sound_error());
		return;
	}

	for (i = 0; i < BUFSIZE; i++) {
		f = 1000.0 * sin(2.0 * M_PI * 4.0 * i / SAMPLERATE) + 1500.0;

		phi += 2.0 * M_PI * f / SAMPLERATE;
		if (phi > M_PI)
			phi -= 2.0 * M_PI;

		buf[i] = 0.5 * sin(phi);
	}

	if (sound_write(buf, BUFSIZE) < 0) {
		druid_errmsg(_("Reading from sound card failed: %s"),
			     sound_error());
		sound_close();
		return;
	}

	sound_close();
}

/* ---------------------------------------------------------------------- */

static gint pttfd = -1;
static gint pttinv = 0;
static gint pttarg = TIOCM_RTS | TIOCM_DTR;

static void druid_init_ptt(const gchar *path, gint inv, gint mode)
{
	druid_close_ptt();
 
	pttinv = inv;
 
	switch (mode) {
	case 0:
		pttarg = TIOCM_RTS;
		break;
	case 1:
		pttarg = TIOCM_DTR;
		break;
	case 2:
		pttarg = TIOCM_RTS | TIOCM_DTR;
		break;
	}
 
	if (!strcasecmp(path, "none"))
		return;

	if ((pttfd = open(path, O_RDWR, 0)) < 0)
		druid_errmsg(_("Cannot open PTT device '%s': %m"), path);
}
 
static void druid_set_ptt(gint ptt)
{
	int arg = pttarg;
 
	if (pttfd == -1)
		return;
 
	if (pttinv)
		ptt = !ptt;
 
	if (ioctl(pttfd, ptt ? TIOCMBIS : TIOCMBIC, &arg) < 0)
		druid_errmsg(_("set_ptt: ioctl: %m"));
}

static void druid_close_ptt(void)
{
	if (pttfd == -1)
		return;

	close(pttfd);
}

/* ---------------------------------------------------------------------- */

static void druid_errmsg(const gchar *fmt, ...)
{
        GtkWidget *dialog;
        va_list args;
        gchar *msg;
 
        va_start(args, fmt);
        msg = g_strdup_vprintf(fmt, args);
        va_end(args);

	dialog = gtk_message_dialog_new(GTK_WINDOW(druid),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"%s", msg);
 
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
 
	g_free(msg);
}

/* ---------------------------------------------------------------------- */

