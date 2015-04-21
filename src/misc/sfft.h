/*
 *    sfft.h  --  Sliding FFT
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

#ifndef _SFFT_H
#define _SFFT_H

#include <glib.h>

#include "cmplx.h"

struct sfft {
	gint fftlen;
	gint first;
	gint last;
	gint ptr;
	complex *twiddles;
	complex *bins;
	complex *history;
	gdouble corr;
};

extern struct sfft *sfft_init(gint, gint, gint);
extern void sfft_free(struct sfft *);

extern complex *sfft_run(struct sfft *, complex);

#endif
