/*
 *    viterbi.c  --  Viterbi decoder
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
#include <limits.h>

#include "viterbi.h"
#include "misc.h"

/* ---------------------------------------------------------------------- */

struct viterbi *viterbi_init(gint k, gint poly1, gint poly2)
{
	struct viterbi *v;
	gint i;

	v = g_new0(struct viterbi, 1);

	v->traceback = PATHMEM - 1;
	v->chunksize = 8;

	v->nstates = 1 << (k - 1);

	v->output = g_new0(int, 1 << k);

	for (i = 0; i < (1 << k); i++)
		v->output[i] = parity(poly1 & i) | (parity(poly2 & i) << 1);

	for (i = 0; i < PATHMEM; i++) {
		v->metrics[i] = g_new0(int, v->nstates);
		v->history[i] = g_new0(int, v->nstates);
	}

	for (i = 0; i < 256; i++) {
		v->mettab[0][i] = 128 - i;
		v->mettab[1][i] = i - 128;
	}

	v->ptr = 0;

	return v;
}

gint viterbi_set_traceback(struct viterbi *v, gint traceback)
{
	if (traceback < 0 || traceback > PATHMEM - 1)
		return -1;

	v->traceback = traceback;
	return 0;
}

gint viterbi_set_chunksize(struct viterbi *v, gint chunksize)
{
	if (chunksize < 1 || chunksize > v->traceback)
		return -1;

	v->chunksize = chunksize;
	return 0;
}

void viterbi_reset(struct viterbi *v)
{
	int i;

	for (i = 0; i < PATHMEM; i++) {
		memset(v->metrics[i], 0, v->nstates * sizeof(int));
		memset(v->history[i], 0, v->nstates * sizeof(int));
	}

	v->ptr = 0;
}

void viterbi_free(struct viterbi *v)
{
	int i;

	if (v) {
		g_free(v->output);

		for (i = 0; i < v->nstates; i++) {
			g_free(v->metrics[i]);
			g_free(v->history[i]);
		}

		g_free(v);
	}
}

static gint traceback(struct viterbi *v, gint *metric);

gint viterbi_decode(struct viterbi *v, guchar *sym, gint *metric)
{
	guint currptr, prevptr;
	gint i, j, met[4], n;

	currptr = v->ptr;
	prevptr = (currptr - 1) % PATHMEM;

#if 0
	sym[0] = (sym[0] < 128) ? 0 : 255;
	sym[1] = (sym[1] < 128) ? 0 : 255;
#endif

	met[0] = v->mettab[0][sym[1]] + v->mettab[0][sym[0]];
	met[1] = v->mettab[0][sym[1]] + v->mettab[1][sym[0]];
	met[2] = v->mettab[1][sym[1]] + v->mettab[0][sym[0]];
	met[3] = v->mettab[1][sym[1]] + v->mettab[1][sym[0]];

	for (n = 0; n < v->nstates; n++) {
		int p0, p1, s0, s1, m0, m1;

		s0 = n;
		s1 = n + v->nstates;

		p0 = s0 >> 1;
		p1 = s1 >> 1;

		m0 = v->metrics[prevptr][p0] + met[v->output[s0]];
		m1 = v->metrics[prevptr][p1] + met[v->output[s1]];

		if (m0 > m1) {
			v->metrics[currptr][n] = m0;
			v->history[currptr][n] = p0;
		} else {
			v->metrics[currptr][n] = m1;
			v->history[currptr][n] = p1;
		}
	}

	v->ptr = (v->ptr + 1) % PATHMEM;

	if ((v->ptr % v->chunksize) == 0)
		return traceback(v, metric);

	if (v->metrics[currptr][0] > INT_MAX / 2) {
		for (i = 0; i < PATHMEM; i++)
			for (j = 0; j < v->nstates; j++)
				v->metrics[i][j] -= INT_MAX / 2;
	}
	if (v->metrics[currptr][0] < INT_MIN / 2) {
		for (i = 0; i < PATHMEM; i++)
			for (j = 0; j < v->nstates; j++)
				v->metrics[i][j] += INT_MIN / 2;
	}

	return -1;
}

static gint traceback(struct viterbi *v, gint *metric)
{
	gint i, bestmetric, beststate;
	guint p, c;

	p = (v->ptr - 1) % PATHMEM;

	/*
	 * Find the state with the best metric
	 */
	bestmetric = INT_MIN;
	beststate = 0;

	for (i = 0; i < v->nstates; i++) {
		if (v->metrics[p][i] > bestmetric) {
			bestmetric = v->metrics[p][i];
			beststate = i;
		}
	}

	/*
	 * Trace back 'traceback' steps, starting from the best state
	 */
	v->sequence[p] = beststate;

	for (i = 0; i < v->traceback; i++) {
		unsigned int prev = (p - 1) % PATHMEM;

		v->sequence[prev] = v->history[p][v->sequence[p]];
		p = prev;
	}

	if (metric)
		*metric = v->metrics[p][v->sequence[p]];

	/*
	 * Decode 'chunksize' bits
	 */
	for (c = i = 0; i < v->chunksize; i++) {
		/*
		 * low bit of state is the previous input bit
		 */
		c = (c << 1) | (v->sequence[p] & 1);
		p = (p + 1) % PATHMEM;
	}

	if (metric)
		*metric = v->metrics[p][v->sequence[p]] - *metric;

	return c;
}

/* ---------------------------------------------------------------------- */

struct encoder *encoder_init(gint k, gint poly1, gint poly2)
{
	struct encoder *e;
	gint i, size;

	e = g_new0(struct encoder, 1);

	size = 1 << k;	/* size of the output table */

	e->output = g_new0(int, size);

	for (i = 0; i < size; i++)
		e->output[i] = parity(poly1 & i) | (parity(poly2 & i) << 1);

	e->shreg = 0;
	e->shregmask = size - 1;

	return e;
}

void encoder_free(struct encoder *e)
{
	if (e) {
		g_free(e->output);
		g_free(e);
	}
}

gint encoder_encode(struct encoder *e, gint bit)
{
	e->shreg = (e->shreg << 1) | !!bit;

	return e->output[e->shreg & e->shregmask];
}

/* ---------------------------------------------------------------------- */
