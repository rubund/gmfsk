/* 
 * Waterfall Spectrum Analyzer Widget
 * Copyright (C) 2001 Tomi Manninen <oh2bns@sral.fi>
 *
 * Based on the Spectrum Widget by Thomas Sailer <sailer@ife.ee.ethz.ch>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __WATERFALL_H__
#define __WATERFALL_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#ifdef HAVE_DFFTW_H
#  include <dfftw.h>
#endif
#ifdef HAVE_FFTW_H
#  include <fftw.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WATERFALL(obj)		GTK_CHECK_CAST(obj, waterfall_get_type(), Waterfall)
#define WATERFALL_CLASS(klass)	GTK_CHECK_CLASS_CAST(klass, waterfall_get_type(), WaterfallClass)
#define IS_WATERFALL(obj)	GTK_CHECK_TYPE(obj, waterfall_get_type())

typedef struct _Waterfall	Waterfall;
typedef struct _WaterfallClass	WaterfallClass;

typedef enum {
	WATERFALL_WINDOW_RECT,
	WATERFALL_WINDOW_TRIA,
	WATERFALL_WINDOW_HAMM
} wf_window_t;

typedef enum {
	WATERFALL_MODE_NORMAL,
	WATERFALL_MODE_SPECTRUM,
	WATERFALL_MODE_SCOPE,
	WATERFALL_MODE_SPECTRUM_PEAK
} wf_mode_t;

typedef enum {
	WATERFALL_MAG_1,
	WATERFALL_MAG_2,
	WATERFALL_MAG_4
} wf_mag_t;

typedef enum {
	WATERFALL_SPEED_HALF,
	WATERFALL_SPEED_NORMAL,
	WATERFALL_SPEED_DOUBLE
} wf_speed_t;

typedef struct {
	wf_mag_t	magnification;
	wf_window_t	window;
	wf_mode_t	mode;
	wf_speed_t	overlap;
	gdouble		ampspan;
	gdouble		reflevel;
	gboolean	direction;
} wf_config_t;

#define	WATERFALL_FFTLEN_1		2048
#define	WATERFALL_FFTLEN_2		(WATERFALL_FFTLEN_1 * 2)
#define	WATERFALL_FFTLEN_4		(WATERFALL_FFTLEN_1 * 4)
#define	WATERFALL_FFTLEN_MAX		WATERFALL_FFTLEN_4

/*
 * These are only defaults now.
 */
#define WATERFALL_DEFAULT_WIDTH		512
#define WATERFALL_DEFAULT_HEIGHT	96

struct _Waterfall 
{
	GtkWidget widget;

	GMutex *mutex;

	gboolean running;
	gboolean paused;

	guint idlefunc;

	GdkGC *pointer1_gc;
	GdkColor pointer1col;
	GdkGC *pointer2_gc;
	GdkColor pointer2col;
	GdkGC *pointer3_gc;
	GdkColor pointer3col;
	GdkGC *grid_gc;
	GdkColor gridcol;
	GdkGC *trace_gc;
	GdkColor tracecol;

	GdkRgbCmap *cmap;

	GdkPixmap *pixmap;

	gboolean fixed;

	/* markers */
	gint pointer;
	gint width;
	gboolean centerline;

	gdouble samplerate;
	gdouble frequency;
	gdouble carrierfreq;
	gdouble bw;

	gdouble centerfreq;
	gdouble startfreq;
	gdouble stopfreq;
	gdouble resolution;

	gboolean lsb;

	gint pixbufsize;
	guchar *pixbuf;
	guchar *pixptr;

	gdouble *inbuf;
	gint inptr;

	gint fftlen;
	gdouble *fft_window;

	fftw_complex *fft_ibuf;
	fftw_complex *fft_obuf;

	fftw_plan fft_plan;

	gdouble *specbuf;
	gdouble *peakbuf;

	GdkCursor *ruler_cursor;
	gboolean ruler_cursor_set;
	gboolean ruler_drag;
	gdouble ruler_ref_f;
	gint ruler_ref_x;

	wf_config_t config;
};

struct _WaterfallClass
{
	GtkWidgetClass parent_class;

	void (* frequency_set) (Waterfall *wf, gint i, gdouble f);
};


GType waterfall_get_type(void);

GtkWidget *waterfall_new(const char *name, void *dummy0, void *dummy1,
			 unsigned int dummy2, unsigned int dummy3);

void waterfall_set_data(Waterfall *wf, gfloat *data, int len);

void waterfall_set_window(Waterfall *wf, wf_window_t type);
void waterfall_set_frequency(Waterfall *wf, gdouble f);
void waterfall_set_center_frequency(Waterfall *wf, gdouble f);
void waterfall_set_carrier_frequency(Waterfall *wf, gdouble f);
void waterfall_set_samplerate(Waterfall *wf, gint samplerate);
void waterfall_set_bandwidth(Waterfall *wf, gdouble bw);
void waterfall_set_centerline(Waterfall *wf, gboolean flag);
void waterfall_set_magnification(Waterfall *wf, wf_mag_t mag);
void waterfall_set_speed(Waterfall *wf, wf_speed_t speed);
void waterfall_set_mode(Waterfall *wf, wf_mode_t mode);
void waterfall_set_pause(Waterfall *wf, gboolean flag);
void waterfall_set_ampspan(Waterfall *wf, gdouble ampspan);
void waterfall_set_reflevel(Waterfall *wf, gdouble reflevel);
void waterfall_set_dir(Waterfall *wf, gboolean dir);
void waterfall_set_fixed(Waterfall *wf, gboolean fixed);
void waterfall_set_lsb(Waterfall *wf, gboolean lsb);

void waterfall_get_config(Waterfall *wf, wf_config_t *config);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __WATERFALL_H__ */
