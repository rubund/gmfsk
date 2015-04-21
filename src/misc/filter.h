/*
 *    filter.h  --  FIR filter
 *
 *    Copyright (C) 2001, 2002, 2003, 2004
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

#ifndef _FILTER_H
#define _FILTER_H

#include <glib.h>

#include "cmplx.h"

#define BufferLen	1024

/* ---------------------------------------------------------------------- */

#ifdef __OPTIMIZE__

#ifdef __i386__
#include "filter-i386.h"
#endif				/* __i386__ */


#ifndef __HAVE_ARCH_MAC
extern __inline__ float mac(const gfloat *a, const gfloat *b, guint size)
{
	gfloat sum = 0;
	guint i;

	for (i = 0; i < size; i++)
		sum += (*a++) * (*b++);
	return sum;
}
#endif				/* __HAVE_ARCH_MAC */

#endif				/* __OPTIMIZE__ */

/* ---------------------------------------------------------------------- */

struct filter {
	gint length;
	gint decimateratio;

	gfloat *ifilter;
	gfloat *qfilter;

	gfloat ibuffer[BufferLen];
	gfloat qbuffer[BufferLen];

	gint pointer;
	gint counter;
};

extern struct filter *filter_init(gint len, gint dec, gfloat *ifil, gfloat *qfil);
extern struct filter *filter_init_lowpass(gint len, gint dec, gfloat freq);
extern struct filter *filter_init_bandpass(gint len, gint dec, gfloat f1, gfloat f2);
extern struct filter *filter_init_hilbert(gint len, gint dec);
extern void filter_free(struct filter *f);

extern gint filter_run(struct filter *f, complex in, complex *out);
extern gint filter_I_run(struct filter *f, gfloat in, gfloat *out);
extern gint filter_Q_run(struct filter *f, gfloat in, gfloat *out);

extern void filter_dump(struct filter *f);

/* ---------------------------------------------------------------------- */
#endif				/* _FILTER_H */
