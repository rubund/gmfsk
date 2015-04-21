/*
 *    mfsk.c  --  MFSK modem
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

#include <stdlib.h>
#include <glib.h>

#include "mfsk.h"
#include "trx.h"
#include "fft.h"
#include "sfft.h"
#include "filter.h"
#include "interleave.h"
#include "viterbi.h"

static void mfsk_txinit(struct trx *trx)
{
	struct mfsk *m = (struct mfsk *) trx->modem;

	m->txstate = TX_STATE_PREAMBLE;
	m->bitstate = 0;

	m->counter = 0;
}

static void mfsk_rxinit(struct trx *trx)
{
	struct mfsk *m = (struct mfsk *) trx->modem;

	m->rxstate = RX_STATE_DATA;
	m->synccounter = 0;
	m->symcounter = 0;
	m->met1 = 0.0;
	m->met2 = 0.0;

	m->counter = 0;
}

static void mfsk_free(struct mfsk *s)
{
	if (s) {
		fft_free(s->fft);
		sfft_free(s->sfft);
		filter_free(s->hilbert);

		g_free(s->pipe);

		encoder_free(s->enc);
		viterbi_free(s->dec1);
		viterbi_free(s->dec2);

		filter_free(s->filt);

		g_free(s);
	}
}

static void mfsk_destructor(struct trx *trx)
{
	struct mfsk *s = (struct mfsk *) trx->modem;

	mfsk_free(s);

	trx->modem = NULL;

	trx->txinit = NULL;
	trx->rxinit = NULL;
	trx->txprocess = NULL;
	trx->rxprocess = NULL;
	trx->destructor = NULL;
}

void mfsk_init(struct trx *trx)
{
	struct mfsk *s;
	double bw, cf, flo, fhi;

	s = g_new0(struct mfsk, 1);

	switch (trx->mode) {
	case MODE_MFSK16:
		s->symlen = 512;
		s->symbits = 4;
		s->basetone = 64;		/* 1000 Hz */
                break;

	case MODE_MFSK8:
		s->symlen = 1024;
		s->symbits = 5;
		s->basetone = 128;		/* 1000 Hz */
                break;

	default:
		mfsk_free(s);
		return;
	}

	s->numtones = 1 << s->symbits;
	s->tonespacing = (double) SampleRate / s->symlen;

	if (!(s->fft = fft_init(s->symlen, FFT_FWD))) {
		g_warning("mfsk_init: init_fft failed\n");
		mfsk_free(s);
		return;
	}
	if (!(s->sfft = sfft_init(s->symlen, s->basetone, s->basetone + s->numtones))) {
		g_warning("mfsk_init: init_sfft failed\n");
		mfsk_free(s);
		return;
	}
	if (!(s->hilbert = filter_init_hilbert(37, 1))) {
		g_warning("mfsk_init: init_hilbert failed\n");
		mfsk_free(s);
		return;
	}

	s->pipe = g_new0(struct rxpipe, 2 * s->symlen);

	if (!(s->enc = encoder_init(K, POLY1, POLY2))) {
		g_warning("mfsk_init: encoder_init failed\n");
		mfsk_free(s);
		return;
	}
	if (!(s->dec1 = viterbi_init(K, POLY1, POLY2))) {
		g_warning("mfsk_init: viterbi_init failed\n");
		mfsk_free(s);
		return;
	}
	if (!(s->dec2 = viterbi_init(K, POLY1, POLY2))) {
		g_warning("mfsk_init: viterbi_init failed\n");
		mfsk_free(s);
		return;
	}
	viterbi_set_traceback(s->dec1, 45);
	viterbi_set_traceback(s->dec2, 45);
	viterbi_set_chunksize(s->dec1, 1);
	viterbi_set_chunksize(s->dec2, 1);

	if (!(s->txinlv = interleave_init(s->symbits, INTERLEAVE_FWD))) {
		g_warning("mfsk_init: interleave_init failed\n");
		mfsk_free(s);
		return;
	}
	if (!(s->rxinlv = interleave_init(s->symbits, INTERLEAVE_REV))) {
		g_warning("mfsk_init: interleave_init failed\n");
		mfsk_free(s);
		return;
	}

	bw = (s->numtones - 1) * s->tonespacing;
	cf = 1000.0 + bw / 2.0;

	flo = (cf - bw) / SampleRate;
	fhi = (cf + bw) / SampleRate;

	if ((s->filt = filter_init_bandpass(127, 1, flo, fhi)) == NULL) {
		g_warning("mfsk_init: filter_init failed\n");
		mfsk_free(s);
		return;
	}

	trx->modem = s;

	trx->txinit = mfsk_txinit;
	trx->rxinit = mfsk_rxinit;

	trx->txprocess = mfsk_txprocess;
	trx->rxprocess = mfsk_rxprocess;

	trx->destructor = mfsk_destructor;

	trx->samplerate = SampleRate;
	trx->fragmentsize = s->symlen;
	trx->bandwidth = (s->numtones - 1) * s->tonespacing;
}
