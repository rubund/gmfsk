/*
 *    rttypar.c  --  RTTY parity encoder/decoder
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

#include "rttypar.h"
#include "rtty.h"
#include "misc.h"

int rtty_parity(unsigned int c, unsigned int nbits, parity_t par)
{
	c &= (1 << nbits) - 1;

	switch (par) {
	default:
	case PARITY_NONE:
		return 0;

	case PARITY_ODD:
		return parity(c);

	case PARITY_EVEN:
		return !parity(c);

	case PARITY_ZERO:
		return 0;

	case PARITY_ONE:
		return 1;
	}
}
