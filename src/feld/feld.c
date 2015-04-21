/*
 *    feld.c  --  FELDHELL modem
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

#include <stdlib.h>
#include <stdio.h>

#include "trx.h"
#include "feld.h"
#include "filter.h"
#include "fftfilt.h"

static void feld_txinit(struct trx *trx)
{
	struct feld *s = (struct feld *) trx->modem;

	s->txcounter = 0.0;
	s->preamble = 3;
	s->postamble = 3;
	return;
}

static void feld_rxinit(struct trx *trx)
{
	struct feld *s = (struct feld *) trx->modem;

	s->rxcounter = 0.0;
	s->agc = 0.0;
	return;
}

#define	unref(obj)	if (obj) g_object_unref(G_OBJECT(obj))

static void feld_free(struct feld *s)
{
        if (s) {
                filter_free(s->hilbert);
		fftfilt_free(s->fftfilt);

		unref(s->pixmap);
		unref(s->gc_white);
		unref(s->gc_black);
		unref(s->context);
		unref(s->layout);

                g_free(s);
        }
}

static void feld_destructor(struct trx *trx)
{
	struct feld *s = (struct feld *) trx->modem;

	feld_free(s);

        trx->modem = NULL;
        trx->txinit = NULL;
        trx->rxinit = NULL;
        trx->txprocess = NULL;
        trx->rxprocess = NULL;
        trx->destructor = NULL;
}

void feld_init(struct trx *trx)
{
	GdkColor color;
	GdkColormap *cmap;
	GdkVisual *visual;
	struct feld *s;
	double lp;
	PangoFontDescription *fontdesc;

	s = g_new0(struct feld, 1);

	if ((s->hilbert = filter_init_hilbert(37, 1)) == NULL) {
		feld_free(s);
		return;
	}

	lp = BandWidth / 2.0 / SampleRate;

	if ((s->txfilt = filter_init_lowpass(65, 1, lp)) == NULL) {
		feld_free(s);
		return;
	}

	lp = trx->hell_bandwidth / 2.0 / SampleRate;

	if ((s->fftfilt = fftfilt_init(0, lp, 1024)) == NULL) {
		feld_free(s);
		return;
	}

	cmap = gdk_colormap_get_system();
	visual = gdk_colormap_get_visual(cmap);

	s->depth = visual->depth;

	s->pixmap = gdk_pixmap_new(NULL, PIXMAP_W, PIXMAP_H, s->depth);
	gdk_drawable_set_colormap(s->pixmap, cmap);

	s->gc_black = gdk_gc_new(s->pixmap);
	gdk_color_black(cmap, &color);
	gdk_gc_set_foreground(s->gc_black, &color);

	s->gc_white = gdk_gc_new(s->pixmap);
	gdk_color_white(cmap, &color);
	gdk_gc_set_foreground(s->gc_white, &color);

	s->context = gdk_pango_context_get();

	gdk_pango_context_set_colormap(s->context, cmap);
	pango_context_set_base_dir(s->context, PANGO_DIRECTION_LTR);
	pango_context_set_language(s->context, gtk_get_default_language());

	fontdesc = pango_font_description_from_string(trx->hell_font);
	pango_context_set_font_description(s->context, fontdesc);
	pango_font_description_free(fontdesc);

	s->layout = pango_layout_new(s->context);

	trx->modem = s;

	trx->txinit = feld_txinit;
	trx->rxinit = feld_rxinit;

	trx->txprocess = feld_txprocess;
	trx->rxprocess = feld_rxprocess;

	trx->destructor = feld_destructor;

	trx->samplerate = SampleRate;
	trx->bandwidth = trx->hell_bandwidth;
}
