/*
 *    delay.c  --  Delay line
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

#include "delay.h"

/* ---------------------------------------------------------------------- */

struct delay *delay_init(gint len)
{
	struct delay *s;

	s = g_new0(struct delay, 1);

	s->delayline = g_new0(guchar, len);
	s->len = len;
	s->ptr = 0;

	return s;
}

void delay_free(struct delay *s)
{
	if (s) {
		g_free(s->delayline);
		g_free(s);
	}
}

guchar delay_run(struct delay *s, guchar data)
{
	unsigned char ret;

	ret = s->delayline[s->ptr];

	s->delayline[s->ptr] = data;

	s->ptr = (s->ptr + 1) % s->len;

	return ret;
}

/* ---------------------------------------------------------------------- */

