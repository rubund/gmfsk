/*
 *    baudot.c  --  BAUDOT encoder/decoder
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

#include <ctype.h>

#include "baudot.h"

static unsigned char letters[32] = {
	'\0',	'E',	'\n',	'A',	' ',	'S',	'I',	'U',
	'\r',	'D',	'R',	'J',	'N',	'F',	'C',	'K',
	'T',	'Z',	'L',	'W',	'H',	'Y',	'P',	'Q',
	'O',	'B',	'G',	'·',	'M',	'X',	'V',	'·'
};

#if 0
/*
 * ITA-2 version of the figures case.
 */
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\'',	'8',	'7',
	'\r',	'·',	'4',	'\a',	',',	'·',	':',	'(',
	'5',	'+',	')',	'2',	'·',	'6',	'0',	'1',
	'9',	'?',	'·',	'·',	'.',	'/',	'=',	'·'
};
#endif
#if 0
/*
 * U.S. version of the figures case.
 */
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\a',	'8',	'7',
	'\r',	'$',	'4',	'\'',	',',	'!',	':',	'(',
	'5',	'"',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	'·',	'.',	'/',	';',	'·'
};
#endif
#if 1
/*
 * A mix of the two. This is what seems to be what people actually use.
 */
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\'',	'8',	'7',
	'\r',	'$',	'4',	'\a',	',',	'!',	':',	'(',
	'5',	'+',	')',	'2',	'H',	'6',	'0',	'1',
	'9',	'?',	'&',	'·',	'.',	'/',	'=',	'·'
};
#endif

int baudot_enc(unsigned char data)
{
	int i, c, mode;

	mode = 0;
	c = -1;

	if (islower(data))
		data = toupper(data);

	for (i = 0; i < 32; i++) {
		if (data == letters[i]) {
			mode |= BAUDOT_LETS;
			c = i;
		}
		if (data == figures[i]) {
			mode |= BAUDOT_FIGS;
			c = i;
		}
		if (c != -1)
			return (mode | c);
	}

	return -1;
}

char baudot_dec(int *mode, unsigned char data)
{
	int out = 0;

	switch (data) {
	case 0x1F:		/* letters */
		*mode = BAUDOT_LETS;
		break;
	case 0x1B:		/* figures */
		*mode = BAUDOT_FIGS;
		break;
	case 0x04:		/* unshift-on-space */
		*mode = BAUDOT_LETS;
		return ' ';
		break;
	default:
		if (*mode == BAUDOT_LETS)
			out = letters[data];
		else
			out = figures[data];
		break;
	}

	return out;
}
