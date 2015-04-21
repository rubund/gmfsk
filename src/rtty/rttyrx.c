/*
 *    rttyrx.c  --  RTTY demodulator
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

#include "trx.h"
#include "rtty.h"
#include "filter.h"
#include "fftfilt.h"
#include "misc.h"
#include "baudot.h"
#include "rttypar.h"

static double bbfilt(struct rtty *s, double in)
{
	double out;
	int i;

	s->bbfilter[s->filterptr] = in;
	s->filterptr = (s->filterptr + 1) % s->symbollen;

	out = s->bbfilter[0];

	for (i = 1; i < s->symbollen; i++)
		out += s->bbfilter[i];

	return out / s->symbollen;
}

static void update_syncscope(struct rtty *s)
{
	float data[MaxSymLen];
	int i, j;

	for (i = 0; i < s->symbollen; i++) {
		j = (i + s->pipeptr) % s->symbollen;
		data[i] = 0.5 + 0.85 * s->pipe[j] / s->shift;
	}

	trx_set_scope(data, s->symbollen, FALSE);
}

static inline complex mixer(struct trx *trx, complex in)
{
	struct rtty *s = (struct rtty *) trx->modem;
	complex z;

	c_re(z) = cos(s->phaseacc);
	c_im(z) = sin(s->phaseacc);

	z = cmul(z, in);

	s->phaseacc -= 2.0 * M_PI * trx->frequency / SampleRate;

	if (s->phaseacc > M_PI)
		s->phaseacc -= 2.0 * M_PI;
	else if (s->phaseacc < M_PI)
		s->phaseacc += 2.0 * M_PI;

	return z;
}

static unsigned char bitreverse(unsigned char in, int n)
{
	unsigned char out = 0;
	int i;

	for (i = 0; i < n; i++)
		out = (out << 1) | ((in >> i) & 1);

	return out;
}

static int decode_char(struct rtty *s)
{
	unsigned int parbit, par, data;

	parbit = (s->rxdata >> s->nbits) & 1;
	par = rtty_parity(s->rxdata, s->nbits, s->parity);

	if (s->parity != PARITY_NONE && parbit != par) {
//		fprintf(stderr, "P");
		return 0;
	}

	data = s->rxdata & ((1 << s->nbits) - 1);

	if (s->msb)
		data = bitreverse(data, s->nbits);

	if (s->nbits == 5)
		return baudot_dec(&s->rxmode, data);

	return data;
}

static int rttyrx(struct rtty *s, int bit)
{
	int flag = 0;
	unsigned char c;

	switch (s->rxstate) {
	case RTTY_RX_STATE_IDLE:
		if (!bit) {
			s->rxstate = RTTY_RX_STATE_START;
			s->counter = s->symbollen / 2;
		}
		break;

	case RTTY_RX_STATE_START:
		if (--s->counter == 0) {
			if (!bit) {
				s->rxstate = RTTY_RX_STATE_DATA;
				s->counter = s->symbollen;
				s->bitcntr = 0;
				s->rxdata = 0;
				flag = 1;
			} else
				s->rxstate = RTTY_RX_STATE_IDLE;
		}
		break;

	case RTTY_RX_STATE_DATA:
		if (--s->counter == 0) {
			s->rxdata |= bit << s->bitcntr++;
			s->counter = s->symbollen;
			flag = 1;
		}

		if (s->bitcntr == s->nbits) {
			if (s->parity == PARITY_NONE)
				s->rxstate = RTTY_RX_STATE_STOP;
			else
				s->rxstate = RTTY_RX_STATE_PARITY;
		}
		break;

	case RTTY_RX_STATE_PARITY:
		if (--s->counter == 0) {
			s->rxstate = RTTY_RX_STATE_STOP;
			s->rxdata |= bit << s->bitcntr++;
			s->counter = s->symbollen;
			flag = 1;
		}
		break;

	case RTTY_RX_STATE_STOP:
		if (--s->counter == 0) {
			if (bit) {
				c = decode_char(s);
				trx_put_rx_char(c);
				flag = 1;
			} else {
//				fprintf(stderr, "F");
			}
			s->rxstate = RTTY_RX_STATE_STOP2;
			s->counter = s->symbollen / 2;
		}
		break;

	case RTTY_RX_STATE_STOP2:
		if (--s->counter == 0)
			s->rxstate = RTTY_RX_STATE_IDLE;
		break;
	}

	return flag;
}

int rtty_rxprocess(struct trx *trx, float *buf, int len)
{
	struct rtty *s = (struct rtty *) trx->modem;
	complex z, *zp;
	int n, i, bit, rev;
	double f;

	rev = (trx->reverse != 0) ^ (s->reverse != 0);

	while (len-- > 0) {
		/* create analytic signal... */
		c_re(z) = c_im(z) = *buf++;

		filter_run(s->hilbert, z, &z);

		/* ...so it can be shifted in frequency */
		z = mixer(trx, z);

		n = fftfilt_run(s->fftfilt, z, &zp);

		for (i = 0; i < n; i++) {
			static complex prev;

			f = carg(ccor(prev, zp[i])) * SampleRate / (2 * M_PI);
			prev = zp[i];

			f = bbfilt(s, f);
			s->pipe[s->pipeptr] = f;
			s->pipeptr = (s->pipeptr + 1) % s->symbollen;

			if (s->counter == s->symbollen / 2)
				update_syncscope(s);

//			f = bbfilt(s, f);
			if (rev)
				bit = (f > 0.0);
			else
				bit = (f < 0.0);

			if (rttyrx(s, bit) && trx->afcon) {
				if (f > 0.0)
					f = f - s->shift / 2;
				else
					f = f + s->shift / 2;

//				fprintf(stderr, "bit=%d f=% f\n", bit, f);

				if (fabs(f) < s->shift / 2)
					trx_set_freq(trx->frequency + f / 256);
			}
		}
	}

	return 0;
}
