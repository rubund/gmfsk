/*
 *    main.c  --  gMFSK main
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

#include "interface.h"
#include "support.h"

#include "waterfall.h"
#include "gtkdial.h"
#include "miniscope.h"
#include "papertape.h"

#include "main.h"
#include "conf.h"
#include "druid.h"
#include "ascii.h"
#include "trx.h"
#include "macro.h"
#include "log.h"
#include "ptt.h"
#include "picture.h"
#include "fft.h"
#include "hamlib.h"
#include "cwirc.h"
#include "snd.h"
#include "qsodata.h"

GtkWidget *appwindow;
GtkWidget *WFPopupMenu;
Waterfall *waterfall;

GtkTextTag *rxtag;
GtkTextTag *txtag;
GtkTextTag *hltag;

gchar *SoftString = "gMFSK v" VERSION " (Gnome MFSK) by OH2BNS";

/*
 * These get used a lot so lets have static pointers to them.
 */
static GtkScrolledWindow	*rxscrolledwindow;
static GtkSpinButton		*freqspinbutton;
static GtkDial			*metricdial;

static GtkTextView		*rxtextview;
static GtkTextView		*txtextview;
static GtkTextBuffer		*rxbuffer;
static GtkTextBuffer		*txbuffer;

/* ---------------------------------------------------------------------- */

static gboolean errmsg_idle_func(gpointer data)
{
	GtkWidget *dialog;
	gchar *msg = (gchar *) data;

	gdk_threads_enter();

	dialog = gtk_message_dialog_new(GTK_WINDOW(appwindow),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"%s", msg);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);

	g_free(data);

	gdk_threads_leave();

	return FALSE;
}

void errmsg(const gchar *fmt, ...)
{
	va_list args;
	gchar *msg;

	va_start(args, fmt);
	msg = g_strdup_vprintf(fmt, args);
	va_end(args);

	g_idle_add(errmsg_idle_func, (gpointer) msg);
}

/* ---------------------------------------------------------------------- */

void push_button(const gchar *name)
{
	GtkToggleButton *button;

	button = GTK_TOGGLE_BUTTON(lookup_widget(appwindow, name));
	gtk_toggle_button_set_active(button, TRUE);
}

/* ---------------------------------------------------------------------- */

struct statusbar {
	GtkStatusbar *bar;
	gchar *str;
};

static guint main_timeout_id = 0;

static gboolean statusbar_timeout_callback(gpointer data)
{
	GtkStatusbar *b;

	gdk_threads_enter();

	b = GTK_STATUSBAR(lookup_widget(appwindow, "mainstatusbar"));

	gtk_statusbar_pop(b, 0);
	gtk_statusbar_push(b, 0, "");

	gdk_threads_leave();

	return FALSE;
}

static gboolean statusbar_callback(gpointer data)
{
	struct statusbar *s = (struct statusbar *) data;

	gdk_threads_enter();

	gtk_statusbar_pop(s->bar, 0);
	gtk_statusbar_push(s->bar, 0, s->str);

	gdk_threads_leave();

	g_free(s->str);
	g_free(s);

	return FALSE;
}

void statusbar_set_main(const gchar *message)
{
	struct statusbar *s;

	s = g_new(struct statusbar, 1);

	s->bar = GTK_STATUSBAR(lookup_widget(appwindow, "mainstatusbar"));
	s->str = g_strdup(message);

	g_idle_add(statusbar_callback, s);

	if (main_timeout_id)
		g_source_remove(main_timeout_id);

	main_timeout_id = g_timeout_add(10000, statusbar_timeout_callback, NULL);
}

void statusbar_set_mode(gint mode)
{
	struct statusbar *s;

	s = g_new(struct statusbar, 1);

	s->bar = GTK_STATUSBAR(lookup_widget(appwindow, "modestatusbar"));
	s->str = g_strdup(trx_mode_names[mode]);

	g_idle_add(statusbar_callback, s);
}

void statusbar_set_trxstate(gint state)
{
	struct statusbar *s;

	s = g_new(struct statusbar, 1);

	s->bar = GTK_STATUSBAR(lookup_widget(appwindow, "trxstatusbar"));
	s->str = g_strdup(trx_state_names[state]);

	g_idle_add(statusbar_callback, s);
}

/* ---------------------------------------------------------------------- */

void textview_insert_pixbuf(gchar *name, GdkPixbuf *pixbuf)
{
	GtkTextView *view;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	view = GTK_TEXT_VIEW(lookup_widget(appwindow, name));

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
}

void textbuffer_insert_end(GtkTextBuffer *buf, const gchar *str, int len)
{
	GtkTextIter end;

	gtk_text_buffer_get_end_iter(buf, &end);
	gtk_text_buffer_insert(buf, &end, str, len);
}

void textbuffer_delete_end(GtkTextBuffer *buffer, gint count)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_end_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_iter_backward_chars(&start, count);
	gtk_text_buffer_delete(buffer, &start, &end);
}

void textview_scroll_end(GtkTextView *view)
{
	GtkTextBuffer *buffer;
	GtkTextMark *mark;

	buffer = gtk_text_view_get_buffer(view);
	mark = gtk_text_buffer_get_mark(buffer, "end");
	gtk_text_view_scroll_mark_onscreen(view, mark);
}

/* ---------------------------------------------------------------------- */

void send_char(gunichar c)
{
	GtkEditable *editable;
	gchar str[8];
	gint pos, len;

	len = g_unichar_to_utf8(c, str);

	switch (trx_get_mode()) {
	case MODE_FELDHELL:
	case MODE_FMHELL:
		editable = GTK_EDITABLE(lookup_widget(appwindow, "txentry"));
		pos = gtk_editable_get_position(editable);
		gtk_editable_insert_text(editable, str, len, &pos);
		gtk_editable_set_position(editable, pos);
		break;
	default:
		textbuffer_insert_end(txbuffer, str, len);
		textview_scroll_end(txtextview);
		break;
	}
}

void send_string(const gchar *str)
{
	GtkEditable *editable;
	gint pos, len;

	len = strlen(str);

	switch (trx_get_mode()) {
	case MODE_FELDHELL:
	case MODE_FMHELL:
		editable = GTK_EDITABLE(lookup_widget(appwindow, "txentry"));
		pos = gtk_editable_get_position(editable);
		gtk_editable_insert_text(editable, str, len, &pos);
		gtk_editable_set_position(editable, pos);
		break;
	default:
		textbuffer_insert_end(txbuffer, str, len);
		textview_scroll_end(txtextview);
		break;
	}
}

static gboolean clear_tx_text_idle_func(gpointer data)
{
	gdk_threads_enter();
	gtk_text_buffer_set_text(txbuffer, "", -1);
	gdk_threads_leave();

	return FALSE;
}

void clear_tx_text(void)
{
	g_idle_add(clear_tx_text_idle_func, NULL);
}

/* ---------------------------------------------------------------------- */

void insert_rx_text(gchar *tag, gchar *str, int len)
{
	GtkTextIter end;
	GError *err = NULL;
	gchar *to = "UTF-8";
	gchar *fm = "ISO-8859-1";
//	gchar *fm = "KOI8-R";
	gchar *fb = ".";

	gtk_text_buffer_get_end_iter(rxbuffer, &end);

	str = g_convert_with_fallback(str, len, to, fm, fb, NULL, NULL, &err);

	if (!str) {
		if (err) {
			g_warning(_("insert_rx_text: conversion failed: %s"), err->message);
			g_error_free(err);
		}
		return;
	}

	gtk_text_buffer_insert_with_tags_by_name(rxbuffer, &end, str, len, tag, NULL);
	g_free(str);
}

static gboolean main_loop(gpointer unused)
{
	static gboolean crflag = FALSE;
	static guchar databuf[30];
	static gint dataptr = 0;
	gboolean textflag = FALSE;
	gchar *p, *tag;
	gint c;

	gdk_threads_enter();

	/*
	 * Check for received picture data.
	 */
	while ((c = trx_get_rx_picdata()) >= 0) {
		if ((c & 0xFF000000) > 0) {
			gint width, height, color;

			color  = (c & 0xFF000000) >> 24;
			width  = (c & 0x00FFF000) >> 12;
			height = (c & 0x00000FFF);

			if (color == 'C')
				color = 1;
			else
				color = 0;

			picture_start(width, height, color);

			continue;
		}

		picture_write(c & 0xFF);
	}

	/*
	 * Check for received papertape data.
	 */
	while ((c = trx_get_rx_data()) >= 0) {
		Papertape *t1, *t2;

		databuf[dataptr++] = c;

		if (dataptr < 30)
			continue;

		t1 = PAPERTAPE(lookup_widget(appwindow, "rxtape1"));
		c = papertape_set_data(t1, databuf, 30);

		if (c == 0) {
			t1 = PAPERTAPE(lookup_widget(appwindow, "rxtape4"));
			t2 = PAPERTAPE(lookup_widget(appwindow, "rxtape3"));
			papertape_copy_data(t1, t2);

			t1 = PAPERTAPE(lookup_widget(appwindow, "rxtape3"));
			t2 = PAPERTAPE(lookup_widget(appwindow, "rxtape2"));
			papertape_copy_data(t1, t2);

			t1 = PAPERTAPE(lookup_widget(appwindow, "rxtape2"));
			t2 = PAPERTAPE(lookup_widget(appwindow, "rxtape1"));
			papertape_copy_data(t1, t2);

			t1 = PAPERTAPE(lookup_widget(appwindow, "rxtape1"));
			papertape_clear(t1);
		}

		dataptr = 0;
	}

	/*
	 * Check for received characters.
	 */
	while ((c = trx_get_rx_char()) >= 0) {
		if (c == 0)
			continue;

		/* CR-LF reduced to a single LF */
		if (crflag && c == 10) {
			crflag = FALSE;
			continue;
		} else if (c == 13)
			crflag = TRUE;

		/* backspace is a special case */
		if (c == 8) {
			textbuffer_delete_end(rxbuffer, 1);
			log_to_file(LOG_RX, "<BS>");
			continue;
		}

		if (c < 32) {
			p = g_strdup(ascii[c]);
			tag = "hltag";
		} else {
			p = g_strdup_printf("%c", c);
			tag = "rxtag";
		}

		insert_rx_text(tag, p, -1);
		log_to_file(LOG_RX, p);
		g_free(p);

		textflag = TRUE;
	}

	/*
	 * Check for transmitted characters.
	 */
	while ((c = trx_get_echo_char()) >= 0) {
		if (c == 0)
			continue;

		/* backspace is a special case */
		if (c == 8) {
			textbuffer_delete_end(rxbuffer, 1);
			log_to_file(LOG_TX, "<BS>");
			continue;
		}

		if (c < 32)
			p = g_strdup(ascii[c]);
		else
			p = g_strdup_printf("%c", c);

		insert_rx_text("txtag", p, -1);
		log_to_file(LOG_TX, p);
		g_free(p);

		textflag = TRUE;
	}

	if (textflag == TRUE)
		textview_scroll_end(rxtextview);

	gtk_dial_set_value(metricdial, trx_get_metric());

	if (!GTK_WIDGET_HAS_FOCUS(freqspinbutton))
		gtk_spin_button_set_value(freqspinbutton, trx_get_freq());

	gdk_threads_leave();

	waterfall_set_bandwidth(waterfall, trx_get_bandwidth());

	return TRUE;
}

/* ---------------------------------------------------------------------- */

static gchar *timestring(void)
{
	static gchar buf[64];
	struct tm *tm;
	time_t t;

	time(&t);
	tm = gmtime(&t);

	strftime(buf, sizeof(buf), "%H:%M:%SZ", tm);
	buf[sizeof(buf) - 1] = 0;

	return buf;
}

/* ---------------------------------------------------------------------- */

static gint clock_cb(gpointer ptr)
{
	GtkWidget *widget;

	gdk_threads_enter();

	widget = lookup_widget(appwindow, "clocklabel");
	gtk_label_set_text(GTK_LABEL(widget), timestring());

	gdk_threads_leave();

	return TRUE;
}

static void frequency_set_cb(GtkWidget *widget, gfloat freq, gpointer data)
{
	trx_set_freq(freq);
}

/* ---------------------------------------------------------------------- */

/*
 * Workaround for old versions of popt
 */
#ifndef POPT_ARGFLAG_OPTIONAL
#define POPT_ARGFLAG_OPTIONAL	0
#endif

static gint rundruid = 0;
static gint cwircshmid = 0;
static gchar *testmode = "none";
static gint modem = -1;

static const struct poptOption options[] =
{
	{ "modem", '\0', POPT_ARG_INT, &modem, 1,
	  N_("Start with specified modem"), N_("MODEM") },

	{ "run-druid", '\0', POPT_ARG_NONE, &rundruid, 1,
	  N_("Run first-time configuration druid"), NULL },

	{ "cwirc", '\0', POPT_ARG_INT, &cwircshmid, 1,
	  N_("Enable CWirc slave mode"), N_("SHMID") },

	{ "testmode", '\0', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL,
	  &testmode, 1,
	  N_("Enable test mode (write audio data to stdin/stdout)"),
	  N_("MODE") },

	{ NULL, '\0', 0, NULL, 0 }
};

GtkWidget *find_menu_item(const gchar *path)
{
	GtkWidget *w1, *w2;
	gint i;

	w1 = gnome_app_find_menu_pos(GNOME_APP(appwindow)->menubar, path, &i);

	if (w1 == NULL) {
		g_warning(_("Menu item \"%s\" not found!\n"), path);
		return NULL;
	}

	w2 = GTK_WIDGET(g_list_nth_data(GTK_MENU_SHELL(w1)->children, i - 1));
	return w2;
}

int main(int argc, char *argv[])
{
	GtkStatusbar *statusbar;
	GnomeUIInfo *uiinfo;
	GtkTextIter iter;
	GtkTextTag *tag;
	gchar *wisdom;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	g_thread_init(NULL);
	gdk_threads_init();

	/* FIXME: options needs to be run through gettext() */
	gnome_program_init(PACKAGE, VERSION, LIBGNOMEUI_MODULE,
			   argc, argv,
			   GNOME_PARAM_POPT_TABLE, options,
			   GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR,
			   NULL);

	if (cwircshmid && cwirc_init(cwircshmid))
		return 1;

	/* Init GConf */
	conf_init();

	if (rundruid || conf_get_int("misc/druidlevel") < DRUID_LEVEL)
		if (druid_run() == FALSE)
			return 1;

	wisdom = conf_get_string("misc/fftwwisdom");
	if (wisdom[0])
		fft_set_wisdom(wisdom);
	else
		g_warning(_("FFTW wisdom not found. This is normal if you are running gMFSK for the first time."));
	g_free(wisdom);

	/* create main window */
	appwindow = create_appwindow();

	qsodata_init();

	/* parse testmode option */
	if (!testmode || strcasecmp(testmode, "none")) {
		guint flags;

		sound_get_flags(&flags);
		flags &= ~SND_FLAG_TESTMODE_MASK;

		if (!testmode || !strcasecmp(testmode, "both")) {
			flags |= SND_FLAG_TESTMODE_TX;
			flags |= SND_FLAG_TESTMODE_RX;
			gtk_window_set_title(GTK_WINDOW(appwindow), 
					     _("gMFSK -- testmode (both)"));
		} else if (!strcasecmp(testmode, "rx")) {
			flags |= SND_FLAG_TESTMODE_RX;
			gtk_window_set_title(GTK_WINDOW(appwindow), 
					     _("gMFSK -- testmode (rx)"));
		} else if (!strcasecmp(testmode, "tx")) {
			flags |= SND_FLAG_TESTMODE_TX;
			gtk_window_set_title(GTK_WINDOW(appwindow), 
					     _("gMFSK -- testmode (tx)"));
		} else {
			g_warning(_("Invalid testmode '%s'\n"), testmode);
			return 2;
		}

		sound_set_flags(flags);
	}

	/* connect waterfall "frequency set" signal */
	waterfall = WATERFALL(lookup_widget(appwindow, "waterfall"));
	gtk_signal_connect(GTK_OBJECT(waterfall), "frequency_set",
			   GTK_SIGNAL_FUNC(frequency_set_cb),
			   NULL);

	/* create waterfall popup menu */
	WFPopupMenu = create_wfpopupmenu();

	rxscrolledwindow = GTK_SCROLLED_WINDOW(lookup_widget(appwindow, "rxscrolledwindow"));
	freqspinbutton = GTK_SPIN_BUTTON(lookup_widget(appwindow, "freqspinbutton"));
	metricdial = GTK_DIAL(lookup_widget(appwindow, "metricdial"));
	rxtextview = GTK_TEXT_VIEW(lookup_widget(appwindow, "rxtext"));
	txtextview = GTK_TEXT_VIEW(lookup_widget(appwindow, "txtext"));
	rxbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(rxtextview));
	txbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtextview));

	statusbar = GTK_STATUSBAR(lookup_widget(appwindow, "mainstatusbar"));
	uiinfo = (GnomeUIInfo *) gtk_object_get_data(GTK_OBJECT(appwindow), "menubar1_uiinfo");
	if (uiinfo)
		gnome_app_install_statusbar_menu_hints(statusbar, uiinfo);

	rxtag = gtk_text_buffer_create_tag(rxbuffer, "rxtag", NULL);
	txtag = gtk_text_buffer_create_tag(rxbuffer, "txtag", NULL);
	hltag = gtk_text_buffer_create_tag(rxbuffer, "hltag", NULL);

	/* editability tag and mark for the TX buffer */
	tag = gtk_text_buffer_create_tag(txbuffer, "editabletag", NULL);
	g_object_set(G_OBJECT(tag), "editable", FALSE, NULL);
	g_object_set(G_OBJECT(tag), "foreground", "grey30", NULL);
	g_object_set(G_OBJECT(tag), "background", "grey90", NULL);
	gtk_text_buffer_get_start_iter(txbuffer, &iter);
	gtk_text_buffer_create_mark(txbuffer, "editablemark", &iter, TRUE);

	/* load config files and configure things */
	conf_load();
	macroconfig_load();

	/* set word wrap for rx window */
	gtk_text_view_set_wrap_mode(rxtextview, GTK_WRAP_WORD);

	/* create end marks for tx and rx buffers */
	gtk_text_buffer_get_end_iter(txbuffer, &iter);
	gtk_text_buffer_create_mark(txbuffer, "end", &iter, FALSE);
	gtk_text_buffer_get_end_iter(rxbuffer, &iter);
	gtk_text_buffer_create_mark(rxbuffer, "end", &iter, FALSE);

	/* periodic task - main loop */
	g_timeout_add(100, main_loop, NULL);

	/* periodic task - update clock */
	g_timeout_add(1000, clock_cb, NULL);

	/* show the main window */
	gtk_widget_show(appwindow);

	/* make trx thread start in correct mode */
	if (cwirc_extension_mode)
		trx_set_mode(MODE_FELDHELL);
	else if (modem >= 0)
		trx_set_mode(modem);
	else
		trx_set_mode(conf_get_int("misc/lastmode"));

	statusbar_set_mode(trx_get_mode());

	/* initialize the trx queues */
	trx_init_queues();

	/* start the trx thread */
	trx_set_state(TRX_STATE_RX);
	waterfall_set_samplerate(waterfall, 8000);
	waterfall_set_frequency(waterfall, 1000.0);
	waterfall_set_center_frequency(waterfall, 1500.0);
	waterfall_set_bandwidth(waterfall, trx_get_bandwidth());

	if (cwirc_extension_mode) {
		GtkWidget *w;

		/* Force FELDHELL mode */
		w = find_menu_item("Mode/FELDHELL");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), TRUE);

		/* Disable the Mode menu */
		w = find_menu_item("Mode");
		gtk_widget_set_sensitive(w, FALSE);
	} else {
		GtkWidget *w = NULL;

		switch (trx_get_mode()) {
		default:
		case MODE_MFSK16:
			w = find_menu_item("_Mode/MFSK16");
			break;
		case MODE_MFSK8:
			w = find_menu_item("_Mode/MFSK8");
			break;
		case MODE_OLIVIA:
			w = find_menu_item("_Mode/OLIVIA");
			break;
		case MODE_RTTY:
			w = find_menu_item("_Mode/RTTY");
			break;
		case MODE_THROB1:
			w = find_menu_item("_Mode/THROB (1 tps)");
			break;
		case MODE_THROB2:
			w = find_menu_item("_Mode/THROB (2 tps)");
			break;
		case MODE_THROB4:
			w = find_menu_item("_Mode/THROB (4 tps)");
			break;
		case MODE_BPSK31:
			w = find_menu_item("_Mode/PSK31 (BPSK)");
			break;
		case MODE_QPSK31:
			w = find_menu_item("_Mode/PSK31 (QPSK)");
			break;
		case MODE_PSK63:
			w = find_menu_item("_Mode/PSK63");
			break;
		case MODE_MT63:
			w = find_menu_item("_Mode/MT63");
			break;
		case MODE_FELDHELL:
			w = find_menu_item("_Mode/FELDHELL");
			break;
		case MODE_CW:
			w = find_menu_item("_Mode/CW");
			break;
		}

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), TRUE);
	}

	/* Initialize hamlib */
#if WANT_HAMLIB
	hamlib_init();
#endif

	/* Open PTT and set to RX */
	conf_set_ptt();
	ptt_set(0);

	/* let GTK do it's job */
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	ptt_close();

#if WANT_HAMLIB
	hamlib_close();
#endif
	log_to_file_activate(FALSE);

	wisdom = fft_get_wisdom();
	conf_set_string("misc/fftwwisdom", wisdom);
	fftw_free(wisdom);

	conf_set_int("misc/lastmode", trx_get_mode());

	conf_clear();

	return 0;
}

/* ---------------------------------------------------------------------- */

