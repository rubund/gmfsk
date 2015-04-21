/*
 *    misc.c  --  Miscellaneous helper functions (non-optimized version)
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

#include "misc.h"

#ifndef __OPTIMIZE__

/* ---------------------------------------------------------------------- */

/*
 * Hamming weight (number of bits that are ones).
 */
guint32 hweight32(guint32 w) 
{
	w = (w & 0x55555555) + ((w >>  1) & 0x55555555);
	w = (w & 0x33333333) + ((w >>  2) & 0x33333333);
	w = (w & 0x0F0F0F0F) + ((w >>  4) & 0x0F0F0F0F);
	w = (w & 0x00FF00FF) + ((w >>  8) & 0x00FF00FF);
	w = (w & 0x0000FFFF) + ((w >> 16) & 0x0000FFFF);
	return w;
}

guint16 hweight16(guint16 w)
{
	w = (w & 0x5555) + ((w >> 1) & 0x5555);
	w = (w & 0x3333) + ((w >> 2) & 0x3333);
	w = (w & 0x0F0F) + ((w >> 4) & 0x0F0F);
	w = (w & 0x00FF) + ((w >> 8) & 0x00FF);
	return w;
}

guint8 hweight8(guint8 w)
{
	w = (w & 0x55) + ((w >> 1) & 0x55);
	w = (w & 0x33) + ((w >> 2) & 0x33);
	w = (w & 0x0F) + ((w >> 4) & 0x0F);
	return w;
}

/* ---------------------------------------------------------------------- */

/*
 * Parity function. Return one if `w' has odd number of ones, zero otherwise.
 */
gint parity(guint32 w)
{
	return hweight32(w) & 1;
}

/* ---------------------------------------------------------------------- */

/*
 * Reverse order of bits.
 */
guint32 rbits32(guint32 w)
{
	w = ((w >>  1) & 0x55555555) | ((w <<  1) & 0xAAAAAAAA);
	w = ((w >>  2) & 0x33333333) | ((w <<  2) & 0xCCCCCCCC);
	w = ((w >>  4) & 0x0F0F0F0F) | ((w <<  4) & 0xF0F0F0F0);
	w = ((w >>  8) & 0x00FF00FF) | ((w <<  8) & 0xFF00FF00);
	w = ((w >> 16) & 0x0000FFFF) | ((w << 16) & 0xFFFF0000);
	return w;
}

guint16 rbits16(guint16 w)
{
	w = ((w >> 1) & 0x5555) | ((w << 1) & 0xAAAA);
	w = ((w >> 2) & 0x3333) | ((w << 2) & 0xCCCC);
	w = ((w >> 4) & 0x0F0F) | ((w << 4) & 0xF0F0);
	w = ((w >> 8) & 0x00FF) | ((w << 8) & 0xFF00);
	return w;
}

guint8 rbits8(guint8 w)
{
	w = ((w >> 1) & 0x55) | ((w << 1) & 0xFF);
	w = ((w >> 2) & 0x33) | ((w << 2) & 0xCC);
	w = ((w >> 4) & 0x0F) | ((w << 4) & 0xF0);
	return w;
}

/* ---------------------------------------------------------------------- */

/*
 * Integer base-2 logarithm
 */
/*
guint log2(guint x)
{
	int y = 0;

	x >>= 1;

	while (x) {
		x >>= 1;
		y++;
	}

	return y;
}
*/

/* ---------------------------------------------------------------------- */

/*
 * Gray encoding and decoding (8 bit)
 */
guint8 grayencode(guint8 data)
{
	guint8 bits = data;

	bits ^= data >> 1;
	bits ^= data >> 2;
	bits ^= data >> 3;
	bits ^= data >> 4;
	bits ^= data >> 5;
	bits ^= data >> 6;
	bits ^= data >> 7;

	return bits;
}

guint8 graydecode(guint8 data)
{
	return data ^ (data >> 1);
}

/* ---------------------------------------------------------------------- */

/*
 * Hamming window function
 */
gdouble hamming(gdouble x)
{
	return 0.54 - 0.46 * cos(2 * M_PI * x);
}

/* ---------------------------------------------------------------------- */

/*
 * Sinc etc...
 */
gdouble sinc(gdouble x)
{
	return (fabs(x) < 1e-10) ? 1.0 : (sin(M_PI * x) / (M_PI * x));
}

gdouble cosc(gdouble x)
{
	return (fabs(x) < 1e-10) ? 0.0 : ((1.0 - cos(M_PI * x)) / (M_PI * x));
}

/* ---------------------------------------------------------------------- */

gfloat clamp(gfloat x, gfloat min, gfloat max)
{
	return (x < min) ? min : ((x > max) ? max : x);
}

/* ---------------------------------------------------------------------- */

gfloat decayavg(gfloat average, gfloat input, gfloat weight)
{
	return input * (1.0 / weight) + average * (1.0 - (1.0 / weight));
}

/* ---------------------------------------------------------------------- */

#endif				/* __OPTIMIZE__ */
