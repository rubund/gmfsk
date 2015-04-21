/*
 *    throb.c  --  THROB modem
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

#include <stdlib.h>
#include <stdio.h>

#include "trx.h"
#include "throb.h"
#include "filter.h"
#include "fft.h"
#include "tab.h"
#include "misc.h"
#include "fftfilt.h"

static void throb_txinit(struct trx *trx)
{
	struct throb *s = (struct throb *) trx->modem;
	s->preamble = 4;
}

static void throb_rxinit(struct trx *trx)
{
	struct throb *s = (struct throb *) trx->modem;

	s->rxcntr = s->rxsymlen;

	s->waitsync = 1;
	s->deccntr = 0;
	s->symptr = 0;
	s->shift = 0;
}

static void throb_free(struct throb *s)
{
	int i;

	if (s) {
		g_free(s->txpulse);

		filter_free(s->hilbert);
		filter_free(s->syncfilt);
		fftfilt_free(s->fftfilt);

		for (i = 0; i < NumTones; i++)
			g_free(s->rxtone[i]);

		g_free(s);
	}
}

static void throb_destructor(struct trx *trx)
{
	throb_free((struct throb *) trx->modem);

	trx->modem = NULL;
	trx->txinit = NULL;
	trx->rxinit = NULL;
	trx->txprocess = NULL;
	trx->rxprocess = NULL;
	trx->destructor = NULL;
}

/*
 * Make a semi raised cosine pulse of length 'len'.
 */
static float *mk_semi_pulse(int len)
{
	float *pulse, x;
	int i, j;

	pulse = g_new0(gfloat, len);

	for (i = 0; i < len; i++) {
		if (i < len / 5) {
			x = M_PI * i / (len / 5.0);
			pulse[i] = 0.5 * (1 - cos(x));
		}

		if (i >= len / 5 && i < len * 4 / 5)
			pulse[i] = 1.0;

		if (i >= len * 4 / 5) {
			j = i - len * 4 / 5;
			x = M_PI * j / (len / 5.0);
			pulse[i] = 0.5 * (1 + cos(x));
		}
	}

	return pulse;
}

/*
 * Make a full raised cosine pulse of length 'len'.
 */
static float *mk_full_pulse(int len)
{
	float *pulse;
	int i;

	pulse = g_new0(gfloat, len);

	for (i = 0; i < len; i++)
		pulse[i] = 0.5 * (1 - cos(2 * M_PI * i / len));

	return pulse;
}

/*
 * Make a 32 times downsampled complex prototype tone for rx.
 */
static complex *mk_rxtone(double freq, float *pulse, int len)
{
	complex *tone;
	double x;
	int i;

	tone = g_new0(complex, len / DownSample);

	for (i = 0; i < len; i += DownSample) {
		x = -2.0 * M_PI * freq * i / SampleRate;
		c_re(tone[i / DownSample]) = pulse[i] * cos(x);
		c_im(tone[i / DownSample]) = pulse[i] * sin(x);
	}

	return tone;
}

void throb_init(struct trx *trx)
{
	struct throb *s;
	double bw;
	float *fp;
	int i;

	s = g_new0(struct throb, 1);

	switch (trx->mode) {
	case MODE_THROB1:
		s->symlen = SymbolLen1;
		s->txpulse = mk_semi_pulse(SymbolLen1);
		fp = mk_semi_pulse(SymbolLen1 / DownSample);
		for (i = 0; i < NumTones; i++)
			s->freqs[i] = ThrobToneFreqsNar[i];
		bw = 36.0 / SampleRate;
		break;

	case MODE_THROB2:
		s->symlen = SymbolLen2;
		s->txpulse = mk_semi_pulse(SymbolLen2);
		fp = mk_semi_pulse(SymbolLen2 / DownSample);
		for (i = 0; i < NumTones; i++)
			s->freqs[i] = ThrobToneFreqsNar[i];
		bw = 36.0 / SampleRate;
		break;

	case MODE_THROB4:
		s->symlen = SymbolLen4;
		s->txpulse = mk_full_pulse(SymbolLen4);
		fp = mk_full_pulse(SymbolLen4 / DownSample);
		for (i = 0; i < NumTones; i++)
			s->freqs[i] = ThrobToneFreqsWid[i];
		bw = 72.0 / SampleRate;
		break;

	default:
		throb_free(s);
		return;
	}

	s->rxsymlen = s->symlen / DownSample;

	if ((s->hilbert = filter_init_hilbert(37, 1)) == NULL) {
		throb_free(s);
		return;
	}

	if ((s->fftfilt = fftfilt_init(0, bw, FilterFFTLen)) == NULL) {
		throb_free(s);
		return;
	}

	if ((s->syncfilt = filter_init(s->symlen / DownSample, 1, fp, NULL)) == NULL) {
		throb_free(s);
		return;
	}

	for (i = 0; i < NumTones; i++) {
		s->rxtone[i] = mk_rxtone(s->freqs[i], s->txpulse, s->symlen);

		if (!s->rxtone[i]) {
			throb_free(s);
			return;
		}
	}

	trx->modem = s;

	trx->txinit = throb_txinit;
	trx->rxinit = throb_rxinit;

	trx->txprocess = throb_txprocess;
	trx->rxprocess = throb_rxprocess;

	trx->destructor = throb_destructor;

	trx->reverse = 0;
	trx->samplerate = SampleRate;
	trx->fragmentsize = s->symlen;
	trx->bandwidth = s->freqs[8] - s->freqs[0];
	trx->syncpos = 0.5;
}
