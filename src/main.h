/*
 *    main.h  --  gMFSK main
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

#ifndef _MAIN_H
#define _MAIN_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "waterfall.h"

#ifdef __cplusplus
extern "C" {
#endif

extern GtkWidget *appwindow;
extern GtkWidget *WFPopupMenu;
extern Waterfall *waterfall;

extern GtkTextTag *rxtag;
extern GtkTextTag *txtag;
extern GtkTextTag *hltag;

extern gchar *SoftString;

extern void errmsg(const gchar *fmt, ...);

extern void statusbar_set_main(const gchar *message);
extern void statusbar_set_mode(gint mode);
extern void statusbar_set_trxstate(gint state);

extern void push_button(const gchar *name);

extern void send_char(gunichar c);
extern void send_string(const gchar *str);

extern void clear_tx_text(void);

extern void textview_insert_pixbuf(gchar *name, GdkPixbuf *pixbuf);

extern void textbuffer_insert_end(GtkTextBuffer *buf, const gchar *str, int len);
extern void textbuffer_delete_end(GtkTextBuffer *buffer, gint count);
extern void textview_scroll_end(GtkTextView *view);

extern GtkWidget *find_menu_item(const gchar *path);

#ifdef __cplusplus
}
#endif

#endif
