/*
 *    feldtx.c  --  FELDHELL transmitter
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

#include "trx.h"
#include "feld.h"
#include "misc.h"
#include "snd.h"
#include "filter.h"

static gboolean dxmode = FALSE;

static gint get_font_data(struct feld *s, gunichar c, gfloat *data, gint *len)
{
	GdkImage *image;
	gint i, j, w, length;
	gfloat pixval;
	gchar chr[8];

	gdk_threads_enter();

	i = g_unichar_to_utf8(c, chr);
	pango_layout_set_text(s->layout, chr, i);
	pango_layout_get_pixel_size(s->layout, &w, NULL);
	// get_layout_position (entry, &x, &y); ?????????

	gdk_draw_rectangle(s->pixmap, s->gc_black, TRUE, 0, 0, PIXMAP_W, PIXMAP_H);
	gdk_draw_layout(s->pixmap, s->gc_white, 0, 0, s->layout);
	image = gdk_image_get(s->pixmap, 0, 0, PIXMAP_W, PIXMAP_H);

	pixval = (gfloat) (1 << s->depth);

	if (dxmode)
		length = *len / PIXMAP_H / 2;
	else
		length = *len / PIXMAP_H;

	w = MIN(w + 1, length);
	w = MIN(w, PIXMAP_W);

	for (i = 0; i < w; i++) {
		for (j = PIXMAP_H - 1; j >= 0; j--)
			*data++ = gdk_image_get_pixel(image, i, j) / pixval;

		if (dxmode == FALSE)
			continue;

		for (j = PIXMAP_H - 1; j >= 0; j--)
			*data++ = gdk_image_get_pixel(image, i, j) / pixval;
	}

	gdk_image_destroy(image);

	gdk_threads_leave();

	if (dxmode)
		*len = 2 * w * PIXMAP_H;
	else
		*len = w * PIXMAP_H;

	return 0;
}

static inline double nco(struct feld *s, double freq)
{
	double x = sin(s->txphacc);

	s->txphacc += 2.0 * M_PI * freq / SampleRate;

	if (s->txphacc > M_PI)
		s->txphacc -= 2.0 * M_PI;

	return x;
}

#define	FNTBUFLEN	(2*PIXMAP_W*PIXMAP_H)

static void tx_char(struct trx *trx, gunichar c)
{
	struct feld *s = (struct feld *) trx->modem;
	float fntbuf[FNTBUFLEN];
	int fntlen = FNTBUFLEN;
	int outlen = 0;
	float pixel, *ptr;
	int i;

	/* handle tune signal */
	if (c == -1) {
		for (i = 0; i < fntlen; i++)
			fntbuf[i] = nco(s, trx->frequency);

		sound_write(fntbuf, fntlen);
		return;
	}

	/* get font data */
	if (get_font_data(s, c, fntbuf, &fntlen) < 0)
		return;

	ptr = fntbuf;

	pixel = *ptr++;
	fntlen--;

	/* upsample, filter and modulate */
	for (;;) {
		float x;

		filter_I_run(s->txfilt, pixel, &x);

		trx->outbuf[outlen++] = x * nco(s, trx->frequency);

		if (outlen >= OUTBUFSIZE) {
			g_warning("feldtx: outbuf overflow\n");
			break;
		}

		s->txcounter += UpSampleInc;

		if (s->txcounter < 1.0)
			continue;

		s->txcounter -= 1.0;

		if (fntlen == 0)
			break;

		pixel = *ptr++;
		fntlen--;
	}

	/* write to soundcard */
	sound_write(trx->outbuf, outlen);

	/* rx echo */
	feld_rxprocess(trx, trx->outbuf, outlen);
}

int feld_txprocess(struct trx *trx)
{
	struct feld *s = (struct feld *) trx->modem;
	gunichar c;

	if (trx->tune) {
		trx->tune = 0;
		s->preamble = 0;
		s->postamble = 0;
		tx_char(trx, -1);
		return 0;
	}

	if (s->preamble-- > 0) {
		tx_char(trx, '.');
		return 0;
	}

	c = trx_get_tx_char();

	/* if TX buffer empty */
	if (c == -1) {
		/* stop if requested to... */
		if (trx->stopflag) {
			if (s->postamble-- > 0) {
				tx_char(trx, '.');
				return 0;
			}
			tx_char(trx, ' ');
			return -1;
		}

		/* send idle character */
		c = '.';
	}

	if (c == '\r' || c == '\n')
		c = ' ';

	if (trx->hell_upper)
		c = g_unichar_toupper(c);

	tx_char(trx, c);

	return 0;
}
