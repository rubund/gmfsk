/*
 *    sfft.c  --  Sliding FFT
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
#include <math.h>

#include "sfft.h"
#include "misc.h"

#define	STABCOEFF	0.9999

/* ---------------------------------------------------------------------- */

struct sfft *sfft_init(gint len, gint first, gint last)
{
	struct sfft *s;
	gint i;

	s = g_new0(struct sfft, 1);

	s->twiddles = g_new0(complex, len);
	s->history  = g_new0(complex, len);
	s->bins     = g_new0(complex, len);

	s->fftlen = len;
	s->first = first;
	s->last = last;

	for (i = 0; i < len; i++) {
		c_re(s->twiddles[i]) = cos(i * 2.0 * M_PI / len) * STABCOEFF;
		c_im(s->twiddles[i]) = sin(i * 2.0 * M_PI / len) * STABCOEFF;
	}

	s->corr = pow(STABCOEFF, len);

	return s;
}

void sfft_free(struct sfft *s)
{
	if (s) {
		g_free(s->twiddles);
		g_free(s->history);
		g_free(s->bins);
		g_free(s);
	}
}

/*
 * Sliding FFT, complex input, complex output
 */
complex *sfft_run(struct sfft *s, complex new)
{
	complex old, z;
	gint i;

	/* restore the sample fftlen samples back */
	old = s->history[s->ptr];
	c_re(old) *= s->corr;
	c_im(old) *= s->corr;

	/* save the new sample */
	s->history[s->ptr] = new;

	/* advance the history pointer */
	s->ptr = (s->ptr + 1) % s->fftlen;

	/* calculate the wanted bins */
	for (i = s->first; i < s->last; i++) {
		z = s->bins[i];
		z = csub(z, old);
		z = cadd(z, new);
		s->bins[i] = cmul(z, s->twiddles[i]);
	}

	return s->bins;
}

/* ---------------------------------------------------------------------- */
