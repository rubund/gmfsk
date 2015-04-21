/*
 *    genfilt.c  --  Generate an inline assembler FIR filter optimized
 *                   for the Pentium.
 *
 *    Copyright (C) 2001, 2002, 2003
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This is inspired by the works of Phil Karn and Thomas Sailer.
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
#include <stdio.h>

#define asmline(s)		puts("\t\t\"" s ";\\n\\t\"")
#define asmline2(s1,d,s2)	printf("\t\t\"%s%d%s;\\n\\t\"\n", s1, d, s2)

int main(int argc, char **argv)
{
	int i, len;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filter_length>\n", argv[0]);
		return 1;
	}

	len = atoi(argv[1]);

#if 0
	puts("#ifndef _FILTER_I386_H");
	puts("#define _FILTER_I386_H");
	puts("#define __HAVE_ARCH_MAC");

	puts("extern inline float mac(const float *a, const float *b)");
	puts("{");
	puts("\tfloat f;");
#endif

	puts("\tasm volatile (");

	asmline("flds (%1)");
	asmline("fmuls (%2)");
	asmline("flds 4(%1)");
	asmline("fmuls 4(%2)");

	for (i = 2; i < len; i++) {
		asmline2("flds ", i * 4, "(%1)");
		asmline2("fmuls ", i * 4, "(%2)");
		asmline("fxch %%st(2)");
		asmline("faddp");
	}

	asmline("faddp");

	puts("\t\t: \"=t\" (f) : \"r\" (a) , \"r\" (b) : \"memory\");");

#if 0
	puts("\treturn f;");
	puts("}");

	puts("#endif");
#endif

	return 0;
}
