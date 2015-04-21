/*
 *    trx.h  --  Modem engine
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
                                                                                
#ifndef	_TRX_H
#define	_TRX_H

#include <glib.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif 

/* ---------------------------------------------------------------------- */

#define	TRX_RX_CMD	0x21AF	/* special character for switching to rx */

#define	OUTBUFSIZE	16384

/* ---------------------------------------------------------------------- */

typedef enum {
	MODE_MFSK16 = 0,
	MODE_MFSK8,
	MODE_OLIVIA,
	MODE_RTTY,
	MODE_THROB1,
	MODE_THROB2,
	MODE_THROB4,
	MODE_BPSK31,
	MODE_QPSK31,
	MODE_PSK63,
	MODE_MT63,
	MODE_FELDHELL,
	MODE_FMHELL,
	MODE_CW
} trx_mode_t;

extern char *trx_mode_names[];

typedef enum {
	TRX_STATE_PAUSE = 0,
	TRX_STATE_RX,
	TRX_STATE_TX,
	TRX_STATE_TUNE,
	TRX_STATE_ABORT,
	TRX_STATE_FLUSH
} trx_state_t;

extern char *trx_state_names[];

struct trx {
	trx_mode_t mode;
	trx_state_t state;

	gint samplerate;
	gint fragmentsize;

	gint afcon;
	gint squelchon;
	gint stopflag;
	gint tune;
	gint reverse;

	gfloat frequency;
	gfloat bandwidth;
	gfloat metric;
	gfloat syncpos;
	gfloat txoffset;

	gint backspaces;
	guchar *txstr;
	guchar *txptr;

	gfloat outbuf[OUTBUFSIZE];

	gfloat mfsk_squelch;

	gfloat olivia_squelch;
	gint olivia_tones;
	gint olivia_bw;
	gint olivia_smargin;
	gint olivia_sinteg;
	gboolean olivia_esc;

	gfloat rtty_squelch;
	gfloat rtty_shift;
	gfloat rtty_baud;
	gint rtty_bits;
	gint rtty_parity;
	gint rtty_stop;
	gboolean rtty_reverse;
	gboolean rtty_msbfirst;

	gfloat throb_squelch;

	gfloat psk31_squelch;

	gfloat mt63_squelch;
	gint mt63_bandwidth;
	gint mt63_interleave;
	gboolean mt63_cwid;
	gboolean mt63_esc;

	gchar *hell_font;
	gboolean hell_upper;
	gfloat hell_bandwidth;
	gfloat hell_agcattack;
	gfloat hell_agcdecay;

	gfloat cw_squelch;
	gfloat cw_speed;
	gfloat cw_bandwidth;

	void *modem;

	void (*txinit) (struct trx *trx);
	void (*rxinit) (struct trx *trx);

	gint (*txprocess) (struct trx *trx);
	gint (*rxprocess) (struct trx *trx, gfloat *buf, gint len);

	void (*destructor) (struct trx *trx);
};

/* ---------------------------------------------------------------------- */

extern gint trx_init(void);

extern void trx_set_mode(trx_mode_t);
extern trx_mode_t trx_get_mode(void);
extern char *trx_get_mode_name(void);

extern void trx_set_state(trx_state_t);
extern void trx_set_state_wait(trx_state_t);
extern trx_state_t trx_get_state(void);

extern void trx_set_freq(gfloat);
extern gfloat trx_get_freq(void);

extern void trx_set_metric(gfloat);
extern gfloat trx_get_metric(void);

extern void trx_set_sync(gfloat);
extern gfloat trx_get_sync(void);

extern void trx_set_txoffset(gfloat);
extern gfloat trx_get_txoffset(void);

extern void trx_set_mfsk_parms(gfloat);

extern void trx_set_olivia_parms(gfloat, gint, gint, gint, gint, gboolean);

extern void trx_set_rtty_parms(gfloat, gfloat, gfloat,
			       gint, gint, gint,
			       gboolean, gboolean);

extern void trx_set_throb_parms(gfloat);

extern void trx_set_psk31_parms(gfloat);

extern void trx_set_mt63_parms(gfloat, gint, gint, gboolean, gboolean);

extern void trx_set_hell_parms(gchar *, gboolean, gfloat, gfloat, gfloat);

extern void trx_set_cw_parms(gfloat, gfloat, gfloat);

extern void trx_set_afc(gboolean on);
extern void trx_set_squelch(gboolean on);
extern void trx_set_reverse(gboolean on);

extern gfloat trx_get_bandwidth(void);

extern void trx_set_scope(gfloat *data, gint len, gboolean autoscale);
extern void trx_set_phase(gfloat phase, gboolean highlight);
extern void trx_set_highlight(gboolean highlight);

extern gint trx_get_samplerate(void);

/* ---------------------------------------------------------------------- */

extern void trx_init_queues(void);

extern void trx_put_echo_char(guint c);
extern gint trx_get_echo_char(void);

extern void trx_put_rx_char(guint c);
extern gint trx_get_rx_char(void);

extern void trx_put_rx_data(guint c);
extern gint trx_get_rx_data(void);

extern void trx_put_rx_picdata(guint c);
extern gint trx_get_rx_picdata(void);

extern void trx_put_tx_picture(gpointer picbuf);
extern gpointer trx_get_tx_picture(void);

/* ---------------------------------------------------------------------- */

extern void trx_queue_backspace(void);
extern gunichar trx_get_tx_char(void);

/* ---------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif 

#endif
