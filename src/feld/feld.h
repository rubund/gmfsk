/*
 *    feld.h  --  FELDHELL modem
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

#ifndef _FELD_H
#define _FELD_H

#include <gnome.h>

#include "cmplx.h"
#include "trx.h"

#define	SampleRate	8000
#define	ColumnRate	17.5
#define BandWidth	245.0

#define	RxColumnLen	30
#define	TxColumnLen	14

#define	RxPixRate	((RxColumnLen)*(ColumnRate))
#define	TxPixRate	((TxColumnLen)*(ColumnRate))

#define	DownSampleInc	((double)(RxPixRate)/(SampleRate))
#define	UpSampleInc	((double)(TxPixRate)/(SampleRate))

#define	PIXMAP_W	14
#define	PIXMAP_H	(TxColumnLen)

struct feld {
        /*
         * Common stuff
         */

	/*
	 * RX related stuff
	 */
	double rxphacc;
	double rxcounter;

	struct filter *hilbert;
	struct fftfilt *fftfilt;

	double agc;

	/*
	 * TX related stuff
	 */
	double txphacc;
	double txcounter;

	struct filter *txfilt;

	GdkPixmap *pixmap;
	gint depth;

	GdkGC *gc_black;
	GdkGC *gc_white;

	PangoLayout *layout;
	PangoContext *context;

	int preamble;
	int postamble;
};

/* in feld.c */
extern void feld_init(struct trx *trx);

/* in feldrx.c */
extern int feld_rxprocess(struct trx *trx, float *buf, int len);

/* in feldtx.c */
extern int feld_txprocess(struct trx *trx);

#endif
