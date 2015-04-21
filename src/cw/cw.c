/*
 *    cw.c  --  morse code modem
 *
 *    Copyright (C) 2004
 *      Lawrence Glaister (ve7it@shaw.ca)
 *    This modem borrowed heavily from other gmfsk modems and
 *    also from the unix-cw project. I would like to thank those
 *    authors for enriching my coding experience by providing
 *    and supporting open source.
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
#include <string.h>

#include "trx.h"
#include "cw.h"
#include "morse.h"
#include "fftfilt.h"

static void cw_txinit(struct trx *trx)
{
	struct cw *c = (struct cw *) trx->modem;
	c->phaseacc = 0;
}

static void cw_free(struct cw *s)
{
	if (s) {
		fftfilt_free(s->fftfilt);
		g_free(s);
	}
}

static void cw_rxinit(struct trx *trx)
{
	struct cw *c = (struct cw *) trx->modem;
	c->cw_receive_state = RS_IDLE;	// set us waiting for a tone
	c->s_ctr = 0;			// reset audio sample counter
	c->cw_rr_current = 0;		// reset decoding pointer
	c->agc_peak = 0;		// reset agc
	c->dec_ctr = 0;			// reset decimation counter
}

static void cw_destructor(struct trx *trx)
{
	struct cw *s = (struct cw *) trx->modem;

	cw_free(s);

	trx->modem = NULL;
	trx->txinit = NULL;
	trx->rxinit = NULL;
	trx->txprocess = NULL;
	trx->rxprocess = NULL;
	trx->destructor = NULL;
}

void cw_init(struct trx *trx)
{
	struct cw *s;
	double lp;
	int i;

	s = g_new0(struct cw, 1);

	s->cw_send_speed = trx->cw_speed;
	s->cw_receive_speed = trx->cw_speed;
	s->cw_noise_spike_threshold = INITIAL_NOISE_THRESHOLD;
	s->cw_adaptive_receive_threshold = 2 * DOT_MAGIC / trx->cw_speed;
//	s->cw_gap = INITIAL_GAP;	/* Initially no 'Farnsworth' gap */  

	memset(s->cw_receive_representation_buffer, 0,
	       sizeof(s->cw_receive_representation_buffer));

	// block of variables that get up dated each time speed changes
	s->cw_in_sync = FALSE;	/* Synchronization flag */
	cw_sync_parameters(s);

	// init the cw rx tracking arrays, sums and indexes to intial rx speed
	s->cw_dt_dot_tracking_sum = AVERAGE_ARRAY_LENGTH * s->cw_send_dot_length;
	s->cw_dt_dash_tracking_sum = AVERAGE_ARRAY_LENGTH * s->cw_send_dash_length;
	for (i = 0; i < AVERAGE_ARRAY_LENGTH; i++) {
		s->cw_dot_tracking_array[i] = s->cw_send_dot_length;
		s->cw_dash_tracking_array[i] = s->cw_send_dash_length;
	}
	s->cw_dt_dot_index = 0;
	s->cw_dt_dash_index = 0;

	lp = trx->cw_bandwidth / 2.0 / SampleRate;

	if ((s->fftfilt = fftfilt_init(0, lp, 1024)) == NULL) {
		cw_free(s);
		return;
	}

	if (cw_tables_init() == FALSE) {
		g_warning("cw_tables_init failed\n");
		cw_free(s);
		return;
	}

	// setup function pointers for cw processes
	trx->modem = s;
	trx->txinit = cw_txinit;
	trx->rxinit = cw_rxinit;
	trx->txprocess = cw_txprocess;
	trx->rxprocess = cw_rxprocess;
	trx->destructor = cw_destructor;

	trx->samplerate = SampleRate;
	trx->fragmentsize = MaxSymLen;
	trx->bandwidth = trx->cw_bandwidth;
}

/**
 * cw_sync_parameters()
 *
 * Synchronize the dot, dash, end of element, end of character, and end
 * of word timings and ranges to new values of Morse speed, 'Farnsworth'
 * gap, or receive tolerance.
 */
void cw_sync_parameters(struct cw *c)
{
	// Do nothing if we are already synchronized with speed/gap.
	if (c->cw_in_sync) 
		return;

	// Send parameters:
	// Calculate the length of a Dot as 1200000 / speed in wpm,
	// and the length of a Dash as three Dot lengths.
	c->cw_send_dot_length = DOT_MAGIC / c->cw_send_speed;
	c->cw_send_dash_length = 3 * c->cw_send_dot_length;

	// calculate basic dot timing as number of samples of audio needed
	// this needs to fit in the trx outbuf[OUTBUFSIZE] so tx logic works
	c->symbollen = SampleRate * c->cw_send_dot_length / USECS_PER_SEC;
	if (c->symbollen > OUTBUFSIZE) {
		g_error("CW - fatal: dot length too long: %d\n", c->symbollen);
		/* abort() */
	}

	// An end of element length is one Dot, end of character is
	// three Dots total, and end of word is seven Dots total.

//	c->cw_additional_delay  = c->cw_gap * c->cw_send_dot_length;

	/*
	 * For 'Farnsworth', there also needs to be an adjustment
	 * delay added to the end of words, otherwise the rhythm
	 * is lost on word end.  I don't know if there is an
	 * "official" value for this, but 2.33 or so times the
	 * gap is the correctly scaled value, and seems to sound
	 * okay.
	 *
	 * Thanks to Michael D. Ivey <ivey@gweezlebur.com> for
	 * identifying this in earlier versions of cwlib.
	 */
//	c->cw_adjustment_delay  = (7 * c->cw_additional_delay) / 3;

	// Receive parameters:
	c->cw_receive_speed = DOT_MAGIC / (c->cw_adaptive_receive_threshold / 2);

	// receive routines track speeds, but we put hard limits
	// on the speeds here if necessary.
	// (dot/dash threshold is 2 dots timing)
	if (c->cw_receive_speed < CW_MIN_SPEED) {
		c->cw_receive_speed = CW_MIN_SPEED;
		c->cw_adaptive_receive_threshold = 2 * DOT_MAGIC / CW_MIN_SPEED;
	}

	if (c->cw_receive_speed > CW_MAX_SPEED) {
		c->cw_receive_speed = CW_MAX_SPEED;
		c->cw_adaptive_receive_threshold = 2 * DOT_MAGIC / CW_MAX_SPEED;
	}

	// Calculate the basic receive dot and dash lengths.
	c->cw_receive_dot_length = DOT_MAGIC / c->cw_receive_speed;
	c->cw_receive_dash_length = 3 * c->cw_receive_dot_length;

	// Set the parameters in sync flag.
	c->cw_in_sync = TRUE;
}
