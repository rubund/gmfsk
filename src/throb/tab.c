/*
 *    tab.c  --  THROB tables
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

#include "throb.h"
#include "tab.h"

unsigned int ThrobTonePairs[NumChars][2] = {
	{5, 5},			/* idle... no print */
	{4, 5},			/* A */
	{1, 2},			/* B */
	{1, 3},			/* C */
	{1, 4},			/* D */
	{4, 6},			/* SHIFT (was E) */
	{1, 5},			/* F */
	{1, 6},			/* G */
	{1, 7},			/* H */
	{3, 7},			/* I */
	{1, 8},			/* J */
	{2, 3},			/* K */
	{2, 4},			/* L */
	{2, 8},			/* M */
	{2, 5},			/* N */
	{5, 6},			/* O */
	{2, 6},			/* P */
	{2, 9},			/* Q */
	{3, 4},			/* R */
	{3, 5},			/* S */
	{1, 9},			/* T */
	{3, 6},			/* U */
	{8, 9},			/* V */
	{3, 8},			/* W */
	{3, 3},			/* X */
	{2, 2},			/* Y */
	{1, 1},			/* Z */
	{3, 9},			/* 1 */
	{4, 7},			/* 2 */
	{4, 8},			/* 3 */
	{4, 9},			/* 4 */
	{5, 7},			/* 5 */
	{5, 8},			/* 6 */
	{5, 9},			/* 7 */
	{6, 7},			/* 8 */
	{6, 8},			/* 9 */
	{6, 9},			/* 0 */
	{7, 8},			/* , */
	{7, 9},			/* . */
	{8, 8},			/* ' */
	{7, 7},			/* / */
	{6, 6},			/* ) */
	{4, 4},			/* ( */
	{9, 9},			/* E */
	{2, 7}			/* space */
};

unsigned char ThrobCharSet[NumChars] = {
	'\0',			/* idle */
	'A',
	'B',
	'C',
	'D',
	'\0',			/* shift */
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0',
	',',
	'.',
	'\'',
	'/',
	')',
	'(',
	'E',
	' '
};

double ThrobToneFreqsNar[NumTones] = {-32, -24, -16,  -8,  0,  8, 16, 24, 32};
double ThrobToneFreqsWid[NumTones] = {-64, -48, -32, -16,  0, 16, 32, 48, 64};

