/*
 *    psk31.h  --  PSK31 modem
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

#ifndef _PSK31_H
#define _PSK31_H

#include "cmplx.h"
#include "trx.h"
#include "viterbi.h"

#define	SampleRate	8000
#define PipeLen		64

struct psk31 {
	/*
	 * Common stuff
	 */
	int symbollen;
	int qpsk;

	double phaseacc;
	complex prevsymbol;
	unsigned int shreg;

	/*
	 * RX related stuff
	 */
	struct filter *fir1;
	struct filter *fir2;

	struct encoder *enc;
	struct viterbi *dec;

	double bitclk;
	float syncbuf[16];

	double pipe[PipeLen];
	unsigned int pipeptr;

	unsigned int dcdshreg;
	int dcd;

	complex quality;

	/*
	 * TX related stuff
	 */
	double *txshape;
	int preamble;
};

/* in psk31.c */
extern void psk31_init(struct trx *trx);

/* in psk31rx.c */
extern int psk31_rxprocess(struct trx *trx, float *buf, int len);

/* in psk31tx.c */
extern int psk31_txprocess(struct trx *trx);

#endif
