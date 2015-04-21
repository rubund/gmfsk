/*
 *    psk31rx.c  --  PSK31 demodulator
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

#include "psk31.h"
#include "filter.h"
#include "varicode.h"
#include "coeff.h"

static void rx_bit(struct trx *trx, int bit)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	int c;

	s->shreg = (s->shreg << 1) | !!bit;

	if ((s->shreg & 3) == 0) {
		c = psk_varicode_decode(s->shreg >> 2);

		if (c != -1)
			trx_put_rx_char(c);

		s->shreg = 0;
	}
}

static void rx_qpsk(struct trx *trx, int bits)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	unsigned char sym[2];
	int c;

	if (s->qpsk && trx->reverse)
		bits = (4 - bits) & 3;

	sym[0] = (bits & 1) ? 255 : 0;
	sym[1] = (bits & 2) ? 0 : 255;		/* top bit is flipped */

	c = viterbi_decode(s->dec, sym, NULL);

	if (c != -1) {
		rx_bit(trx, c & 0x80);
		rx_bit(trx, c & 0x40);
		rx_bit(trx, c & 0x20);
		rx_bit(trx, c & 0x10);
		rx_bit(trx, c & 0x08);
		rx_bit(trx, c & 0x04);
		rx_bit(trx, c & 0x02);
		rx_bit(trx, c & 0x01);
	}
}

static void rx_symbol(struct trx *trx, complex symbol)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	double phase, error;
	int bits, n;

	if ((phase = carg(ccor(s->prevsymbol, symbol))) < 0)
		phase += 2 * M_PI;

	if (s->qpsk) {
		bits = ((int) (phase / M_PI_2 + 0.5)) & 3;
		n = 4;
	} else {
		bits = (((int) (phase / M_PI + 0.5)) & 1) << 1;
		n = 2;
	}

	c_re(s->quality) = 0.02 * cos(n * phase) + 0.98 * c_re(s->quality);
	c_im(s->quality) = 0.02 * sin(n * phase) + 0.98 * c_im(s->quality);

	trx->metric = 100.0 * cpwr(s->quality);

	s->dcdshreg = (s->dcdshreg << 2) | bits;

	switch (s->dcdshreg) {
	case 0xAAAAAAAA:	/* DCD on by preamble */
		s->dcd = TRUE;
		c_re(s->quality) = 1;
		c_im(s->quality) = 0;
		break;

	case 0:			/* DCD off by postamble */
		s->dcd = FALSE;
		c_re(s->quality) = 0;
		c_im(s->quality) = 0;
		break;

	default:
		if (100.0 * cpwr(s->quality) > trx->psk31_squelch)
			s->dcd = TRUE;
		else
			s->dcd = FALSE;
		break;
	}

	trx_set_phase(phase, s->dcd);

	if (s->dcd == TRUE || trx->squelchon == FALSE) {
		if (s->qpsk)
			rx_qpsk(trx, bits);
		else
			rx_bit(trx, !bits);

		if (trx->afcon == TRUE) {
			error = (phase - bits * M_PI / 2);

			if (error < M_PI / 2)
				error += 2 * M_PI;
			if (error > M_PI / 2)
				error -= 2 * M_PI;

			error *= (double) SampleRate / (s->symbollen * 2 * M_PI);

			//fprintf(stderr, "error: %f\n", error);
			trx_set_freq(trx->frequency - (error / 16.0));
		}
	}

	s->prevsymbol = symbol;
}

static void update_syncscope(struct trx *trx)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	float data[16];
	unsigned int i, ptr;

	ptr = s->pipeptr - 24;

	for (i = 0; i < 16; i++)
		data[i] = s->pipe[ptr++ % PipeLen];

	trx_set_scope(data, 16, TRUE);
}

int psk31_rxprocess(struct trx *trx, float *buf, int len)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	double delta;
	complex z;
	int i;

	delta = 2.0 * M_PI * trx->frequency / SampleRate;

	while (len-- > 0) {
		/* Mix with the internal NCO */
		c_re(z) = *buf * cos(s->phaseacc);
		c_im(z) = *buf * sin(s->phaseacc);
		buf++;

		s->phaseacc += delta;

		if (s->phaseacc > M_PI)
			s->phaseacc -= 2.0 * M_PI;

		/* Filter and downsample by 16 or 8 */
		if (filter_run(s->fir1, z, &z)) {
			double sum;
			int idx;

			/* Do the second filtering */
			filter_run(s->fir2, z, &z);

			/* save amplitude value for the sync scope */
			s->pipe[s->pipeptr] = cmod(z);

			/* Now the sync correction routine... */
			idx = (int) s->bitclk;
			s->syncbuf[idx] = cmod(z);

			sum = 0.0;
			for (i = 0; i < 8; i++)
				sum += s->syncbuf[i];
			for (i = 8; i < 16; i++)
				sum -= s->syncbuf[i];

			s->bitclk -= sum / 5.0;
			//trx->metric = 50 + sum * 100;
			//fprintf(stderr, "sync: %f\n", sum);

			/* bit clock */
			s->bitclk += 1;
			if (s->bitclk >= 16) {
				s->bitclk -= 16;
				rx_symbol(trx, z);
				update_syncscope(trx);
			}

			s->pipeptr = (s->pipeptr + 1) % PipeLen;
		}
	}

	return 0;
}
