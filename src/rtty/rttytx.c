/*
 *    rttytx.c  --  RTTY modulator
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
#include "rtty.h"
#include "snd.h"
#include "baudot.h"
#include "rttypar.h"

static inline double nco(struct rtty *s, double freq)
{
	s->phaseacc += 2.0 * M_PI * freq / SampleRate;

	if (s->phaseacc > M_PI)
		s->phaseacc -= 2.0 * M_PI;

	return cos(s->phaseacc);
}

static void send_symbol(struct trx *trx, int symbol)
{
	struct rtty *s = (struct rtty *) trx->modem;
	double freq;
	int i, rev;

	rev = (trx->reverse != 0) ^ (s->reverse != 0);

	if (rev)
		symbol = !symbol;

	if (symbol)
		freq = trx->frequency - s->shift / 2.0;
	else
		freq = trx->frequency + s->shift / 2.0;

	freq += trx->txoffset;

	for (i = 0; i < s->symbollen; i++)
		trx->outbuf[i] = nco(s, freq);

	sound_write(trx->outbuf, s->symbollen);
}

static void send_stop(struct trx *trx)
{
	struct rtty *s = (struct rtty *) trx->modem;
	double freq;
	int i, rev;

	rev = (trx->reverse != 0) ^ (s->reverse != 0);

	if (rev)
		freq = trx->frequency + s->shift / 2.0;
	else
		freq = trx->frequency - s->shift / 2.0;

	freq += trx->txoffset;

	for (i = 0; i < s->stoplen; i++)
		trx->outbuf[i] = nco(s, freq);

	sound_write(trx->outbuf, s->stoplen);
}

static void send_char(struct trx *trx, int c)
{
	struct rtty *s = (struct rtty *) trx->modem;
	int i, j;

	if (s->nbits == 5) {
		if (c == BAUDOT_LETS)
			c = 0x1F;
		if (c == BAUDOT_FIGS)
			c = 0x1B;
	}

	/* start bit */
	send_symbol(trx, 0);

	/* data bits */
	for (i = 0; i < s->nbits; i++) {
		j = (s->msb) ? (s->nbits - 1 - i) : i;
		send_symbol(trx, (c >> j) & 1);
	}

	/* parity bit */
	if (s->parity != PARITY_NONE)
		send_symbol(trx, rtty_parity(c, s->nbits, s->parity));

	/* stop bit(s) */
	send_stop(trx);

	if (s->nbits == 5)
		c = baudot_dec(&s->rxmode, c);

	if (c != 0)
		trx_put_echo_char(c);
}

static void send_idle(struct trx *trx)
{
	struct rtty *s = (struct rtty *) trx->modem;

	if (s->nbits == 5)
		send_char(trx, BAUDOT_LETS);
	else
		send_char(trx, 0);
}

int rtty_txprocess(struct trx *trx)
{
	struct rtty *s = (struct rtty *) trx->modem;
	int c;

	if (trx->tune) {
		trx->tune = 0;
		send_symbol(trx, 1);
		return 0;
	}

	if (s->preamble > 0) {
		send_symbol(trx, 1);
		s->preamble--;
		return 0;
	}

	c = trx_get_tx_char();

	/* TX buffer empty */
	if (c == -1) {
		/* stop if requested to... */
		if (trx->stopflag)
			return -1;

		/* send idle character */
		send_idle(trx);
		s->txmode = BAUDOT_LETS;
		return 0;
	}

	if (s->nbits != 5) {
		send_char(trx, c);
		return 0;
	}

	/* unshift-on-space */
	if (c == ' ') {
		send_char(trx, BAUDOT_LETS);
		send_char(trx, 0x04);
		s->txmode = BAUDOT_LETS;
		return 0;
	}

	if ((c = baudot_enc(c)) < 0)
		return 0;

	/* switch case if necessary */
	if ((c & s->txmode) == 0) {
		if (s->txmode == BAUDOT_FIGS) {
			send_char(trx, BAUDOT_LETS);
			s->txmode = BAUDOT_LETS;
		} else {
			send_char(trx, BAUDOT_FIGS);
			s->txmode = BAUDOT_FIGS;
		}
	}

	send_char(trx, c & 0x1F);

	return 0;
}
