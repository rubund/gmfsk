/* 
 * Papertape Widget
 * Copyright (C) 2002,2003 Tomi Manninen <oh2bns@sral.fi>
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

#include "papertape.h"

#include <gtk/gtkgc.h>
#include <gtk/gtkmain.h>
#include <math.h>

#include <stdio.h>
#include <string.h>

#define PRIO G_PRIORITY_LOW

static void papertape_class_init(PapertapeClass *klass);
static void papertape_init(Papertape *tape);
static void papertape_destroy(GtkObject *object);
static gint papertape_expose(GtkWidget *widget, GdkEventExpose *event);
static void papertape_realize(GtkWidget *widget);
static void papertape_unrealize(GtkWidget *widget);
static void papertape_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void papertape_send_configure (Papertape *tape);

static void set_idle_callback(Papertape *tape);
static gint idle_callback(gpointer data);

static GtkWidgetClass *parent_class = NULL;
static PapertapeClass *papertape_class = NULL;


GType papertape_get_type(void)
{
	static GType papertape_type = 0;

	if (!papertape_type)
	{
		static const GTypeInfo papertape_info =
		{
			sizeof(PapertapeClass),
			NULL,
			NULL,
			(GClassInitFunc) papertape_class_init,
			NULL,
			NULL,
			sizeof(Papertape),
			0,
			(GInstanceInitFunc) papertape_init
		};
		papertape_type = g_type_register_static(GTK_TYPE_WIDGET,
							"Papertape",
							&papertape_info, 0);
	}
	return papertape_type;
}

static void papertape_class_init(PapertapeClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	parent_class = gtk_type_class(gtk_widget_get_type());
	papertape_class = class;

	gdk_rgb_init();
//	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
//	gtk_widget_set_default_visual(gdk_rgb_get_visual());

	object_class->destroy = papertape_destroy;

	widget_class->expose_event = papertape_expose;
	widget_class->realize = papertape_realize;
	widget_class->unrealize = papertape_unrealize;
	widget_class->size_allocate = papertape_size_allocate;
}

static void papertape_init(Papertape *tape)
{
	GtkWidget *widget = GTK_WIDGET(tape);

	widget->requisition.width = PAPERTAPE_WIDTH;
	widget->requisition.height = PAPERTAPE_HEIGHT;

	tape->mutex = g_mutex_new();

	tape->idlefunc = 0;

	tape->pixmap = NULL;
	tape->pixbuf = NULL;
	tape->pixptr = 0;
}

static void papertape_realize(GtkWidget *widget)
{
	Papertape *tape;
	GdkWindowAttr attributes;
	gint attributes_mask;
	gint size;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_PAPERTAPE(widget));

	tape = PAPERTAPE(widget);
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
	gdk_window_set_user_data(widget->window, tape);

	widget->style = gtk_style_attach(widget->style, widget->window);
	gtk_style_set_background(widget->style, widget->window,
				 GTK_STATE_NORMAL);

	/* create backing store */
	tape->pixmap = gdk_pixmap_new(widget->window,
				      widget->allocation.width,
				      widget->allocation.height, -1);

	size = widget->allocation.width * widget->allocation.height;

	tape->pixbuf = g_malloc(size * sizeof(guchar));
	memset(tape->pixbuf, 255, size * sizeof(guchar));

	memset(tape->save, 255, sizeof(tape->save));

	papertape_send_configure(PAPERTAPE(widget));
}

static void papertape_unrealize(GtkWidget *widget)
{
	Papertape *tape;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_PAPERTAPE(widget));

	tape = PAPERTAPE(widget);

	if (tape->idlefunc)
		gtk_idle_remove(tape->idlefunc);
	tape->idlefunc = 0;

	if (tape->pixmap)
		gdk_pixmap_unref(tape->pixmap);
	tape->pixmap = NULL;

	g_free(tape->pixbuf);
	tape->pixbuf = NULL;

	if (tape->mutex)
		g_mutex_free(tape->mutex);
	tape->mutex = NULL;

	if (GTK_WIDGET_CLASS(parent_class)->unrealize)
		(*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
}

static void papertape_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	Papertape *tape;
	gint neww, newh, oldw, oldh, w, h, x, y;
	guchar *newbuf, *oldbuf;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(IS_PAPERTAPE(widget));
	g_return_if_fail(allocation != NULL);

	tape = PAPERTAPE(widget);

	g_mutex_lock(tape->mutex);

	oldw = widget->allocation.width;
	oldh = widget->allocation.height;

	neww = 2 * (allocation->width / 2);
	newh = PAPERTAPE_HEIGHT;

//	g_print("old w=%d h=%d, new w=%d h=%d\n", oldw, oldh, neww, newh);

	widget->allocation = *allocation;
	widget->allocation.width = neww;
	widget->allocation.height = newh;

	if (!tape->pixbuf)
		tape->pixbuf = g_malloc(neww * newh * sizeof(guchar));

	if (neww != oldw || newh != oldh) {
		newbuf = g_malloc(neww * newh * sizeof(guchar));
		oldbuf = tape->pixbuf;

		memset(newbuf, 255, neww * newh * sizeof(guchar));

		w = MIN(oldw, neww);
		h = MIN(oldh, newh);

		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++)
				newbuf[y * neww + x] = oldbuf[y * oldw + x];

		g_free(oldbuf);
		tape->pixbuf = newbuf;
	}

	if (GTK_WIDGET_REALIZED(widget)) {
		if (tape->pixmap)
			gdk_pixmap_unref(tape->pixmap);

		tape->pixmap = gdk_pixmap_new(widget->window, neww, newh, -1);

		gdk_window_move_resize (widget->window,
					allocation->x, allocation->y,
					allocation->width, allocation->height);
		papertape_send_configure(PAPERTAPE(widget));
	}

	g_mutex_unlock(tape->mutex);
}

static void papertape_send_configure(Papertape *tape)
{
	GtkWidget *widget;
	GdkEventConfigure event;

	widget = GTK_WIDGET(tape);

	event.type = GDK_CONFIGURE;
	event.window = widget->window;
	event.send_event = TRUE;
	event.x = widget->allocation.x;
	event.y = widget->allocation.y;
	event.width = widget->allocation.width;
	event.height = widget->allocation.height;
  
	gtk_widget_event(widget, (GdkEvent*)&event);
}

/* ---------------------------------------------------------------------- */

GtkWidget *papertape_new(const char *name, void *dummy0, void *dummy1,
			 unsigned int dummy2, unsigned int dummy3)
{
	return GTK_WIDGET(gtk_type_new(papertape_get_type()));
}

static void papertape_destroy(GtkObject *object)
{
	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_PAPERTAPE(object));

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static gint papertape_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(IS_PAPERTAPE(widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	set_idle_callback(PAPERTAPE(widget));
	return FALSE;
}

/* ---------------------------------------------------------------------- */

static void draw(Papertape *tape)
{
	GtkWidget *widget;

	widget = GTK_WIDGET(tape);
	g_return_if_fail(GTK_WIDGET_DRAWABLE(widget));
	g_return_if_fail(tape->pixmap);

	/* draw the papertape */
	gdk_draw_gray_image(tape->pixmap,
			    widget->style->base_gc[widget->state],
			    0, 0,
			    widget->allocation.width,
			    widget->allocation.height,
			    GDK_RGB_DITHER_NORMAL,
			    tape->pixbuf,
			    widget->allocation.width);

	/* draw to screen */
	gdk_draw_pixmap(widget->window,
			widget->style->base_gc[widget->state],
			tape->pixmap, 
			0, 0, 0, 0,
			widget->allocation.width, widget->allocation.height);
}

/* ---------------------------------------------------------------------- */

static void set_idle_callback(Papertape *tape)
{
	g_mutex_lock(tape->mutex);

	if (!tape->idlefunc)
		tape->idlefunc = gtk_idle_add_priority(G_PRIORITY_LOW,
						       idle_callback,
						       tape);

	g_mutex_unlock(tape->mutex);
}

static gint idle_callback(gpointer data)
{
	Papertape *tape;

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(IS_PAPERTAPE(data), FALSE);

	tape = PAPERTAPE(data);

	gdk_threads_enter();

	if (GTK_WIDGET_DRAWABLE(GTK_WIDGET(data)))
		draw(tape);

	gdk_threads_leave();

	g_mutex_lock(tape->mutex);
	PAPERTAPE(data)->idlefunc = 0;
	g_mutex_unlock(tape->mutex);

	return FALSE;  /* don't call this callback again */
}

/* ---------------------------------------------------------------------- */

gint papertape_set_data(Papertape *tape, guchar *data, guint len)
{
	GtkWidget *widget;
	gint i, n, pos, width, height, size;

	g_return_val_if_fail(tape != NULL, -1);
	g_return_val_if_fail(IS_PAPERTAPE(tape), -1);

	g_mutex_lock(tape->mutex);

	widget = GTK_WIDGET(tape);

	width = widget->allocation.width;
	height = widget->allocation.height;
	size = width * height;

	n = MIN(len, height / 2);
	pos = (tape->pixptr % width) + (size - width);

	for (i = 0; i < height / 2; i++) {
		tape->pixbuf[pos] = tape->save[i];
		tape->pixbuf[pos + 1] = tape->save[i];

		pos = (pos - width) % size;
	}

	for (i = 0; i < height / 2; i++) {
		if (i < n) {
			tape->pixbuf[pos] = data[i];
			tape->pixbuf[pos + 1] = data[i];
		} else {
			tape->pixbuf[pos] = 0;
			tape->pixbuf[pos + 1] = 0;
		}

		pos = (pos - width) % size;
	}

	for (i = 0; i < height / 2; i++) {
		if (i < n)
			tape->save[i] = data[i];
		else
			tape->save[i] = 0;
	}

	tape->pixptr = (tape->pixptr + 2) % width;

	g_mutex_unlock(tape->mutex);

	set_idle_callback(tape);

	return tape->pixptr;
}

void papertape_clear(Papertape *tape)
{
	GtkWidget *widget;
	gint size;

	g_return_if_fail(tape != NULL);
	g_return_if_fail(IS_PAPERTAPE(tape));

	widget = GTK_WIDGET(tape);

	size = widget->allocation.width * widget->allocation.height;

	memset(tape->pixbuf, 255, size * sizeof(guchar));

	tape->pixptr = 0;

	set_idle_callback(tape);
}

void papertape_copy_data(Papertape *dst, Papertape *src)
{
	GtkWidget *srcwidget, *dstwidget;
	gint srcsize, dstsize;

	g_return_if_fail(dst != NULL);
	g_return_if_fail(src != NULL);
	g_return_if_fail(IS_PAPERTAPE(dst));
	g_return_if_fail(IS_PAPERTAPE(src));
	g_return_if_fail(dst->pixbuf != NULL);
	g_return_if_fail(src->pixbuf != NULL);

	dstwidget = GTK_WIDGET(dst);
	srcwidget = GTK_WIDGET(src);

	srcsize = srcwidget->allocation.width * srcwidget->allocation.height;
	dstsize = dstwidget->allocation.width * dstwidget->allocation.height;

	g_return_if_fail(dstsize == srcsize);

	memcpy(dst->pixbuf, src->pixbuf, dstsize * sizeof(guchar));

	set_idle_callback(dst);
}

/* ---------------------------------------------------------------------- */
