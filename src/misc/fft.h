/*
 *    fft.h  --  Fast Fourier Transform
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

#ifndef _FFT_H
#define _FFT_H

#include <glib.h>

#ifdef HAVE_DFFTW_H
 #include <dfftw.h>
#endif
#ifdef HAVE_FFTW_H
 #include <fftw.h>
#endif

#define	FFT_FWD	FFTW_FORWARD
#define	FFT_REV	FFTW_BACKWARD

#include "cmplx.h"

struct fft {
	gint len;
	fftw_plan plan;
	fftw_complex *in;
	fftw_complex *out;
};

extern struct fft *fft_init(gint len, gint dir);

extern void fft_free(struct fft *s);

extern void fft_clear_inbuf(struct fft *s);
extern void fft_clear_outbuf(struct fft *s);

extern void fft_run(struct fft *s);

extern void fft_load_wisdom(const gchar *filename);
extern void fft_save_wisdom(const gchar *filename);

extern void fft_set_wisdom(const gchar *wisdom);
extern char *fft_get_wisdom(void);


#endif
