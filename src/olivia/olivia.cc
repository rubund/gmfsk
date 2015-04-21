/*
 *    olivia.cc  --  OLIVIA modem
 *
 *    Copyright (C) 2005
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

#include "main.h"
#include "trx.h"
#include "mfsk.h"
#include "snd.h"

extern "C" void olivia_init(struct trx *trx);

struct olivia {
	MFSK_Transmitter < float >*Tx;
	 MFSK_Receiver < float >*Rx;

	gint16 *txbuffer;
	gfloat *txfbuffer;
	gint txbufferlen;

	gint16 *rxbuffer;
	gint rxbufferlen;

	gint escape;
};

static void olivia_txinit(struct trx *trx)
{
	struct olivia *s = (struct olivia *) trx->modem;
	guint8 c;

//      fprintf(stderr, "olivia_txinit()\n");

	s->Rx->Flush();

	while (s->Rx->GetChar(c) > 0)
		trx_put_rx_char(c);

	s->Tx->Start();

	s->escape = 0;
}

static void olivia_rxinit(struct trx *trx)
{
	struct olivia *s = (struct olivia *) trx->modem;

//      fprintf(stderr, "olivia_rxinit()\n");

	s->Rx->Reset();

	s->escape = 0;
}

static void olivia_free(struct olivia *s)
{
//      fprintf(stderr, "olivia_free(%p)\n", s);

	if (s) {
		delete s->Tx;
		delete s->Rx;

		g_free(s->txbuffer);
		g_free(s->txfbuffer);
		g_free(s->rxbuffer);

		g_free(s);
	}
}

static void olivia_destructor(struct trx *trx)
{
	struct olivia *s = (struct olivia *) trx->modem;

//      fprintf(stderr, "olivia_destructor()\n");

	statusbar_set_main("");

	olivia_free(s);

	trx->modem = NULL;
	trx->txinit = NULL;
	trx->rxinit = NULL;
	trx->txprocess = NULL;
	trx->rxprocess = NULL;
	trx->destructor = NULL;
}

static gint olivia_unescape(struct trx *trx, gint c)
{
	struct olivia *s = (struct olivia *) trx->modem;

	if (trx->olivia_esc == 0)
		return c;

	if (s->escape) {
		s->escape = 0;
		return c + 128;
	}

	if (c == 127) {
		s->escape = 1;
		return -1;
	}

	return c;
}

static int olivia_txprocess(struct trx *trx)
{
	struct olivia *s = (struct olivia *) trx->modem;
	gint c, i, len;
	guint8 ch;

	/*
	 * The encoder works with BitsPerSymbol length blocks. If the
	 * modem already has that many characters buffered, don't try
	 * to read any more. If stopflag is set, we will always read 
	 * whatever there is.
	 */
	if (trx->stopflag || (s->Tx->GetReadReady() < s->Tx->BitsPerSymbol)) {
		if ((c = trx_get_tx_char()) == -1) {
			if (trx->stopflag)
				s->Tx->Stop();
		} else {
			/* Replace un-representable characters with a dot */
			if (c > (trx->olivia_esc ? 255 : 127))
				c = '.';

			if (c > 127) {
				c &= 127;
				s->Tx->PutChar(127);
			}

			s->Tx->PutChar(c);
		}
        }

	if (s->Tx->GetChar(ch) > 0)
		if ((c = olivia_unescape(trx, ch)) != -1)
			trx_put_echo_char(c);

	if ((len = s->Tx->Output(s->txbuffer)) > 0) {
		for (i = 0; i < len; i++)
			s->txfbuffer[i] = (gfloat) (s->txbuffer[i] / 32767.0);

		sound_write(s->txfbuffer, len);
	}

	if (!s->Tx->Running())
		return -1;

	return 0;
}

static int olivia_rxprocess(struct trx *trx, float *buf, int len)
{
	struct olivia *s = (struct olivia *) trx->modem;
	gint i, c;
	guint8 ch = 0;
	gfloat snr;
	gchar *msg;

//      fprintf(stderr, "olivia_rxprocess(%d)\n", len);

	if (len > s->rxbufferlen) {
		s->rxbuffer = (gint16 *) g_realloc(s->rxbuffer, len * sizeof(gint16));
		s->rxbufferlen = len;
	}

	for (i = 0; i < len; i++)
		s->rxbuffer[i] = (gint16) (buf[i] * 32767.0);

	s->Rx->SyncThreshold = trx->squelchon ? trx->olivia_squelch : 0.0;

	s->Rx->Process(s->rxbuffer, len);

	if ((snr = s->Rx->SignalToNoiseRatio()) > 99.9)
		snr = 99.9;

	trx_set_metric(snr);

	msg = g_strdup_printf("SNR: %4.1f | Freq: %+4.1f/%4.1f Hz | Time: %5.3f/%5.3f sec",
			      s->Rx->SignalToNoiseRatio(),
			      s->Rx->FrequencyOffset(),
			      s->Rx->TuneMargin(),
			      s->Rx->TimeOffset(),
			      s->Rx->BlockPeriod());

	statusbar_set_main(msg);

	g_free(msg);

	while (s->Rx->GetChar(ch) > 0)
		if ((c = olivia_unescape(trx, ch)) != -1 && c > 7)
			trx_put_rx_char(c);

	return 0;
}

void olivia_init(struct trx *trx)
{
	struct olivia *s;

//      fprintf(stderr, "olivia_init()\n");

	s = g_new0(struct olivia, 1);

	s->Tx = new MFSK_Transmitter < float >;
	s->Rx = new MFSK_Receiver < float >;

	switch (trx->olivia_tones) {
	case 0:
		s->Tx->Tones = 4;
		break;
	case 1:
		s->Tx->Tones = 8;
		break;
	case 2:
		s->Tx->Tones = 16;
		break;
	case 3:
		s->Tx->Tones = 32;
		break;
	case 4:
		s->Tx->Tones = 64;
		break;
	case 5:
		s->Tx->Tones = 128;
		break;
	case 6:
		s->Tx->Tones = 256;
		break;
	}

	switch (trx->olivia_bw) {
	case 0:
		s->Tx->Bandwidth = 125;
		break;
	case 1:
		s->Tx->Bandwidth = 250;
		break;
	case 2:
		s->Tx->Bandwidth = 500;
		break;
	case 3:
		s->Tx->Bandwidth = 1000;
		break;
	case 4:
		s->Tx->Bandwidth = 2000;
		break;
	}

	s->Tx->SampleRate = 8000;
	s->Tx->OutputSampleRate = 8000;

	if (s->Tx->Preset() < 0) {
		g_warning("olivia_init: transmitter preset failed!");
		olivia_free(s);
		return;
	}

	s->txbufferlen = s->Tx->MaxOutputLen;
	s->txbuffer = g_new(gint16, s->txbufferlen);
	s->txfbuffer = g_new(gfloat, s->txbufferlen);

	s->Rx->Tones = s->Tx->Tones;
	s->Rx->Bandwidth = s->Tx->Bandwidth;

	s->Rx->SyncMargin = trx->olivia_smargin;
	s->Rx->SyncIntegLen = trx->olivia_sinteg;
	s->Rx->SyncThreshold = trx->squelchon ? trx->olivia_squelch : 0.0;

	s->Rx->SampleRate = 8000;
	s->Rx->InputSampleRate = 8000;

	if (s->Rx->Preset() < 0) {
		g_warning("olivia_init: receiver preset failed!");
		olivia_free(s);
		return;
	}

	s->rxbufferlen = 0;
	s->rxbuffer = NULL;

	trx->modem = s;

	trx->txinit = olivia_txinit;
	trx->rxinit = olivia_rxinit;

	trx->txprocess = olivia_txprocess;
	trx->rxprocess = olivia_rxprocess;

	trx->destructor = olivia_destructor;

	trx->samplerate = 8000;
	trx->fragmentsize = 1024;

	trx->bandwidth = s->Tx->Bandwidth;
	trx->frequency = 500.0 + trx->bandwidth / 2.0;
}
