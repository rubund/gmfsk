/*
 *    trx.c  --  Modem engine
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
#include <pthread.h>

#include "main.h"
#include "trx.h"
#include "snd.h"
#include "ptt.h"
#include "misc.h"
#include "picture.h"

#include "waterfall.h"
#include "miniscope.h"

#include "support.h"

#define	TRX_DEBUG	0

extern void mfsk_init(struct trx *trx);
extern void olivia_init(struct trx *trx);
extern void rtty_init(struct trx *trx);
extern void throb_init(struct trx *trx);
extern void psk31_init(struct trx *trx);
extern void mt63_init(struct trx *trx);
extern void feld_init(struct trx *trx);
extern void cw_init(struct trx *trx);

/* ---------------------------------------------------------------------- */

char *trx_mode_names[] = {
	"MFSK16",
	"MFSK8",
	"OLIVIA",
	"RTTY",
	"THROB1",
	"THROB2",
	"THROB4",
	"BPSK31",
	"QPSK31",
	"PSK63",
	"MT63",
	"FELDHELL",
	"FMHELL",
	"CW"
};

char *trx_state_names[] = {
	"PAUSED",
	"RECEIVE",
	"TRANSMIT",
	"TUNING",
	"ABORTED",
	"FLUSHING"
};

/* ---------------------------------------------------------------------- */

static pthread_mutex_t trx_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t trx_cond = PTHREAD_COND_INITIALIZER;
static pthread_t trx_thread;
static struct trx trx;

/* ---------------------------------------------------------------------- */

#define BLOCKLEN	512

static void receive_loop(void)
{
	gfloat *buf;
	gint len;

	if (sound_open_for_read(trx.samplerate) < 0) {
		errmsg(_("sound_open_for_read: %s"), sound_error());
		trx_set_state(TRX_STATE_ABORT);
		return;
	}

	trx.rxinit(&trx);
	statusbar_set_trxstate(TRX_STATE_RX);

	for (;;) {
		pthread_mutex_lock(&trx_mutex);

		if (trx.state != TRX_STATE_RX) {
			pthread_mutex_unlock(&trx_mutex);
			break;
		}

		pthread_mutex_unlock(&trx_mutex);

		len = BLOCKLEN;

		if (sound_read(&buf, &len) < 0) {
			errmsg(_("%s"), sound_error());
			trx_set_state(TRX_STATE_ABORT);
			break;
		}

		waterfall_set_data(waterfall, buf, len);
		trx.rxprocess(&trx, buf, len);
	}

	sound_close();
}

static void transmit_loop(void)
{
	gfloat f = 0.0;

	if (sound_open_for_write(trx.samplerate) < 0) {
		errmsg(_("sound_open_for_write: %s"), sound_error());
		trx_set_state(TRX_STATE_ABORT);
		return;
	}

	trx_set_scope(&f, 1, FALSE);
	trx_set_phase(0.0, FALSE);
	trx.metric = 0.0;

	trx.txinit(&trx);
	ptt_set(1);

	for (;;) {
		pthread_mutex_lock(&trx_mutex);

		if (trx.state == TRX_STATE_ABORT) {
			pthread_mutex_unlock(&trx_mutex);
			break;
		}

		if (trx.state == TRX_STATE_RX || trx.state == TRX_STATE_PAUSE) {
			statusbar_set_trxstate(TRX_STATE_FLUSH);
			trx.stopflag = 1;
		}

		if (trx.state == TRX_STATE_TUNE) {
			statusbar_set_trxstate(TRX_STATE_TUNE);
			trx.tune = 1;
		}

		if (trx.state == TRX_STATE_TX) {
			statusbar_set_trxstate(TRX_STATE_TX);
		}

		pthread_mutex_unlock(&trx_mutex);

		if (trx.txprocess(&trx) < 0)
			break;
	}

	sound_close();
	ptt_set(0);

	pthread_mutex_lock(&trx_mutex);
	g_free(trx.txstr);
	trx.txptr = NULL;
	trx.txstr = NULL;
	trx.backspaces = 0;
	trx.stopflag = 0;
	trx.tune = 0;
	pthread_mutex_unlock(&trx_mutex);

	clear_tx_text();
}

static void *trx_loop(void *args)
{
	for (;;) {
		pthread_mutex_lock(&trx_mutex);

		if (trx.state == TRX_STATE_PAUSE || \
		    trx.state == TRX_STATE_ABORT)
			break;

		/* signal a state change (other than pause or abort) */
		pthread_cond_signal(&trx_cond);

		switch (trx.state) {
		case TRX_STATE_RX:
			pthread_mutex_unlock(&trx_mutex);
			receive_loop();
			break;

		case TRX_STATE_TX:
		case TRX_STATE_TUNE:
			pthread_mutex_unlock(&trx_mutex);
			transmit_loop();
			break;

		default:
			pthread_mutex_unlock(&trx_mutex);
			break;
		}
	}

	trx.destructor(&trx);

	trx.modem = NULL;
	trx.state = TRX_STATE_PAUSE;
	trx.metric = 0.0;

	g_free(trx.txstr);
	trx.txstr = NULL;
	trx.txptr = NULL;

	/* do this before sending the signal, otherwise there is a race */
	statusbar_set_trxstate(TRX_STATE_PAUSE);

	/* signal a state change */
	pthread_cond_signal(&trx_cond);

	pthread_mutex_unlock(&trx_mutex);

	/* this will exit the trx thread */
	return NULL;
}

/* ---------------------------------------------------------------------- */

gint trx_init(void)
{
	pthread_mutex_lock(&trx_mutex);

	if (trx.modem) {
		pthread_mutex_unlock(&trx_mutex);
		return -1;
	}

	switch (trx.mode) {
	case MODE_MFSK16:
	case MODE_MFSK8:
		mfsk_init(&trx);
		break;

	case MODE_OLIVIA:
		olivia_init(&trx);
		break;

	case MODE_RTTY:
		rtty_init(&trx);
		break;

	case MODE_THROB1:
	case MODE_THROB2:
	case MODE_THROB4:
		throb_init(&trx);
		break;

	case MODE_BPSK31:
	case MODE_QPSK31:
	case MODE_PSK63:
		psk31_init(&trx);
		break;

	case MODE_MT63:
		mt63_init(&trx);
		break;

	case MODE_FELDHELL:
	case MODE_FMHELL:
		feld_init(&trx);
		break;

	case MODE_CW:
		cw_init(&trx);
		break;
	}

	if (trx.modem == NULL) {
		errmsg(_("Modem initialization failed!"));
		pthread_mutex_unlock(&trx_mutex);
		return -1;
	}

	trx.stopflag = 0;

	pthread_mutex_unlock(&trx_mutex);

	if (pthread_create(&trx_thread, NULL, trx_loop, NULL) < 0) {
		errmsg(_("pthread_create: %m"));
		trx.state = TRX_STATE_PAUSE;
		return -1;
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

void trx_set_mode(trx_mode_t mode)
{
#if TRX_DEBUG
	fprintf(stderr,
		"trx_set_mode: old=%s new=%s\n",
		trx_mode_names[trx.mode],
		trx_mode_names[mode]);
#endif

	trx.mode = mode;
}

trx_mode_t trx_get_mode(void)
{
	return trx.mode;
}

char *trx_get_mode_name(void)
{
	return trx_mode_names[trx.mode];
}

void trx_set_state(trx_state_t state)
{
#if TRX_DEBUG
	fprintf(stderr,
		"trx_set_state: old=%s new=%s\n", 
		trx_state_names[trx.state], 
		trx_state_names[state]);
#endif

	pthread_mutex_lock(&trx_mutex);

	if (trx.state == state) {
		pthread_mutex_unlock(&trx_mutex);
		return;
	}
	trx.state = state;

	pthread_mutex_unlock(&trx_mutex);

	if (state == TRX_STATE_PAUSE || state == TRX_STATE_ABORT)
		return;

	/* (re-)initialize the trx */
	trx_init();
}

void trx_set_state_wait(trx_state_t state)
{
#if TRX_DEBUG
	fprintf(stderr,
		"trx_set_state_wait: old=%s new=%s\n", 
		trx_state_names[trx.state], 
		trx_state_names[state]);
#endif

	pthread_mutex_lock(&trx_mutex);

	if (trx.state == state) {
		pthread_mutex_unlock(&trx_mutex);
		return;
	}

	trx.state = state;

	if (trx.modem) {
		/* now wait for the main trx loop to respond */
		pthread_cond_wait(&trx_cond, &trx_mutex);
	}

	pthread_mutex_unlock(&trx_mutex);

	if (state == TRX_STATE_PAUSE || state == TRX_STATE_ABORT)
		return;

	/* (re-)initialize the trx */
	trx_init();
}

trx_state_t trx_get_state(void)
{
	return trx.state;
}

void trx_set_freq(gfloat freq)
{
	trx.frequency = freq;
}

gfloat trx_get_freq(void)
{
	return trx.frequency;
}

gfloat trx_get_bandwidth(void)
{
	return trx.bandwidth;
}

void trx_set_afc(gboolean on)
{
	trx.afcon = on;
}

void trx_set_squelch(gboolean on)
{
	trx.squelchon = on;
}

void trx_set_reverse(gboolean on)
{
	trx.reverse = on;
}

void trx_set_metric(gfloat metric)
{
	trx.metric = metric;
}

gfloat trx_get_metric(void)
{
	return trx.metric;
}

void trx_set_sync(gfloat sync)
{
	trx.syncpos = CLAMP(sync, 0.0, 1.0);
}

gfloat trx_get_sync(void)
{
	trx_mode_t mode;
	gfloat sync;

	pthread_mutex_lock(&trx_mutex);
	mode = trx.mode;
	sync = trx.syncpos;
	pthread_mutex_unlock(&trx_mutex);

	if (mode == MODE_THROB1 || mode == MODE_THROB2 || mode == MODE_THROB4)
		return sync;

	return -1.0;
}

void trx_set_txoffset(gfloat txoffset)
{
	trx.txoffset = txoffset;
}

gfloat trx_get_txoffset(void)
{
	return trx.txoffset;
}

void trx_set_mfsk_parms(gfloat squelch)
{
	trx.mfsk_squelch = squelch;
}

void trx_set_olivia_parms(gfloat squelch,
			  gint tones, gint bw,
			  gint smargin, gint sinteg,
			  gboolean esc)
{
	trx.olivia_squelch = squelch;
	trx.olivia_tones = tones;
	trx.olivia_bw = bw;
	trx.olivia_smargin = smargin;
	trx.olivia_sinteg = sinteg;
	trx.olivia_esc = esc;
}

void trx_set_rtty_parms(gfloat squelch, gfloat shift, gfloat baud,
			gint bits, gint parity, gint stop,
			gboolean reverse, gboolean msbfirst)
{
	trx.rtty_squelch = squelch;
	trx.rtty_shift = shift;
	trx.rtty_baud = baud;
	trx.rtty_bits = bits;
	trx.rtty_stop = stop;
	trx.rtty_reverse = reverse;
	trx.rtty_msbfirst = msbfirst;
}

void trx_set_throb_parms(gfloat squelch)
{
	trx.throb_squelch = squelch;
}

void trx_set_psk31_parms(gfloat squelch)
{
	trx.psk31_squelch = squelch;
}

void trx_set_mt63_parms(gfloat squelch,
			gint bandwidth, gint interleave,
			gboolean cwid, gboolean esc)
{
	trx.mt63_squelch = squelch;
	trx.mt63_bandwidth = bandwidth;
	trx.mt63_interleave = interleave;
	trx.mt63_cwid = cwid;
	trx.mt63_esc = esc;
}

void trx_set_hell_parms(gchar *font,
			gboolean upper,
			gfloat bandwidth, gfloat agcattack, gfloat agcdecay)
{
	trx.hell_font = font;
	trx.hell_upper = upper;
	trx.hell_bandwidth = bandwidth;
	trx.hell_agcattack = agcattack;
	trx.hell_agcdecay = agcdecay;
}

void trx_set_cw_parms(gfloat squelch, gfloat speed, gfloat bandwidth)
{
	trx.cw_squelch = squelch;
	trx.cw_speed = speed;
	trx.cw_bandwidth = bandwidth;
}

void trx_set_scope(gfloat *data, gint len, gboolean autoscale)
{
	Miniscope *m = MINISCOPE(lookup_widget(appwindow, "miniscope"));
	miniscope_set_data(m, data, len, autoscale);
}

void trx_set_phase(gfloat phase, gboolean highlight)
{
	Miniscope *m = MINISCOPE(lookup_widget(appwindow, "miniscope"));
	miniscope_set_phase(m, phase, highlight);
}

void trx_set_highlight(gboolean highlight)
{
	Miniscope *m = MINISCOPE(lookup_widget(appwindow, "miniscope"));
	miniscope_set_highlight(m, highlight);
}

gint trx_get_samplerate(void)
{
	return trx.samplerate;
}

/* ---------------------------------------------------------------------- */

static GAsyncQueue *rx_queue 			= NULL;
static GAsyncQueue *rx_hell_data_queue 		= NULL;
static GAsyncQueue *rx_picture_data_queue 	= NULL;
static GAsyncQueue *echo_queue 			= NULL;
static GAsyncQueue *tx_picture_queue 		= NULL;

void trx_init_queues(void)
{
	rx_queue 		= g_async_queue_new();
	rx_hell_data_queue 	= g_async_queue_new();
	rx_picture_data_queue 	= g_async_queue_new();
	echo_queue 		= g_async_queue_new();
	tx_picture_queue 	= g_async_queue_new();
}

/*
 * A very ugly hack. You can not g_async_queue_push() a 0 ...
 */
static inline gpointer gint_to_pointer(gint data)
{
	return (data == 0) ? GINT_TO_POINTER(-1) : GINT_TO_POINTER(data);
}

static inline gint gpointer_to_int(gpointer data)
{
	return (GPOINTER_TO_INT(data) == -1) ? 0 : GPOINTER_TO_INT(data);
}

void trx_put_rx_char(guint data)
{
	g_return_if_fail(rx_queue);
	g_async_queue_push(rx_queue, gint_to_pointer(data));
}

gint trx_get_rx_char(void)
{
	gpointer data;

	g_return_val_if_fail(rx_queue, -1);
	data = g_async_queue_try_pop(rx_queue);
	return data ? gpointer_to_int(data) : -1;
}

void trx_put_rx_data(guint data)
{
	g_return_if_fail(rx_hell_data_queue);
	g_async_queue_push(rx_hell_data_queue, gint_to_pointer(data));
}

gint trx_get_rx_data(void)
{
	gpointer data;

	g_return_val_if_fail(rx_hell_data_queue, -1);
	data = g_async_queue_try_pop(rx_hell_data_queue);
	return data ? gpointer_to_int(data) : -1;
}

void trx_put_rx_picdata(guint data)
{
	g_return_if_fail(rx_picture_data_queue);
	g_async_queue_push(rx_picture_data_queue, gint_to_pointer(data));
}

gint trx_get_rx_picdata(void)
{
	gpointer data;

	g_return_val_if_fail(rx_picture_data_queue, -1);
	data = g_async_queue_try_pop(rx_picture_data_queue);
	return data ? gpointer_to_int(data) : -1;
}

void trx_put_echo_char(guint data)
{
	g_return_if_fail(echo_queue);
	g_async_queue_push(echo_queue, gint_to_pointer(data));
}

gint trx_get_echo_char(void)
{
	gpointer data;

	g_return_val_if_fail(echo_queue, -1);
	data = g_async_queue_try_pop(echo_queue);
	return data ? gpointer_to_int(data) : -1;
}

void trx_put_tx_picture(gpointer picbuf)
{
	g_return_if_fail(tx_picture_queue);
	g_return_if_fail(picbuf);
	g_async_queue_push(tx_picture_queue, picbuf);
}

gpointer trx_get_tx_picture(void)
{
	g_return_val_if_fail(tx_picture_queue, NULL);
	return g_async_queue_try_pop(tx_picture_queue);
}

/* ---------------------------------------------------------------------- */

void trx_queue_backspace(void)
{
	trx.backspaces++;
}

/*
 * Check if 'str' contains any space characters or the RX_CMD character.
 */
static gboolean check_for_spaces(gchar *str)
{
	gunichar chr;

	while (*str) {
		chr = g_utf8_get_char(str);

		if (g_unichar_isspace(chr) || chr == TRX_RX_CMD)
			return TRUE;

		str = g_utf8_next_char(str);
	}

	return FALSE;
}

gunichar trx_get_hell_tx_char(void)
{
	GtkEditable *editable;
	gunichar chr;
	gchar *str;
	gint pos;
	gboolean spc;

	gdk_threads_enter();

	editable = GTK_EDITABLE(lookup_widget(appwindow, "txentry"));
	str = gtk_editable_get_chars(editable, 0, -1);

	chr = g_utf8_get_char(str);
	spc = check_for_spaces(str);

	g_free(str);

	/*
	 * If there are no spaces in the text, then send idle.
	 */
	if ((chr == 0) || (spc == FALSE && trx.state == TRX_STATE_TX)) {
		gdk_threads_leave();
		return -1;
	}

	/*
	 * Delete the first character and move cursor one step
	 * backwards.
	 */
	pos = gtk_editable_get_position(editable);
	gtk_editable_delete_text(editable, 0, 1);
	gtk_editable_set_position(editable, pos - 1);

	if (chr == TRX_RX_CMD) {
		push_button("rxbutton");
		chr = -1;
	}

	gdk_threads_leave();

	return chr;
}

gunichar trx_get_normal_tx_char(void)
{
	GtkTextView *view;
	GtkTextBuffer *buffer;
	GtkTextIter iter1, iter2;
	GtkTextMark *mark1, *mark2;
	gunichar chr;

	gdk_threads_enter();

	view = GTK_TEXT_VIEW(lookup_widget(appwindow, "txtext"));
	buffer = gtk_text_view_get_buffer(view);

	/* get the iterator at TX mark */
	mark1 = gtk_text_buffer_get_mark(buffer, "editablemark");
	gtk_text_buffer_get_iter_at_mark(buffer, &iter1, mark1);

	/* get the iterator at the insert mark */
	mark2 = gtk_text_buffer_get_insert(buffer);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter2, mark2);

	if (gtk_text_iter_equal(&iter1, &iter2)) {
		gdk_threads_leave();
		return -1;
	}

	chr = gtk_text_iter_get_char(&iter1);

	if (chr == 0) {
		gdk_threads_leave();
		return -1;
	}

	gtk_text_iter_forward_char(&iter1);
	gtk_text_buffer_get_start_iter(buffer, &iter2);
	gtk_text_buffer_apply_tag_by_name(buffer, "editabletag",
					  &iter2, &iter1);
	gtk_text_buffer_move_mark_by_name(buffer, "editablemark", &iter1);

	if (chr == TRX_RX_CMD) {
		push_button("rxbutton");
		chr = -1;
	}

	gdk_threads_leave();

	return chr;
}

gunichar trx_get_tx_char(void)
{
        GError *err = NULL;
	guchar utf[8], *str;
	gunichar chr;
	gint len;
	gchar *to = "ISO-8859-1";
	gchar *fm = "UTF-8";
	gchar *fb = ".";

	if (trx.mode == MODE_FELDHELL || trx.mode == MODE_FMHELL)
		return trx_get_hell_tx_char();

	if (trx.txstr && trx.txptr && *trx.txptr)
		return *trx.txptr++;

	g_free(trx.txstr);
	trx.txptr = NULL;
	trx.txstr = NULL;

	if (trx.backspaces > 0) {
		trx.backspaces--;
		return 8;
	}

	chr = trx_get_normal_tx_char();

	if (chr == -1 || 0xFFFC)
		return chr;

	len = g_unichar_to_utf8(chr, utf);
	str = g_convert_with_fallback(utf, len, to, fm, fb, NULL, NULL, &err);

	if (!str) {
		if (err) {
			g_warning(_("trx_get_tx_char: conversion failed: %s"), err->message);
			g_error_free(err);
		}
		return -1;
	}

	trx.txptr = trx.txstr = str;

	return *trx.txptr++;
}

/* ---------------------------------------------------------------------- */

