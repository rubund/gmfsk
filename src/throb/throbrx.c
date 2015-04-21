/*
 *    throbrx.c  --  THROB demodulator
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

#include <stdio.h>
#include <string.h>

#include "trx.h"
#include "throb.h"
#include "tab.h"
#include "filter.h"
#include "misc.h"
#include "fft.h"
#include "fftfilt.h"

static inline complex mixer(struct trx *trx, complex in)
{
	struct throb *s = (struct throb *) trx->modem;
	complex z;
	float f;

	c_re(z) = cos(s->phaseacc);
	c_im(z) = sin(s->phaseacc);

	z = cmul(z, in);

	f = trx->frequency;

	s->phaseacc -= 2.0 * M_PI * f / SampleRate;

	if (s->phaseacc < -M_PI)
		s->phaseacc += 2.0 * M_PI;
	if (s->phaseacc >  M_PI)
		s->phaseacc -= 2.0 * M_PI;

	return z;
}

static int findtones(complex *word, int *tone1, int *tone2)
{
	double max1, max2;
	int maxtone, i;

	max1 = 0;
	*tone1 = 0;
	for (i = 0; i < NumTones; i++) {
		if (cmod(word[i]) > max1) {
			max1 = cmod(word[i]);
			*tone1 = i;
		}
	}

	maxtone = *tone1;

	max2 = 0;
	*tone2 = 0;
	for (i = 0; i < NumTones; i++) {
		if (i == *tone1)
			continue;
		if (cmod(word[i]) > max2) {
			max2 = cmod(word[i]);
			*tone2 = i;
		}
	}

	if (max1 > max2 * 2)
		*tone2 = *tone1;

	if (*tone1 > *tone2) {
		i = *tone1;
		*tone1 = *tone2;
		*tone2 = i;
	}

	return maxtone;
}

static void decodechar(struct trx *trx, int tone1, int tone2)
{
	struct throb *s = (struct throb *) trx->modem;
	int i;

	if (s->shift == TRUE) {
		if (tone1 == 0 && tone2 == 8)
			trx_put_rx_char('?');

		if (tone1 == 1 && tone2 == 7)
			trx_put_rx_char('@');

		if (tone1 == 2 && tone2 == 6)
			trx_put_rx_char('=');

		if (tone1 == 4 && tone2 == 4)
			trx_put_rx_char('\n');

		s->shift = FALSE;
		return;
	}

	if (tone1 == 3 && tone2 == 5) {
		s->shift = TRUE;
		return;
	}

	for (i = 0; i < NumChars; i++) {
		if (ThrobTonePairs[i][0] == tone1 + 1 && 
		    ThrobTonePairs[i][1] == tone2 + 1) {
			trx_put_rx_char(ThrobCharSet[i]);
			break;
		}
	}

	return;
}

static void throb_rx(struct trx *trx, complex in)
{
	struct throb *s = (struct throb *) trx->modem;
	complex rxword[NumTones];
	int i, tone1, tone2, maxtone;

	/* store input */
	s->symbol[s->symptr] = in;

	/* check counter */
	if (s->rxcntr > 0.0)
		return;

	/* correlate against all tones */
	for (i = 0; i < NumTones; i++)
		rxword[i] = cmac(s->rxtone[i], s->symbol, s->symptr + 1, s->rxsymlen);

	/* find the strongest tones */
	maxtone = findtones(rxword, &tone1, &tone2);

	/* decode */
	decodechar(trx, tone1, tone2);

	if (trx->afcon == TRUE) {
		complex z1, z2;
		double f;

		z1 = rxword[maxtone];
		z2 = cmac(s->rxtone[maxtone], s->symbol, s->symptr + 2, s->rxsymlen);

		f = carg(ccor(z1, z2)) / (2 * DownSample * M_PI / SampleRate);
		f -= s->freqs[maxtone];

		trx_set_freq(trx->frequency + f / 8.0);
	}

	/* done with this symbol, start over */
	s->rxcntr = s->rxsymlen;
	s->waitsync = 1;
}

static void throb_sync(struct trx *trx, complex in)
{
	struct throb *s = (struct throb *) trx->modem;
	float f, maxval = 0;
	int i, maxpos = 0;

	/* "rectify", filter and store input */
	filter_I_run(s->syncfilt, cmod(in), &f);
	s->syncbuf[s->symptr] = f;

	/* check counter if we are waiting for sync */
	if (s->waitsync == 0 || s->rxcntr > (s->rxsymlen / 2.0))
		return;

	for (i = 0; i < s->rxsymlen; i++) {
		f = s->syncbuf[(i + s->symptr + 1) % s->rxsymlen];
		s->dispbuf[i] = f;
	}

	for (i = 0; i < s->rxsymlen; i++) {
		if (s->dispbuf[i] > maxval) {
			maxpos = i;
			maxval = s->dispbuf[i];
		}
	}

	/* correct sync */
	s->rxcntr += (maxpos - s->rxsymlen / 2) / 8.0;
	s->waitsync = 0;

	trx_set_scope(s->dispbuf, s->rxsymlen, TRUE);
}

int throb_rxprocess(struct trx *trx, float *buf, int len)
{
	struct throb *s = (struct throb *) trx->modem;
	complex z, *zp;
	int i, n;

	while (len-- > 0) {
		/* create analytic signal */
		c_re(z) = c_im(z) = *buf++;

		filter_run(s->hilbert, z, &z);

		/* shift down to 0 +- 32 (64) Hz */
		z = mixer(trx, z);

		/* low pass filter */
		n = fftfilt_run(s->fftfilt, z, &zp);

		/* downsample by 32 and push to the receiver */
		for (i = 0; i < n; i++) {
			if (++s->deccntr >= DownSample) {
				s->rxcntr -= 1.0;

				/* do symbol sync */
				throb_sync(trx, zp[i]);

				/* decode */
				throb_rx(trx, zp[i]);

				s->symptr = (s->symptr + 1) % s->rxsymlen;
				s->deccntr = 0;
			}
		}
	}

	return 0;
}
