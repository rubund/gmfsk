/*
 *    remlog.h  --  Remote logging (IPC)
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

#ifndef _REMLOG_H
#define _REMLOG_H

#define	LOG_MVERSION	"1"
#define	LOG_MKEY	1238
#define	LOG_MTYPE	88
#define	LOG_MSEPARATOR	1

#define	LOG_MSG_LEN	1024

typedef struct {
	long mtype;
	char mtext[LOG_MSG_LEN];
} msgtype;

#endif
