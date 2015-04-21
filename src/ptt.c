/*
 *    ptt.c  --  PTT control
 *
 *    Copyright (C) 2001, 2002, 2003, 2004
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef HAVE_LINUX_PPDEV_H
#include <linux/ppdev.h>
#endif

#include "main.h"
#include "ptt.h"
#include "hamlib.h"
#include "cwirc.h"

/* ---------------------------------------------------------------------- */

static gint openptt(void);

static gint pttfd = -1;
static gint pttinv = 0;
static gint pttarg = TIOCM_RTS | TIOCM_DTR;
static gchar *pttpath = NULL;
static enum { NOPORT, SERPORT, PARPORT } pttmode = NOPORT;

/* ---------------------------------------------------------------------- */

void ptt_init(const gchar *path, gint inverted, gint mode)
{
	if (cwirc_extension_mode)
		return;

	/* If it's open then close it */
	ptt_close();

	pttinv = inverted;

	switch (mode) {
	case 0:
		pttarg = TIOCM_RTS;
		break;
	case 1:
		pttarg = TIOCM_DTR;
		break;
	case 2:
		pttarg = TIOCM_RTS | TIOCM_DTR;
		break;
	}

	g_free(pttpath);

	if (!strcasecmp(path, "none"))
		pttpath = NULL;
	else
		pttpath = g_strdup(path);
}

void ptt_close(void)
{
	if (pttfd != -1) {
#ifdef HAVE_LINUX_PPDEV_H
		if (pttmode == PARPORT)
			ioctl(pttfd, PPRELEASE);
#endif
		close(pttfd);

		pttfd = -1;
	}
}

void ptt_set(gint ptt)
{
	if (cwirc_extension_mode)
		return;

#if WANT_HAMLIB
	/* try hamlib ptt first */
	hamlib_set_ptt(ptt);
#endif

	if (!pttpath)
		return;

	/* If it's closed, try to open it */
	if (pttfd < 0 && (pttfd = openptt()) < 0)
		return;

	if (pttinv)
		ptt = !ptt;
	else
		ptt = !!ptt;

	if (pttmode == SERPORT) {
		guint arg = pttarg;
		if (ioctl(pttfd, ptt ? TIOCMBIS : TIOCMBIC, &arg) < 0)
			errmsg(_("set_ptt: ioctl: %m"));
	}

#ifdef HAVE_LINUX_PPDEV_H
	if (pttmode == PARPORT) {
		guchar arg = ptt;
		if (ioctl(pttfd, PPWDATA, &arg) < 0)
			errmsg(_("set_ptt: ioctl: %m"));
	}
#endif
}

/* ---------------------------------------------------------------------- */

static gint openptt(void)
{
	gint fd;
	guint serarg = TIOCM_RTS | TIOCM_DTR;
#ifdef HAVE_LINUX_PPDEV_H
	guchar pararg;
#endif

	if ((fd = open(pttpath, O_RDWR, 0)) < 0) {
		errmsg(_("Cannot open PTT device '%s': %m"), pttpath);
		return -1;
	}

	pttmode = NOPORT;

#ifdef HAVE_LINUX_PPDEV_H
	if (!ioctl(fd, PPCLAIM, 0) && !ioctl(fd, PPRDATA, &pararg))
		pttmode = PARPORT;
#endif

	if (pttmode == NOPORT && !ioctl(fd, TIOCMBIC, &serarg))
		pttmode = SERPORT;

	if (pttmode == NOPORT) {
#ifdef HAVE_LINUX_PPDEV_H
		errmsg(_("Device '%s' is neither a parallel nor a serial port"), pttpath);
#else
		errmsg(_("Device '%s' is not a serial port"), pttpath);
#endif
		return -1;
	}

	return fd;
}

/* ---------------------------------------------------------------------- */

