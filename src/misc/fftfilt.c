/*
 *    fftfilt.c  --  Fast convolution FIR filter
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

#include <stdlib.h>
#include <math.h>

#include "fftfilt.h"
#include "fft.h"
#include "misc.h"

#undef	DEBUG

struct fftfilt *fftfilt_init(gdouble f1, gdouble f2, gint len)
{
	struct fftfilt *s;

	s = g_new0(struct fftfilt, 1);

	if ((s->fft = fft_init(len, FFT_FWD)) == NULL) {
		fftfilt_free(s);
		return NULL;
	}

	if ((s->ift = fft_init(len, FFT_REV)) == NULL) {
		fftfilt_free(s);
		return NULL;
	}

	if ((s->tmpfft = fft_init(len, FFT_FWD)) == NULL) {
		fftfilt_free(s);
		return NULL;
	}

	s->ovlbuf = g_new0(complex, len / 2);
	s->filter = g_new0(complex, len);

	s->filterlen = len;
	s->inptr = 0;

	fftfilt_set_freqs(s, f1, f2);

	return s;
}

void fftfilt_free(struct fftfilt *s)
{
	if (s) {
		fft_free(s->fft);
		fft_free(s->ift);
		fft_free(s->tmpfft);
		g_free(s->ovlbuf);
		g_free(s->filter);
		g_free(s);
	}
}

void fftfilt_set_freqs(struct fftfilt *s, gdouble f1, gdouble f2)
{
	gint len = s->filterlen / 2 + 1;
	gdouble t, h, x;
	gint i;

	fft_clear_inbuf(s->tmpfft);

	for (i = 0; i < len; i++) {
		t = i - (len - 1.0) / 2.0;
		h = i / (len - 1.0);

		x = (2 * f2 * sinc(2 * f2 * t) -
		     2 * f1 * sinc(2 * f1 * t)) * hamming(h);

		c_re(s->tmpfft->in[i]) = x;
		c_im(s->tmpfft->in[i]) = 0.0;
#ifdef DEBUG
                fprintf(stderr, "% e\t", x);
#endif
	}

	fft_run(s->tmpfft);

	/*
	 * Scale down by 'filterlen' because inverse transform is 
	 * unscaled in FFTW.
	 */
	for (i = 0; i < s->filterlen; i++) {
		c_re(s->filter[i]) = c_re(s->tmpfft->out[i]) / s->filterlen;
		c_im(s->filter[i]) = c_im(s->tmpfft->out[i]) / s->filterlen;
	}

#ifdef DEBUG
	for (i = 0; i < s->filterlen; i++)
		fprintf(stderr, "% e\n", 10 * log10(cpwr(s->filter[i])));
#endif
}

/*
 * Filter with fast convolution (overlap-add algorithm).
 */
gint fftfilt_run(struct fftfilt *s, complex in, complex **out)
{
	gint i;

	/* collect filterlen/2 input samples */
	s->fft->in[s->inptr++] = in;

	if (s->inptr < s->filterlen / 2)
		return 0;

	/* FFT */
	fft_run(s->fft);

	/* multiply with the filter shape */
	for (i = 0; i < s->filterlen; i++)
		s->ift->in[i] = cmul(s->fft->out[i], s->filter[i]);

	/* IFFT */
	fft_run(s->ift);

	/* overlap and add */
	for (i = 0; i < s->filterlen / 2; i++) {
		c_re(s->ift->out[i]) += c_re(s->ovlbuf[i]);
		c_im(s->ift->out[i]) += c_im(s->ovlbuf[i]);
	}
	*out = s->ift->out;

	/* save the second half for overlapping */
	for (i = 0; i < s->filterlen / 2; i++) {
		c_re(s->ovlbuf[i]) = c_re(s->ift->out[i + s->filterlen / 2]);
		c_im(s->ovlbuf[i]) = c_im(s->ift->out[i + s->filterlen / 2]);
	}

	/* clear inbuf */
	fft_clear_inbuf(s->fft);
	s->inptr = 0;

	/* signal the caller there is filterlen/2 samples ready */
	return s->filterlen / 2;
}
