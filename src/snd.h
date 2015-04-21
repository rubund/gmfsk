/*
 *    snd.h  --  Sound card routines.
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

#ifndef _SND_H
#define _SND_H

#ifdef __cplusplus
extern "C" {
#endif

#define	SND_FLAG_TESTMODE_RX	(1 << 0)
#define	SND_FLAG_TESTMODE_TX	(1 << 1)
#define	SND_FLAG_8BIT		(1 << 2)
#define	SND_FLAG_STEREO		(1 << 3)
#define	SND_FLAG_FULLDUP	(1 << 4)

#define	SND_FLAG_TESTMODE_MASK	(SND_FLAG_TESTMODE_RX|SND_FLAG_TESTMODE_TX)

typedef struct {
	gchar		*device;
	gint		samplerate;
	gint		txoffset;
	gint		rxoffset;
	guint		flags;
} gmfsk_snd_config_t;

extern void sound_set_conf(gmfsk_snd_config_t *cfg);

extern void sound_set_flags(guint);
extern void sound_get_flags(guint *);

extern int sound_open_for_write(int srate);
extern int sound_open_for_read(int srate);

extern void sound_close(void);

extern int sound_write(float *buf, int count);
extern int sound_read(float **buf, int *count);

extern char *sound_error(void);

#ifdef __cplusplus
}
#endif

#endif
