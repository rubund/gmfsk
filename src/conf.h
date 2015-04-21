/*
 *    conf.h  --  Configuration
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

#ifndef _CONF_H
#define _CONF_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void conf_init(void);
extern void conf_load(void);
extern void conf_clear(void);

extern void conf_set_ptt(void);

extern void conf_set_string(const gchar *key, const gchar *val);
extern void conf_set_bool(const gchar *key, gboolean val);
extern void conf_set_int(const gchar *key, gint val);
extern void conf_set_float(const gchar *key, gdouble val);

extern gchar *conf_get_string(const gchar *key);
extern gboolean conf_get_bool(const gchar *key);
extern gint conf_get_int(const gchar *key);
extern gdouble conf_get_float(const gchar *key);

extern gchar *conf_get_filename(const gchar *key);

#define	conf_get_mycall()	conf_get_string("info/mycall")
#define	conf_get_myname()	conf_get_string("info/myname")
#define	conf_get_myqth()	conf_get_string("info/myqth")
#define	conf_get_myloc()	conf_get_string("info/myloc")
#define	conf_get_myemail()	conf_get_string("info/myemail")

#define	conf_get_timefmt()	conf_get_string("misc/timefmt")
#define	conf_get_datefmt()	conf_get_string("misc/datefmt")

#define conf_get_qsobands()	conf_get_string("misc/bands")

#define	conf_get_logfile()	conf_get_filename("misc/logfile")
#define	conf_get_picrxdir()	conf_get_filename("misc/picrxdir")
#define	conf_get_pictxdir()	conf_get_filename("misc/pictxdir")

#define	conf_get_pttdev()	conf_get_filename("ptt/dev")
#define conf_get_pttinv()	conf_get_bool("ptt/8bit")
#define conf_get_pttmode()	conf_get_int("ptt/mode")

#ifdef __cplusplus
}
#endif

#endif
