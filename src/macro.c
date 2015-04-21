/*
 *    macro.c  --  Fixtexts and macros
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

#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "support.h"
#include "main.h"
#include "conf.h"
#include "qsodata.h"
#include "macro.h"
#include "interface.h"
#include "trx.h"
#include "picture.h"

#define	NUMMACROS	12

struct macro {
	gchar *name;
	gchar *text;
};

static struct macro macros[NUMMACROS];

#define confcpy(d,s)	{ strncpy((d),(s),sizeof(d)); d[sizeof(d)-1] = 0; }

static GtkWidget *macroconfigwin = NULL;   

/* ---------------------------------------------------------------------- */

static void set_macro_button_names(void)
{
	GtkButton *button;
	GtkLabel *label;
	gchar *str;
	gint i;

	for (i = 0; i < NUMMACROS; i++) {
		str = g_strdup_printf("macrobutton%d", i + 1);
		button = GTK_BUTTON(lookup_widget(appwindow, str));
		label = GTK_LABEL(GTK_BIN(button)->child);
		g_free(str);

		str = g_strdup_printf("%s (F%d)", macros[i].name, i + 1);
		gtk_label_set_text(label, str);
		g_free(str);
	}
}

/* ---------------------------------------------------------------------- */

static void fill_macroconfig(GtkWidget *win, gint n)
{
	GtkEntry *entry;
	GtkTextView *view;
	GtkTextBuffer *buffer;
	gchar *str;

	/* should not happen... */
	if (n < 1 || n > NUMMACROS) {
		errmsg(_("fill_macroconfig: invalid macro number %d\n"), n);
		return;
	}

	str = g_strdup_printf("Macro %d", n);
	gtk_window_set_title(GTK_WINDOW(win), str);
	g_free(str);

	g_object_set_data(G_OBJECT(win), "macronumber", GINT_TO_POINTER(n));

	entry = GTK_ENTRY(lookup_widget(win, "macroconfigentry"));
	gtk_entry_set_text(entry, macros[n - 1].name);

	if (!macros[n - 1].text)
		return;

	view = GTK_TEXT_VIEW(lookup_widget(win, "macroconfigtext"));
	buffer = gtk_text_view_get_buffer(view);
	gtk_text_buffer_set_text(buffer, macros[n - 1].text, -1);
}

static void apply_macroconfig(GtkWidget *win)
{
	GtkEditable *editable;
	GtkTextView *view;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	gchar *key;
	gint n;

	n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(win), "macronumber"));

	if (n < 1 || n > NUMMACROS) {
		errmsg(_("apply_macroconfig: invalid macro number %d\n"), n);
		return;
	}

	editable = GTK_EDITABLE(lookup_widget(win, "macroconfigentry"));

	g_free(macros[n - 1].name);
	macros[n - 1].name = gtk_editable_get_chars(editable, 0, -1);

	view = GTK_TEXT_VIEW(lookup_widget(win, "macroconfigtext"));
	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_bounds(buf, &start, &end);

	g_free(macros[n - 1].text);
	macros[n - 1].text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

	key = g_strdup_printf("macro/name%d", n);
	conf_set_string(key, macros[n - 1].name);
	g_free(key);

	key = g_strdup_printf("macro/text%d", n);
	conf_set_string(key, macros[n - 1].text);
	g_free(key);

	set_macro_button_names();
}

/* ---------------------------------------------------------------------- */

void macroconfig_load(void)
{
	gchar *key;
	gint i;

	for (i = 0; i < NUMMACROS; i++) {
		key = g_strdup_printf("macro/name%d", i + 1);
		macros[i].name = conf_get_string(key);
		g_free(key);

		key = g_strdup_printf("macro/text%d", i + 1);
		macros[i].text = conf_get_string(key);
		g_free(key);
	}

	set_macro_button_names();
}

/* ---------------------------------------------------------------------- */

static void macroconfig_response_callback(GtkDialog *dialog,
					  gint id,
					  gpointer data)
{
	GError *error = NULL;
	GtkWidget *w;
	GtkTextBuffer *b;

	switch (id) {
	case GTK_RESPONSE_HELP:
		gnome_help_display("gmfsk.xml", "gmfsk-settings-menu", &error);
		if (error != NULL) {
			errmsg(error->message);
			g_error_free(error);
		}
		return;

	case GTK_RESPONSE_REJECT:
		w = lookup_widget(GTK_WIDGET(dialog), "macroconfigentry");
		gtk_editable_delete_text(GTK_EDITABLE(w), 0, -1);
		w = lookup_widget(GTK_WIDGET(dialog), "macroconfigtext");
		b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
		gtk_text_buffer_set_text(b, "", -1);
		break;

	case GTK_RESPONSE_OK:
		apply_macroconfig(GTK_WIDGET(dialog));
		/* note fall-trough */

	default:				/* CANCEL and NONE */
		gtk_widget_destroy(GTK_WIDGET(dialog));
		macroconfigwin = NULL;
		break;
	}
}

void macroconfig(gint n)
{
	if (macroconfigwin) {
		gtk_window_present(GTK_WINDOW(macroconfigwin));
		return;
	}

	macroconfigwin = create_macroconfigdialog();
	fill_macroconfig(macroconfigwin, n);

	g_signal_connect(G_OBJECT(macroconfigwin), "response",
			 G_CALLBACK(macroconfig_response_callback),
			 NULL);

	gtk_widget_show(macroconfigwin);
}

/* ---------------------------------------------------------------------- */

static void run_command(gchar *cmd)
{
	gchar *outbuf;
	GError *error = NULL;
	gboolean ret;

	cmd++;				/* skip   '('	*/
	cmd[strlen(cmd) - 1] = 0;	/* delete ')'	*/

	ret = g_spawn_command_line_sync(cmd, &outbuf, NULL, NULL, &error);

	if (ret == FALSE || outbuf == NULL) {
		if (error) {
			errmsg(_("run_command: %s"), error->message);
			g_error_free(error);
		}
	}

	if (outbuf == NULL)
		return;

	/* delete the last end-of-line if there is one */
	if (outbuf[strlen(outbuf) - 1] == '\n')
		outbuf[strlen(outbuf) - 1] = 0;

	send_string(outbuf);
	g_free(outbuf);
}

static void send_time(const gchar *fmt, gboolean utc)
{
	gchar buf[256];
	struct tm *tm;
	time_t t;

	time(&t);

	if (utc)
		tm = gmtime(&t);
	else
		tm = localtime(&t);

	strftime(buf, sizeof(buf), fmt, tm);
	buf[sizeof(buf) - 1] = 0;

	send_string(buf);
}

/* ---------------------------------------------------------------------- */

static gchar *getword(gchar **ptr)
{
	gchar *word, *p;

	switch (**ptr) {
	case '(':
		if ((p = strchr(*ptr, ')')) == NULL)
			return NULL;
		p++;
		break;
	case '{':
		if ((p = strchr(*ptr, '}')) == NULL)
			return NULL;
		p++;
		break;
	default:
		for (p = *ptr; *p && isalnum(*p); p++)
			;
		break;
	}

	word = g_memdup(*ptr, p - *ptr + 1);
	word[p - *ptr] = 0;

	*ptr = p;

	return word;
}

void send_macro(gint n)
{
	gchar *p, *str, *word;
	gunichar c;

	/* should not happen... */
	if (n < 1 || n > NUMMACROS) {
		errmsg(_("send_macro: invalid macro number %d\n"), n);
		return;
	}

	str = g_strdup(macros[n - 1].text);
	p = str;

	while (p && *p) {
		c = g_utf8_get_char(p);
		p = g_utf8_next_char(p);

		if (c == '$') {
			gchar *cmd = NULL;
			gchar *arg = NULL;

			word = getword(&p);

			if (word == NULL)
				continue;

			switch (*word) {
			case '(':
				run_command(word);
				g_free(word);
				continue;
			case '{':
				cmd = word + 1;
				word[strlen(word) - 1] = 0;
				break;
			default:
				cmd = word;
				break;
			}

			if ((arg = strchr(cmd, ':')) != NULL)
				*arg++ = 0;

			if (!strcasecmp(cmd, "$"))
				send_char('$');

			/*
			 * Version string
			 */
			if (!strcasecmp(cmd, "soft"))
				send_string(SoftString);

			/*
			 * Buttons
			 */
			if (!strcasecmp(cmd, "tx"))
				push_button("txbutton");

			if (!strcasecmp(cmd, "rx"))
				send_char(TRX_RX_CMD);

			/*
			 * My station info
			 */
			if (!strcasecmp(cmd, "mycall"))
				send_string(conf_get_mycall());

			if (!strcasecmp(cmd, "myname"))
				send_string(conf_get_myname());

			if (!strcasecmp(cmd, "myqth"))
				send_string(conf_get_myqth());

			if (!strcasecmp(cmd, "myloc"))
				send_string(conf_get_myloc());

			if (!strcasecmp(cmd, "myemail"))
				send_string(conf_get_myemail());

			/*
			 * Time and date
			 */
			if (!strcasecmp(cmd, "time"))
				send_time(conf_get_timefmt(), FALSE);

			if (!strcasecmp(cmd, "utctime"))
				send_time(conf_get_timefmt(), TRUE);

			if (!strcasecmp(cmd, "date"))
				send_time(conf_get_datefmt(), FALSE);

			if (!strcasecmp(cmd, "utcdate"))
				send_time(conf_get_datefmt(), TRUE);

			/*
			 * QSO data
			 */
			if (!strcasecmp(cmd, "call"))
				send_string(qsodata_get_call());

			if (!strcasecmp(cmd, "band"))
				send_string(qsodata_get_band());

			if (!strcasecmp(cmd, "rxrst"))
				send_string(qsodata_get_rxrst());

			if (!strcasecmp(cmd, "txrst"))
				send_string(qsodata_get_txrst());

			if (!strcasecmp(cmd, "name"))
				send_string(qsodata_get_name());

			if (!strcasecmp(cmd, "qth"))
				send_string(qsodata_get_qth());

			if (!strcasecmp(cmd, "notes"))
				send_string(qsodata_get_notes());

			/*
			 * QSO logging
			 */
			if (!strcasecmp(cmd, "startqso"))
				qsodata_set_starttime(TRUE);

			if (!strcasecmp(cmd, "logqso"))
				qsodata_log();

			if (!strcasecmp(cmd, "clearqso"))
				qsodata_clear();

			/*
			 * Mode
			 */
			if (!strcasecmp(cmd, "mode"))
				send_string(trx_get_mode_name());

			/*
			 * Pictures
			 */
			if (!strcasecmp(cmd, "pic"))
				picture_send(arg, FALSE);

			if (!strcasecmp(cmd, "picc"))
				picture_send(arg, TRUE);

			/* an unknown macro gets ignored */

			g_free(word);
			continue;
		}

		send_char(c);
	}

	g_free(str);
}

/* ---------------------------------------------------------------------- */
