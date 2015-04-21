/*
 *    mfskrx.c  --  MFSK demodulator
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
                                                                                
/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mfsk.h"
#include "filter.h"
#include "sfft.h"
#include "varicode.h"
#include "misc.h"
#include "picture.h"

static void recvpic(struct trx *trx, complex z)
{
	struct mfsk *m = (struct mfsk *) trx->modem;

	m->picf += carg(ccor(m->prevz, z)) * SampleRate / (2.0 * M_PI);
	m->prevz = z;

	if ((m->counter % SAMPLES_PER_PIXEL) == 0) {
		m->picf = 256 * (m->picf / SAMPLES_PER_PIXEL - 1000) / trx->bandwidth;

		trx_put_rx_picdata(CLAMP(m->picf, 0.0, 255.0));

		m->picf = 0.0;
	}
}

static void recvchar(struct mfsk *m, int c)
{
	gint h, w;
	gboolean color;

	if (c == -1)
		return;

	memmove(m->picheader, m->picheader + 1, sizeof(m->picheader) - 1);

	m->picheader[sizeof(m->picheader) - 2] = (c == 0) ? ' ' : c;
	m->picheader[sizeof(m->picheader) - 1] = 0;

	if (picture_check_header(m->picheader, &w, &h, &color)) {
		if (m->symbolbit == 4)
			m->rxstate = RX_STATE_PICTURE_START_1;
		else
			m->rxstate = RX_STATE_PICTURE_START_2;
		
		m->picturesize = SAMPLES_PER_PIXEL * w * h * (color ? 3 : 1);
		m->counter = 0;

		if (color)
			trx_put_rx_picdata(('C' << 24) | (w << 12) | h);
		else
			trx_put_rx_picdata(('M' << 24) | (w << 12) | h);

		memset(m->picheader, ' ', sizeof(m->picheader));
	}

	trx_put_rx_char(c);
}

static void recvbit(struct mfsk *m, int bit)
{
	int c;

	m->datashreg = (m->datashreg << 1) | !!bit;

	/* search for "001" */
	if ((m->datashreg & 7) == 1) {
		/* the "1" belongs to the next symbol */
		c = varidec(m->datashreg >> 1);

		recvchar(m, c);

		/* we already received this */
		m->datashreg = 1;
	}
}

static void decodesymbol(struct trx *trx, unsigned char symbol)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	int c, met;

	m->symbolpair[0] = m->symbolpair[1];
	m->symbolpair[1] = symbol;

	m->symcounter = m->symcounter ? 0 : 1;

	/* MFSK16 doesn't need a vote */
	if (trx->mode == MODE_MFSK16 && m->symcounter)
		return;

	if (m->symcounter) {
		if ((c = viterbi_decode(m->dec1, m->symbolpair, &met)) == -1)
			return;

		m->met1 = decayavg(m->met1, met, 32.0);

		if (m->met1 < m->met2)
			return;

		trx->metric = m->met1 / 2.5;
	} else {
		if ((c = viterbi_decode(m->dec2, m->symbolpair, &met)) == -1)
			return;

		m->met2 = decayavg(m->met2, met, 32.0);

		if (m->met2 < m->met1)
			return;

		trx->metric = m->met2 / 2.5;
	}

	if (trx->squelchon && trx->metric < trx->mfsk_squelch)
		return;

	recvbit(m, c);

//	fprintf(stderr, "met1=%-5.1f met2=%-5.1f %d\n", 
//		m->met1, m->met2, m->met1 > m->met2 ? 1 : 2);
}

static void softdecode(struct trx *trx, complex *bins)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	float tone, sum, *b;
	unsigned char *symbols;
	int i, j, k;

	b = alloca(m->symbits * sizeof(float));
	symbols = alloca(m->symbits * sizeof(unsigned char));

	for (i = 0; i < m->symbits; i++)
		b[i] = 0.0;

	/* avoid divide by zero later */
	sum = 1e-10;

	/* gray decode and form soft decision samples */
	for (i = 0; i < m->numtones; i++) {
		j = graydecode(i);

		if (trx->reverse)
			k = (m->numtones - 1) - i;
		else
			k = i;

		tone = cmod(bins[k + m->basetone]);

		for (k = 0; k < m->symbits; k++)
			b[k] += (j & (1 << (m->symbits - k - 1))) ? tone : -tone;

		sum += tone;
	}

	/* shift to range 0...255 */
	for (i = 0; i < m->symbits; i++)
		symbols[i] = clamp(128.0 + (b[i] / sum * 128.0), 0, 255);

	interleave_syms(m->rxinlv, symbols);

	for (i = 0; i < m->symbits; i++) {
		/*
		 * The picture decoder needs to know exactly when the
		 * header is detected.
		 */
		m->symbolbit = i + 1;
		decodesymbol(trx, symbols[i]);
	}
}

static complex mixer(struct trx *trx, complex in)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	complex z;
	float f;

	f = trx->frequency - trx->bandwidth / 2;

	/* Basetone is always 1000 Hz */
	f -= 1000.0;

	c_re(z) = cos(m->phaseacc);
	c_im(z) = sin(m->phaseacc);

	z = cmul(z, in);

	m->phaseacc -= 2.0 * M_PI * f / SampleRate;

	if (m->phaseacc > M_PI)
		m->phaseacc -= 2.0 * M_PI;
	else if (m->phaseacc < M_PI)
		m->phaseacc += 2.0 * M_PI;

	return z;
}

static int harddecode(struct trx *trx, complex *in)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	double x, max = 0.0;
	int i, symbol = 0;

	in += m->basetone;

	for (i = 0; i < m->numtones; i++) {
		if ((x = cmod(in[i])) > max) {
			max = x;
			symbol = i;
		}
	}

	return symbol;
}

static void update_syncscope(struct mfsk *m)
{
	float *data;
	int i, j;

	data = alloca(2 * m->symlen * sizeof(float));

	for (i = 0; i < 2 * m->symlen; i++) {
		j = (i + m->pipeptr) % (2 * m->symlen);
		data[i] = cmod(m->pipe[j].vector[m->prev1symbol]);
	}

	trx_set_scope(data, 2 * m->symlen, TRUE);
}

static void synchronize(struct mfsk *m)
{
	int i, j, syn = -1;
	float val, max = 0.0;

	if (m->currsymbol == m->prev1symbol)
		return;
	if (m->prev1symbol == m->prev2symbol)
		return;

	j = m->pipeptr;

	for (i = 0; i < 2 * m->symlen; i++) {
		val = cmod(m->pipe[j].vector[m->prev1symbol]);

		if (val > max) {
			max = val;
			syn = i;
		}

		j = (j + 1) % (2 * m->symlen);
	}

	m->synccounter += (int) floor((syn - m->symlen) / 16.0 + 0.5);
}

static void afc(struct trx *trx)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	complex z;
	float x;

	if (trx->afcon == FALSE || trx->metric < trx->mfsk_squelch)
		return;

	if (m->currsymbol != m->prev1symbol)
		return;

	z = ccor(m->prev1vector, m->currvector);
	x = carg(z) / m->symlen / (2.0 * M_PI / SampleRate);

	if (x > -m->tonespacing / 2.0 &&  x < m->tonespacing / 2.0)
		trx_set_freq(trx->frequency + (x / 8.0));
}

int mfsk_rxprocess(struct trx *trx, float *buf, int len)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	complex z, *bins;
	int i;

	while (len-- > 0) {
		/* create analytic signal... */
		c_re(z) = c_im(z) = *buf++;

		filter_run(m->hilbert, z, &z);

		/* ...so it can be shifted in frequency */
		z = mixer(trx, z);

		filter_run(m->filt, z, &z);

		if (m->rxstate == RX_STATE_PICTURE_START_2) {
			if (m->counter++ == 352) {
				m->counter = 0;
				m->rxstate = RX_STATE_PICTURE;
			}
			continue;
		}
		if (m->rxstate == RX_STATE_PICTURE_START_1) {
			if (m->counter++ == 352 + m->symlen) {
				m->counter = 0;
				m->rxstate = RX_STATE_PICTURE;
			}
			continue;
		}

		if (m->rxstate == RX_STATE_PICTURE) {
			if (m->counter++ == m->picturesize) {
				m->counter = 0;
				m->rxstate = RX_STATE_DATA;
			} else
				recvpic(trx, z);
			continue;
		}

		/* feed it to the sliding FFT */
		bins = sfft_run(m->sfft, z);

		/* copy current vector to the pipe */
		for (i = 0; i < m->numtones; i++)
			m->pipe[m->pipeptr].vector[i] = bins[i + m->basetone];

		if (--m->synccounter <= 0) {
			m->synccounter = m->symlen;

			m->currsymbol = harddecode(trx, bins);
			m->currvector = bins[m->currsymbol + m->basetone];

			/* decode symbol */
			softdecode(trx, bins);

			/* update the scope */
			update_syncscope(m);

			/* symbol sync */
			synchronize(m);

			/* frequency tracking */
			afc(trx);

			m->prev2symbol = m->prev1symbol;
			m->prev2vector = m->prev1vector;
			m->prev1symbol = m->currsymbol;
			m->prev1vector = m->currvector;
		}

		m->pipeptr = (m->pipeptr + 1) % (2 * m->symlen);
	}

	return 0;
}
