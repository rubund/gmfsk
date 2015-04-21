/*
 *    cwtx.c  --  morse code modulator
 *
 *    Copyright (C) 2004
 *      Lawrence Glaister (ve7it@shaw.ca)
 *    This modem borrowed heavily from other gmfsk modems and
 *    also from the unix-cw project. I would like to thank those
 *    authors for enriching my coding experience by providing
 *    and supporting open source.

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
#include "cw.h"
#include "morse.h"
#include "snd.h"

/* Define the amplitude envelop for key down events (32 samples long)      */
/* this is 1/2 cycle of a raised cosine                                    */
/* the tables with 32 entries give about 4ms rise and fall times           */
/* when using 8000 samples/sec. This shaping of the cw pulses is           */
/* very necssary to avoid having a very wide and clicky cw signal          */
/* when using the sound card to gen cw. When using the rig key input       */
/* the shaping is done in the rig hardware, but we want to be able to      */
/* pick one cw signal out of a cluster and be able to respond on his freq. */
#define KNUM 32
double kdshape[KNUM] = {
	0.00240750255310301, 0.00960708477768751,
	0.02152941088003600, 0.03805966253618680,
	0.05903864465505320, 0.08426431851158830, 
	0.11349374748686800, 0.14644543667658500,
	0.18280204383628200, 0.22221343555548300, 
	0.26430005922814900, 0.30865659834558700,
	0.35485587590940700, 0.40245296837259500, 
	0.45098949048925500, 0.49999800980765500,
	0.54900654829266300, 0.59754312772456200, 
	0.64514031509964400, 0.69133972425796200,
	0.73569643038517400, 0.77778325487450100, 
	0.81719487928327800, 0.85355174876454100,
	0.88650372738152000, 0.91573347010241700, 
	0.94095947900139100, 0.96193881423287900,
	0.97846943367117300, 0.99039213868324900, 
	0.99759210729604500, 0.99999999999295900
};

double kushape[KNUM] = {
	0.99999999999295900, 0.99759210729604500, 
	0.99039213868324900, 0.97846943367117300,
	0.96193881423287900, 0.94095947900139100, 
	0.91573347010241700, 0.88650372738152000,
	0.85355174876454100, 0.81719487928327800, 
	0.77778325487450100, 0.73569643038517400,
	0.69133972425796200, 0.64514031509964400, 
	0.59754312772456200, 0.54900654829266300,
	0.49999800980765500, 0.45098949048925500, 
	0.40245296837259500, 0.35485587590940700,
	0.30865659834558700, 0.26430005922814900, 
	0.22221343555548300, 0.18280204383628200,
	0.14644543667658500, 0.11349374748686800, 
	0.08426431851158830, 0.05903864465505320,
	0.03805966253618680, 0.02152941088003600, 
	0.00960708477768751, 0.00240750255310301
};

static inline double nco(struct cw *s, double freq)
{
	s->phaseacc += 2.0 * M_PI * freq / SampleRate;

	if (s->phaseacc > M_PI)
		s->phaseacc -= 2.0 * M_PI;

	return cos(s->phaseacc);
}

/*
=====================================================================
 send_symbol()
 Sends a part of a morse character (one dot duration) of either
 sound at the correct freq or silence. Rise and fall time is controlled
 with a raised cosine shape.
=======================================================================
*/
static void send_symbol(struct trx *trx, int symbol)
{
	struct cw *s = (struct cw *) trx->modem;
	double freq;
	int i;
	static int lastkey = 0;

	freq = trx->frequency;
	freq += trx->txoffset;

	if ((lastkey == 0) && (symbol == 1)) {
		/* key going down */
		for (i = 0; i < KNUM; i++)
			trx->outbuf[i] = nco(s, freq) * kdshape[i];
		for (; i < s->symbollen; i++)
			trx->outbuf[i] = nco(s, freq);
	}

	if ((lastkey == 1) && (symbol == 0)) {
		/* key going up */
		for (i = 0; i < KNUM; i++)
			trx->outbuf[i] = nco(s, freq) * kushape[i];
		for (; i < s->symbollen; i++)
			trx->outbuf[i] = 0.0;

	}

	if ((lastkey == 0) && (symbol == 0)) {
		/* key is up */
		for (i = 0; i < s->symbollen; i++)
			trx->outbuf[i] = 0.0;
	}

	if ((lastkey == 1) && (symbol == 1)) {
		/* key is down */
		for (i = 0; i < s->symbollen; i++)
			trx->outbuf[i] = nco(s, freq);
	}

	sound_write(trx->outbuf, s->symbollen);
	lastkey = symbol;
}

/*
=====================================================================
send_ch()
sends a morse character and the space afterwards
=======================================================================
*/
static void send_ch(struct trx *trx, int symbol)
{
	struct cw *s = (struct cw *) trx->modem;
	int code;

//	fprintf(stderr,"sending %c\n",(char)symbol);

	// compute number of samples of audio in one dot at current wpm
	if (s->cw_send_speed != trx->cw_speed) {
		s->cw_send_speed = trx->cw_speed;
		s->cw_in_sync = FALSE;
	}

	cw_sync_parameters(s);


	/* handle word space separately (7 dots spacing) */
	/* last char already had 2 dots of inter-character spacing sent with it */
	if ((symbol == ' ') || (symbol == '\n')) {
		send_symbol(trx, 0);
		send_symbol(trx, 0);
		send_symbol(trx, 0);
		send_symbol(trx, 0);
		send_symbol(trx, 0);
		trx_put_echo_char(symbol);
		return;
	}

	/* convert character code to a morse representation */
	if ((symbol < UCHAR_MAX + 1) && (symbol >= 0))
		code = cw_tx_lookup(symbol);
	else
		code = 0x4L;

//	fprintf(stderr,"code 0x%08X\n", code);

	/* loop sending out binary bits of cw character */
	while (code > 1) {
		send_symbol(trx, code & 1);
		code = code >> 1;
	}
	if (symbol != 0)
		trx_put_echo_char(symbol);
}

/*
=====================================================================
 cw_txprocess()
 Read charcters from screen and send them out the sound card.
 This is called repeatedly from a thread during tx.
=======================================================================
*/
int cw_txprocess(struct trx *trx)
{
//	struct cw *s = (struct cw *) trx->modem;
	int c;

	if (trx->tune) {
		trx->tune = 0;
		send_symbol(trx, 1);
		return 0;
	}

	c = trx_get_tx_char();

	/* TX buffer empty */
	if (c == -1) {
		/* stop if requested to... */
		if (trx->stopflag)
			return -1;

		send_symbol(trx, 0);
		return 0;
	}

	send_ch(trx, c);

	return 0;
}
