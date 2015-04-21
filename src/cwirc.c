/*
 *    cwirc.c  --  CWirc interface
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnome.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "cwirc.h"


/* Definitions */
#define AUDIOBUF_SIZE		22050   /* Samples */


/* Global variables */
gboolean cwirc_extension_mode = FALSE;


struct cwirc_extension_api {
        gint semid;
        gint16 out_audiobuf[AUDIOBUF_SIZE];
        gint out_audiobuf_start;
        gint out_audiobuf_end;
        gint in_key;
        gint pid;
};

static gint gmfsk_srate = CWIRC_SRATE;
static struct cwirc_extension_api *cwirc_api_shm;

/* 
 * Init CWirc mode: attach to the shared memory segment and set
 *                  the cwirc_extension_mode flag.
 */
gint cwirc_init(gint shmid)
{
	cwirc_api_shm = shmat(shmid, NULL, 0);

	if (cwirc_api_shm == (struct cwirc_extension_api *) -1) {
		g_critical(_("Can't attach to the CWirc API shared memory block!\n"));
		return -1;
	}

	g_message(_("Working in CWirc slave mode.\n"));

	cwirc_extension_mode = TRUE;

	return 0;
}

/*
 * Sound write routine replacement that keys CWirc instead of writing 
 * sound to a sound device
 */
gint cwirc_sound_write(gfloat *buf, gint count)
{
	struct sembuf sops;
	static gint semno = 0;
	static gint samplebuf_size = 0;
	static gdouble srates_conv_ratio;
	static gint sbi = 0;
	static gdouble sbd = 0;
	static gint keycnt = 0;
	gint i;

	srates_conv_ratio = (gdouble) gmfsk_srate / 44100;
	while (1) {
		/* Is the sample buffer empty ? */
		if (samplebuf_size == 0) {
			/* Acquire semaphore to get samples from CWirc */
			sops.sem_num = semno;
			sops.sem_op = -1;
			sops.sem_flg = SEM_UNDO;
			if (semop(cwirc_api_shm->semid, &sops, 1) == -1)
				continue;

			/* Count samples, adjust ring buffer pointers */
			while (cwirc_api_shm->out_audiobuf_start !=
			       cwirc_api_shm->out_audiobuf_end) {
				cwirc_api_shm->out_audiobuf_start++;
				if (cwirc_api_shm->out_audiobuf_start >= AUDIOBUF_SIZE)
					cwirc_api_shm->out_audiobuf_start -= AUDIOBUF_SIZE;
				samplebuf_size++;
			}

			/* Release the semaphore and switch semaphore */
			sops.sem_num = semno;
			sops.sem_op = 1;
			sops.sem_flg = SEM_UNDO;
			if (semop(cwirc_api_shm->semid, &sops, 1) == -1)
				continue;
			semno = semno ? 0 : 1;
		}

		i = sbd;

		/* Key CWirc */
		if (buf[i] < -.25 || buf[i] > .25)
			keycnt = 10;
		if (keycnt > 0) {
			keycnt--;
			cwirc_api_shm->in_key = 1;
		} else
			cwirc_api_shm->in_key = 0;

		sbi++;
		sbd += srates_conv_ratio;
		i = sbd;

		if (sbi == samplebuf_size)
			samplebuf_size = sbi = 0;

		if (i == count) {
			sbd -= count;
			break;
		}
	}

	return count;
}


/*
 * Sound read routine that grab samples from CWirc instead of a sound device
 */
gint cwirc_sound_read(gfloat *buf, gint count)
{
	struct sembuf sops;
	static gint semno = 0;
	static gint16 samplebuf[AUDIOBUF_SIZE * 2];
	static gint samplebuf_size = 0;
	static gdouble srates_conv_ratio;
	static gint sbi = 0;
	static gdouble sbd = 0;
	static gdouble prev_val = 0,val;
	gdouble prev_val_weight,val_weight;
	gint i;

	srates_conv_ratio = (gdouble) gmfsk_srate / 44100;
	while (1) {
		/* Is the sample buffer empty ? */
		if (samplebuf_size == 0) {
			/* Acquire semaphore to get samples from CWirc */
			sops.sem_num = semno;
			sops.sem_op = -1;
			sops.sem_flg = SEM_UNDO;
			if (semop(cwirc_api_shm->semid, &sops, 1) == -1)
				continue;

			/* Grab samples, adjust ring buffer pointers */
			while (cwirc_api_shm->out_audiobuf_start !=
			       cwirc_api_shm->out_audiobuf_end) {
				samplebuf[samplebuf_size] = cwirc_api_shm->out_audiobuf[cwirc_api_shm->out_audiobuf_start++];
				if (cwirc_api_shm->out_audiobuf_start >= AUDIOBUF_SIZE)
					cwirc_api_shm->out_audiobuf_start -= AUDIOBUF_SIZE;
				samplebuf_size++;
			}

			/* Release the semaphore and switch semaphore */
			sops.sem_num = semno;
			sops.sem_op = 1;
			sops.sem_flg = SEM_UNDO;
			if (semop(cwirc_api_shm->semid, &sops, 1) == -1)
				continue;
			semno = semno ? 0 : 1;
		}

		i = sbd;

		/* Perform cheap downsampling here just in case gmfsk_srate != 44100 */
		val = samplebuf[sbi] / 32768.0;
		val_weight=((double)i + 1) - sbd;
		prev_val_weight=srates_conv_ratio - val_weight;
		buf[i] = (prev_val * prev_val_weight + val * val_weight) / srates_conv_ratio;
		prev_val = val;

		sbi++;
		sbd += srates_conv_ratio;
		i = sbd;

		if (sbi == samplebuf_size)
			samplebuf_size = sbi = 0;

		if (i == count) {
			/* Introduces a little noise, otherwise the feldhell module gMFSK
			   doesn't seem to start (???) */
			for (i = 0; i < count; i++) {
				buf[i] += (((double) rand() * 2) / RAND_MAX - 1) / 100000;
				if (buf[i] < -1)
					buf[i] = -1;
				else if (buf[i] > 1)
					buf[i] = 1;
			}

			/* Cap the samples (paranoia feature) */
			for(i = 0; i < count; i++) {
				if (buf[i] < -1.0)
					buf[i] = -1.0;
				else if (buf[i] > 1.0)
					buf[i] = 1.0;
			}

			sbd -= count;
			break;
		}
	}

	return count;
}
