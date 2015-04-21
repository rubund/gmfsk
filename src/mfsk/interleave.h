/*
 *    interleave.h  --  MFSK (de)interleaver
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

#ifndef _INTERLEAVE_H
#define _INTERLEAVE_H

/* ---------------------------------------------------------------------- */

#define	INTERLEAVE_FWD	0
#define	INTERLEAVE_REV	1

struct interleave {
	int size;
	int direction;
	unsigned char *table;
};

extern struct interleave *interleave_init(int size, int dir);
extern void interleave_free(struct interleave *s);
extern void interleave_syms(struct interleave *s, unsigned char *syms);
extern void interleave_bits(struct interleave *s, unsigned int *bits);

/* ---------------------------------------------------------------------- */

#endif
