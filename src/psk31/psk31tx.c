/*
 *    psk31tx.c  --  PSK31 modulator
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

#include <ctype.h>

#include "psk31.h"
#include "varicode.h"
#include "snd.h"

static void send_symbol(struct trx *trx, int sym)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	float delta;
	complex symbol;
	int i;

	if (s->qpsk && trx->reverse)
		sym = (4 - sym) & 3;

	/* differential QPSK modulation - top bit flipped */
	switch (sym) {
	case 0:
		c_re(symbol) = -1.0;	/* 180 degrees */
		c_im(symbol) =  0.0;
		break;
	case 1:
		c_re(symbol) =  0.0;	/* 270 degrees */
		c_im(symbol) = -1.0;
		break;
	case 2:
		c_re(symbol) =  1.0;	/*   0 degrees */
		c_im(symbol) =  0.0;
		break;
	case 3:
		c_re(symbol) =  0.0;	/*  90 degrees */
		c_im(symbol) =  1.0;
		break;
	}
	symbol = cmul(s->prevsymbol, symbol);

	delta = 2.0 * M_PI * (trx->frequency + trx->txoffset) / SampleRate;

	for (i = 0; i < s->symbollen; i++) {
		float ival, qval;

		ival = s->txshape[i] * c_re(s->prevsymbol) +
			(1.0 - s->txshape[i]) * c_re(symbol);

		qval = s->txshape[i] * c_im(s->prevsymbol) + 
			(1.0 - s->txshape[i]) * c_im(symbol);

		trx->outbuf[i] = ival * cos(s->phaseacc) +
				 qval * sin(s->phaseacc);

		s->phaseacc += delta;

		if (s->phaseacc > M_PI)
			s->phaseacc -= 2.0 * M_PI;
	}

	sound_write(trx->outbuf, s->symbollen);

	/* save the current symbol */
	s->prevsymbol = symbol;
}

static void send_bit(struct trx *trx, int bit)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	unsigned int sym;

	if (s->qpsk)
		sym = encoder_encode(s->enc, bit);
	else
		sym = bit << 1;

	send_symbol(trx, sym);
}

static void send_char(struct trx *trx, unsigned char c)
{
	char *code;

	code = psk_varicode_encode(c);

	while (*code) {
		send_bit(trx, (*code - '0'));
		code++;
	}

	send_bit(trx, 0);
	send_bit(trx, 0);
}

static void flush_tx(struct trx *trx)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	int i;

	/* flush the encoder (QPSK only) */
	if (s->qpsk) {
		send_bit(trx, 0);
		send_bit(trx, 0);
		send_bit(trx, 0);
		send_bit(trx, 0);
		send_bit(trx, 0);
	}

	/* DCD off sequence (unmodulated carrier) */
	for (i = 0; i < 32; i++)
		send_symbol(trx, 2);
}

int psk31_txprocess(struct trx *trx)
{
	struct psk31 *s = (struct psk31 *) trx->modem;
	int c;

	if (trx->tune) {
		trx->tune = 0;
		send_symbol(trx, 0);	/* send phase reversals */
		return 0;
	}

	if (s->preamble > 0) {
		s->preamble--;
		send_symbol(trx, 0);	/* send phase reversals */
		return 0;
	}

	c = trx_get_tx_char();

	/* TX buffer empty */
	if (c == -1) {
		/* stop if requested to... */
		if (trx->stopflag) {
			flush_tx(trx);
			return -1;	/* we're done */
		}

		send_symbol(trx, 0);	/* send phase reversals */
		send_symbol(trx, 0);
		return 0;
	}

	send_char(trx, c);

	trx_put_echo_char(c);

	return 0;
}
