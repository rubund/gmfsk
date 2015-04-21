/*
 *    psk31.c  --  PSK31 modem
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

#include "psk31.h"
#include "filter.h"
#include "coeff.h"

#define	K	5
#define	POLY1	0x17
#define	POLY2	0x19

static void psk31_txinit(struct trx *trx)
{
	struct psk31 *s = (struct psk31 *) trx->modem;

	s->phaseacc = 0;

	c_re(s->prevsymbol) = 1.0;
	c_im(s->prevsymbol) = 0.0;

	s->preamble = 32;

	s->shreg = 0;
}

static void psk31_rxinit(struct trx *trx)
{
	struct psk31 *s = (struct psk31 *) trx->modem;

	s->phaseacc = 0;

	c_re(s->prevsymbol) = 1.0;
	c_im(s->prevsymbol) = 0.0;

	c_re(s->quality) = 0.0;
	c_im(s->quality) = 0.0;

	s->shreg = 0;
	s->dcdshreg = 0;
	s->dcd = 0;

	s->bitclk = 0;
}

static void psk31_free(struct psk31 *s)
{
	if (s) {
		filter_free(s->fir1);
		filter_free(s->fir2);

		encoder_free(s->enc);
		viterbi_free(s->dec);

		g_free(s->txshape);
		g_free(s);
	}
}

static void psk31_destructor(struct trx *trx)
{
	struct psk31 *s = (struct psk31 *) trx->modem;

	psk31_free(s);

	trx->modem = NULL;
	trx->txinit = NULL;
	trx->rxinit = NULL;
	trx->txprocess = NULL;
	trx->rxprocess = NULL;
	trx->destructor = NULL;
}

void psk31_init(struct trx *trx)
{
	struct psk31 *s;
	int i;

	s = g_new0(struct psk31, 1);

	switch (trx->mode) {
	case MODE_BPSK31:
		s->symbollen = 256;
		s->qpsk = 0;
		break;
	case MODE_QPSK31:
		s->symbollen = 256;
		s->qpsk = 1;
		break;
	case MODE_PSK63:
		s->symbollen = 128;
		s->qpsk = 0;
		break;
	default:
		psk31_free(s);
		return;
	}

	s->fir1 = filter_init(FIRLEN, s->symbollen / 16, fir1c, fir1c);
	s->fir2 = filter_init(FIRLEN, 1, fir2c, fir2c);

	if (!s->fir1 || !s->fir2) {
		psk31_free(s);
		return;
	}

	if (s->qpsk) {
		s->enc = encoder_init(K, POLY1, POLY2);
		s->dec = viterbi_init(K, POLY1, POLY2);

		if (!s->enc || !s->dec) {
			psk31_free(s);
			return;
		}
	}

	s->txshape = g_new(gdouble, s->symbollen);

	/* raised cosine shape for the transmitter */
	for (i = 0; i < s->symbollen; i++)
		s->txshape[i] = 0.5 * cos(i * M_PI / s->symbollen) + 0.5;

        trx->modem = s;

	trx->txinit = psk31_txinit;
	trx->rxinit = psk31_rxinit;

	trx->txprocess = psk31_txprocess;
	trx->rxprocess = psk31_rxprocess;

	trx->destructor = psk31_destructor;

	trx->samplerate = SampleRate;
	trx->fragmentsize = s->symbollen;
	trx->bandwidth = (double) SampleRate / s->symbollen;
}
