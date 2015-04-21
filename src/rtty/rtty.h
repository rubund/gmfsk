/*
 *    rtty.h  --  RTTY modem
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

#ifndef _RTTY_H
#define _RTTY_H

#include "cmplx.h"
#include "trx.h"

#define	SampleRate	8000
#define	MaxSymLen	1024

typedef enum {
	RTTY_RX_STATE_IDLE = 0,
	RTTY_RX_STATE_START,
	RTTY_RX_STATE_DATA,
	RTTY_RX_STATE_PARITY,
	RTTY_RX_STATE_STOP,
	RTTY_RX_STATE_STOP2
} rtty_rx_state_t;

typedef enum {
	PARITY_NONE = 0,
	PARITY_EVEN,
	PARITY_ODD,
	PARITY_ZERO,
	PARITY_ONE
} parity_t;

struct rtty {
	/*
	 * Common stuff
	 */
	double shift;
	int symbollen;
	int nbits;
	parity_t parity;
	int stoplen;
	int reverse;
	int msb;

	double phaseacc;

	/*
	 * RX related stuff
	 */
	struct filter *hilbert;
	struct fftfilt *fftfilt;

	double pipe[MaxSymLen];
	unsigned int pipeptr;

	double bbfilter[MaxSymLen];
	unsigned int filterptr;

	rtty_rx_state_t rxstate;

	int counter;
	int bitcntr;
	int rxdata;

	double prevsymbol;

	int rxmode;

	/*
	 * TX related stuff
	 */
	int txmode;
	int preamble;
};

/* in rtty.c */
extern void rtty_init(struct trx *trx);

/* in rttyrx.c */
extern int rtty_rxprocess(struct trx *trx, float *buf, int len);

/* in rttytx.c */
extern int rtty_txprocess(struct trx *trx);

#endif
