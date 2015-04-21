/*
 *    rtty.c  --  RTTY modem
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
#include <stdio.h>

#include "trx.h"
#include "rtty.h"
#include "baudot.h"
#include "filter.h"
#include "fftfilt.h"

static void rtty_txinit(struct trx *trx)
{
        struct rtty *r = (struct rtty *) trx->modem;

	r->rxmode = BAUDOT_LETS;
	r->txmode = BAUDOT_LETS;

	/* start each transmission with 440ms of MARK tone */
	r->preamble = 20;
}

static void rtty_free(struct rtty *s)
{
	if (s) {
		filter_free(s->hilbert);
		g_free(s);
	}
}

static void rtty_rxinit(struct trx *trx)
{
        struct rtty *r = (struct rtty *) trx->modem;

	r->rxmode = BAUDOT_LETS;
	r->txmode = BAUDOT_LETS;
}

static void rtty_destructor(struct trx *trx)
{
	struct rtty *s = (struct rtty *) trx->modem;

	rtty_free(s);

        trx->modem = NULL;
        trx->txinit = NULL;
        trx->rxinit = NULL;
        trx->txprocess = NULL;
        trx->rxprocess = NULL;
        trx->destructor = NULL;
}

void rtty_init(struct trx *trx)
{
	struct rtty *s;
	double flo, fhi, bw;

	s = g_new0(struct rtty, 1);

	s->shift = trx->rtty_shift;
	s->symbollen = (int) (SampleRate / trx->rtty_baud + 0.5);

	switch (trx->rtty_bits) {
	case 0:
		s->nbits = 5;
		break;
	case 1:
		s->nbits = 7;
		break;
	case 2:
		s->nbits = 8;
		break;
	}

	switch (trx->rtty_parity) {
	case 0:
		s->parity = PARITY_NONE;
		break;
	case 1:
		s->parity = PARITY_EVEN;
		break;
	case 2:
		s->parity = PARITY_ODD;
		break;
	case 3:
		s->parity = PARITY_ZERO;
		break;
	case 4:
		s->parity = PARITY_ONE;
		break;
	}

	switch (trx->rtty_stop) {
	case 0:
		s->stoplen = (int) (1.0 * SampleRate / trx->rtty_baud + 0.5);
		break;
	case 1:
		s->stoplen = (int) (1.5 * SampleRate / trx->rtty_baud + 0.5);
		break;
	case 2:
		s->stoplen = (int) (2.0 * SampleRate / trx->rtty_baud + 0.5);
		break;
	}

	s->reverse = (trx->rtty_reverse == TRUE) ? 1 : 0;
	s->msb = (trx->rtty_msbfirst == TRUE) ? 1 : 0;

	if (s->symbollen > MaxSymLen || s->stoplen > MaxSymLen) {
		fprintf(stderr, "RTTY: symbol length too big\n");
		rtty_free(s);
		return;
	}

	bw = trx->rtty_baud * 1.1;
	flo = (s->shift / 2 - bw) / SampleRate;
	fhi = (s->shift / 2 + bw) / SampleRate;
	flo = 0;
//	fhi = 0.25;
//	fprintf(stderr, "flo=%f fhi=%f\n", flo * SampleRate, fhi * SampleRate);

	if ((s->hilbert = filter_init_hilbert(37, 1)) == NULL) {
		g_warning("rtty_init: init_hilbert failed\n");
		rtty_free(s);
		return;
	}

	if ((s->fftfilt = fftfilt_init(flo, fhi, 2048)) == NULL) {
		g_warning("rtty_init: init_fftfilt failed\n");
		rtty_free(s);
		return;
	}

        trx->modem = s;

	trx->txinit = rtty_txinit;
	trx->rxinit = rtty_rxinit;

	trx->txprocess = rtty_txprocess;
	trx->rxprocess = rtty_rxprocess;

	trx->destructor = rtty_destructor;

	trx->samplerate = SampleRate;
	trx->fragmentsize = 256;
	trx->bandwidth = s->shift;
}
