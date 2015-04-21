/*
 *    feldrx.c  --  FELDHELL receiver
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

#include "trx.h"
#include "feld.h"
#include "filter.h"
#include "fftfilt.h"
#include "misc.h"

#undef  MAX
#define MAX(a,b)		(((a)>(b))?(a):(b))
#undef  CLAMP
#define CLAMP(x,low,high)	(((x)>(high))?(high):(((x)<(low))?(low):(x)))

static inline complex mixer(struct trx *trx, complex in)
{
	struct feld *s = (struct feld *) trx->modem;
	complex z;

	c_re(z) = cos(s->rxphacc);
	c_im(z) = sin(s->rxphacc);

	z = cmul(z, in);

	s->rxphacc -= 2.0 * M_PI * trx->frequency / SampleRate;

	if (s->rxphacc > M_PI)
		s->rxphacc -= 2.0 * M_PI;
	else if (s->rxphacc < M_PI)
		s->rxphacc += 2.0 * M_PI;

	return z;
}

static void feld_rx(struct trx *trx, complex z)
{
	struct feld *s = (struct feld *) trx->modem;
	double x;

	s->rxcounter += DownSampleInc;

	if (s->rxcounter < 1.0)
		return;

	s->rxcounter -= 1.0;

	x = cmod(z);

	if (x > s->agc)
		s->agc = x;
	else
		s->agc *= (1 - 0.02 / RxColumnLen);

	x = 255 * CLAMP(1.0 - x / s->agc, 0.0, 1.0);
	trx_put_rx_data((int) x);

	trx->metric = s->agc / 10.0;
}

int feld_rxprocess(struct trx *trx, float *buf, int len)
{
	struct feld *s = (struct feld *) trx->modem;
	complex z, *zp;
	int i, n;

	if (trx->bandwidth != trx->hell_bandwidth) {
		float lp = trx->hell_bandwidth / 2.0 / SampleRate;

		fftfilt_set_freqs(s->fftfilt, 0, lp);

		trx->bandwidth = trx->hell_bandwidth;
	}

	while (len-- > 0) {
		/* create analytic signal... */
		c_re(z) = c_im(z) = *buf++;

		filter_run(s->hilbert, z, &z);

		/* ...so it can be shifted in frequency */
		z = mixer(trx, z);

		n = fftfilt_run(s->fftfilt, z, &zp);

		for (i = 0; i < n; i++)
			feld_rx(trx, zp[i]);
	}

	return 0;
}
