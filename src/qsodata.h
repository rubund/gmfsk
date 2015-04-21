/*
 *    qsodata.h  --  QSO data area functions
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

#ifndef	_QSODATA_H
#define	_QSODATA_H

typedef enum {
	QSODATA_BAND_COMBO,
	QSODATA_BAND_ENTRY
} band_mode_t;

extern void qsodata_init(void);

extern void qsodata_clear(void);
extern void qsodata_log(void);

extern void qsodata_set_starttime(gboolean force);

extern G_CONST_RETURN gchar *qsodata_get_call(void);
extern G_CONST_RETURN gchar *qsodata_get_band(void);
extern G_CONST_RETURN gchar *qsodata_get_txrst(void);
extern G_CONST_RETURN gchar *qsodata_get_rxrst(void);
extern G_CONST_RETURN gchar *qsodata_get_name(void);
extern G_CONST_RETURN gchar *qsodata_get_qth(void);
extern G_CONST_RETURN gchar *qsodata_get_loc(void);
extern G_CONST_RETURN gchar *qsodata_get_notes(void);

extern void qsodata_set_band_mode(band_mode_t mode);
extern void qsodata_set_freq(const gchar *freq);

#endif
