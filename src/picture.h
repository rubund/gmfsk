/*
 *    picture.h  --  MFSK pictures support
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

#ifndef _PICTURE_H
#define _PICTURE_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* ---------------------------------------------------------------------- */

typedef struct _Picbuf	Picbuf;

/* ---------------------------------------------------------------------- */

extern gboolean picture_check_header(gchar *str, gint *w, gint *h, gboolean *color);

extern void picture_start(gint width, gint height, gboolean color);
extern void picture_stop(void);

extern void picture_write(guchar data);

extern void picture_send(gchar *filename, gboolean color);

/* ---------------------------------------------------------------------- */

extern Picbuf *picbuf_new_from_pixbuf(GdkPixbuf *pixbuf, gboolean color);

extern gboolean picbuf_get_color(Picbuf *picbuf);
extern gint picbuf_get_width(Picbuf *picbuf);
extern gint picbuf_get_height(Picbuf *picbuf);

extern gchar *picbuf_make_header(Picbuf *picbuf);

extern gboolean picbuf_get_data(Picbuf *picbuf, guchar *data, gint *len);
extern gdouble picbuf_get_percentage(Picbuf *picbuf);

extern void picbuf_free(Picbuf *picbuf);

/* ---------------------------------------------------------------------- */

#endif
