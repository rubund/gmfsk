/*
 *    qsodata.c  --  QSO data area functions
 *
 *    Copyright (C) 2001, 2002, 2003, 2004
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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "support.h"

#include "main.h"
#include "qsodata.h"
#include "remlog.h"
#include "trx.h"

#define	FIELD_LEN	256

static gchar call[FIELD_LEN]  = "";
static gchar band[FIELD_LEN]  = "";
static gchar txrst[FIELD_LEN] = "";
static gchar rxrst[FIELD_LEN] = "";
static gchar name[FIELD_LEN]  = "";
static gchar qth[FIELD_LEN]   = "";
static gchar loc[FIELD_LEN]   = "";
static gchar notes[FIELD_LEN] = "";

static time_t starttime = 0;

static GtkWidget *qsobandcombo = NULL;
static GtkWidget *qsobandentry = NULL;

/* ---------------------------------------------------------------------- */

static msgtype msgbuf;

static void log_msg_reset(void)
{
	msgbuf.mtext[0] = 0;
}

static void log_msg_append(gchar *type, gchar *data)
{
	gchar *entry;

	entry = g_strdup_printf("%s:%s%c", type, data, LOG_MSEPARATOR);

	if (strlen(msgbuf.mtext) + strlen(entry) + 1 > LOG_MSG_LEN) {
		g_warning(_("log message too long, entry dropped: %s\n"), entry);
		g_free(entry);
		return;
	}

	strcat(msgbuf.mtext, entry);
	g_free(entry);
}

static int log_msg_send(void)
{
	gint msqid, len;

	if ((msqid = msgget(LOG_MKEY, 0666 | IPC_CREAT)) == -1) {
		g_print("msgget: %s\n", strerror(errno));
		return -1;
	}

	msgbuf.mtype = LOG_MTYPE;

	/* allow for the NUL */
	len = strlen(msgbuf.mtext) + 1;

	if (msgsnd(msqid, &msgbuf, len, IPC_NOWAIT) < 0) {
		g_print("msgsnd: %m\n");
		return -1;
	}

#if 0
	g_print("msg sent: (%02ld) '%s'\n", msgbuf.mtype, msgbuf.mtext);
#endif

	return 0;
}

/* ---------------------------------------------------------------------- */

static void qsodata_set_entry(const gchar *name, gchar *str)
{
	GtkWidget *w;

	if ((w = lookup_widget(appwindow, name)) != NULL)
		gtk_entry_set_text(GTK_ENTRY(w), str);	
}

static void qsodata_init_band(void)
{
	GtkWidget *e;
	gchar *s;

	g_return_if_fail(qsobandcombo);

	e = GTK_COMBO(qsobandcombo)->entry;
	s = gtk_editable_get_chars(GTK_EDITABLE(e), 0, -1);
	g_strlcpy(band, s, FIELD_LEN);
	g_free(s);
}

void qsodata_clear(void)
{
	qsodata_set_entry("qsocallentry", "");
	qsodata_set_entry("qsotxrstentry", "");
	qsodata_set_entry("qsorxrstentry", "");
	qsodata_set_entry("qsonameentry", "");
	qsodata_set_entry("qsoqthentry", "");
	qsodata_set_entry("qsolocentry", "");
	qsodata_set_entry("qsonotesentry", "");

	qsodata_init_band();

	call[0] = '\0';
	txrst[0] = '\0';
	rxrst[0] = '\0';
	name[0] = '\0';
	qth[0] = '\0';
	loc[0] = '\0';
	notes[0] = '\0';

	starttime = 0;
}

void qsodata_log(void)
{
	char date[32], stime[32], etime[32], *mode;
	struct tm *tm;
	time_t t;

	g_strup(call);

	tm = gmtime(&starttime);
	strftime(date, sizeof(date), "%d %b %Y", tm);
	strftime(stime, sizeof(stime), "%H%M", tm);

	time(&t);
        tm = gmtime(&t);
	strftime(etime, sizeof(etime), "%H%M", tm);

	mode = trx_get_mode_name();

	log_msg_reset();
	log_msg_append("program", "gMFSK v" VERSION);
	log_msg_append("version", LOG_MVERSION);
	log_msg_append("date", date);
	log_msg_append("time", stime);
	log_msg_append("endtime", etime);
	log_msg_append("call", call);
	log_msg_append("mhz", band);
	log_msg_append("mode", mode);
	log_msg_append("tx", txrst);
	log_msg_append("rx", rxrst);
	log_msg_append("name", name);
	log_msg_append("qth", qth);
	log_msg_append("locator", loc);
	log_msg_append("notes", notes);
	log_msg_send();
}

/* ---------------------------------------------------------------------- */

void qsodata_set_starttime(gboolean force)
{
	if (force || starttime == 0)
		time(&starttime);
}

/* ---------------------------------------------------------------------- */

static void qsodata_entry_changed_cb(GtkEditable *e, gpointer p)
{
	gchar *data = (gchar *) p;
	gchar *str;

	str = gtk_editable_get_chars(e, 0, -1);
	g_strlcpy(data, str, FIELD_LEN);
	g_free(str);

	qsodata_set_starttime(FALSE);
}

/* ---------------------------------------------------------------------- */

void qsodata_init(void)
{
	GtkWidget *w, *table = lookup_widget(appwindow, "qsodatatable");

	/*
	 * Create and show the band combo.
	 */
	qsobandcombo = gtk_combo_new();
	gtk_table_attach(GTK_TABLE(table), qsobandcombo, 1, 2, 1, 2,
			 (GtkAttachOptions) (0),
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 0, 0);
	gtk_widget_set_size_request(qsobandcombo, 70, -1);
	gtk_widget_show(qsobandcombo);
	gtk_widget_show(GTK_COMBO(qsobandcombo)->entry);
	g_object_set_data(G_OBJECT(appwindow), "qsobandcombo", qsobandcombo);

	/*
	 * Create the band entry (leave hidden).
	 */
	qsobandentry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), qsobandentry, 1, 2, 1, 2,
			 (GtkAttachOptions) (0),
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 0, 0);
	gtk_widget_set_size_request(qsobandentry, 70, -1);
	gtk_editable_set_editable(GTK_EDITABLE(qsobandentry), FALSE);

	/*
	 * Band data doesn't get initialized otherwise...
	 */
	qsodata_init_band();

	/*
	 * Connect all of them (except the band entry) to the 'changed' signal.
	 */
	g_signal_connect(G_OBJECT(GTK_COMBO(qsobandcombo)->entry), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) band);

	w = lookup_widget(appwindow, "qsocallentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) call);

	w = lookup_widget(appwindow, "qsotxrstentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) txrst);

	w = lookup_widget(appwindow, "qsorxrstentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) rxrst);

	w = lookup_widget(appwindow, "qsonameentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) name);

	w = lookup_widget(appwindow, "qsoqthentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) qth);

	w = lookup_widget(appwindow, "qsolocentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) loc);

	w = lookup_widget(appwindow, "qsonotesentry");
	g_signal_connect(G_OBJECT(w), "changed",
			 G_CALLBACK(qsodata_entry_changed_cb),
			 (gpointer) notes);
}

/* ---------------------------------------------------------------------- */

G_CONST_RETURN gchar *qsodata_get_call(void)
{
	return call;
}

G_CONST_RETURN gchar *qsodata_get_band(void)
{
	return band;
}

G_CONST_RETURN gchar *qsodata_get_txrst(void)
{
	return txrst;
}

G_CONST_RETURN gchar *qsodata_get_rxrst(void)
{
	return rxrst;
}

G_CONST_RETURN gchar *qsodata_get_name(void)
{
	return name;
}

G_CONST_RETURN gchar *qsodata_get_qth(void)
{
	return qth;
}

G_CONST_RETURN gchar *qsodata_get_loc(void)
{
	return loc;
}

G_CONST_RETURN gchar *qsodata_get_notes(void)
{
	return notes;
}

/* ---------------------------------------------------------------------- */

void qsodata_set_band_mode(band_mode_t mode)
{
	static band_mode_t qsodata_band_mode = QSODATA_BAND_COMBO;

	g_return_if_fail(qsobandcombo);
	g_return_if_fail(qsobandentry);

	if (qsodata_band_mode == mode)
		return;

	switch (mode) {
	case QSODATA_BAND_COMBO:
		gtk_widget_hide(qsobandentry);

		gtk_widget_show(qsobandcombo);
		gtk_widget_show(GTK_COMBO(qsobandcombo)->entry);

		qsodata_init_band();
		break;

	case QSODATA_BAND_ENTRY:
		gtk_widget_hide(qsobandcombo);
		gtk_widget_hide(GTK_COMBO(qsobandcombo)->entry);

		gtk_widget_show(qsobandentry);
		break;
	}

	qsodata_band_mode = mode;
}

void qsodata_set_freq(const gchar *freq)
{
	g_return_if_fail(qsobandentry);

	gtk_entry_set_text(GTK_ENTRY(qsobandentry), freq);

	g_strlcpy(band, freq, FIELD_LEN);
}

/* ---------------------------------------------------------------------- */

