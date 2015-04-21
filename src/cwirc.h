/*
 *    cwirc.h  --  CWirc interface
 *
 *    Copyright (C) 2003
 *      Pierre-Philippe Coupard (pcoupard@easyconnect.fr)
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

#ifndef _CWIRC_H

/* Defines */
#define	CWIRC_SRATE	44100


/* Variables */
extern gboolean cwirc_extension_mode;


/* Prototypes */
extern gint cwirc_init(gint shmid);
extern gint cwirc_sound_write(gfloat *buf, gint count);
extern gint cwirc_sound_read(gfloat *buf, gint count);

#endif
