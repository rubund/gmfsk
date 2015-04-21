/*
 *    cmplx.c  --  Complex arithmetic (non-optimized version)
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

#ifndef __OPTIMIZE__

#include "cmplx.h"

/*
 * Complex multiplication.
 */
complex cmul(complex x, complex y)
{
	complex z;

	c_re(z) = c_re(x) * c_re(y) - c_im(x) * c_im(y);
	c_im(z) = c_re(x) * c_im(y) + c_im(x) * c_re(y);

	return z;
}

/*
 * Complex addition.
 */
complex cadd(complex x, complex y)
{
	complex z;

	c_re(z) = c_re(x) + c_re(y);
	c_im(z) = c_im(x) + c_im(y);

	return z;
}

/*
 * Complex subtraction.
 */
complex csub(complex x, complex y)
{
	complex z;

	c_re(z) = c_re(x) - c_re(y);
	c_im(z) = c_im(x) - c_im(y);

	return z;
}

/*
 * Complex multiply-accumulate.
 */
complex cmac(complex *a, complex *b, int ptr, int len)
{
	complex z;
	int i;

	c_re(z) = 0.0;
	c_im(z) = 0.0;

	ptr = ptr % len;

	for (i = 0; i < len; i++) {
		z = cadd(z, cmul(a[i], b[ptr]));
		ptr = (ptr + 1) % len;
	}

	return z;
}

/*
 * Complex ... yeah, what??? Returns a complex number that has the
 * properties: |z| = |x| * |y|  and  arg(z) = arg(y) - arg(x)
 */
complex ccor(complex x, complex y)
{
	complex z;

	c_re(z) = c_re(x) * c_re(y) + c_im(x) * c_im(y);
	c_im(z) = c_re(x) * c_im(y) - c_im(x) * c_re(y);

	return z;
}

/*
 * Real part of the complex ???
 */
double ccorI(complex x, complex y)
{
	return c_re(x) * c_re(y) + c_im(x) * c_im(y);
}

/*
 * Imaginary part of the complex ???
 */
double ccorQ(complex x, complex y)
{
	return c_re(x) * c_im(y) - c_im(x) * c_re(y);
}

/*
 * Modulo (absolute value) of a complex number.
 */
double cmod(complex x)
{
	return sqrt(c_re(x) * c_re(x) + c_im(x) * c_im(x));
}

/*
 * Square of the absolute value (power).
 */
double cpwr(complex x)
{
	return (c_re(x) * c_re(x) + c_im(x) * c_im(x));
}

/*
 * Argument of a complex number.
 */
double carg(complex x)
{
	return atan2(c_im(x), c_re(x));
}

/*
 * Complex square root.
 */
complex csqrt(complex x)
{
	complex z;

	c_re(z) = sqrt(cmod(x) + c_re(x)) / M_SQRT2;
	c_im(z) = c_im(x) / c_re(z) / 2;

	return z;
}

#endif				/* __OPTIMIZE__ */
