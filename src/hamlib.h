/*
 *    hamlib.h  --  Hamlib (rig control) interface
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

#ifndef _HAMLIB_H
#define _HAMLIB_H

extern GSList *riglist_get_list(void);
extern gint riglist_get_names(GPtrArray **list);

extern gint riglist_get_id_from_index(gint idx);
extern gint riglist_get_index_from_id(gint id);

extern gint riglist_get_speeds(gint rigid, gint **speeds);
extern gint riglist_get_index_from_speed(gint rigid, gint speed);
extern gint riglist_get_speed_from_index(gint rigid, gint idx);

extern void hamlib_init(void);
extern void hamlib_close(void);
extern gboolean hamlib_active(void);

extern void hamlib_set_ptt(gint ptt);
extern void hamlib_set_qsy(void);

extern void hamlib_set_conf(gboolean, gboolean, gboolean, gint);

#endif

