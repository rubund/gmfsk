/*
 *    filter.c  --  FIR filter
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

#include <stdlib.h>
#include <string.h>

#include "filter.h"

#undef	DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

/* ---------------------------------------------------------------------- */

/*
 * This gets used when not optimising
 */
#ifndef	__OPTIMIZE__
gfloat mac(const gfloat *a, const gfloat *b, guint len)
{ 
	float sum = 0;
	guint i;

	for (i = 0; i < len; i++)
		sum += (*a++) * (*b++);
	return sum;
}
#endif

/* ---------------------------------------------------------------------- */

/*
 * Sinc done properly.
 */
static inline gdouble sinc(gdouble x)
{
	if (fabs(x) < 1e-10)
		return 1.0;
	else
		return sin(M_PI * x) / (M_PI * x);
}

/*
 * Don't ask...
 */
static inline gdouble cosc(gdouble x)
{
	if (fabs(x) < 1e-10)
		return 0.0;
	else
		return (1.0 - cos(M_PI * x)) / (M_PI * x);
}

/*
 * Hamming window function.
 */
static inline gdouble hamming(gdouble x)
{
	return 0.54 - 0.46 * cos(2 * M_PI * x);
}

/* ---------------------------------------------------------------------- */

/*
 * Create a band pass FIR filter with 6 dB corner frequencies
 * of 'f1' and 'f2'. (0 <= f1 < f2 <= 0.5)
 */
static gfloat *mk_filter(gint len, gint hilbert, gfloat f1, gfloat f2)
{
	gfloat *fir;
	gfloat t, h, x;
	gint i;

	fir = g_new(gfloat, len);

	for (i = 0; i < len; i++) {
		t = i - (len - 1.0) / 2.0;
		h = i * (1.0 / (len - 1.0));

		if (!hilbert) {
			x = (2 * f2 * sinc(2 * f2 * t) -
			     2 * f1 * sinc(2 * f1 * t)) * hamming(h);
		} else {
			x = (2 * f2 * cosc(2 * f2 * t) -
			     2 * f1 * cosc(2 * f1 * t)) * hamming(h);
			/*
			 * The actual filter code assumes the impulse response
			 * is in time reversed order. This will be anti-
			 * symmetric so the minus sign handles that for us.
			 */
			x = -x;
		}

		fir[i] = x;
	}

	return fir;
}

struct filter *filter_init(gint len, gint dec, gfloat *itaps, gfloat *qtaps)
{
	struct filter *f;

	f = g_new0(struct filter, 1);

	f->length = len;
	f->decimateratio = dec;

	if (itaps)
		f->ifilter = g_memdup(itaps, len * sizeof(gfloat));

	if (qtaps)
		f->qfilter = g_memdup(qtaps, len * sizeof(gfloat));

	f->pointer = len;
	f->counter = 0;

	return f;
}

struct filter *filter_init_lowpass(gint len, gint dec, gfloat freq)
{
	struct filter *f;
	gfloat *i, *q;

	i = mk_filter(len, 0, 0.0, freq);
	q = mk_filter(len, 0, 0.0, freq);

	f = filter_init(len, dec, i, q);

	g_free(i);
	g_free(q);

	return f;
}

struct filter *filter_init_bandpass(gint len, gint dec, gfloat f1, gfloat f2)
{
	struct filter *f;
	gfloat *i, *q;

	i = mk_filter(len, 0, f1, f2);
	q = mk_filter(len, 0, f1, f2);

	f = filter_init(len, dec, i, q);

	g_free(i);
	g_free(q);

	return f;
}

struct filter *filter_init_hilbert(gint len, gint dec)
{
	struct filter *f;
	gfloat *i, *q;

	i = mk_filter(len, 0, 0.05, 0.45);
	q = mk_filter(len, 1, 0.05, 0.45);

	f = filter_init(len, dec, i, q);

	g_free(i);
	g_free(q);

	return f;
}

void filter_free(struct filter *f)
{
	if (f) {
		g_free(f->ifilter);
		g_free(f->qfilter);
		g_free(f);
	}
}

/* ---------------------------------------------------------------------- */

gint filter_run(struct filter *f, complex in, complex *out)
{
	gfloat *iptr = f->ibuffer + f->pointer;
	gfloat *qptr = f->qbuffer + f->pointer;

	f->pointer++;
	f->counter++;

	*iptr = c_re(in);
	*qptr = c_im(in);

	if (f->counter == f->decimateratio) {
		out->re = mac(iptr - f->length, f->ifilter, f->length);
		out->im = mac(qptr - f->length, f->qfilter, f->length);
	}

	if (f->pointer == BufferLen) {
		iptr = f->ibuffer + BufferLen - f->length;
		qptr = f->qbuffer + BufferLen - f->length;
		memcpy(f->ibuffer, iptr, f->length * sizeof(float));
		memcpy(f->qbuffer, qptr, f->length * sizeof(float));
		f->pointer = f->length;
	}

	if (f->counter == f->decimateratio) {
		f->counter = 0;
		return 1;
	}

	return 0;
}

gint filter_I_run(struct filter *f, gfloat in, gfloat *out)
{
	gfloat *iptr = f->ibuffer + f->pointer;

	f->pointer++;
	f->counter++;

	*iptr = in;

	if (f->counter == f->decimateratio) {
		*out = mac(iptr - f->length, f->ifilter, f->length);
	}

	if (f->pointer == BufferLen) {
		iptr = f->ibuffer + BufferLen - f->length;
		memcpy(f->ibuffer, iptr, f->length * sizeof(float));
		f->pointer = f->length;
	}

	if (f->counter == f->decimateratio) {
		f->counter = 0;
		return 1;
	}

	return 0;
}

gint filter_Q_run(struct filter *f, gfloat in, gfloat *out)
{
	gfloat *qptr = f->ibuffer + f->pointer;

	f->pointer++;
	f->counter++;

	*qptr = in;

	if (f->counter == f->decimateratio) {
		*out = mac(qptr - f->length, f->qfilter, f->length);
	}

	if (f->pointer == BufferLen) {
		qptr = f->qbuffer + BufferLen - f->length;
		memcpy(f->qbuffer, qptr, f->length * sizeof(float));
		f->pointer = f->length;
	}

	if (f->counter == f->decimateratio) {
		f->counter = 0;
		return 1;
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

void filter_dump(struct filter *f)
{
	gint i;

	fprintf(stderr, "# len = %d\n", f->length);

	for (i = 0; i < f->length; i++) {
		if (f->ifilter)
			fprintf(stderr, "% .10f  ", f->ifilter[i]);
		else
			fprintf(stderr, "             ");

		if (f->qfilter)
			fprintf(stderr, "% .10f\n", f->qfilter[i]);
		else
			fprintf(stderr, "\n");
	}
}

/* ---------------------------------------------------------------------- */
