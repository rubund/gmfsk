/*
 *    callbacks.c  --  GUI callbacks
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

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "main.h"
#include "conf.h"
#include "confdialog.h"
#include "trx.h"
#include "macro.h"
#include "log.h"
#include "qsodata.h"
#include "hamlib.h"

/* ---------------------------------------------------------------------- */

gboolean
on_appwindow_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	trx_set_state(TRX_STATE_ABORT);
	gtk_main_quit();

	return TRUE;
}

/* ---------------------------------------------------------------------- */

void
on_send_file1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *widget;

	widget = g_object_get_data(G_OBJECT(appwindow), "txfileselectdialog");

	if (widget) {
		gtk_window_present(GTK_WINDOW(widget));
		return;
	}

	widget = create_txfileselection();
	gtk_widget_show(widget);

	g_object_set_data(G_OBJECT(appwindow), "txfileselectdialog", widget);
}


void
on_log_to_file1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (log_to_file_activate(GTK_CHECK_MENU_ITEM(menuitem)->active) == FALSE)
		errmsg(_("Unable to open log file"));
}


void
on_clear_tx_window1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkEditable *editable;
	GtkTextView *view;
	GtkTextBuffer *buffer;

	switch (trx_get_mode()) {
	case MODE_FELDHELL:
	case MODE_FMHELL:
		editable = GTK_EDITABLE(lookup_widget(appwindow, "txentry"));
		gtk_editable_delete_text(editable, 0, -1);
		break;
	default:
		view = GTK_TEXT_VIEW(lookup_widget(appwindow, "txtext"));
		buffer = gtk_text_view_get_buffer(view);
		gtk_text_buffer_set_text(buffer, "", -1);
		break;
	}
}


void
on_clear_rx_window1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	Papertape *tape;
	GtkTextView *view;
	GtkTextBuffer *buffer;

	switch (trx_get_mode()) {
	case MODE_FELDHELL:
	case MODE_FMHELL:
		tape = PAPERTAPE(lookup_widget(appwindow, "rxtape1"));
		papertape_clear(tape);
		tape = PAPERTAPE(lookup_widget(appwindow, "rxtape2"));
		papertape_clear(tape);
		tape = PAPERTAPE(lookup_widget(appwindow, "rxtape3"));
		papertape_clear(tape);
		tape = PAPERTAPE(lookup_widget(appwindow, "rxtape4"));
		papertape_clear(tape);
		break;
	default:
		view = GTK_TEXT_VIEW(lookup_widget(appwindow, "rxtext"));
		buffer = gtk_text_view_get_buffer(view);
		gtk_text_buffer_set_text(buffer, "", -1);
		break;
	}
}


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	trx_set_state(TRX_STATE_ABORT);
	gtk_main_quit();
}

/* ---------------------------------------------------------------------- */

static void set_sens(gboolean afc, gboolean sql, gboolean rev, gboolean frq)
{
	GtkWidget *w;

	w = lookup_widget(appwindow, "afcbutton");
	gtk_widget_set_sensitive(w, afc);
	w = lookup_widget(appwindow, "squelchbutton");
	gtk_widget_set_sensitive(w, sql);
	w = lookup_widget(appwindow, "reversebutton");
	gtk_widget_set_sensitive(w, rev);
	w = lookup_widget(appwindow, "freqspinbutton");
	gtk_widget_set_sensitive(w, frq);
	w = lookup_widget(appwindow, "freqlabel");
	gtk_widget_set_sensitive(w, frq);
	w = lookup_widget(appwindow, "qsybutton");
#if WANT_HAMLIB
	gtk_widget_set_sensitive(w, frq && hamlib_active());
#else
	gtk_widget_set_sensitive(w, FALSE);
#endif
}

static void set_scope_mode(miniscope_mode_t mode)
{
	GtkWidget *widget;

	widget = lookup_widget(appwindow, "miniscope");
	miniscope_set_mode(MINISCOPE(widget), mode);
}

void
on_mfsk1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_MFSK16) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_MFSK16);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, TRUE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_mfsk2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_MFSK8) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_MFSK8);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, TRUE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_olivia1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_OLIVIA) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_OLIVIA);
			trx_set_state_wait(state);
		}

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());

		set_sens(FALSE, TRUE, FALSE, FALSE);
		set_scope_mode(MINISCOPE_MODE_BLANK);

		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, TRUE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_rtty1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_RTTY) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_RTTY);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, FALSE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_throb1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_THROB1) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_THROB1);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, FALSE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, TRUE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_throb2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_THROB2) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_THROB2);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, FALSE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, TRUE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_throb4_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_THROB4) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_THROB4);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, FALSE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, TRUE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_bpsk31_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_BPSK31) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_BPSK31);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, TRUE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_PHASE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_qpsk31_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_QPSK31) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_QPSK31);
			trx_set_state_wait(state);
		}

		set_sens(TRUE, TRUE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_PHASE);

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());
		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_psk63_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_PSK63) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_PSK63);
			trx_set_state_wait(state);
		}

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());

		set_sens(TRUE, TRUE, TRUE, TRUE);
		set_scope_mode(MINISCOPE_MODE_PHASE);

		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_mt63_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_MT63) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_MT63);
			trx_set_state_wait(state);
		}

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());

		set_sens(FALSE, TRUE, FALSE, FALSE);
		set_scope_mode(MINISCOPE_MODE_BLANK);

		waterfall_set_centerline(waterfall, FALSE);
		waterfall_set_fixed(waterfall, TRUE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_feldhell1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_FELDHELL) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_FELDHELL);
			trx_set_state_wait(state);
		}

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());

		set_sens(FALSE, FALSE, FALSE, TRUE);
		set_scope_mode(MINISCOPE_MODE_BLANK);

		waterfall_set_centerline(waterfall, TRUE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 1);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "%");

		statusbar_set_mode(trx_get_mode());
	}
}


void
on_cw1_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(menuitem);
	GtkWidget *w;

	if (item->active) {
		if (trx_get_mode() != MODE_CW) {
			gint state = trx_get_state();

			trx_set_state_wait(TRX_STATE_ABORT);
			trx_set_mode(MODE_CW);
			trx_set_state_wait(state);
		}

		waterfall_set_samplerate(waterfall, trx_get_samplerate());
		waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
		waterfall_set_frequency(waterfall, trx_get_freq());

		set_sens(FALSE, TRUE, FALSE, TRUE);
		set_scope_mode(MINISCOPE_MODE_SCOPE);

		waterfall_set_centerline(waterfall, TRUE);
		waterfall_set_fixed(waterfall, FALSE);

		w = lookup_widget(appwindow, "trxnotebook");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(w), 0);

		w = lookup_widget(appwindow, "metricdial");
		gtk_dial_set_unit(GTK_DIAL(w), "WPM");

		statusbar_set_mode(trx_get_mode());
	}
}

/* ---------------------------------------------------------------------- */

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *widget;

	widget = g_object_get_data(G_OBJECT(appwindow), "configdialog");

	if (widget) {
		gtk_window_present(GTK_WINDOW(widget));
		return;
	}

	widget = confdialog_init();

	if (widget) {
		gtk_widget_show(widget);
		g_object_set_data(G_OBJECT(appwindow), "configdialog", widget);
	}
}


void
on_macroconfig1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(1);
}


void
on_macroconfig2_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(2);
}


void
on_macroconfig3_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(3);
}


void
on_macroconfig4_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(4);
}


void
on_macroconfig5_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(5);
}


void
on_macroconfig6_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(6);
}


void
on_macroconfig7_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(7);
}


void
on_macroconfig8_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(8);
}


void
on_macroconfig9_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(9);
}


void
on_macroconfig10_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(10);
}


void
on_macroconfig11_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(11);
}


void
on_macroconfig12_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	macroconfig(12);
}

/* ---------------------------------------------------------------------- */

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *aboutwindow;

	aboutwindow = create_aboutwindow();
	gtk_widget_show(aboutwindow);
}


void
on_contents1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GError *error = NULL;

	gnome_help_display("gmfsk.xml", NULL, &error);

	if (error != NULL) {
		errmsg(error->message);
		g_error_free(error);
	}
}


/* ---------------------------------------------------------------------- */

void
on_pausebutton_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean b = gtk_toggle_button_get_active(togglebutton);

	if (b && trx_get_state() != TRX_STATE_ABORT) {
		trx_set_state(TRX_STATE_PAUSE);
	}
}


void
on_rxbutton_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean b = gtk_toggle_button_get_active(togglebutton);

	if (b)  trx_set_state(TRX_STATE_RX);
}


void
on_txbutton_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget *widget;
	gboolean b = gtk_toggle_button_get_active(togglebutton);

	if (b) {
		trx_set_state(TRX_STATE_TX);

		/* set focus to the tx widget */
		switch (trx_get_mode()) {
		case MODE_FELDHELL:
		case MODE_FMHELL:
			widget = lookup_widget(appwindow, "txentry");
			gtk_widget_grab_focus(widget);
			break;
		default:
			widget = lookup_widget(appwindow, "txtext");
			gtk_widget_grab_focus(widget);
			break;
		}
	}
}


void
on_abortbutton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
	gint state = trx_get_state();

	if (state != TRX_STATE_ABORT && state != TRX_STATE_PAUSE) {
		trx_set_state_wait(TRX_STATE_ABORT);

//		/* clear tx typeahead buffer */
//		while (trx_get_tx_char() != -1);

		/* switch to rx */
		trx_set_state_wait(TRX_STATE_RX);
		push_button("rxbutton");
	}
}


static gint
tune_timeout_cb                        (gpointer unused)
{
	gdk_threads_enter();
	if (trx_get_state() == TRX_STATE_TUNE)
		push_button("rxbutton");
	gdk_threads_leave();

	return FALSE;
}

void
on_tunebutton_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gboolean b = gtk_toggle_button_get_active(togglebutton);

	if (b) {
		trx_set_state(TRX_STATE_TUNE);
		g_timeout_add(30000, tune_timeout_cb, NULL);
	}
}

/* ---------------------------------------------------------------------- */

void
on_qsoentry_changed                    (GtkEditable     *editable,
                                        gpointer         user_data)
{
	qsodata_set_starttime(FALSE);
}


void
on_logbutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	qsodata_log();
}


void
on_newbutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	qsodata_clear();
}

/* ---------------------------------------------------------------------- */

void
on_txtext_delete_from_cursor           (GtkTextView     *textview,
                                        GtkDeleteType    type,
                                        gint             count,
                                        gpointer         user_data)
{
	GtkTextBuffer *textbuffer;
	GtkTextMark *mark1, *mark2;
	GtkTextIter iter1, iter2;

	if (type != GTK_DELETE_CHARS || count != -1)
		return;

	textbuffer = gtk_text_view_get_buffer(textview);

	/* get the iterator at TX mark */
	mark1 = gtk_text_buffer_get_mark(textbuffer, "editablemark");
	gtk_text_buffer_get_iter_at_mark(textbuffer, &iter1, mark1);

	/* get the iterator at the insert mark */
	mark2 = gtk_text_buffer_get_insert(textbuffer);
	gtk_text_buffer_get_iter_at_mark(textbuffer, &iter2, mark2);

	/* If they're not the same we have nothing to do */
	if (!gtk_text_iter_equal(&iter1, &iter2))
		return;

	/*
	 * User hit backspace at the TX mark, queue a BS character
	 * and delete the previous char. It should be non-editable
	 * so the default handler won't delete it.
	 */
	trx_queue_backspace();
	gtk_text_iter_backward_char(&iter1);
	gtk_text_buffer_delete(textbuffer, &iter1, &iter2);
}


void
on_txtext_populate_popup               (GtkTextView     *textview,
                                        GtkMenu         *menu,
                                        gpointer         user_data)
{
	GtkWidget *menu_item;

	/* Add the separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

	/* Add clear button */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
	// gtk_widget_set_sensitive(menu_item, TRUE);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_clear_tx_window1_activate), NULL);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
}

/* ---------------------------------------------------------------------- */

void
on_macrobutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(1);
}


gboolean
on_macrobutton1_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(1);
	return FALSE;
}


void
on_macrobutton2_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(2);
}


gboolean
on_macrobutton2_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(2);
	return FALSE;
}


void
on_macrobutton3_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(3);
}


gboolean
on_macrobutton3_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(3);
	return FALSE;
}


void
on_macrobutton4_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(4);
}


gboolean
on_macrobutton4_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(4);
	return FALSE;
}


void
on_macrobutton5_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(5);
}


gboolean
on_macrobutton5_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(5);
	return FALSE;
}


void
on_macrobutton6_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(6);
}


gboolean
on_macrobutton6_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(6);
	return FALSE;
}


void
on_macrobutton7_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(7);
}


gboolean
on_macrobutton7_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(7);
	return FALSE;
}


void
on_macrobutton8_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(8);
}


gboolean
on_macrobutton8_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(8);
	return FALSE;
}


void
on_macrobutton9_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(9);
}


gboolean
on_macrobutton9_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(9);
	return FALSE;
}


void
on_macrobutton10_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(10);
}


gboolean
on_macrobutton10_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(10);
	return FALSE;
}


void
on_macrobutton11_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(11);
}


gboolean
on_macrobutton11_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(11);
	return FALSE;
}


void
on_macrobutton12_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	send_macro(12);
}


gboolean
on_macrobutton12_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3)
		macroconfig(12);
	return FALSE;
}

/* ---------------------------------------------------------------------- */

gboolean
on_waterfall_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget *w = WFPopupMenu;
	gint i;

	/* we are interested in button three */
	if (event->button != 3)
		return FALSE;

	/* activate the correct mode radiobutton */
	i = conf_get_int("wf/mode") + 2;
	w = GTK_WIDGET(g_list_nth_data(GTK_MENU_SHELL(w)->children, i));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), TRUE);

	gtk_menu_popup(GTK_MENU(WFPopupMenu), NULL, NULL, NULL, NULL,
		       event->button, event->time);

	return FALSE;
}

void
on_waterfall1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
		conf_set_int("wf/mode", 0);
}


void
on_spectrum1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
		conf_set_int("wf/mode", 1);
}


void
on_scope1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
		conf_set_int("wf/mode", 2);
}

void
on_wfpause_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	waterfall_set_pause(waterfall, GTK_CHECK_MENU_ITEM(menuitem)->active);
}

void
on_wfproperties_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	confdialog_select_node("Waterfall:Settings");
}

/* ---------------------------------------------------------------------- */

void
on_freqspinbutton_changed              (GtkEditable     *editable,
                                        gpointer         user_data)
{
	GtkSpinButton *spin;
	gfloat freq;

	spin = GTK_SPIN_BUTTON(editable);
	freq = gtk_spin_button_get_value_as_float(spin);
	waterfall_set_frequency(waterfall, freq);
}


void
on_afcbutton_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	trx_set_afc(gtk_toggle_button_get_active(togglebutton));
}


void
on_squelchbutton_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	trx_set_squelch(gtk_toggle_button_get_active(togglebutton));
}


void
on_reversebutton_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	trx_set_reverse(gtk_toggle_button_get_active(togglebutton));
}

/* ---------------------------------------------------------------------- */

static void
destroy_txfileselection(void)
{
	GtkWidget *w;

	w = g_object_get_data(G_OBJECT(appwindow), "txfileselectdialog");

	if (w != NULL)
		gtk_widget_destroy(w);

	g_object_set_data(G_OBJECT(appwindow), "txfileselectdialog", NULL);
}

gboolean
on_txfileselection_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	destroy_txfileselection();
	return FALSE;
}


void
on_txfileselect_ok_button_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkFileSelection *f;
	GtkWidget *w;
	gchar *filename, *contents, *converted;
	GError *err = NULL;

	w = g_object_get_data(G_OBJECT(appwindow), "txfileselectdialog");
	f = GTK_FILE_SELECTION(w);
	filename = g_strdup(gtk_file_selection_get_filename(f));

	destroy_txfileselection();

	if (g_file_get_contents(filename, &contents, NULL, &err) == FALSE) {
		if (err != NULL) {
			errmsg(_("Error reading file: %s"), err->message);
			g_free(filename);
		}
		return;
	}

	if ((converted = g_locale_to_utf8(contents, -1, NULL, NULL, &err)) == NULL) {
		if (err != NULL) {
			errmsg(_("Error converting file to UTF8: %s"), err->message);
			g_free(filename);
			g_free(contents);
		}
		return;
	}

	send_string(converted);

	g_free(filename);
	g_free(contents);
	g_free(converted);
}


void
on_txfileselect_cancel_button_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{
	destroy_txfileselection();
}

/* ---------------------------------------------------------------------- */

static gchar *qsodatatext = NULL;

static void word_start(GtkTextIter *iter)
{
	while (gtk_text_iter_backward_char(iter))
		if (!g_unichar_isalnum(gtk_text_iter_get_char(iter)))
			break;

	if (!g_unichar_isalnum(gtk_text_iter_get_char(iter)))
		gtk_text_iter_forward_char(iter);
}

static void word_end(GtkTextIter *iter)
{
	while (gtk_text_iter_forward_char(iter))
		if (!g_unichar_isalnum(gtk_text_iter_get_char(iter)))
			break;
}

gboolean
on_rxtext_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkTextIter iter, start, end;
	GtkTextBuffer *buffer;
	gint x, y;

	g_free(qsodatatext);
	qsodatatext = NULL;

	if (event->button != 3)
		return FALSE;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

	/* If there already is a selection, use it */
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		qsodatatext = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		return FALSE;
	}

	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget),
					      GTK_TEXT_WINDOW_WIDGET,
					      event->x, event->y,
					      &x, &y);

	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, x, y);

	if (!gtk_text_iter_inside_word(&iter))
		return FALSE;

	start = iter;
	end = iter;

	word_start(&start);
	word_end(&end);
	qsodatatext = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	gtk_text_buffer_move_mark_by_name(buffer, "selection_bound", &start);
	gtk_text_buffer_move_mark_by_name(buffer, "insert", &end);

	return FALSE;
}

static void
on_rxtext_qsodata_activated            (GtkMenuItem *menuitem,
                                        gpointer data)
{
	gchar *widgetname = (gchar *) data;
	GtkWidget *w;

	if ((w = lookup_widget(appwindow, widgetname)) != NULL)
		gtk_entry_set_text(GTK_ENTRY(w), qsodatatext);
}

static void
on_rxtext_menu_selection_done          (GtkMenuShell *menushell,
					gpointer data)
{
	GtkTextView *view;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	view = GTK_TEXT_VIEW(lookup_widget(appwindow, "rxtext"));
	buffer = gtk_text_view_get_buffer(view);

	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_move_mark_by_name(buffer, "selection_bound", &iter);
	gtk_text_buffer_move_mark_by_name(buffer, "insert", &iter);
}

void
on_rxtext_populate_popup               (GtkTextView     *textview,
                                        GtkMenu         *menu,
                                        gpointer         user_data)
{
	GtkWidget *menu_item;
	gboolean flag = FALSE;

	if (qsodatatext != NULL)
		flag = TRUE;

	g_signal_connect(G_OBJECT(menu), "selection-done",
			 G_CALLBACK(on_rxtext_menu_selection_done), NULL);

	/* Prepend separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

	/* Prepend clear button */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
	// gtk_widget_set_sensitive(menu_item, TRUE);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_clear_rx_window1_activate), NULL);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

	/* Append separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	/* Append qsodata buttons */
	menu_item = gtk_menu_item_new_with_label(_("Callsign"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsocallentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Name"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsonameentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("QTH"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsoqthentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Locator"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsolocentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Sent RST"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsotxrstentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Received RST"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsorxrstentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Notes"));
	gtk_widget_set_sensitive(menu_item, flag);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate",
			 G_CALLBACK(on_rxtext_qsodata_activated),
			 (gpointer) "qsonotesentry");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
}

/* ---------------------------------------------------------------------- */

void
on_confrestartbutton_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	gint state = trx_get_state();

	trx_set_state_wait(TRX_STATE_ABORT);
	trx_set_state_wait(state);

	waterfall_set_bandwidth(waterfall, trx_get_bandwidth());
	waterfall_set_frequency(waterfall, trx_get_freq());
}

/* ---------------------------------------------------------------------- */

gboolean
on_modeeventbox_button_press_event     (GtkWidget       *eventwidget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	switch (trx_get_mode()) {
	case MODE_MFSK16:
	case MODE_MFSK8:
		confdialog_select_node("Modems:MFSK");
		break;
	case MODE_OLIVIA:
		confdialog_select_node("Modems:OLIVIA");
		break;
	case MODE_RTTY:
		confdialog_select_node("Modems:RTTY");
		break;
	case MODE_THROB1:
	case MODE_THROB2:
	case MODE_THROB4:
		confdialog_select_node("Modems:THROB");
		break;
	case MODE_BPSK31:
	case MODE_QPSK31:
	case MODE_PSK63:
		confdialog_select_node("Modems:PSK31");
		break;
	case MODE_MT63:
		confdialog_select_node("Modems:MT63");
		break;
	case MODE_FELDHELL:
	case MODE_FMHELL:
		confdialog_select_node("Modems:HELL");
		break;
	case MODE_CW:
		confdialog_select_node("Modems:CW");
		break;
	}

	return TRUE;
}

/* ---------------------------------------------------------------------- */

void
on_qsybutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
#if WANT_HAMLIB
	hamlib_set_qsy();
#else
	g_warning("on_qsybutton_clicked: Hamlib support not compiled in");
#endif
}
