/*
 *    log.c  --  Received text logging
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

#include <gnome.h>

#include "support.h"
#include "main.h"
#include "conf.h"
#include "log.h"

G_LOCK_DEFINE_STATIC(log_mutex);
static FILE *logfile = NULL;
static gboolean retflag = TRUE;
static log_t logtype  = LOG_RX;

static gchar *lognames[] = {
	"RX",
	"TX"
};

/* ---------------------------------------------------------------------- */

#define	TIMESTR_BUF_LEN		64

void log_to_file(log_t type, const gchar *s)
{
	gchar timestr[TIMESTR_BUF_LEN];
	struct tm *tm;
	time_t t;
	
	G_LOCK(log_mutex);

	if (!logfile) {
		G_UNLOCK(log_mutex);
		return;
	}

	/* add timestamp to logged data */
	time(&t);
	tm = gmtime(&t);
	strftime(timestr, TIMESTR_BUF_LEN, "%Y-%m-%d %H:%M", tm);
	timestr[TIMESTR_BUF_LEN - 1] = 0;

	if (retflag)
		fprintf(logfile, "%s (%sZ): ", lognames[type], timestr);
	else if (logtype != type)
		fprintf(logfile, "\n%s (%sZ): ", lognames[type], timestr);

	if (fputs(s, logfile) == EOF || fflush(logfile) == EOF) {
		errmsg(_("Error writing to log file: %m"));
		fclose(logfile);
		logfile = NULL;
	}

	if (s[strlen(s) - 1] == '\n')
		retflag = TRUE;
	else
		retflag = FALSE;

	logtype = type;

	G_UNLOCK(log_mutex);
}

static void log_to_file_start(void)
{
	time_t t;
	gchar *str;

	time(&t);
	str = asctime(gmtime(&t));
	str[strlen(str) - 1] = 0;
	fprintf(logfile, _("\n--- Logging started at %s UTC ---\n"), str);
}

static void log_to_file_stop(void)
{
	time_t t;
	gchar *str;

	time(&t);
	str = asctime(gmtime(&t));
	str[strlen(str) - 1] = 0;
	fprintf(logfile, _("\n--- Logging stopped at %s UTC ---\n"), str);
}

gboolean log_to_file_activate(gboolean start)
{
	gchar *logfilename;
	gboolean ret = TRUE;

	G_LOCK(log_mutex);
	if (start) {
		if (!logfile) {
			logfilename = conf_get_logfile();
			if ((logfile = fopen(logfilename, "a")) == NULL)
				ret = FALSE;
			else
				log_to_file_start();
			g_free(logfilename);
		}
	} else {
		if (logfile) {
			log_to_file_stop();
			fclose(logfile);
			logfile = NULL;
		}
	}
	G_UNLOCK(log_mutex);

	return ret;
}

/* ---------------------------------------------------------------------- */

