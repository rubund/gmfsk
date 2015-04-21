/*
 *    cw.h  --  morse code modem
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

#ifndef _CW_H
#define _CW_H

#include "cmplx.h"
#include "trx.h"

#define	SampleRate	8000
#define	MaxSymLen	512

#define	DEC_RATIO	8		/* decimation ratio for the receiver */

/* Limits on values of CW send and timing parameters */
#define	CW_MIN_SPEED		5	/* Lowest WPM allowed */
#define	CW_MAX_SPEED		60	/* Highest WPM allowed */
/*
 * Representation characters for Dot and Dash.  Only the following
 * characters are permitted in Morse representation strings.
 */
#define	CW_DOT_REPRESENTATION	'.'
#define	CW_DASH_REPRESENTATION	'-'

/* CW function return status codes. */
#define	CW_SUCCESS		0
#define	CW_ERROR		-1

#define	ASC_NUL		'\0'	/* End of string */
#define	ASC_SPACE	' '	/* ASCII space char */

/* Tone and timing magic numbers. */
#define	TONE_MAGIC 	1190000	/* Kernel tone delay magic number */
#define	DOT_MAGIC	1200000	/* Dot length magic number.  The Dot
				   length is 1200000/WPM Usec */
#define	TONE_SILENT	0	/* 0Hz = silent 'tone' */
#define	USECS_PER_SEC	1000000	/* Microseconds in a second */

#define	INITIAL_SEND_SPEED	18	/* Initial send speed in WPM */
#define	INITIAL_RECEIVE_SPEED	18	/* Initial receive speed in WPM */
//#define INITIAL_GAP		0	/* Initial fransworth gap setting */
//#define INITIAL_TOLERANCE	60	/* Initial tolerance setting */
//#define INITIAL_ADAPTIVE	TRUE	/* Initial adaptive receive setting */

/* Initial adaptive speed threshold */
#define	INITIAL_THRESHOLD	((DOT_MAGIC / INITIAL_RECEIVE_SPEED) * 2)

/* Initial noise filter threshold */
#define	INITIAL_NOISE_THRESHOLD	((DOT_MAGIC / CW_MAX_SPEED) / 2)

struct cw {
	/*
	 * Common stuff
	 */
	int symbollen;		/* length of a dot in sound samples (tx) */
	double phaseacc;	/* used by NCO for rx/tx tones */

	/*
	 * RX related stuff
	 */
	unsigned int s_ctr;	/* sample counter for timing cw rx */
	unsigned int dec_ctr;	/* decimation counter for rx */

	double agc_peak;	/* threshold for tone detection */

	struct fftfilt *fftfilt;

	enum {
		RS_IDLE = 0,
		RS_IN_TONE,
		RS_AFTER_TONE
	} cw_receive_state;	/* Indicates receive state */

	enum {			/* functions used by cw process routine */
		CW_RESET_EVENT,
		CW_KEYDOWN_EVENT,
		CW_KEYUP_EVENT,
		CW_QUERY_EVENT
	} cw_event;

	/* storage for sync scope data */
	double pipe[MaxSymLen];
	unsigned int pipeptr;

//	int preamble;		/* used for initial delay on tx, in dot times */


	/* user configurable data - local copy passed in from gui */
	int cw_send_speed;		/* Initially 18 WPM */
	int cw_receive_speed;		/* Initially 18 WPM */
	int cw_noise_spike_threshold;	/* Initially ignore any tone < 10mS */
//	int cw_gap;			/* Initially no 'Farnsworth' gap */  

	/*
	 * The following variables must be recalculated each time any of
	 * the above Morse parameters associated with speeds, gap, 
	 * tolerance, or threshold change.  Keeping these in step means 
	 * that we then don't have to spend time calculating them on the fly.
	 *
	 * Since they have to be kept in sync, the problem of how to 
	 * have them calculated on first call if none of the above 
	 * parameters has been changed is taken care of with a 
	 * synchronization flag.  Doing this saves us from otherwise 
	 * having to have a 'library initialize' function.
	 */
	int cw_in_sync;			/* Synchronization flag */
	/* Sending parameters: */
	int cw_send_dot_length;		/* Length of a send Dot, in Usec */
	int cw_send_dash_length;	/* Length of a send Dash, in Usec */
// to be used for farnsworth timing
//	int cw_additional_delay;	/* More delay at the end of a char */
//	int cw_adjustment_delay;	/* More delay at the end of a word */

	/* Receiving parameters: */
	int cw_receive_dot_length;	/* Length of a receive Dot, in Usec */
	int cw_receive_dash_length;	/* Length of a receive Dash, in Usec */

	/*
	 * Receive buffering.  This is a fixed-length representation, filled in
	 * as tone on/off timings are taken.
	 */
#define	RECEIVE_CAPACITY	256	/* Way longer than any representation */
	char cw_receive_representation_buffer[RECEIVE_CAPACITY];
	int cw_rr_current;		/* Receive buffer current location */
	unsigned int cw_rr_start_timestamp;	/* Tone start timestamp */
	unsigned int cw_rr_end_timestamp;	/* Tone end timestamp */

	/*
	 * variable which is automatically maintained from the Morse input
	 * stream, rather than being settable by the user.
	 */
	int cw_adaptive_receive_threshold;	/* 2-dot threshold for adaptive speed */

	/*
	 * Receive adaptive speed tracking.  We keep a small array of dot
	 * lengths, and a small array of dash lengths.  We also keep a
	 * running sum of the elements of each array, and an index to the
	 * current array position.
	 */
#define	AVERAGE_ARRAY_LENGTH	10	/* Keep 10 dot/dash lengths */
	int cw_dot_tracking_array[AVERAGE_ARRAY_LENGTH];
	int cw_dash_tracking_array[AVERAGE_ARRAY_LENGTH];
	/* Dot and dash length arrays */
	int cw_dt_dot_index;
	int cw_dt_dash_index;		/* Circular indexes into the arrays */
	int cw_dt_dot_tracking_sum;
	int cw_dt_dash_tracking_sum;	/* Running sum of array members */
};

/* in cw.c */
extern void cw_init(struct trx *trx);
extern void cw_sync_parameters(struct cw *c);

/* in cwrx.c */
extern int cw_rxprocess(struct trx *trx, float *buf, int len);

/* in cwtx.c */
extern int cw_txprocess(struct trx *trx);

#endif
