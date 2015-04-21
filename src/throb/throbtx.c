/*
 *    throbtx.c  --  THROB modulator
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

#include <ctype.h>

#include "trx.h"
#include "throb.h"
#include "snd.h"
#include "tab.h"

static void send_throb(struct trx *trx, int symbol)
{
	struct throb *s = (struct throb *) trx->modem;
	float f;
	int tone1, tone2;
	double w1, w2;
	int i;

	if (symbol < 0 || symbol >= NumChars)
		return;

	tone1 = ThrobTonePairs[symbol][0] - 1;
	tone2 = ThrobTonePairs[symbol][1] - 1;

	if (trx->reverse) {
		tone1 = (NumTones - 1) - tone1;
		tone2 = (NumTones - 1) - tone2;
	}

	f = trx->frequency + trx->txoffset;

	w1 = 2.0 * M_PI * (f + s->freqs[tone1]) / SampleRate;
	w2 = 2.0 * M_PI * (f + s->freqs[tone2]) / SampleRate;

	for (i = 0; i < s->symlen; i++)
		trx->outbuf[i] = s->txpulse[i] * 
				 (sin(w1 * i) + sin(w2 * i)) / 2.0;

	sound_write(trx->outbuf, s->symlen);
}

int throb_txprocess(struct trx *trx)
{
	struct throb *s = (struct throb *) trx->modem;
	int i, c, sym;

	if (trx->tune) {
		trx->tune = 0;
		send_throb(trx, 0);	/* send idle throbs */
		return 0;
	}

	if (s->preamble > 0) {
		send_throb(trx, 0);	/* send idle throbs */
		s->preamble--;
		return 0;
	}

	c = trx_get_tx_char();

	/* TX buffer empty */
	if (c == -1) {
		/* stop if requested to... */
		if (trx->stopflag) {
			send_throb(trx, 0);
			return -1;
		}

		send_throb(trx, 0);	/* send idle throbs */
		return 0;
	}

	/* handle the special cases first */
	switch (c) {
	case '?':
		send_throb(trx, 5);	/* shift */
		send_throb(trx, 20);
		trx_put_echo_char(c);
		return 0;

	case '@':
		send_throb(trx, 5);	/* shift */
		send_throb(trx, 13);
		trx_put_echo_char(c);
		return 0;

	case '=':
		send_throb(trx, 5);	/* shift */
		send_throb(trx, 9);
		trx_put_echo_char(c);
		return 0;

	case '\r':
	case '\n':
		send_throb(trx, 5);	/* shift */
		send_throb(trx, 0);
		trx_put_echo_char(c);
		return 0;

	default:
		break;
	}

	/* map lower case character to upper case */
	if (islower(c))
		c = toupper(c);

	/* see if the character can be found in our character set */
	for (sym = -1, i = 0; i < NumChars; i++)
		if (c == ThrobCharSet[i])
			sym = i;

	/* send a space for unknown chars */
	if (sym == -1) {
		c = ' ';
		sym = 44;
	}

	send_throb(trx, sym);
	trx_put_echo_char(c);

	return 0;
}
