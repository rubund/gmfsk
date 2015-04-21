/* 
 * Miniature Oscilloscope Widget
 * Copyright (C) 2001 Tomi Manninen <oh2bns@sral.fi>
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

#ifndef __MINISCOPE_H__
#define __MINISCOPE_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MINISCOPE(obj)		GTK_CHECK_CAST(obj, miniscope_get_type(), Miniscope)
#define MINISCOPE_CLASS(klass)	GTK_CHECK_CLASS_CAST(klass, miniscope_get_type(), MiniscopeClass)
#define IS_MINISCOPE(obj)	GTK_CHECK_TYPE(obj, miniscope_get_type())

typedef struct _Miniscope	Miniscope;
typedef struct _MiniscopeClass	MiniscopeClass;

/*
 * These are only defaults now.
 */
#define MINISCOPE_DEFAULT_WIDTH		64
#define MINISCOPE_DEFAULT_HEIGHT	64

#define	MINISCOPE_MAX_LEN		2048

typedef enum {
        MINISCOPE_MODE_SCOPE,
        MINISCOPE_MODE_PHASE,
        MINISCOPE_MODE_BLANK
} miniscope_mode_t;

struct _Miniscope 
{
	GtkWidget widget;

	GMutex *mutex;
	guint idlefunc;
	miniscope_mode_t mode;

	GdkGC *red_gc;
	GdkColor redcol;
	GdkGC *green_gc;
	GdkColor greencol;
	GdkGC *yellow_gc;
	GdkColor yellowcol;

	GdkPixmap *pixmap;

	gfloat buf[MINISCOPE_MAX_LEN];
	gint len;

	gfloat phase;
	gboolean highlight;
};

struct _MiniscopeClass
{
	GtkWidgetClass parent_class;

	void (* sync_set) (Miniscope *m, gint i, gfloat f);
};


GType miniscope_get_type(void);

GtkWidget *miniscope_new(const char *name, void *dummy0, void *dummy1,
			 unsigned int dummy2, unsigned int dummy3);

void miniscope_set_data(Miniscope *m, gfloat *data, int len, gboolean scale);
void miniscope_set_phase(Miniscope *m, gfloat phase, gboolean highlight);
void miniscope_set_mode(Miniscope *m, miniscope_mode_t mode);
void miniscope_set_highlight(Miniscope *m, gboolean highlight);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __MINISCOPE_H__ */
