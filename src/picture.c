/*
 *    picture.c  --  MFSK pictures support
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

#include <ctype.h>

#include "main.h"
#include "picture.h"
#include "conf.h"
#include "trx.h"
#include "qsodata.h"

#include "interface.h"
#include "support.h"

/* ---------------------------------------------------------------------- */

gboolean picture_check_header(gchar *str, gint *w, gint *h, gboolean *color)
{
	gchar *p;

	*w = 0;
	*h = 0;
	*color = FALSE;

	if ((p = strstr(str, "Pic:")) == NULL)
		return FALSE;

	p += 4;

	while (isdigit(*p))
		*w = (*w * 10) + (*p++ - '0');

	if (*p++ != 'x')
		return FALSE;

	while (isdigit(*p))
		*h = (*h * 10) + (*p++ - '0');

	if (*p == 'C') {
		*color = TRUE;
		p++;
	}

	if (*p != ';')
		return FALSE;

	if (*w == 0 || *h == 0 || *w > 4095 || *h > 4095)
		return FALSE;

	return TRUE;
}

/* ---------------------------------------------------------------------- */

typedef	struct _Picture	Picture;

struct _Picture {
	gboolean active;

	GtkWidget *window;

	GtkWidget *image;
	GdkPixbuf *pixbuf;

	gint width;
	gint height;
	gint rowstride;

	gboolean color;

	gint x;
	gint y;
	gint rgb;
};	

static Picture *picture = NULL;

/* ---------------------------------------------------------------------- */

static gboolean delete_callback(GtkWidget *widget,
				GdkEvent *event,
				gpointer data)
{
	Picture *p = (Picture *) data;

	p->active = FALSE;
	g_object_unref(G_OBJECT(p->pixbuf));

	return FALSE;
}

void on_pictureclosebutton_clicked(GtkButton *button, gpointer data)
{
	Picture *p = (Picture *) data;
	GtkWidget *w;

	p->active = FALSE;
	g_object_unref(G_OBJECT(p->pixbuf));

	w = gtk_widget_get_toplevel(GTK_WIDGET(button));
	gtk_widget_destroy(w);
}

void on_picturesavebutton_clicked(GtkButton *button, gpointer data)
{
	Picture *pic = (Picture *) data;
	GtkWidget *widget;
	GtkFileSelection *f;
	GError *error = NULL;
	gboolean ret;
	gint resp;
	gchar *dir, *file;
	struct tm *tm;
	time_t t;

	widget = create_rxpicfileselection();
	f = GTK_FILE_SELECTION(widget);

	time(&t);
	tm = gmtime(&t);

	dir = conf_get_picrxdir();
	file = g_strdup_printf("%s%cPic_%s_%02d%02d%02d_%02d%02d%02d.png",
			       dir,
			       G_DIR_SEPARATOR,
			       qsodata_get_call(),
			       tm->tm_year % 100,
			       tm->tm_mon + 1,
			       tm->tm_mday,
			       tm->tm_hour,
			       tm->tm_min,
			       tm->tm_sec);
	gtk_file_selection_set_filename(f, file);
	g_free(dir);
	g_free(file);

	resp = gtk_dialog_run(GTK_DIALOG(widget));

	if (resp != GTK_RESPONSE_OK) {
		gtk_widget_destroy(widget);
		return;
	}

	file = g_strdup(gtk_file_selection_get_filename(f));
	gtk_widget_destroy(widget);

	ret = gdk_pixbuf_save(pic->pixbuf, file, "png", &error, NULL);

	if (ret == FALSE && error != NULL) {
		errmsg(_("Error saving picture: %s: %s"), file, error->message);
		g_error_free(error);
	}

	g_free(file);
}

/* ---------------------------------------------------------------------- */

void picture_start(gint w, gint h, gboolean color)
{
	GtkWidget *widget;

	picture_stop();

	picture = g_new0(Picture, 1);

	picture->width = w;
	picture->height = h;
	picture->color = color;

	picture->x = 0;
	picture->y = 0;
	picture->rgb = 0;

	picture->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, w, h);
	picture->rowstride = gdk_pixbuf_get_rowstride(picture->pixbuf);
	gdk_pixbuf_fill(picture->pixbuf, 0xaaaaaaff);

	if (TRUE) {
		picture->window = create_picwindow();

		picture->image = gtk_image_new_from_pixbuf(picture->pixbuf);

		widget = lookup_widget(picture->window, "pictureframe");
		gtk_container_add(GTK_CONTAINER(widget), picture->image);

		gtk_widget_show_all(picture->window);

		g_signal_connect((gpointer) picture->window,
				 "delete_event",
				 G_CALLBACK(delete_callback),
				 (gpointer) picture);

		widget = lookup_widget(picture->window, "picturesavebutton");

		g_signal_connect((gpointer) widget,
				 "clicked",
				 G_CALLBACK(on_picturesavebutton_clicked),
				 (gpointer) picture);

		widget = lookup_widget(picture->window, "pictureclosebutton");

		g_signal_connect((gpointer) widget,
				 "clicked",
				 G_CALLBACK(on_pictureclosebutton_clicked),
				 (gpointer) picture);
	} else {
		textview_insert_pixbuf("rxtext", picture->pixbuf);
	}

	picture->active = TRUE;
}

void picture_stop(void)
{
	if (picture) {
		picture->active = FALSE;
		picture = NULL;
	}
}

void picture_write(guchar data)
{
	guchar *ptr;

	if (picture == NULL || picture->active == FALSE)
		return;

	ptr = gdk_pixbuf_get_pixels(picture->pixbuf);

	if (picture->x >= picture->width) {
		g_warning(_("picture write: x >= width (%d >= %d)"),
			  picture->x,
			  picture->width);
		return;
	}
	if (picture->y >= picture->height) {
		g_warning(_("picture write: y >= height (%d >= %d)"),
			  picture->y,
			  picture->height);
		return;
	}

	ptr += picture->y * picture->rowstride + picture->x * 3;

	if (picture->color) {
		ptr[picture->rgb] = data;
	} else {
		ptr[0] = data;
		ptr[1] = data;
		ptr[2] = data;
	}

	if (++picture->x == picture->width) {
		picture->x = 0;

		if (picture->color && ++picture->rgb < 3)
			return;

		picture->rgb = 0;
		picture->y++;
	}

	if (picture->image)
		gtk_widget_queue_draw(picture->image);
}

/* ---------------------------------------------------------------------- */

void picture_send(gchar *filename, gboolean color)
{
	GError *error = NULL;
	GdkPixbuf *pixbuf;
	Picbuf *picbuf;
	gchar *file;

	if (!filename || !strcmp(filename, "?")) {
		GtkFileSelection *f;
		GtkWidget *w;
		gchar *dir;
		gint res;

		w = create_txpicfileselection();
		f = GTK_FILE_SELECTION(w);

		dir = conf_get_pictxdir();
		file = g_strdup_printf("%s%c", dir, G_DIR_SEPARATOR);

		gtk_file_selection_set_filename(f, file);

		g_free(dir);
		g_free(file);

		res = gtk_dialog_run(GTK_DIALOG(w));

		switch (res) {
		case GTK_RESPONSE_OK:
			file = g_strdup(gtk_file_selection_get_filename(f));
			gtk_widget_destroy(w);
			break;
		default:
			gtk_widget_destroy(w);
			return;
		}
	} else
		file = g_strdup(filename);

	pixbuf = gdk_pixbuf_new_from_file(file, &error);
	g_free(file);

	if (!pixbuf) {
		if (error) {
			errmsg(_("send_picture: %s"), error->message);
			g_error_free(error);
		}
		return;
	}

	picbuf = picbuf_new_from_pixbuf(pixbuf, color);
	g_object_unref(G_OBJECT(pixbuf));

	if (!picbuf) {
		errmsg(_("send_picture: pixbuf conversion failed!"));
		return;
	}

	pixbuf = create_pixbuf("gmfsk/gnome-screenshot.png");

	if (!pixbuf) {
		errmsg(_("send_picture: couldn't load picture icon!"));
		picbuf_free(picbuf);
		return;
	}

	textview_insert_pixbuf("txtext", pixbuf);
	g_object_unref(G_OBJECT(pixbuf));

	trx_put_tx_picture(picbuf);
}

/* ---------------------------------------------------------------------- */

struct _Picbuf {
	gboolean color;

	gint width;
	gint height;

	gint datalen;
	guchar *data;
	gint dataptr;
};

Picbuf *picbuf_new_from_pixbuf(GdkPixbuf *pixbuf, gboolean color)
{
	Picbuf *picbuf;
	guchar *pixels, *src, *dst;
	gint channels, rowstride, x, y;

	g_return_val_if_fail(pixbuf != NULL, NULL);
	g_return_val_if_fail(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB, NULL);
	g_return_val_if_fail(gdk_pixbuf_get_bits_per_sample(pixbuf) == 8, NULL);

	picbuf = g_new0(Picbuf, 1);

	picbuf->color = color;
	picbuf->width = gdk_pixbuf_get_width(pixbuf);
	picbuf->height = gdk_pixbuf_get_height(pixbuf);
	picbuf->datalen = picbuf->width * picbuf->height * (color ? 3 : 1);
	picbuf->data = g_malloc(picbuf->datalen);
	picbuf->dataptr = 0;

	channels = gdk_pixbuf_get_n_channels(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);

	pixels = gdk_pixbuf_get_pixels(pixbuf);

	for (y = 0; y < picbuf->height; y++) {
		src = pixels + y * rowstride;
		dst = picbuf->data + y * picbuf->width * (color ? 3 : 1);

		for (x = 0; x < picbuf->width; x++) {
			if (color) {
				dst[0 * picbuf->width] = src[0];
				dst[1 * picbuf->width] = src[1];
				dst[2 * picbuf->width] = src[2];
			} else {
				dst[0] = (src[0] + src[1] + src[2]) / 3;
			}

			src += channels;
			dst += 1;
		}
	}

	return picbuf;
}

gboolean picbuf_get_color(Picbuf *picbuf)
{
	return picbuf->color;
}

gint picbuf_get_width(Picbuf *picbuf)
{
	return picbuf->width;
}

gint picbuf_get_height(Picbuf *picbuf)
{
	return picbuf->height;
}

gchar *picbuf_make_header(Picbuf *picbuf)
{
	return g_strdup_printf("Pic:%dx%d%s;",
				picbuf->width,
				picbuf->height,
				picbuf->color ? "C" : "");
}

gboolean picbuf_get_data(Picbuf *picbuf, guchar *data, gint *len)
{
	gint n = 0;

	while ((n < *len) && (picbuf->dataptr < picbuf->datalen))
		data[n++] = picbuf->data[picbuf->dataptr++];

	*len = n;

	return (picbuf->dataptr == picbuf->datalen) ? FALSE : TRUE;
}

gdouble picbuf_get_percentage(Picbuf *picbuf)
{
	return 100.0 * picbuf->dataptr / picbuf->datalen;
}

void picbuf_free(Picbuf *picbuf)
{
	g_free(picbuf->data);
	g_free(picbuf);
}

/* ---------------------------------------------------------------------- */
