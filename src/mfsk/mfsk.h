/*
 *    mfsk.h  --  MFSK modem
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

#ifndef _MFSK_H
#define _MFSK_H

#include "cmplx.h"
#include "trx.h"
#include "viterbi.h"
#include "interleave.h"
#include "fftfilt.h"
#include "delay.h"
#include "picture.h"

#define	SampleRate		(8000)
#define	SAMPLES_PER_PIXEL	(SampleRate / 1000)	/* 1 ms per pixel */

#define	K	7
#define	POLY1	0x6d
#define	POLY2	0x4f

struct rxpipe {
	complex vector[32];	/* numtones <= 32 */
	int symbol;
};

struct mfsk {
	/*
	 * Common stuff
	 */
	double phaseacc;

	int symlen;
	int symbits;
	int numtones;
	int basetone;
	double tonespacing;

	int counter;

	/*
	 * RX related stuff
	 */
	int rxstate;

	struct filter *hilbert;
	struct sfft *sfft;

	struct filter *filt;

	struct viterbi *dec1;
	struct viterbi *dec2;
	struct interleave *rxinlv;

	struct rxpipe *pipe;
	unsigned int pipeptr;

	unsigned int datashreg;

	complex currvector;
	complex prev1vector;
	complex prev2vector;

	int currsymbol;
	int prev1symbol;
	int prev2symbol;

	float met1;
	float met2;

	int synccounter;

	unsigned char symbolpair[2];
	int symcounter;

	int picturesize;
	char picheader[16];
	complex prevz;
	double picf;

	int symbolbit;

	/*
	 * TX related stuff
	 */
	int txstate;

	struct fft *fft;

	struct encoder *enc;
	struct interleave *txinlv;

	unsigned int bitshreg;
	int bitstate;

	Picbuf *picbuf;
};

enum {
	TX_STATE_PREAMBLE,
	TX_STATE_START,
	TX_STATE_DATA,
	TX_STATE_END,
	TX_STATE_FLUSH,
	TX_STATE_FINISH,
	TX_STATE_TUNE,
	TX_STATE_PICTURE_START,
	TX_STATE_PICTURE
};

enum {
	RX_STATE_DATA,
	RX_STATE_PICTURE_START_1,
	RX_STATE_PICTURE_START_2,
	RX_STATE_PICTURE
};

/* in mfsk.c */
extern void mfsk_init(struct trx *trx);

/* in mfskrx.c */
extern int mfsk_rxprocess(struct trx *trx, float *buf, int len);

/* in mfsktx.c */
extern int mfsk_txprocess(struct trx *trx);

#endif
