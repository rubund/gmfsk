/* 
 * Papertape Widget
 * Copyright (C) 2002 Tomi Manninen <oh2bns@sral.fi>
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

#ifndef __PAPERTAPE_H__
#define __PAPERTAPE_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PAPERTAPE(obj)		GTK_CHECK_CAST(obj, papertape_get_type(), Papertape)
#define PAPERTAPE_CLASS(klass)	GTK_CHECK_CLASS_CAST(klass, papertape_get_type(), PapertapeClass)
#define IS_PAPERTAPE(obj)	GTK_CHECK_TYPE(obj, papertape_get_type())

typedef struct _Papertape	Papertape;
typedef struct _PapertapeClass	PapertapeClass;

#define PAPERTAPE_WIDTH		512	/* must be even */
#define PAPERTAPE_HEIGHT	60

struct _Papertape 
{
	GtkWidget widget;

	GMutex *mutex;

	guint idlefunc;

	GdkPixmap *pixmap;

	guchar *pixbuf;
	gint pixptr;

	guchar save[PAPERTAPE_HEIGHT / 2];
};

struct _PapertapeClass
{
	GtkWidgetClass parent_class;
};


GType papertape_get_type(void);

GtkWidget *papertape_new(const char *name,
			 void *dummy0,
			 void *dummy1,
			 unsigned int dummy2,
			 unsigned int dummy3);

extern gint papertape_set_data(Papertape *tape, guchar *data, guint len);
extern void papertape_clear(Papertape *tape);
extern void papertape_copy_data(Papertape *dst, Papertape *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PAPERTAPE_H__ */
