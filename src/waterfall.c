/* 
 * Waterfall Spectrum Analyzer Widget
 * Copyright (C) 2001,2002,2003 Tomi Manninen <oh2bns@sral.fi>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "waterfall.h"

#include <gtk/gtkgc.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <math.h>

#include <stdio.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#define fftw_new(type, n)	((type *) fftw_malloc(sizeof(type) * (n)))

#define	RULER_HEIGHT	20

static void waterfall_class_init(WaterfallClass *klass);
static void waterfall_init(Waterfall *wf);
static void waterfall_destroy(GtkObject *object);
static gint waterfall_expose(GtkWidget *widget, GdkEventExpose *event);
static void waterfall_realize(GtkWidget *widget);
static void waterfall_unrealize(GtkWidget *widget);
static void waterfall_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static gint waterfall_button_press(GtkWidget *widget, GdkEventButton *event);
static gint waterfall_button_release(GtkWidget *widget, GdkEventButton *event);
static gint waterfall_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gint waterfall_leave_notify(GtkWidget *widget, GdkEventCrossing *event);
static void waterfall_send_configure (Waterfall *wf);

static void set_idle_callback(Waterfall *wf);
static gint idle_callback(gpointer data);

static void setwindow(gdouble *window, gint len, wf_window_t type);
static void calculate_frequencies(Waterfall *wf);

/* ---------------------------------------------------------------------- */

static GtkWidgetClass *parent_class = NULL;
static WaterfallClass *waterfall_class = NULL;

enum {
  FREQUENCY_SET_SIGNAL,
  LAST_SIGNAL
};

static gint waterfall_signals[LAST_SIGNAL] = { 0 };

/* ---------------------------------------------------------------------- */

GType waterfall_get_type(void)
{
	static GType waterfall_type = 0;

	if (!waterfall_type)
	{
		static const GTypeInfo waterfall_info =
		{
			sizeof(WaterfallClass),
			NULL,
			NULL,
			(GClassInitFunc) waterfall_class_init,
			NULL,
			NULL,
			sizeof(Waterfall),
			0,
			(GInstanceInitFunc) waterfall_init
		};
		waterfall_type = g_type_register_static(GTK_TYPE_WIDGET,
							"Waterfall",
							&waterfall_info, 0);
	}
	return waterfall_type;
}

static void waterfall_class_init(WaterfallClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	parent_class = gtk_type_class(gtk_widget_get_type());
	waterfall_class = class;

	gdk_rgb_init();
//	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
//	gtk_widget_set_default_visual(gdk_rgb_get_visual());

	object_class->destroy = waterfall_destroy;
//	object_class->finalize = waterfall_finalize;

	widget_class->expose_event = waterfall_expose;
	widget_class->realize = waterfall_realize;
	widget_class->unrealize = waterfall_unrealize;
	widget_class->size_allocate = waterfall_size_allocate;
	widget_class->button_press_event = waterfall_button_press;
	widget_class->button_release_event = waterfall_button_release;
	widget_class->motion_notify_event = waterfall_motion_notify;
	widget_class->leave_notify_event = waterfall_leave_notify;

	waterfall_signals[FREQUENCY_SET_SIGNAL] = \
		g_signal_new("frequency_set",
			     G_TYPE_FROM_CLASS(object_class),
			     G_SIGNAL_RUN_FIRST,
			     0,
			     NULL,
			     NULL,
			     g_cclosure_marshal_VOID__FLOAT,
			     GTK_TYPE_NONE, 1, GTK_TYPE_FLOAT);

	class->frequency_set = NULL;
}

static void waterfall_init(Waterfall *wf)
{
	GtkWidget *widget = GTK_WIDGET(wf);
	guint32 colormap[256];
	gint i;

	widget->requisition.width = WATERFALL_DEFAULT_WIDTH;
	widget->requisition.height = WATERFALL_DEFAULT_HEIGHT;

	wf->mutex = g_mutex_new();

	wf->running = TRUE;
	wf->paused = FALSE;

	wf->idlefunc = 0;

	/* initialize the colors */
	gdk_color_parse("magenta", &wf->pointer1col);
	wf->pointer1_gc = NULL;

	gdk_color_parse("blue",    &wf->pointer2col);
	wf->pointer2_gc = NULL;

	gdk_color_parse("red",     &wf->pointer3col);
	wf->pointer3_gc = NULL;

	gdk_color_parse("grey75",  &wf->gridcol);
	wf->grid_gc = NULL;

	gdk_color_parse("green",   &wf->tracecol);
	wf->trace_gc = NULL;

	/* compute cmap */
#if 0
	for (i = 0; i < 256; i++) {
		guchar r, g, b;

		if (i < 128) {
			r = 0;
			b = 256 - 2 * i;
			g = 2 * i;
		} else {
			r = 2 * (i - 128);
			b = 0;
			g = 256 - 2 * (i - 128);
		}
		colormap[i] = (r << 16) + (g << 8) + b;
	}
#else
	for (i = 0; i < 256; i++)
		colormap[i] = 65793 * i;
#endif
	wf->cmap = gdk_rgb_cmap_new(colormap, 256);

	wf->pixmap = NULL;
	wf->pixbufsize = 0;
	wf->pixbuf = NULL;
	wf->pixptr = 0;

	wf->pointer = -1;
	wf->centerline = FALSE;

	wf->fixed = FALSE;

	wf->config.magnification = WATERFALL_MAG_1;
	wf->config.window = WATERFALL_WINDOW_TRIA;
	wf->config.mode = WATERFALL_MODE_NORMAL;
	wf->config.overlap = 1024;
	wf->config.ampspan = 100.0;
	wf->config.reflevel = 0.0;
	wf->config.direction = TRUE;

	wf->fftlen = WATERFALL_FFTLEN_1;
	wf->samplerate = 8000.0;

	wf->centerfreq = 1500.0;
	wf->frequency = 1000.0;
	wf->carrierfreq = 0.0;
	wf->bw = 200.0;

	calculate_frequencies(wf);

	wf->lsb = FALSE;

	wf->ruler_cursor = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
	wf->ruler_cursor_set = FALSE;
	wf->ruler_drag = FALSE;
	wf->ruler_ref_f = 0.0;
	wf->ruler_ref_x = 0;

	wf->fft_plan = fftw_create_plan(wf->fftlen,
					FFTW_FORWARD,
					FFTW_MEASURE | \
					FFTW_OUT_OF_PLACE | \
					FFTW_USE_WISDOM);

	wf->specbuf = g_new(gdouble, WATERFALL_FFTLEN_MAX);
	wf->peakbuf = g_new(gdouble, WATERFALL_FFTLEN_MAX);

	wf->fft_ibuf = fftw_new(fftw_complex, WATERFALL_FFTLEN_MAX);
	wf->fft_obuf = fftw_new(fftw_complex, WATERFALL_FFTLEN_MAX);

	wf->fft_window = g_new(gdouble, WATERFALL_FFTLEN_MAX);

	wf->inbuf = g_new(gdouble, WATERFALL_FFTLEN_MAX);

	for (i = 0; i < WATERFALL_FFTLEN_MAX; i++) {
		wf->specbuf[i] = -1.0;
		wf->peakbuf[i] = -1.0;

		c_re(wf->fft_ibuf[i]) = 0.0;
		c_im(wf->fft_ibuf[i]) = 0.0;

		c_re(wf->fft_obuf[i]) = 0.0;
		c_im(wf->fft_obuf[i]) = 0.0;

		wf->inbuf[i] = 0.0;
	}
	wf->inptr = 0;

	setwindow(wf->fft_window, wf->fftlen, WATERFALL_WINDOW_TRIA);
}

static void alloc_pixbuf(Waterfall *wf, gboolean restart)
{
	gint w, h, oldsize, newsize;
	guchar *oldbuf, *oldptr;

	w = wf->fftlen / 2;
	h = GTK_WIDGET(wf)->allocation.height - RULER_HEIGHT;

	oldsize = wf->pixbufsize;
	newsize = 2 * w * h;

	if (newsize < 0)
		return;

	if (!restart && wf->pixbuf && oldsize == newsize)
		return;

	oldbuf = wf->pixbuf;
	oldptr = wf->pixptr;

	wf->pixbuf = g_new0(guchar, newsize);
	wf->pixptr = wf->pixbuf;

	if (!restart && oldbuf) {
		memcpy(wf->pixbuf, oldbuf, MIN(oldsize, newsize));
		wf->pixptr += oldptr - oldbuf;
	}

	wf->pixbufsize = newsize;
}

static void waterfall_realize(GtkWidget *widget)
{
	Waterfall *wf;
	GdkWindowAttr attributes;
	gint attributes_mask;
	GdkGCValues gc_values;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_WATERFALL(widget));

	wf = WATERFALL(widget);

	g_mutex_lock(wf->mutex);

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(widget);
	attributes.colormap = gtk_widget_get_colormap(widget);
	attributes.event_mask = \
		gtk_widget_get_events(widget) | \
		GDK_EXPOSURE_MASK | \
		GDK_BUTTON_PRESS_MASK | \
		GDK_BUTTON_RELEASE_MASK | \
		GDK_POINTER_MOTION_MASK | \
		GDK_POINTER_MOTION_HINT_MASK | \
		GDK_LEAVE_NOTIFY_MASK;

	attributes_mask = \
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
					&attributes, attributes_mask);
	gdk_window_set_user_data(widget->window, wf);

	widget->style = gtk_style_attach(widget->style, widget->window);
	gtk_style_set_background(widget->style, widget->window,
				 GTK_STATE_NORMAL);

	if (!gdk_color_alloc(widget->style->colormap, &wf->pointer1col))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  wf->pointer1col.red,
			  wf->pointer1col.green,
			  wf->pointer1col.blue);
	gc_values.foreground = wf->pointer1col;
	wf->pointer1_gc = gtk_gc_get(widget->style->depth,
				     widget->style->colormap,
				     &gc_values,
				     GDK_GC_FOREGROUND);

	if (!gdk_color_alloc(widget->style->colormap, &wf->pointer2col))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  wf->pointer2col.red,
			  wf->pointer2col.green,
			  wf->pointer2col.blue);
	gc_values.foreground = wf->pointer2col;
	wf->pointer2_gc = gtk_gc_get(widget->style->depth,
				     widget->style->colormap,
				     &gc_values,
				     GDK_GC_FOREGROUND);

	if (!gdk_color_alloc(widget->style->colormap, &wf->pointer3col))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  wf->pointer3col.red,
			  wf->pointer3col.green,
			  wf->pointer3col.blue);
	gc_values.foreground = wf->pointer3col;
	wf->pointer3_gc = gtk_gc_get(widget->style->depth,
				     widget->style->colormap,
				     &gc_values,
				     GDK_GC_FOREGROUND);

	if (!gdk_color_alloc(widget->style->colormap, &wf->gridcol))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  wf->gridcol.red,
			  wf->gridcol.green,
			  wf->gridcol.blue);
	gc_values.foreground = wf->gridcol;
	wf->grid_gc = gtk_gc_get(widget->style->depth,
				 widget->style->colormap,
				 &gc_values,
				 GDK_GC_FOREGROUND);

	if (!gdk_color_alloc(widget->style->colormap, &wf->tracecol))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  wf->tracecol.red,
			  wf->tracecol.green,
			  wf->tracecol.blue);
	gc_values.foreground = wf->tracecol;
	wf->trace_gc = gtk_gc_get(widget->style->depth,
				  widget->style->colormap,
				  &gc_values,
				  GDK_GC_FOREGROUND);

	if (wf->pixmap)
		gdk_pixmap_unref(wf->pixmap);

	wf->pixmap = gdk_pixmap_new(widget->window,
				    widget->allocation.width,
				    widget->allocation.height, -1);

	alloc_pixbuf(wf, FALSE);

	calculate_frequencies(wf);

	g_mutex_unlock(wf->mutex);

	waterfall_send_configure(WATERFALL(widget));
}

static void waterfall_unrealize(GtkWidget *widget)
{
	Waterfall *wf;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_WATERFALL(widget));

	wf = WATERFALL(widget);

	g_mutex_lock(wf->mutex);

	if (wf->idlefunc)
		gtk_idle_remove(wf->idlefunc);

	if (wf->pointer1_gc)
		gtk_gc_release(wf->pointer1_gc);
	wf->pointer1_gc = NULL;

	if (wf->pointer2_gc)
		gtk_gc_release(wf->pointer2_gc);
	wf->pointer2_gc = NULL;

	if (wf->pixmap)
		gdk_pixmap_unref(wf->pixmap);
	wf->pixmap = NULL;

	if (wf->cmap)
		gdk_rgb_cmap_free(wf->cmap);
	wf->cmap = NULL;

	g_free(wf->pixbuf);
	wf->pixbuf = NULL;
	wf->pixptr = NULL;
	wf->pixbufsize = 0;

	g_free(wf->specbuf);
	g_free(wf->peakbuf);
	wf->specbuf = NULL;
	wf->peakbuf = NULL;

	fftw_free(wf->fft_ibuf);
	fftw_free(wf->fft_obuf);
	wf->fft_ibuf = NULL;
	wf->fft_obuf = NULL;

	g_free(wf->inbuf);
	wf->inbuf = NULL;

	if (wf->fft_plan)
		fftw_destroy_plan(wf->fft_plan);
	wf->fft_plan = NULL;

	if (wf->ruler_cursor)
		gdk_cursor_unref(wf->ruler_cursor);
	wf->ruler_cursor = NULL;

	if (GTK_WIDGET_CLASS(parent_class)->unrealize)
		(*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);

	g_mutex_unlock(wf->mutex);
}

static void waterfall_size_allocate(GtkWidget *widget,
				    GtkAllocation *allocation)
{
	Waterfall *wf;
	gint width, height;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_WATERFALL(widget));
	g_return_if_fail(allocation != NULL);

	wf = WATERFALL(widget);

	g_mutex_lock(wf->mutex);

	width = MIN(allocation->width, wf->fftlen / 2);
	height = allocation->height;


	widget->allocation = *allocation;
	widget->allocation.width = width;
	widget->allocation.height = height;

	wf->inptr = 0;

	alloc_pixbuf(wf, FALSE);

	calculate_frequencies(wf);

	if (GTK_WIDGET_REALIZED(widget)) {
		if (wf->pixmap)
			gdk_pixmap_unref(wf->pixmap);

		wf->pixmap = gdk_pixmap_new(widget->window, width, height, -1);

		gdk_window_move_resize(widget->window,
				       allocation->x, allocation->y,
				       allocation->width, allocation->height);

		waterfall_send_configure(WATERFALL(widget));
	}

	g_mutex_unlock(wf->mutex);
}

static void waterfall_send_configure(Waterfall *wf)
{
	GtkWidget *widget;
	GdkEventConfigure event;

	widget = GTK_WIDGET(wf);

	event.type = GDK_CONFIGURE;
	event.window = widget->window;
	event.send_event = TRUE;
	event.x = widget->allocation.x;
	event.y = widget->allocation.y;
	event.width = widget->allocation.width;
	event.height = widget->allocation.height;
  
	gtk_widget_event(widget, (GdkEvent*)&event);
}

static gint waterfall_button_press(GtkWidget *widget, GdkEventButton *event)
{
	Waterfall *wf = WATERFALL(widget);
	gdouble resolution, span, f;

	if (wf->config.mode == WATERFALL_MODE_SCOPE)
		return FALSE;

	if (event->button != 1)
		return FALSE;

	if (event->y < RULER_HEIGHT) {
		gtk_grab_add(widget);

		wf->ruler_drag = TRUE;
		wf->ruler_ref_x = event->x;
		wf->ruler_ref_f = wf->centerfreq;

		return FALSE;
	}

	if (wf->fixed)
		return FALSE;

	resolution = wf->samplerate / wf->fftlen;
	span = resolution * widget->allocation.width;

	f = wf->centerfreq - span / 2 + event->x * resolution;

	f = MAX(f, wf->bw / 2.0);
	f = MIN(f, wf->samplerate / 2.0 - wf->bw / 2.0);

	wf->frequency = f;

	calculate_frequencies(wf);

	gtk_signal_emit(GTK_OBJECT(wf), 
			waterfall_signals[FREQUENCY_SET_SIGNAL],
			wf->frequency);

	return FALSE;
}

static gint waterfall_button_release(GtkWidget *widget, GdkEventButton *event)
{
	Waterfall *wf = WATERFALL(widget);

	if (wf->ruler_drag) {
		wf->ruler_drag = FALSE;
		gtk_grab_remove(widget);
	}

	return FALSE;
}

static gint waterfall_motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	Waterfall *wf = WATERFALL(widget);
	GdkModifierType mods;
	gint x, y;

	if (wf->config.mode == WATERFALL_MODE_SCOPE)
		return FALSE;

	if (event->is_hint || (event->window != widget->window)) {
		gdk_window_get_pointer(widget->window, &x, &y, &mods);
	} else {
		x = event->x;
		y = event->y;
		mods = event->state;
	}

	if (wf->ruler_drag && (mods & GDK_BUTTON1_MASK)) {
		gdouble res = wf->samplerate / wf->fftlen;

		wf->centerfreq = wf->ruler_ref_f + res * (wf->ruler_ref_x - x);

		calculate_frequencies(wf);
		return FALSE;
	}

	if (y < RULER_HEIGHT) {
		wf->pointer = -1;

		if (!wf->ruler_cursor_set) {
			gdk_window_set_cursor(widget->window, wf->ruler_cursor);
			wf->ruler_cursor_set = TRUE;
		}
	} else {
		if (wf->fixed)
			wf->pointer = -1;
		else
			wf->pointer = x;

		if (wf->ruler_cursor_set) {
			gdk_window_set_cursor(widget->window, NULL);
			wf->ruler_cursor_set = FALSE;
		}
	}

	set_idle_callback(wf);

	return FALSE;
}

static gint waterfall_leave_notify(GtkWidget *widget, GdkEventCrossing *event)
{
	WATERFALL(widget)->pointer = -1;

	set_idle_callback(WATERFALL(widget));

	return FALSE;
}

/* ---------------------------------------------------------------------- */

GtkWidget *waterfall_new(const char *name, void *dummy0, void *dummy1,
			 unsigned int dummy2, unsigned int dummy3)
{
	return GTK_WIDGET(gtk_type_new(waterfall_get_type()));
}

static void waterfall_destroy(GtkObject *object)
{
	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_WATERFALL(object));

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static gint waterfall_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(IS_WATERFALL(widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	set_idle_callback(WATERFALL(widget));

	return FALSE;
}

/* ---------------------------------------------------------------------- */

static inline gint ftox(Waterfall *wf, gdouble f)
{
	return (gint) ((f - wf->startfreq) / wf->resolution + 0.5);
}

static inline gint ftow(Waterfall *wf, gdouble f)
{
	return (gint) (f / wf->resolution + 0.5);
}

static inline gdouble xtof(Waterfall *wf, gint x)
{
	return wf->startfreq + x * wf->resolution;
}

static void draw_markers(Waterfall *wf)
{
	gint x1, x2, x3;
	GdkGC *gc;

	x1 = ftox(wf, wf->frequency - (wf->bw / 2.0));
	x2 = ftox(wf, wf->frequency);
	x3 = ftox(wf, wf->frequency + (wf->bw / 2.0));

	gc = (wf->fixed) ? wf->pointer3_gc : wf->pointer2_gc;

	gdk_draw_line(wf->pixmap, gc,
		      x1, RULER_HEIGHT,
		      x1, GTK_WIDGET(wf)->allocation.height - 1);
	if (wf->centerline) {
		gdk_draw_line(wf->pixmap, gc,
			      x2, RULER_HEIGHT,
			      x2, GTK_WIDGET(wf)->allocation.height - 1);
	}
	gdk_draw_line(wf->pixmap, gc,
		      x3, RULER_HEIGHT,
		      x3, GTK_WIDGET(wf)->allocation.height - 1);

	if (wf->fixed || wf->pointer == -1)
		return;

	x1 = ftox(wf, xtof(wf, wf->pointer) - wf->bw / 2);
	x2 = wf->pointer;
	x3 = ftox(wf, xtof(wf, wf->pointer) + wf->bw / 2);

	gc = wf->pointer1_gc;

	gdk_draw_line(wf->pixmap, gc,
		      x1, RULER_HEIGHT,
		      x1, GTK_WIDGET(wf)->allocation.height - 1);
	if (wf->centerline) {
		gdk_draw_line(wf->pixmap, gc,
			      x2, RULER_HEIGHT,
			      x2, GTK_WIDGET(wf)->allocation.height - 1);
	}
	gdk_draw_line(wf->pixmap, gc,
		      x3, RULER_HEIGHT,
		      x3, GTK_WIDGET(wf)->allocation.height - 1);
}

struct tic {
	gboolean major;
	gdouble freq;
	gchar str[32];
	gint strw;
	gint strh;
	gint strx;
	gint x;

	struct tic *next;
};

static struct tic *build_tics(Waterfall *wf)
{
	struct tic *list, *p;
	gdouble realfreq, f;
	gint i, freq, width;

	list = NULL;

	width = GTK_WIDGET(wf)->allocation.width;
	f = wf->startfreq;

	for (i = 0; i < width; i++) {
		if (wf->lsb)
			realfreq = f - wf->carrierfreq;
		else
			realfreq = f + wf->carrierfreq;

		realfreq = fabs(realfreq);

		freq = 100 * floor(realfreq / 100.0 + 0.5);

		if (freq < realfreq || freq >= realfreq + wf->resolution) {
			f += wf->resolution;
			continue;
		}

		p = g_new0(struct tic, 1);

		p->major = FALSE;
		p->freq = freq;
		p->x = i;

		if ((freq % 500) == 0) {
			GdkFont *font;
			gint hz, khz;

			khz = freq / 1000;
			hz  = freq % 1000;

			if (khz > 9)
				sprintf(p->str, "%d.%03d", khz, hz);
			else
				sprintf(p->str, "%d", freq);

			font = gtk_style_get_font(GTK_WIDGET(wf)->style);

			p->strw = gdk_string_width(font, p->str);
			p->strh = gdk_string_height(font, p->str);
			p->strx = CLAMP(i - p->strw / 2, 0, width - p->strw);

			p->major = TRUE;
		}

		f += wf->resolution;

		p->next = list;
		list = p;
	}

	return list;
}

static void free_tics(struct tic *list)
{
	while (list) {
		struct tic *p = list;
		list = list->next;
		g_free(p);
	}
}

static void draw_waterfall(Waterfall *wf)
{
	GtkWidget *widget;
	struct tic *tics, *list;
	guchar *ptr;

	widget = GTK_WIDGET(wf);

	ptr = wf->pixptr + (gint) (wf->startfreq / wf->resolution);

	/* draw waterfall */
	gdk_draw_indexed_image(wf->pixmap,
			       widget->style->base_gc[widget->state],
			       0, RULER_HEIGHT,
			       widget->allocation.width,
			       widget->allocation.height - RULER_HEIGHT,
			       GDK_RGB_DITHER_NORMAL,
			       ptr,
			       wf->fftlen / 2,
			       wf->cmap);

	/* draw ruler */
	gdk_draw_rectangle(wf->pixmap, widget->style->black_gc, TRUE,
			   0, 0, widget->allocation.width, RULER_HEIGHT);

	list = build_tics(wf);
	tics = list;

	while (tics) {
		if (tics->major) {
			gdk_draw_line(wf->pixmap, wf->grid_gc,
				      tics->x, RULER_HEIGHT - 9,
				      tics->x, RULER_HEIGHT - 1);

			gdk_draw_string(wf->pixmap,
					gtk_style_get_font(widget->style),
					wf->grid_gc,
					tics->strx,
					tics->strh + 2,
					tics->str);
		} else {
			gdk_draw_line(wf->pixmap, wf->grid_gc,
				      tics->x, RULER_HEIGHT - 5,
				      tics->x, RULER_HEIGHT - 1);
		}
		tics = tics->next;
	}

	free_tics(list);

	/* draw markers */
	draw_markers(wf);

	/* draw to screen */
	gdk_draw_pixmap(widget->window,
			widget->style->base_gc[widget->state],
			wf->pixmap, 
			0, 0, 0, 0,
			widget->allocation.width,
			widget->allocation.height);
}

static void draw_spectrum(Waterfall *wf, gboolean peak)
{
	GtkWidget *widget;
	GdkSegment seg[6];
	GdkPoint *pnt;
	gint i, h;
	struct tic *tics, *list;

	widget = GTK_WIDGET(wf);

	/* clear window */
	gdk_draw_rectangle(wf->pixmap, widget->style->black_gc, TRUE,
			   0,
			   0,
			   widget->allocation.width,
			   widget->allocation.height);

	/* draw grid */
	h = widget->allocation.height - RULER_HEIGHT - 1;
	for (i = 0; i < 6; i++) {
		seg[i].x1 = 0;
		seg[i].x2 = widget->allocation.width - 1;
		seg[i].y1 = RULER_HEIGHT + i * h / 5;
		seg[i].y2 = RULER_HEIGHT + i * h / 5;
	}
	gdk_draw_segments(wf->pixmap, wf->grid_gc, seg, 6);

	/* draw ruler */
	list = build_tics(wf);
	tics = list;

	while (tics) {
		if (tics->major) {
			gdk_draw_line(wf->pixmap, wf->grid_gc,
				      tics->x, RULER_HEIGHT - 9,
				      tics->x, widget->allocation.height - 1);

			gdk_draw_string(wf->pixmap,
					gtk_style_get_font(widget->style),
					wf->grid_gc,
					tics->strx,
					tics->strh + 2,
					tics->str);
		} else {
			gdk_draw_line(wf->pixmap, wf->grid_gc,
				      tics->x, RULER_HEIGHT - 5,
				      tics->x, RULER_HEIGHT - 1);
		}
		tics = tics->next;
	}

	free_tics(list);

	/* draw trace */
	pnt = g_new(GdkPoint, widget->allocation.width);

	h = 1 + RULER_HEIGHT - widget->allocation.height;
	for (i = 0; i < widget->allocation.width; i++) {
		gint f = i + (gint) (wf->startfreq / wf->resolution);

		if (peak)
			wf->peakbuf[i] = MAX(wf->peakbuf[f], wf->specbuf[f]);
		else
			wf->peakbuf[i] = wf->specbuf[f];

		pnt[i].x = i;
		pnt[i].y = RULER_HEIGHT + h * wf->peakbuf[i];
	}
	gdk_draw_lines(wf->pixmap, wf->trace_gc, pnt, widget->allocation.width);

	/* draw markers */
	draw_markers(wf);

	/* draw to screen */
	gdk_draw_pixmap(widget->window,
			widget->style->base_gc[widget->state],
			wf->pixmap, 
			0, 0, 0, 0,
			widget->allocation.width,
			widget->allocation.height);

	g_free(pnt);
}

static void draw_scope(Waterfall *wf)
{
	GtkWidget *widget;
	GdkPoint *pnt;
	GdkSegment *seg, *segp;
	gint w, h, i, nseg;

	widget = GTK_WIDGET(wf);

	w = widget->allocation.width;
	h = widget->allocation.height;

	pnt = g_new(GdkPoint, w);
	seg = g_new(GdkSegment, w / 8 + 2);

	/* calculate grid */
	for (segp = seg, nseg = 0, i = 0; i < w; i += w / 8, segp++, nseg++) {
		segp->x1 = i;
		segp->x2 = i;
		segp->y1 = h / 2 - 5;
		segp->y2 = h / 2 + 5;
	}
	segp->x1 = 0;
	segp->x2 = w - 1;
	segp->y1 = h / 2;
	segp->y2 = h / 2;
	segp++;
	nseg++;
	segp->x1 = w / 2;
	segp->x2 = w / 2;
	segp->y1 = 0;
	segp->y2 = h - 1;
	segp++;
	nseg++;

	/* copy trace */
	for (i = 0; i < w; i++) {
		pnt[i].x = i;
		pnt[i].y = wf->inbuf[i] * h / 2 + h / 2;
	}

	/* clear window */
	gdk_draw_rectangle(wf->pixmap,
			   widget->style->black_gc,
			   TRUE, 0, 0, 
			   widget->allocation.width, 
			   widget->allocation.height);

	/* draw grid */
	gdk_draw_segments(wf->pixmap, wf->grid_gc, seg, nseg);

	/* draw trace */
	gdk_draw_lines(wf->pixmap, wf->trace_gc, pnt, w);

	/* draw to screen */
	gdk_draw_pixmap(widget->window,
			widget->style->base_gc[widget->state],
			wf->pixmap, 
			0, 0, 0, 0,
			widget->allocation.width,
			widget->allocation.height);

	g_free(pnt);
	g_free(seg);
}

static void draw(Waterfall *wf)
{
	GtkWidget *widget;

	widget = GTK_WIDGET(wf);

	g_return_if_fail(wf->pixmap);
	g_return_if_fail(wf->pixbuf);
	g_return_if_fail(wf->pixptr);

	switch (wf->config.mode) {
	case WATERFALL_MODE_NORMAL:
		draw_waterfall(wf);
		break;
	case WATERFALL_MODE_SPECTRUM:
		draw_spectrum(wf, FALSE);
		break;
	case WATERFALL_MODE_SPECTRUM_PEAK:
		draw_spectrum(wf, TRUE);
		break;
	case WATERFALL_MODE_SCOPE:
		draw_scope(wf);
		break;
	default:
		g_warning(_("Unknown waterfall mode: %d\n"), wf->config.mode);
		break;
	}
}

/* ---------------------------------------------------------------------- */

static void set_idle_callback(Waterfall *wf)
{
	g_mutex_lock(wf->mutex);

	if (!wf->idlefunc)
		wf->idlefunc = gtk_idle_add_priority(G_PRIORITY_LOW, 
						     idle_callback, wf);

	g_mutex_unlock(wf->mutex);
}

static gint idle_callback(gpointer data)
{
	Waterfall *wf;

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(IS_WATERFALL(data), FALSE);

	wf = WATERFALL(data);

	gdk_threads_enter();

	if (GTK_WIDGET_DRAWABLE(GTK_WIDGET(data)))
		draw(wf);

	gdk_threads_leave();

	g_mutex_lock(wf->mutex);
	wf->idlefunc = 0;
	g_mutex_unlock(wf->mutex);

	return FALSE;  /* don't call this callback again */
}

/* ---------------------------------------------------------------------- */

#define	cabs(z)		(sqrt(c_re(z) * c_re(z) + c_im(z) * c_im(z)))

static void setdata(Waterfall *wf)
{
	gint i, width, size;
	guchar *ptr;

	for (i = 0; i < wf->fftlen; i++) {
		c_re(wf->fft_ibuf[i]) = wf->inbuf[i] * wf->fft_window[i];
		c_im(wf->fft_ibuf[i]) = 0.0;
	}

	fftw_one(wf->fft_plan, wf->fft_ibuf, wf->fft_obuf);

	width = wf->fftlen / 2;
	size = wf->pixbufsize / 2;

	if (wf->config.direction) {
		wf->pixptr += width;

		if (wf->pixptr > wf->pixbuf + size) {
			memcpy(wf->pixbuf, wf->pixbuf + size, size);
			wf->pixptr = wf->pixbuf;
		}

		ptr = wf->pixptr + size - width;
	} else {
		wf->pixptr -= width;

		if (wf->pixptr < wf->pixbuf) {
			memcpy(wf->pixbuf + size, wf->pixbuf, size);
			wf->pixptr = wf->pixbuf + size;
		}

		ptr = wf->pixptr;
	}

	for (i = 0; i < width; i++) {
		fftw_complex z;
		gdouble x;

		z = wf->fft_obuf[i];
		x = 20 * log10(cabs(z) + 1e-10) - wf->config.reflevel;
		x = CLAMP((x / wf->config.ampspan), -1.0, 0.0);

		/* waterfall data */
		*ptr++ = (guchar) (255.0 + 255.0 * x);

		/* spectrum data */
		wf->specbuf[i] = x;
	}
}

void waterfall_set_data(Waterfall *wf, gfloat *data, int len)
{
	GtkWidget *widget;
	gboolean flag;
	gint i;

	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	if (wf->paused == TRUE)
		return;

	g_return_if_fail(wf->fft_plan);

	widget = GTK_WIDGET(wf);

	g_mutex_lock(wf->mutex);

	flag = FALSE;

	while (len-- > 0) {
		wf->inbuf[wf->inptr++] = *data++;

		if (wf->inptr >= wf->fftlen) {
			setdata(wf);

			for (i = 0; i < wf->inptr - wf->config.overlap; i++)
				wf->inbuf[i] = wf->inbuf[i + wf->config.overlap];
			wf->inptr -= wf->config.overlap;

			flag = TRUE;
		}
	}

	g_mutex_unlock(wf->mutex);

	if (flag == TRUE)
		set_idle_callback(wf);
}

/* ---------------------------------------------------------------------- */

static inline double hamming(double x)
{
	return 0.54 - 0.46 * cos(2.0 * M_PI * x);
}

static void setwindow(gdouble *window, gint len, wf_window_t type)
{
	gdouble pwr = 0.0;
	gint i;

	switch (type) {
	case WATERFALL_WINDOW_RECT:
		for (i = 0; i < len; i++)
			window[i] = 1.0;
		break;
	case WATERFALL_WINDOW_TRIA:
		for (i = 0; i < len; i++) {
			if (i < len / 2)
				window[i] = 2.0 * i / len;
			else
				window[i] = 2.0 * (len - i) / len;
		}
		break;
	case WATERFALL_WINDOW_HAMM:
		for (i = 0; i < len; i++)
			window[i] = hamming(i / (len - 1.0));
		break;
	default:
		g_warning(_("Invalid window function: %d\n"), type);
		return;
	}

	for (i = 0; i < len; i++)
		pwr += window[i] * window[i];

	for (i = 0; i < len; i++)
		window[i] /= pwr;
}

void waterfall_set_window(Waterfall *wf, wf_window_t type)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	g_mutex_lock(wf->mutex);
	setwindow(wf->fft_window, wf->fftlen, type);
	g_mutex_unlock(wf->mutex);

	wf->config.window = type;
}

/* ---------------------------------------------------------------------- */

void calculate_frequencies(Waterfall *wf)
{
	gdouble start, stop, span, res;
	gint width;

	width = GTK_WIDGET(wf)->allocation.width;
	res   = wf->samplerate / wf->fftlen;
	span  = res * width;

	start = wf->centerfreq - span / 2;

	if (start > (wf->frequency - wf->bw / 2))
		start = wf->frequency - wf->bw / 2;

	if (start < 0)
		start = 0;

	stop = start + span;

	if (stop < (wf->frequency + wf->bw / 2))
		stop = wf->frequency + wf->bw / 2;

	if (stop > wf->samplerate / 2)
		stop = wf->samplerate / 2;

	wf->centerfreq = stop - span / 2;
	wf->startfreq = stop - span;
	wf->stopfreq = stop;
	wf->resolution = res;
}

void waterfall_set_frequency(Waterfall *wf, gdouble f)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	f = MAX(f, wf->bw / 2.0);
	f = MIN(f, wf->samplerate / 2.0 - wf->bw / 2.0);

	wf->frequency = f;

	calculate_frequencies(wf);

	gtk_signal_emit(GTK_OBJECT(wf),
			waterfall_signals[FREQUENCY_SET_SIGNAL],
			wf->frequency);

	set_idle_callback(wf);
}

void waterfall_set_center_frequency(Waterfall *wf, gdouble f)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->centerfreq = f;

	calculate_frequencies(wf);

	set_idle_callback(wf);
}

void waterfall_set_carrier_frequency(Waterfall *wf, gdouble f)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->carrierfreq = f;

	set_idle_callback(wf);
}

/* ---------------------------------------------------------------------- */

void waterfall_set_samplerate(Waterfall *wf, gint samplerate)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	g_mutex_lock(wf->mutex);

	if (wf->samplerate != samplerate) {
		memset(wf->pixbuf, 0, wf->pixbufsize);
		wf->samplerate = samplerate;
	}

	g_mutex_unlock(wf->mutex);

	set_idle_callback(wf);
}

void waterfall_set_bandwidth(Waterfall *wf, gdouble bw)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	if (wf->bw != bw) {
		wf->bw = bw;
		set_idle_callback(wf);
	}
}

void waterfall_set_centerline(Waterfall *wf, gboolean flag)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->centerline = flag;

	set_idle_callback(wf);
}

void waterfall_set_magnification(Waterfall *wf, wf_mag_t mag)
{
	gint i;

	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	g_mutex_lock(wf->mutex);

	switch (mag) {
	case WATERFALL_MAG_1:
		if (wf->fftlen == WATERFALL_FFTLEN_1) {
			g_mutex_unlock(wf->mutex);
			return;
		}
		wf->fftlen = WATERFALL_FFTLEN_1;
		break;
	case WATERFALL_MAG_2:
		if (wf->fftlen == WATERFALL_FFTLEN_2) {
			g_mutex_unlock(wf->mutex);
			return;
		}
		wf->fftlen = WATERFALL_FFTLEN_2;
		break;
	case WATERFALL_MAG_4:
		if (wf->fftlen == WATERFALL_FFTLEN_4) {
			g_mutex_unlock(wf->mutex);
			return;
		}
		wf->fftlen = WATERFALL_FFTLEN_4;
		break;
	default:
		g_warning(_("Unsupported magnification: %d\n"), mag);
		g_mutex_unlock(wf->mutex);
		return;
	}

	wf->config.magnification = mag;

	alloc_pixbuf(wf, TRUE);

	if (wf->fft_plan)
		fftw_destroy_plan(wf->fft_plan);

	wf->fft_plan = fftw_create_plan(wf->fftlen,
					FFTW_FORWARD,
					FFTW_ESTIMATE | \
					FFTW_OUT_OF_PLACE | \
					FFTW_USE_WISDOM);

	setwindow(wf->fft_window, wf->fftlen, WATERFALL_WINDOW_TRIA);

	for (i = 0; i < WATERFALL_FFTLEN_MAX; i++) {
		c_re(wf->fft_ibuf[i]) = 0.0;
		c_im(wf->fft_ibuf[i]) = 0.0;

		c_re(wf->fft_obuf[i]) = 0.0;
		c_im(wf->fft_obuf[i]) = 0.0;

		wf->specbuf[i] = -1.0;
		wf->peakbuf[i] = -1.0;

		wf->inbuf[i] = 0.0;
	}
	wf->inptr = 0;

	calculate_frequencies(wf);

	g_mutex_unlock(wf->mutex);
}

void waterfall_set_speed(Waterfall *wf, wf_speed_t speed)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	switch (speed) {
	case WATERFALL_SPEED_HALF:
		wf->config.overlap = 2048;
		break;
	case WATERFALL_SPEED_NORMAL:
		wf->config.overlap = 1024;
		break;
	case WATERFALL_SPEED_DOUBLE:
		wf->config.overlap = 512;
		break;
	}
}

void waterfall_set_mode(Waterfall *wf, wf_mode_t mode)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->config.mode = mode;

	set_idle_callback(wf);
}

void waterfall_set_ampspan(Waterfall *wf, gdouble ampspan)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->config.ampspan = ampspan;

	set_idle_callback(wf);
}

void waterfall_set_reflevel(Waterfall *wf, gdouble reflevel)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->config.reflevel = reflevel;

	set_idle_callback(wf);
}

void waterfall_set_dir(Waterfall *wf, gboolean dir)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->config.direction = dir;
}

void waterfall_set_pause(Waterfall *wf, gboolean flag)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->paused = flag;
}

void waterfall_set_fixed(Waterfall *wf, gboolean flag)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->fixed = flag;
}

void waterfall_set_lsb(Waterfall *wf, gboolean flag)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));

	wf->lsb = flag;

	set_idle_callback(wf);
}

/* ---------------------------------------------------------------------- */

void waterfall_get_config(Waterfall *wf, wf_config_t *config)
{
	g_return_if_fail(wf != NULL);
	g_return_if_fail(IS_WATERFALL(wf));
	g_return_if_fail(config != NULL);

	*config = wf->config;
}

/* ---------------------------------------------------------------------- */
