/*
 *    mfsktx.c  --  MFSK modulator
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

#include <stdlib.h>
#include <string.h>

#include "mfsk.h"
#include "trx.h"
#include "fft.h"
#include "viterbi.h"
#include "varicode.h"
#include "interleave.h"
#include "misc.h"
#include "snd.h"
#include "picture.h"
#include "filter.h"
#include "main.h"

static inline complex mixer(struct trx *trx, complex in)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	complex z;
	float f;

	f = trx->frequency - trx->bandwidth / 2 + trx->txoffset;

	/* Basetone is always at 1000 Hz */
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

static void sendsymbol(struct trx *trx, int sym)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	complex z;
	int i;

	sym = grayencode(sym & (m->numtones - 1));

	if (trx->reverse)
		sym = (m->numtones - 1) - sym;

	fft_clear_inbuf(m->fft);
	c_re(m->fft->in[sym + m->basetone]) = 1.0;

	fft_run(m->fft);

	for (i = 0; i < m->symlen; i++) {
		z = mixer(trx, m->fft->out[i]);
		trx->outbuf[i] = c_re(z);
	}

	sound_write(trx->outbuf, m->symlen);
}

static void sendbit(struct trx *trx, int bit)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	int i, data;

	data = encoder_encode(m->enc, bit);

	for (i = 0; i < 2; i++) {
		m->bitshreg = (m->bitshreg << 1) | ((data >> i) & 1);
		m->bitstate++;

		if (m->bitstate == m->symbits) {
			interleave_bits(m->txinlv, &m->bitshreg);

			sendsymbol(trx, m->bitshreg);

			m->bitstate = 0;
			m->bitshreg = 0;
		}
	}
}

static void sendchar(struct trx *trx, unsigned char c)
{
	char *code = varienc(c);

	while (*code)
		sendbit(trx, (*code++ - '0'));

	trx_put_echo_char(c);
}

static void sendidle(struct trx *trx)
{
	int i;

	sendchar(trx, 0);	/* <NUL> */

	sendbit(trx, 1);

	/* extended zero bit stream */
	for (i = 0; i < 32; i++)
		sendbit(trx, 0);
}

static void flushtx(struct trx *trx)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	int i;

	/* flush the varicode decoder at the other end */
	sendbit(trx, 1);

	/* flush the convolutional encoder and interleaver */
	for (i = 0; i < 107; i++)
		sendbit(trx, 0);

	m->bitstate = 0;
}

static int startpic(struct trx *trx)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	char *header, *p;

	if ((m->picbuf = trx_get_tx_picture()) == NULL)
		return 0;

	header = picbuf_make_header(m->picbuf);

	for (p = header; *p != 0; p++)
		sendchar(trx, *p);

	g_free(header);

	flushtx(trx);

	return 1;
}

static void sendpic(struct trx *trx, guchar *data, gint len)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	float *ptr;
	float freq, f;
	int i, j;

	if (SAMPLES_PER_PIXEL * len > OUTBUFSIZE) {
		g_warning("sendpic: buffer overrun\n");
		return;
	}

	ptr = trx->outbuf;

	freq = trx->frequency + trx->txoffset;

	for (i = 0; i < len; i++) {
		f = freq + trx->bandwidth * (data[i] - 128) / 256.0;

		for (j = 0; j < SAMPLES_PER_PIXEL; j++) {
			*ptr++ = cos(m->phaseacc);

			m->phaseacc += 2.0 * M_PI * f / SampleRate;

			if (m->phaseacc > M_PI)
				m->phaseacc -= 2.0 * M_PI;
		}
	}

	sound_write(trx->outbuf, SAMPLES_PER_PIXEL * len);
}

int mfsk_txprocess(struct trx *trx)
{
	struct mfsk *m = (struct mfsk *) trx->modem;
	unsigned char buf[64], *str;
	int i;

	if (trx->tune) {
		m->txstate = TX_STATE_TUNE;
		trx->tune = 0;
	}

	switch (m->txstate) {
	case TX_STATE_TUNE:
		sendsymbol(trx, 0);
		m->txstate = TX_STATE_FINISH;
		break;

	case TX_STATE_PREAMBLE:
		for (i = 0; i < 32; i++)
			sendbit(trx, 0);

		m->txstate = TX_STATE_START;
		break;

	case TX_STATE_START:
		sendchar(trx, '\r');
		sendchar(trx, 2);		/* STX */
		sendchar(trx, '\r');
		m->txstate = TX_STATE_DATA;
		break;

	case TX_STATE_DATA:
		i = trx_get_tx_char();

		if (i == 0xFFFC) {
			statusbar_set_main("Send picture: start");
			m->counter = 0;

			if (startpic(trx))
				m->txstate = TX_STATE_PICTURE_START;

			break;
		}

		if (i == -1)
			sendidle(trx);
		else
			sendchar(trx, i);

		if (trx->stopflag)
			m->txstate = TX_STATE_END;

		break;

	case TX_STATE_END:
		i = trx_get_tx_char();

		if (i == 0xFFFC) {
			if (startpic(trx))
				m->txstate = TX_STATE_PICTURE;
			break;
		}

		if (i == -1) {
			sendchar(trx, '\r');
			sendchar(trx, 4);		/* EOT */
			sendchar(trx, '\r');
			m->txstate = TX_STATE_FLUSH;
		} else
			sendchar(trx, i);

		break;

	case TX_STATE_FLUSH:
		flushtx(trx);
		return -1;

	case TX_STATE_FINISH:
		return -1;

	case TX_STATE_PICTURE_START:
		memset(buf, 0, sizeof(buf));
		sendpic(trx, buf, 44);

		m->txstate = TX_STATE_PICTURE;
		break;

	case TX_STATE_PICTURE:
		i = sizeof(buf);

		if (picbuf_get_data(m->picbuf, buf, &i) == FALSE) {
			picbuf_free(m->picbuf);
			m->picbuf = NULL;
			m->txstate = TX_STATE_DATA;

			sendpic(trx, buf, i);

			statusbar_set_main("Send picture: done");
			m->counter = 0;

			break;
		}

		sendpic(trx, buf, i);

		if (++m->counter == 8) {
			gdouble x = picbuf_get_percentage(m->picbuf);

			str = g_strdup_printf("Send picture: %.0f%%", x);
			statusbar_set_main(str);
			g_free(str);

			m->counter = 0;
		}

		break;
	}

	return 0;
}
