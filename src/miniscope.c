/* 
 * Miniature Oscilloscope/Phasescope Widget
 * Copyright (C) 2001,2002,2003 Tomi Manninen <oh2bns@sral.fi>
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

#include <gtk/gtkgc.h>
#include <gtk/gtkmain.h>
#include <string.h>
#include <math.h>

#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#include "miniscope.h"

static void miniscope_class_init(MiniscopeClass *klass);
static void miniscope_init(Miniscope *m);
static void miniscope_destroy(GtkObject *object);
static gint miniscope_expose(GtkWidget *widget, GdkEventExpose *event);
static void miniscope_realize(GtkWidget *widget);
static void miniscope_unrealize(GtkWidget *widget);
static void miniscope_size_allocate(GtkWidget *widget,
				    GtkAllocation *allocation);

static void set_idle_callback(Miniscope *m);
static gint idle_callback(gpointer data);

/* ---------------------------------------------------------------------- */

static GtkWidgetClass *parent_class = NULL;
static MiniscopeClass *miniscope_class = NULL;

/* ---------------------------------------------------------------------- */

GType miniscope_get_type(void)
{
	static GType miniscope_type = 0;

	if (!miniscope_type)
	{
		static const GTypeInfo miniscope_info =
		{
			sizeof(MiniscopeClass),
			NULL,
			NULL,
			(GClassInitFunc) miniscope_class_init,
			NULL,
			NULL,
			sizeof(Miniscope),
			0,
			(GInstanceInitFunc) miniscope_init
		};
		miniscope_type = g_type_register_static(GTK_TYPE_WIDGET,
							"Miniscope",
							&miniscope_info, 0);
	}
	return miniscope_type;
}

static void miniscope_class_init(MiniscopeClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	parent_class = gtk_type_class(gtk_widget_get_type());
	miniscope_class = class;

	gdk_rgb_init();
//	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
//	gtk_widget_set_default_visual(gdk_rgb_get_visual());

	object_class->destroy = miniscope_destroy;

	widget_class->expose_event = miniscope_expose;
	widget_class->realize = miniscope_realize;
	widget_class->unrealize = miniscope_unrealize;
	widget_class->size_allocate = miniscope_size_allocate;
}

static void miniscope_init(Miniscope *m)
{
	GtkWidget *widget = GTK_WIDGET(m);

	widget->requisition.width = MINISCOPE_DEFAULT_WIDTH;
	widget->requisition.height = MINISCOPE_DEFAULT_HEIGHT;

	m->mutex = g_mutex_new();
	m->idlefunc = 0;

	m->mode = MINISCOPE_MODE_SCOPE;

	/* initialize the colors */
	m->redcol.red   = 0xFFFF;
	m->redcol.green = 0;
	m->redcol.blue  = 0;
	m->red_gc = NULL;

	m->greencol.red   = 0;
	m->greencol.green = 0xFFFF;
	m->greencol.blue  = 0;
	m->green_gc = NULL;

	m->yellowcol.red   = 0xFFFF;
	m->yellowcol.green = 0xFFFF;
	m->yellowcol.blue  = 0;
	m->yellow_gc = NULL;

	m->pixmap = NULL;
	m->len = 0;

	m->phase = 0;
	m->highlight = FALSE;
}

static void miniscope_realize(GtkWidget *widget)
{
	Miniscope *m;
	GdkWindowAttr attributes;
	gint attributes_mask;
	GdkGCValues gc_values;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_MINISCOPE(widget));

	m = MINISCOPE(widget);
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
		gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;

	attributes_mask = \
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
					&attributes, attributes_mask);
	gdk_window_set_user_data(widget->window, m);

	widget->style = gtk_style_attach(widget->style, widget->window);
	gtk_style_set_background(widget->style, widget->window,
				 GTK_STATE_NORMAL);

	/* allocate red color GC */
	if (!gdk_color_alloc(widget->style->colormap, &m->redcol))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  m->redcol.red,
			  m->redcol.green,
			  m->redcol.blue);
	gc_values.foreground = m->redcol;
	m->red_gc = gtk_gc_get(widget->style->depth,
			       widget->style->colormap,
			       &gc_values,
			       GDK_GC_FOREGROUND);

	/* allocate green color GC */
	if (!gdk_color_alloc(widget->style->colormap, &m->greencol))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  m->greencol.red,
			  m->greencol.green,
			  m->greencol.blue);
	gc_values.foreground = m->greencol;
	m->green_gc = gtk_gc_get(widget->style->depth,
				 widget->style->colormap,
				 &gc_values,
				 GDK_GC_FOREGROUND);

	/* allocate yellow color GC */
	if (!gdk_color_alloc(widget->style->colormap, &m->yellowcol))
		g_warning(_("unable to allocate color: ( %d %d %d )"),
			  m->yellowcol.red,
			  m->yellowcol.green,
			  m->yellowcol.blue);
	gc_values.foreground = m->yellowcol;
	m->yellow_gc = gtk_gc_get(widget->style->depth,
				  widget->style->colormap,
				  &gc_values,
				  GDK_GC_FOREGROUND);

	/* allocate pixmap */
	m->pixmap = gdk_pixmap_new(widget->window,
				   widget->allocation.width,
				   widget->allocation.height, -1);
}

static void miniscope_unrealize(GtkWidget *widget)
{
	Miniscope *m;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_MINISCOPE(widget));

	m = MINISCOPE(widget);

	m->len = 0;

	if (m->idlefunc)
		gtk_idle_remove(m->idlefunc);
	m->idlefunc = 0;

	if (m->red_gc)
		gtk_gc_release(m->red_gc);
	m->red_gc = NULL;

	if (m->green_gc)
		gtk_gc_release(m->green_gc);
	m->green_gc = NULL;

	if (m->yellow_gc)
		gtk_gc_release(m->yellow_gc);
	m->yellow_gc = NULL;

	if (m->pixmap)
		gdk_pixmap_unref(m->pixmap);
	m->pixmap = NULL;

	if (GTK_WIDGET_CLASS(parent_class)->unrealize)
		(*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
}

static void miniscope_size_allocate(GtkWidget *widget,
				    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_MINISCOPE(widget));
	g_return_if_fail(allocation != NULL);

	widget->allocation = *allocation;
	widget->allocation.width = MIN(allocation->width, MINISCOPE_MAX_LEN);

	if (GTK_WIDGET_REALIZED(widget)) {
		Miniscope *m = MINISCOPE(widget);

		if (m->pixmap)
			gdk_pixmap_unref(m->pixmap);

		m->pixmap = gdk_pixmap_new(widget->window,
					   allocation->width,
					   allocation->height, -1);

		gdk_window_move_resize(widget->window,
				       allocation->x, allocation->y,
				       allocation->width, allocation->height);
	}
}

/* ---------------------------------------------------------------------- */

GtkWidget *miniscope_new(const char *name, void *dummy0, void *dummy1,
			 unsigned int dummy2, unsigned int dummy3)
{
	return GTK_WIDGET(g_object_new(miniscope_get_type(), NULL));
}

static void miniscope_destroy(GtkObject *object)
{
	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_MINISCOPE(object));

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static gint miniscope_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(IS_MINISCOPE(widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	set_idle_callback(MINISCOPE(widget));

	return FALSE;
}

/* ---------------------------------------------------------------------- */

static void draw_scope(Miniscope *m)
{
	GtkWidget *widget;
	gint w, h, npts, i;

	widget = GTK_WIDGET(m);

	w = widget->allocation.width;
	h = widget->allocation.height;

	gdk_draw_rectangle(m->pixmap,
			   widget->style->black_gc, TRUE,
			   0, 0, w, h);

	npts = MIN(w, m->len);

	if (npts == 1) {
		GdkPoint pts[2];

		pts[0].x = 0;
		pts[0].y = (1.0 - m->buf[0]) * (h - 1);
		pts[1].x = w;
		pts[1].y = (1.0 - m->buf[0]) * (h - 1);

		if (m->highlight)
			gdk_draw_lines(m->pixmap, m->yellow_gc, pts, 2);
		else
			gdk_draw_lines(m->pixmap, m->green_gc, pts, 2);
	}

	if (npts > 1) {
		GdkPoint *pts;

		/* g_malloc can't fail */
		pts = g_malloc(npts * sizeof(GdkPoint));

		for (i = 0; i < npts; i++) {
			pts[i].x = i * w / (npts - 1);
			pts[i].y = (1.0 - m->buf[i * m->len / npts]) * (h - 1);
		}
		if (m->highlight)
			gdk_draw_lines(m->pixmap, m->yellow_gc, pts, npts);
		else
			gdk_draw_lines(m->pixmap, m->green_gc, pts, npts);

		g_free(pts);
	}

	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			m->pixmap,
			0, 0,
			0, 0,
			w, h);
}

static void draw_phase(Miniscope *m)
{
	GtkWidget *widget;
	GdkSegment seg;
	gint x, y, w, h, d;

	widget = GTK_WIDGET(m);

	w = widget->allocation.width;
	h = widget->allocation.height;

	gdk_draw_rectangle(m->pixmap,
			   widget->style->black_gc, TRUE,
			   0, 0, w, h);

	d = 0.9 * MIN(w, h);
	x = w / 2 - d / 2;
	y = h / 2 - d / 2;

	gdk_draw_arc(m->pixmap,
		     m->green_gc, FALSE,
		     x, y, d, d, 0, 360 * 64);

	seg.x1 = w / 2;
	seg.y1 = h / 2;
	seg.x2 = seg.x1 + 0.4 * d * cos(m->phase - M_PI_2);
	seg.y2 = seg.y1 + 0.4 * d * sin(m->phase - M_PI_2);

	if (m->highlight)
		gdk_draw_segments(m->pixmap, m->yellow_gc, &seg, 1);
	else
		gdk_draw_segments(m->pixmap, m->green_gc, &seg, 1);

	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			m->pixmap,
			0, 0,
			0, 0,
			w, h);
}

static void draw_blank(Miniscope *m)
{
	GtkWidget *widget;
	gint w, h;

	widget = GTK_WIDGET(m);

	w = widget->allocation.width;
	h = widget->allocation.height;

	gdk_draw_rectangle(widget->window,
			   widget->style->black_gc, TRUE,
			   0, 0, w, h);
}

static void draw(Miniscope *m)
{
	g_return_if_fail(m->pixmap);

	switch (m->mode) {
	case MINISCOPE_MODE_SCOPE:
		draw_scope(m);
		break;
	case MINISCOPE_MODE_PHASE:
		draw_phase(m);
		break;
	case MINISCOPE_MODE_BLANK:
		draw_blank(m);
		break;
	default:
		g_warning(_("miniscope: invalid mode %d\n"), m->mode);
		break;
	}
}

/* ---------------------------------------------------------------------- */

static void set_idle_callback(Miniscope *m)
{
	g_mutex_lock(m->mutex);

	if (!m->idlefunc)
		m->idlefunc = gtk_idle_add_priority(G_PRIORITY_LOW,
						    idle_callback, m);

	g_mutex_unlock(m->mutex);
}

static gint idle_callback(gpointer data)
{
	Miniscope *m;

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(IS_MINISCOPE(data), FALSE);

	m = MINISCOPE(data);

	g_mutex_lock(m->mutex);
	m->idlefunc = 0;
	g_mutex_unlock(m->mutex);

	gdk_threads_enter();
	if (GTK_WIDGET_DRAWABLE(GTK_WIDGET(m)))
		draw(m);
	gdk_threads_leave();

	return FALSE;  /* don't call this callback again */
}

/* ---------------------------------------------------------------------- */

void miniscope_set_data(Miniscope *m, gfloat *data, int len, gboolean scale)
{
	gint i;

	g_return_if_fail(m != NULL);
	g_return_if_fail(IS_MINISCOPE(m));

	if (data == NULL || len == 0)
		return;

	g_mutex_lock(m->mutex);

	m->len = MIN(len, MINISCOPE_MAX_LEN);

	memcpy(m->buf, data, m->len * sizeof(gfloat));

	if (scale) {
		gfloat max = 1E-6;
		gfloat min = 1E6;

		for (i = 0; i < m->len; i++) {
			max = MAX(max, m->buf[i]);
			min = MIN(min, m->buf[i]);
		}

		for (i = 0; i < m->len; i++)
			m->buf[i] = (m->buf[i] - min) / (max - min);
	} else {
		for (i = 0; i < m->len; i++)
			m->buf[i] = CLAMP(m->buf[i], 0.0, 1.0);
	}

	g_mutex_unlock(m->mutex);

	set_idle_callback(m);
}

void miniscope_set_phase(Miniscope *m, gfloat phase, gboolean highlight)
{
	g_return_if_fail(m != NULL);
	g_return_if_fail(IS_MINISCOPE(m));

	m->phase = phase;
	m->highlight = highlight;

	set_idle_callback(m);
}

void miniscope_set_highlight(Miniscope *m, gboolean highlight)
{
	g_return_if_fail(m != NULL);
	g_return_if_fail(IS_MINISCOPE(m));

	m->highlight = highlight;

	set_idle_callback(m);
}

void miniscope_set_mode(Miniscope *m, miniscope_mode_t mode)
{
	g_return_if_fail(m != NULL);
	g_return_if_fail(IS_MINISCOPE(m));

	m->mode = mode;

	set_idle_callback(m);
}

/* ---------------------------------------------------------------------- */

