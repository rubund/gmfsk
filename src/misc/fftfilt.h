/*
 *    fftfilt.h  --  Fast convolution FIR filter
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

#ifndef	_FFTFILT_H
#define	_FFTFILT_H

#include <glib.h>

#include "cmplx.h"
#include "fft.h"

/* ---------------------------------------------------------------------- */

struct fftfilt {
	gint filterlen;

        struct fft *fft;
        struct fft *ift;
        struct fft *tmpfft;

	complex *filter;

	gint inptr;

	complex *ovlbuf;
};

/* ---------------------------------------------------------------------- */

extern struct fftfilt *fftfilt_init(gdouble f1, gdouble f2, gint len);
extern void fftfilt_free(struct fftfilt *s);

extern void fftfilt_set_freqs(struct fftfilt *s, gdouble f1, gdouble f2);

extern gint fftfilt_run(struct fftfilt *, complex in, complex **out);

/* ---------------------------------------------------------------------- */

#endif
