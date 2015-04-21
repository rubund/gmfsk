/*
 *    interleave.c  --  MFSK (de)interleaver
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <glib.h>

#include "interleave.h"

/* ---------------------------------------------------------------------- */

struct interleave *interleave_init(int size, int dir)
{
	struct interleave *s;

	s = g_new0(struct interleave, 1);

	s->table = g_new0(unsigned char, 10 * size * size);

	s->size = size;
	s->direction = dir;

	return s;
}

void interleave_free(struct interleave *s)
{
	if (s) {
		g_free(s->table);
		g_free(s);
	}
}

static inline unsigned char *tab(struct interleave *s, int i, int j, int k)
{
	return &s->table[(s->size * s->size * i) + (s->size * j) + k];
}

void interleave_syms(struct interleave *s, unsigned char *syms)
{
	int i, j, k;

	for (k = 0; k < 10; k++) {
		for (i = 0; i < s->size; i++)
			for (j = 0; j < s->size - 1; j++)
				*tab(s, k, i, j) = *tab(s, k, i, j + 1);

		for (i = 0; i < s->size; i++)
			*tab(s, k, i, s->size - 1) = syms[i];

		for (i = 0; i < s->size; i++) {
			if (s->direction == INTERLEAVE_FWD)
				syms[i] = *tab(s, k, i, s->size - i - 1);
			else
				syms[i] = *tab(s, k, i, i);
		}
	}
}

void interleave_bits(struct interleave *s, unsigned int *bits)
{
	unsigned char *syms;
	int i;

	syms = g_alloca(s->size * sizeof(unsigned char));

	for (i = 0; i < s->size; i++)
		syms[i] = (*bits >> (s->size - i - 1)) & 1;

	interleave_syms(s, syms);

	for (*bits = i = 0; i < s->size; i++)
		*bits = (*bits << 1) | syms[i];
}

/* ---------------------------------------------------------------------- */

