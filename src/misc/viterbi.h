/*
 *    viterbi.h  --  Viterbi decoder
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

#ifndef _VITERBI_H
#define _VITERBI_H

#include <glib.h>

/* ---------------------------------------------------------------------- */

#define PATHMEM 64

struct viterbi {
	gint traceback;
	gint chunksize;
	gint nstates;

	gint *output;

	gint *metrics[PATHMEM];
	gint *history[PATHMEM];

	gint sequence[PATHMEM];

	gint mettab[2][256];

	guint ptr;
};

extern struct viterbi *viterbi_init(gint k, gint poly1, gint poly2);

extern gint viterbi_set_traceback(struct viterbi *v, gint traceback);
extern gint viterbi_set_chunksize(struct viterbi *v, gint chunksize);
extern gint viterbi_decode(struct viterbi *v, guchar *sym, gint *metric);
extern void viterbi_reset(struct viterbi *v);
extern void viterbi_free(struct viterbi *v);

/* ---------------------------------------------------------------------- */

struct encoder {
	gint *output;
	guint shreg;
	guint shregmask;
};

extern struct encoder *encoder_init(gint k, gint poly1, gint poly2);
extern void encoder_free(struct encoder *e);
extern gint encoder_encode(struct encoder *e, gint bit);

/* ---------------------------------------------------------------------- */

#endif
