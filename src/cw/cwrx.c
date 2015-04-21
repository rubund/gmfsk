/*
 *    cwrx.c  --  morse code demodulator
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "trx.h"
#include "cw.h"
#include "morse.h"
#include "filter.h"
#include "fftfilt.h"
#include "misc.h"

static int cw_process(struct trx *trx, int cw_event, unsigned char **c);

/*
=======================================================================
update_syncscope()
Routine called to update the display on the sync scope display.
For CW this is an o scope pattern that shows the cw data stream.
=======================================================================
*/
static void update_syncscope(struct cw *s)
{
	float data[MaxSymLen];
	int i, j;

	for (i = 0; i < MaxSymLen; i++) {
		j = (i + s->pipeptr) % MaxSymLen;
		data[i] = s->pipe[j] / s->agc_peak;
	}

	/* set o scope... data[], length, autoscale */
	trx_set_scope(data, MaxSymLen, FALSE);

	/* show rx wpm on quality dial */
	trx_set_metric(s->cw_receive_speed);
}

/*
=====================================================================
 cw_rxprocess()
 Called with a block (512 samples) of audio.
=======================================================================
*/
int cw_rxprocess(struct trx *trx, float *buf, int len)
{
	struct cw *s = (struct cw *) trx->modem;
	complex z, *zp;
	int n, i;
	double delta;
	double value;
	unsigned char *c;

	/* check if user changed filter bandwidth */
	if (trx->bandwidth != trx->cw_bandwidth) {
		fftfilt_set_freqs(s->fftfilt, 0, trx->cw_bandwidth / 2.0 / SampleRate);
		trx->bandwidth = trx->cw_bandwidth;
	}

	/* compute phase increment expected at our specific rx tone freq */
	delta = 2.0 * M_PI * trx->frequency / SampleRate;

	while (len-- > 0) {
		/* Mix with the internal NCO */
		c_re(z) = *buf * cos(s->phaseacc);
		c_im(z) = *buf * sin(s->phaseacc);
		buf++;

		s->phaseacc += delta;

		if (s->phaseacc > M_PI)
			s->phaseacc -= 2.0 * M_PI;

		n = fftfilt_run(s->fftfilt, z, &zp);

		for (i = 0; i < n; i++) {
			/* 
			 * update the basic sample counter used for 
			 * morse timing 
			 */
			s->s_ctr++;

			/* downsample by 8 */
			if (++s->dec_ctr < DEC_RATIO)
				continue;
			else
				s->dec_ctr = 0;

			/* demodulate */
			value = cmod(zp[i]);

			/* 
			 * Compute a variable threshold value for tone 
			 * detection. Fast attack and slow decay.
			 */
			if (value > s->agc_peak)
				s->agc_peak = value;
			else
				s->agc_peak = decayavg(s->agc_peak, value, SampleRate / 10);

			/*
			 * save correlation amplitude value for the 
			 * sync scope
			 */
			s->pipe[s->pipeptr] = value;
			s->pipeptr = (s->pipeptr + 1) % MaxSymLen;
			if (s->pipeptr == MaxSymLen - 1)
				update_syncscope(s);

			if (!trx->squelchon || value > trx->cw_squelch / 5000) {
				/* upward trend means tone starting */
				if ((value > 0.66 * s->agc_peak) && 
				    (s->cw_receive_state != RS_IN_TONE))
					cw_process(trx, CW_KEYDOWN_EVENT, NULL);

				/* downward trend means tone stopping */
				if ((value < 0.33 * s->agc_peak) && 
				    (s->cw_receive_state == RS_IN_TONE))
					cw_process(trx, CW_KEYUP_EVENT, NULL);
			}

			/*
			 * We could put a gear on this... we dont really
			 * need to check for new characters being ready
			 * 1000 times/sec. There does not appear to be 
			 * much overhead the way it is.
			 */
			if (cw_process(trx, CW_QUERY_EVENT, &c) == CW_SUCCESS)
				while (*c)
					trx_put_rx_char(*c++);
		}

	}
	return 0;
}

/* ---------------------------------------------------------------------- */

/* routines swiped from unix-cw project... adapted for gmfsk */

/**
 * cw_compare_timestamps()
 *
 * Compare two timestamps, and return the difference between them in usecs.
 */
int cw_compare_timestamps(unsigned int earlier, unsigned int later)
{
	/*
	 * Compare the timestamps.
	 *
	 * At 4 WPM, the dash length is 3*(1200000/4)=900,000 usecs, and
	 * the word gap is 2,100,000 usecs.  With the maximum Farnsworth
	 * additional delay, the word gap extends to 8,100,000 usecs.
	 * This fits into an int with a lot of room to spare, in fact,
	 * an int can represent ~2000,000,000 usecs, or around 33 minutes.
	 */

	/* Convert sound samples to micro seconds */
	if (earlier >= later) {
		return 0;
	} else
		return (int) (((double) (later - earlier) * USECS_PER_SEC) / SampleRate);
}

/*
=======================================================================
cw_update_tracking()
This gets called everytime we have a dot dash sequence or a dash dot
sequence. Since we have semi validated tone durations, we can try and
track the cw speed by adjusting the cw_adaptive_receive_threshold variable.
This is done by running averages where the newest data replaces
the oldest data.
=======================================================================
*/
void cw_update_tracking(struct trx *trx, int dot, int dash)
{
	struct cw *s = (struct cw *) trx->modem;
	int average_dot;	/* Averaged dot length */
	int average_dash;	/* Averaged dash length */

	// Update the dot data held for averaging.
	s->cw_dt_dot_tracking_sum -= s->cw_dot_tracking_array[s->cw_dt_dot_index];
	s->cw_dot_tracking_array[s->cw_dt_dot_index++] = dot;
	s->cw_dt_dot_index %= AVERAGE_ARRAY_LENGTH;
	s->cw_dt_dot_tracking_sum += dot;

	// Update the dash data held for averaging.
	s->cw_dt_dash_tracking_sum -= s->cw_dash_tracking_array[s->cw_dt_dash_index];
	s->cw_dash_tracking_array[s->cw_dt_dash_index++] = dash;
	s->cw_dt_dash_index %= AVERAGE_ARRAY_LENGTH;
	s->cw_dt_dash_tracking_sum += dash;

	// update speed values based on new dot/dash avgs
	// Recalculate the adaptive threshold from the values currently
	// held in the averaging arrays.  The threshold is calculated as
	// (avg dash length - avg dot length) / 2 + avg dot_length.
	average_dot = s->cw_dt_dot_tracking_sum / AVERAGE_ARRAY_LENGTH;
	average_dash = s->cw_dt_dash_tracking_sum / AVERAGE_ARRAY_LENGTH;
	s->cw_adaptive_receive_threshold = (average_dash - average_dot) / 2 + average_dot;

	// force a recalc and limits checks on all internal timing variables
	s->cw_in_sync = FALSE;
	cw_sync_parameters(s);
}

/*
=======================================================================
cw_process()
    high level cw decoder... gets called with keyup, keydown, reset and
    query commands. 
    Keyup/down influences decoding logic.
    Reset starts everything out fresh.
    The query command returns CW_SUCCESS and the character that has 
    been decoded (may be '*',' ' or [a-z,0-9] or a few others)
    If there is no data ready, CW_ERROR is returned.
=======================================================================
*/
static int cw_process(struct trx *trx, int cw_event, unsigned char **c)
{
	struct cw *s = (struct cw *) trx->modem;
	static int space_sent = TRUE;	// for word space logic
	static int last_element = 0;	// length of last dot/dash
	int element_usec;		// Time difference in usecs

	switch (cw_event) {
	case CW_RESET_EVENT:
		cw_sync_parameters(s);
		s->cw_receive_state = RS_IDLE;
		s->cw_rr_current = 0;	// reset decoding pointer
		s->s_ctr = 0;	// reset audio sample counter
		memset(s->cw_receive_representation_buffer, 0,
		       sizeof(s->cw_receive_representation_buffer));

		break;

	case CW_KEYDOWN_EVENT:
		// A receive tone start can only happen while we
		// are idle, or in the middle of a character.
		if (s->cw_receive_state == RS_IN_TONE)
			return CW_ERROR;

		// first tone in idle state reset audio sample counter
		if (s->cw_receive_state == RS_IDLE) {
			s->s_ctr = 0;
			memset(s->cw_receive_representation_buffer, 0,
			       sizeof(s->cw_receive_representation_buffer));
			s->cw_rr_current = 0;
		}
		// save the timestamp
		s->cw_rr_start_timestamp = s->s_ctr;

		// Set state to indicate we are inside a tone.
		s->cw_receive_state = RS_IN_TONE;
		return CW_ERROR;
		break;

	case CW_KEYUP_EVENT:
		// The receive state is expected to be inside a tone.
		if (s->cw_receive_state != RS_IN_TONE)
			return CW_ERROR;

		// Save the current timestamp 
		s->cw_rr_end_timestamp = s->s_ctr;
		element_usec = cw_compare_timestamps(s->cw_rr_start_timestamp,
						     s->cw_rr_end_timestamp);

		// make sure our timing values are up to date
		cw_sync_parameters(s);

		// If the tone length is shorter than any noise cancelling 
		// threshold that has been set, then ignore this tone.
		if (s->cw_noise_spike_threshold > 0
		    && element_usec <= s->cw_noise_spike_threshold)
			return CW_ERROR;


		// Set up to track speed on dot-dash or dash-dot pairs
		// for this test to work, we need a dot dash pair or a 
		// dash dot pair to validate timing from and force the 
		// speed tracking in the right direction. This method 
		// is fundamentally different than the method in the 
		// unix cw project. Great ideas come from staring at the
		// screen long enough!. Its kind of simple really... 
		// when you have no idea how fast or slow the cw is... 
		// the only way to get a threshold is by having both 
		// code elements and setting the threshold between them
		// knowing that one is supposed to be 3 times longer 
		// than the other. with straight key code... this gets 
		// quite variable, but with most faster cw sent with 
		// electronic keyers, this is one relationship that is 
		// quite reliable.
		if (last_element > 0) {
			// check for dot dash sequence (current should be 3 x last)
			if ((element_usec > 2 * last_element) &&
			    (element_usec < 4 * last_element)) {
				cw_update_tracking(trx, last_element, element_usec);
			}
			// check for dash dot sequence (last should be 3 x current)
			if ((last_element > 2 * element_usec) &&
			    (last_element < 4 * element_usec)) {
				cw_update_tracking(trx, element_usec, last_element);
			}
		}
		last_element = element_usec;

		// ok... do we have a dit or a dah?
		// a dot is anything shorter than 2 dot times
		if (element_usec <= s->cw_adaptive_receive_threshold) {
			s->cw_receive_representation_buffer[s->cw_rr_current++] = CW_DOT_REPRESENTATION;
		} else {
			// a dash is anything longer than 2 dot times
			s->cw_receive_representation_buffer[s->cw_rr_current++] = CW_DASH_REPRESENTATION;
		}

		// We just added a representation to the receive buffer.  
		// If it's full, then reset everything as it probably noise
		if (s->cw_rr_current == RECEIVE_CAPACITY - 1) {
			s->cw_receive_state = RS_IDLE;
			s->cw_rr_current = 0;	// reset decoding pointer
			s->s_ctr = 0;		// reset audio sample counter
			return CW_ERROR;
		} else
			// zero terminate representation
			s->cw_receive_representation_buffer[s->cw_rr_current] = 0;

		// All is well.  Move to the more normal after-tone state.
		s->cw_receive_state = RS_AFTER_TONE;
		return CW_ERROR;
		break;

	case CW_QUERY_EVENT:
		// this should be called quite often 
		// (faster than inter-character gap) It looks after timing
		// key up intervals and determining when a character, 
		// a word space, or an error char '*' should be returned.
		// CW_SUCCESS is returned when there is a printable character.
		// nothing to do if we are in a tone
		if (s->cw_receive_state == RS_IN_TONE)
			return CW_ERROR;

		// in this call we expect a pointer to a char to be valid
		if (c == NULL) {
			// else we had no place to put character...
			g_warning("%s:%d cant return character\n",
				  __FILE__, __LINE__);
			s->cw_receive_state = RS_IDLE;
			s->cw_rr_current = 0;	// reset decoding pointer
			return CW_ERROR;
		}
		// compute length of silence so far
		cw_sync_parameters(s);
		element_usec = cw_compare_timestamps(s->cw_rr_end_timestamp,
						     s->s_ctr);

		// SHORT time since keyup... nothing to do yet
		if (element_usec < (2 * s->cw_receive_dot_length))
			return CW_ERROR;

		// MEDIUM time since keyup... check for character space
		// one shot through this code via receive state logic
		if (element_usec >= (2 * s->cw_receive_dot_length) &&
		    element_usec <= (4 * s->cw_receive_dot_length) &&
		    s->cw_receive_state == RS_AFTER_TONE) {
			/* Look up the representation */
			*c = cw_rx_lookup(s->cw_receive_representation_buffer);

			if (*c == NULL)
				// invalid decode... let user see error
				*c = "*";

			s->cw_receive_state = RS_IDLE;
			s->cw_rr_current = 0;	// reset decoding pointer
			space_sent = FALSE;
			return CW_SUCCESS;
		}

		// LONG time since keyup... check for a word space
		if ((element_usec > (4 * s->cw_receive_dot_length)) && !space_sent) {
			*c = " ";
			space_sent = TRUE;
			return CW_SUCCESS;
		}
		// should never get here... catch all
		return CW_ERROR;
		break;
	}

	// should never get here... catch all
	return CW_ERROR;
}
