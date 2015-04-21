/*
 *    throb.h  --  THROB modem
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

#ifndef _THROB_H
#define _THROB_H

#include "cmplx.h"
#include "trx.h"

#define	SampleRate	8000
#define	DownSample	32

#define	NumTones	9
#define	NumChars	45

#define	SymbolLen1	8192
#define	SymbolLen2	4096
#define	SymbolLen4	2048

#define	MaxRxSymLen	(SymbolLen1 / DownSample)

#define	FilterFFTLen	8192

struct throb {
	/*
	 * Common stuff
	 */
	int symlen;
	double phaseacc;
	double freqs[NumTones];

	/*
	 * RX related stuff
	 */
	struct filter *hilbert;
	struct fftfilt *fftfilt;
	struct filter *syncfilt;

	complex *rxtone[NumTones];
	complex symbol[MaxRxSymLen];

	float syncbuf[MaxRxSymLen];
	float dispbuf[MaxRxSymLen];

	float rxcntr;

	int rxsymlen;
	int symptr;
	int deccntr;
	int shift;
	int waitsync;

	/*
	 * TX related stuff
	 */
	int preamble;
	float *txpulse;
};

/* in throb.c */
extern void throb_init(struct trx *trx);

/* in throbrx.c */
extern int throb_rxprocess(struct trx *trx, float *buf, int len);

/* in throbtx.c */
extern int throb_txprocess(struct trx *trx);

#endif
