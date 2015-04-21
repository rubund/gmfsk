/*
 *    fft.c  --  Fast Fourier Transform
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <math.h>

#include "fft.h"
#include "misc.h"

/* ---------------------------------------------------------------------- */

#define	FLAGS	(FFTW_MEASURE | FFTW_OUT_OF_PLACE | FFTW_USE_WISDOM)

struct fft *fft_init(gint len, gint dir)
{
	struct fft *s;

	s = g_new0(struct fft, 1);

	if ((s->in = fftw_malloc(len * sizeof(fftw_complex))) == NULL) {
		fft_free(s);
		return NULL;
	}

	if ((s->out = fftw_malloc(len * sizeof(fftw_complex))) == NULL) {
		fft_free(s);
		return NULL;
	}

#if 1
	s->plan = fftw_create_plan(len, dir, FLAGS);
#else
	s->plan = fftw_plan_dft_1d(len, s->in, s->out,
				   FFTW_FORWARD,
				   FFTW_MEASURE);
#endif

	if (!s->plan) {
		fft_free(s);
		return NULL;
	}

	s->len = len;

	fft_clear_inbuf(s);
	fft_clear_outbuf(s);

	return s;
}

void fft_free(struct fft *s)
{
	if (s) {
		if (s->plan)
			fftw_destroy_plan(s->plan);
		if (s->in)
			fftw_free(s->in);
		if (s->out)
			fftw_free(s->out);
		g_free(s);
	}
}

void fft_clear_inbuf(struct fft *s)
{
	gint i;

	for (i = 0; i < s->len; i++) {
		c_re(s->in[i]) = 0.0;
		c_im(s->in[i]) = 0.0;
	}
}

void fft_clear_outbuf(struct fft *s)
{
	int i;

	for (i = 0; i < s->len; i++) {
		c_re(s->out[i]) = 0.0;
		c_im(s->out[i]) = 0.0;
	}
}

void fft_run(struct fft *s)
{
#if 1
	fftw_one(s->plan, s->in, s->out);
#else
	fftw_execute(s->plan);
#endif
}

/* ---------------------------------------------------------------------- */

void fft_load_wisdom(const gchar *filename)
{
	FILE *fp;

	if (filename == NULL)
		return;

	if ((fp = fopen(filename, "r")) == NULL) {
		g_warning("fft_load_wisdom: %s: %m\n", filename);
		return;
	}

	if (fftw_import_wisdom_from_file(fp) == FFTW_FAILURE)
		g_warning("fftw_import_wisdom_from_file failed!");

	fclose(fp);
}

void fft_save_wisdom(const gchar *filename)
{
	FILE *fp;

	if (filename == NULL)
		return;

	if ((fp = fopen(filename, "w")) == NULL) {
		g_warning("fft_save_wisdom: %s: %m\n", filename);
		return;
	}

	fftw_export_wisdom_to_file(fp);

	fclose(fp);
}

void fft_set_wisdom(const char *wisdom)
{
	if (wisdom == NULL)
		return;

	if (fftw_import_wisdom_from_string(wisdom) == FFTW_FAILURE)
		g_warning("Error importing FFTW wisdom!!!\n");
}

char *fft_get_wisdom(void)
{
	return fftw_export_wisdom_to_string();
}

/* ---------------------------------------------------------------------- */

