/*
 *    snd.c  --  Sound card routines.
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

#include <gnome.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

//#include <sys/soundcard.h>
#include <sys/ioctl.h>

#include "snd.h"
#include "misc.h"
#include "cwirc.h"
#include "samplerate.h"

#include <alsa/asoundlib.h>

#define	SND_DEBUG	0

/* ---------------------------------------------------------------------- */

#define	SND_BUF_LEN		65536
#define	SRC_BUF_LEN		(8*SND_BUF_LEN)
#define	SND_VOL			0.5

static gmfsk_snd_config_t config;
static gmfsk_snd_config_t newconfig;

static gint snd_fd = -1;
static gint snd_dir = 0;
static snd_pcm_t *alsa_dev;

static gint16 snd_w_buffer[2 * SND_BUF_LEN];
static guint8 snd_b_buffer[2 * SND_BUF_LEN];

static gfloat snd_buffer[SND_BUF_LEN];
static gfloat src_buffer[SRC_BUF_LEN];

SRC_STATE *tx_src_state = NULL;
SRC_DATA *tx_src_data;

SRC_STATE *rx_src_state = NULL;
SRC_DATA *rx_src_data;

static gchar snd_error_str[1024];

static gint write_samples(gfloat *buf, gint count);
static gint read_samples(gfloat *buf, gint count);

/* ---------------------------------------------------------------------- */

void sound_set_conf(gmfsk_snd_config_t *cfg)
{
	g_free(newconfig.device);

	if (cfg) {
		newconfig.device = g_strdup(cfg->device);
		newconfig.samplerate = cfg->samplerate;
		newconfig.txoffset = cfg->txoffset;
		newconfig.rxoffset = cfg->rxoffset;
	} else {
		newconfig.device = NULL;
		newconfig.samplerate = 0;
		newconfig.txoffset = 0;
		newconfig.rxoffset = 0;
	}
}

void sound_set_flags(guint flags)
{
	newconfig.flags = flags;
}

void sound_get_flags(guint *flags)
{
	*flags = newconfig.flags;
}

/* ---------------------------------------------------------------------- */

static void snderr(const gchar *fmt, ...)
{
//	va_list args;
//
//	va_start(args, fmt);
//	vsnprintf(snd_error_str, sizeof(snd_error_str), fmt, args);
//	snd_error_str[sizeof(snd_error_str) - 1] = 0;
//	va_end(args);
//
//	fprintf(stderr, "*** %s ***\n", snd_error_str);
}

#if SND_DEBUG
static void dprintf(const char *fmt, ...)
{
//	va_list args;
//
//	va_start(args, fmt);
//	vfprintf(stderr, fmt, args);
//	va_end(args);
}
#endif

/* ---------------------------------------------------------------------- */

static gint opensnd(gint direction)
{
//#if SND_DEBUG
//	audio_buf_info info;
//	gchar *str;
//#endif
//	guint sndparam, wanted;
//	gint fd;
//
//	if (!config.device) {
//		snderr(_("opensnd: device not set"));
//		return -1;
//	}
//
//#if SND_DEBUG
//	switch (direction) {
//	case O_RDONLY:
//		str = "reading";
//		break;
//	case O_WRONLY:
//		str = "writing";
//		break;
//	case O_RDWR:
//		str = "read/write";
//		break;
//	default:
//		str = "???";
//		break;
//	}
//	dprintf("Opening %s for %s...\n", config.device, str);
//#endif
//
//	/* non-blocking open */
//	if ((fd = open(config.device, direction | O_NONBLOCK)) < 0) {
//		snderr(_("opensnd: open: %s: %m"), config.device);
//		return -1;
//	}
//
//	/* make it block again - (SNDCTL_DSP_NONBLOCK ???) */
//	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK) < 0) {
//		snderr(_("opensnd: ioctl: SNDCTL_DSP_NONBLOCK: %m"));
//		goto error;
//	}
//
//#if SND_DEBUG
//	if (config.flags & SND_FLAG_8BIT)
//		str = "8 bit unsigned";
//	else
//		str = "16 bit signed, native byteorder";
//
//	dprintf("Setting sample format (%s)...\n", str);
//#endif
//
//	if (config.flags & SND_FLAG_8BIT)
//		wanted = AFMT_U8;	/* 8 bit unsigned */
//	else
//		wanted = AFMT_S16_NE;	/* 16 bit signed, native byteorder */
//
//	sndparam = wanted;
//	if (ioctl(fd, SNDCTL_DSP_SETFMT, &sndparam) < 0) {
//		snderr(_("opensnd: ioctl: SNDCTL_DSP_SETFMT: %m"));
//		goto error;
//	}
//	if (sndparam != wanted) {
//		snderr(_("opensnd: Requested sample format not supported"));
//		goto error;
//	}
//
//#if SND_DEBUG
//	dprintf("Setting %s audio...\n",
//		(config.flags & SND_FLAG_STEREO) ? "stereo" : "mono");
//#endif
//
//	if (config.flags & SND_FLAG_STEREO)
//		wanted = 1;		/* stereo */
//	else
//		wanted = 0;		/* mono */
//
//	sndparam = wanted;
//	if (ioctl(fd, SNDCTL_DSP_STEREO, &sndparam) < 0) {
//		snderr(_("opensnd: ioctl: SNDCTL_DSP_STEREO: %m"));
//		goto error;
//	}
//	if (sndparam != wanted) {
//		snderr(_("opensnd: Cannot set %s audio"),
//		       (config.flags & SND_FLAG_STEREO) ? "stereo" : "mono");
//		goto error;
//	}
//
//#if SND_DEBUG
//	dprintf("Setting samplerate to %u...\n", config.samplerate);
//#endif
//
//	sndparam = config.samplerate;
//	if (ioctl(fd, SNDCTL_DSP_SPEED, &sndparam) < 0) {
//		snderr(_("opensnd: ioctl: SNDCTL_DSP_SPEED: %m"));
//		goto error;
//	}
//	if (sndparam != config.samplerate) {
//		g_warning(_("Sampling rate is %u, requested %u\n"),
//			  sndparam,
//			  config.samplerate);
//	}
//	config.samplerate = sndparam;
//
//	/* Try to get ~100ms worth of samples per fragment */
//	sndparam = log2(config.samplerate * 100 / 1000);
//
//	/* double if we are using 16 bit samples */
//	if (!(config.flags & SND_FLAG_8BIT))
//		sndparam += 1;
//
//	/* double if we are using stereo */
//	if (config.flags & SND_FLAG_STEREO)
//		sndparam += 1;
//
//	/* Unlimited amount of buffers for RX, four for TX */
//	if (direction == O_RDONLY)
//		sndparam |= 0x7FFF0000;
//	else
//		sndparam |= 0x00040000;
//
//#if SND_DEBUG
//	dprintf("Setting fragment size (param = 0x%08X)...\n", sndparam);
//#endif
//
//	if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &sndparam) < 0) {
//		snderr(_("opensnd: ioctl: SNDCTL_DSP_SETFRAGMENT: %m"));
//		goto error;
//	}
//
//#if SND_DEBUG
//	if (direction == O_RDONLY) {
//		if (ioctl(fd, SNDCTL_DSP_GETISPACE, &info) < 0) {
//			dprintf("ioctl: SNDCTL_DSP_GETISPACE: %m");
//		}
//	} else {
//		if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
//			dprintf("ioctl: SNDCTL_DSP_GETOSPACE: %m");
//		}
//	}
//
//	dprintf("Audio buffer size: %d bytes, number of buffers: %d\n",
//		info.fragsize, info.fragstotal);
//#endif
//
//#if SND_DEBUG
//	dprintf("-- \n");
//#endif
//
//	return fd;
//
//error:
//	close(fd);
//	return -1;
	return 1;
}

/* ---------------------------------------------------------------------- */

gint sound_open_for_write(gint rate)
{
//	gdouble real_rate;
//	gdouble ratio;
//	gint err;
//
//	/* copy current config */
//	memcpy(&config, &newconfig, sizeof(snd_config_t));
//
//	if (cwirc_extension_mode) {
//		config.samplerate = CWIRC_SRATE;
//		config.txoffset = 0;
//	}
//
//	if (cwirc_extension_mode)
//		snd_fd = 0;
//	else if (config.flags & SND_FLAG_TESTMODE_MASK)
//		snd_fd = 1;
//	else if (!(config.flags & SND_FLAG_FULLDUP))
//		snd_fd = opensnd(O_WRONLY);
//	else if (snd_fd < 0)
//		snd_fd = opensnd(O_RDWR);
//
//	if (snd_fd < 0)
//		return -1;
//
//	snd_dir = O_WRONLY;
//
//	real_rate = config.samplerate * (1.0 + config.txoffset / 1e6);
//
//	if (rate == real_rate) {
//		if (tx_src_state) {
//			src_delete(tx_src_state);
//			tx_src_state = NULL;
//		}
//		return snd_fd;
//	}
//
//	ratio = real_rate / rate;
//
//	if (tx_src_state && tx_src_data && tx_src_data->src_ratio == ratio) {
//		src_reset(tx_src_state);
//		return snd_fd;
//	}
//
//#if SND_DEBUG
//	dprintf("Resampling output from %d to %.1f (%f)\n",
//		rate, real_rate, ratio);
//#endif
//
//	if (tx_src_state)
//		src_delete(tx_src_state);
//
//	tx_src_state = src_new(SRC_SINC_FASTEST, 1, &err);
//
//	if (tx_src_state == NULL) {
//		snderr(_("sound_open_for_write: src_new failed: %s"), src_strerror(err));
//		snd_fd = -1;
//		return -1;
//	}
//
//	if (!tx_src_data)
//		tx_src_data = g_new(SRC_DATA, 1);
//
//	tx_src_data->src_ratio = ratio;
//
//	return snd_fd;
	return 1;
}

gint sound_open_for_read(gint rate)
{
	gint err;
	gint channels, dir;

	char *sound_device = "default";
	if ((err = snd_pcm_open(&alsa_dev, sound_device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		snderr(_("Error opening sound device (%s)"), sound_device);
		return -1;
	}

	snd_pcm_hw_params_t *hwparams = NULL;
	snd_pcm_hw_params_alloca(&hwparams);
	if ((err = snd_pcm_hw_params_any(alsa_dev, hwparams)) < 0) {
		snderr(_("Error initializing hwparams"));
		return -1;
	}
	if ((err = snd_pcm_hw_params_set_access(alsa_dev, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		snderr(_("Error setting acecss mode (SND_PCM_ACCESS_RW_INTERLEAVED): %s"),src_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_set_format(alsa_dev, hwparams, SND_PCM_FORMAT_S16_LE)) < 0) {
		snderr(_("Error setting format (SND_PCM_FORMAT_S16_LE): %s"), src_strerror(err));
		return -1;
	}
	//if (sound_channels == SOUND_CHANNELS_MONO)
		channels = 1;
	//else
	//	channels = 2;

	if ((err = snd_pcm_hw_params_set_channels(alsa_dev, hwparams, channels)) < 0) {
		snderr(_("Error setting channels %d: %s"), channels, src_strerror(err));
		snderr(_("Maybe your sound card does not support this SoundChannels setting (mono-only or stereo-only card)."));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(alsa_dev, hwparams, &rate, 0)) < 0) {
		snderr(_("Error setting sample rate (%d): %s"), rate, src_strerror(err));
		return -1;
	}

	snd_pcm_uframes_t size = 64; /* number of frames */
	
	dir = 0;
	if ((err = snd_pcm_hw_params_set_period_size_near(alsa_dev, hwparams, &size, &dir)) < 0) {
		snderr(_("Error setting buffer size (%d): %s"), size, src_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params(alsa_dev, hwparams)) < 0) {
		snderr(_("Error writing hwparams: %s"), src_strerror(err));
		return -1;
	}

	snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);

	//int buffer_len_in_bytes = *buffer_l * sizeof(short) * channels;
	
//	gdouble real_rate;
//	gdouble ratio;
//	gint err;
//
//	/* copy current config */
//	memcpy(&config, &newconfig, sizeof(snd_config_t));
//
//	if (cwirc_extension_mode) {
//		config.samplerate = CWIRC_SRATE;
//		config.rxoffset = 0;
//	}
//
//	if (cwirc_extension_mode)
//		snd_fd = 0;
//	else if (config.flags & SND_FLAG_TESTMODE_MASK)
//		snd_fd = 0;
//	else if (!(config.flags & SND_FLAG_FULLDUP))
//		snd_fd = opensnd(O_RDONLY);
//	else if (snd_fd < 0)
//		snd_fd = opensnd(O_RDWR);
//
//	if (snd_fd < 0)
//		return -1;
//
//	snd_dir = O_RDONLY;
//
//	real_rate = config.samplerate * (1.0 + config.rxoffset / 1e6);
//
//	if (rate == real_rate) {
//		if (rx_src_state) {
//			src_delete(rx_src_state);
//			rx_src_state = NULL;
//		}
//		return snd_fd;
//	}
//
//	ratio = rate / real_rate;
//
//	if (rx_src_state && rx_src_data && rx_src_data->src_ratio == ratio) {
//		src_reset(rx_src_state);
//		return snd_fd;
//	}
//
//#if SND_DEBUG
//	dprintf("Resampling input from %.1f to %d (%f)\n",
//		real_rate, rate, ratio);
//#endif
//
//	if (rx_src_state)
//		src_delete(rx_src_state);
//
//	rx_src_state = src_new(SRC_SINC_FASTEST, 1, &err);
//
//	if (rx_src_state == NULL) {
//		snderr(_("sound_open_for_read: src_new failed: %s"), src_strerror(err));
//		snd_fd = -1;
//		return -1;
//	}
//
//	if (!rx_src_data)
//		rx_src_data = g_new(SRC_DATA, 1);
//
//	rx_src_data->src_ratio = ratio;
//
//	return snd_fd;
	return 1;
}

void sound_close(void)
{
	snd_pcm_close(alsa_dev);
//	if (cwirc_extension_mode)
//		return;
//
//	if (config.flags & SND_FLAG_TESTMODE_MASK) {
//		snd_fd = -1;
//		return;
//	}
//
//	if (config.flags & SND_FLAG_FULLDUP)
//		return;
//
//#if 0	/* this doesn't seem to work like it should... */
//	if (tx_src_state && snd_dir == O_WRONLY) {
//		tx_src_data->data_in = NULL;
//		tx_src_data->input_frames = 0;
//		tx_src_data->data_out = src_buffer;
//		tx_src_data->output_frames = SRC_BUF_LEN;
//		tx_src_data->end_of_input = 1;
//
//		src_process(tx_src_state, tx_src_data);
//
//		write_samples(src_buffer, tx_src_data->output_frames_gen);
//	}
//#endif
//
//	/* never close stdin/out/err */
//	if (snd_fd > 2) {
//		if (ioctl(snd_fd, SNDCTL_DSP_SYNC, 0) < 0)
//			snderr(_("sound_close: ioctl: SNDCTL_DSP_SYNC: %m"));
//		close(snd_fd);
//		snd_fd = -1;
//	}
}

char *sound_error(void)
{
	return snd_error_str;
}

/* ---------------------------------------------------------------------- */

gint sound_write(gfloat *buf, gint cnt)
{
	gint n;

#if SND_DEBUG > 1
	dprintf("sound_write(%d)\n", cnt);
#endif
//
//	if (cwirc_extension_mode)
//		return cwirc_sound_write(buf, cnt);
//
//	if ((config.flags & SND_FLAG_TESTMODE_MASK) == SND_FLAG_TESTMODE_RX)
//		return cnt;
//
//	if (snd_fd < 0) {
//		snderr(_("sound_write: fd < 0"));
//		return -1;
//	}
//
	if (tx_src_state == NULL)
		return write_samples(buf, cnt);
//
//	tx_src_data->data_in = buf;
//	tx_src_data->input_frames = cnt;
//	tx_src_data->data_out = src_buffer;
//	tx_src_data->output_frames = SRC_BUF_LEN;
//	tx_src_data->end_of_input = 0;
//
//	if ((n = src_process(tx_src_state, tx_src_data)) != 0) {
//		snderr(_("sound_write: src_process: %s"), src_strerror(n));
//		return -1;
//	}
//
//	write_samples(src_buffer, tx_src_data->output_frames_gen);
//
//	return cnt;
	return cnt;
}

static gint write_samples(gfloat *buf, gint count)
{
//	void *p;
//	gint i, j;
//
#if SND_DEBUG > 1
	dprintf("write_samples(%d)\n", count);
#endif

	if (count <= 0) {
		snderr(_("write_samples: count <= 0 (%d)"), count);
		return -1;
	}

	if (count > SND_BUF_LEN) {
		snderr(_("write_samples: count > SND_BUF_LEN (%d)"), count);
		return -1;
	}
//
//	if (cwirc_extension_mode)
//		return cwirc_sound_write(buf, count);
//
//	if (config.flags & SND_FLAG_8BIT) {
//		for (i = j = 0; i < count; i++) {
//			snd_b_buffer[j++] = (buf[i] * 127.0 * SND_VOL) + 128.0;
//			if (config.flags & SND_FLAG_STEREO)
//				snd_b_buffer[j++] = 128;
//		}
//
//		count *= sizeof(guint8);
//		p = snd_b_buffer;
//	} else {
//		for (i = j = 0; i < count; i++) {
//			snd_w_buffer[j++] = buf[i] * 32767.0 * SND_VOL;
//			if (config.flags & SND_FLAG_STEREO)
//				snd_w_buffer[j++] = 0;
//		}
//
//		count *= sizeof(gint16);
//		p = snd_w_buffer;
//	}
//
//	if (config.flags & SND_FLAG_STEREO)
//		count *= 2;
//
//	if ((i = write(snd_fd, p, count)) < 0)
//		snderr(_("write_samples: write: %m"));
//
//	return i;
	return count;
}

/* ---------------------------------------------------------------------- */

gint sound_read(gfloat **buffer, gint *count)
{
	struct timespec ts;
	gint n;

#if SND_DEBUG > 1
	dprintf("sound_read(%d)\n", *count);
#endif

	*count = MIN(*count, SND_BUF_LEN);


	if ((config.flags & SND_FLAG_TESTMODE_MASK) == SND_FLAG_TESTMODE_TX) {
		ts.tv_sec = 0;
		ts.tv_nsec = *count * (1000000000L / 8000);

		nanosleep(&ts, NULL);

		for (n = 0; n < *count; n++)
			snd_buffer[n] = (g_random_double() - 0.5) / 100.0;

		*buffer = snd_buffer;
		return 0;
	}
//
//	if (snd_fd < 0) {
//		snderr(_("sound_read: fd < 0"));
//		return -1;
//	}
//
//	if (rx_src_state == NULL) {
//		*count = read_samples(snd_buffer, *count);
//		*buffer = snd_buffer;
//		return 0;
//	}
//
//	n = floor(*count / rx_src_data->src_ratio / 512 + 0.5) * 512;
	//n = read_samples(src_buffer, n);
	n = *count; //FIXME
	n = read_samples(snd_buffer, n);
//
//	rx_src_data->data_in = src_buffer;
//	rx_src_data->input_frames = n;
//	rx_src_data->data_out = snd_buffer;
//	rx_src_data->output_frames = SND_BUF_LEN;
//	rx_src_data->end_of_input = 0;
//
//	if ((n = src_process(rx_src_state, rx_src_data)) != 0) {
//		snderr(_("sound_read: src_process: %s"), src_strerror(n));
//		return -1;
//	}
//
//	*count = rx_src_data->output_frames_gen;
	*count = n; //FIXME
	*buffer = snd_buffer;
//
//	return 0;
	return 0;
}

static gint read_samples(gfloat *buf, gint count)
{
	gint len, i, j, err;

#if SND_DEBUG > 1
	dprintf("read_samples(%d)\n", count);
#endif

	if (count <= 0) {
		snderr(_("read_samples: count <= 0 (%d)"), count);
		return -1;
	}

	if (count > SND_BUF_LEN) {
		snderr(_("read_samples: count > SND_BUF_LEN (%d)"), count);
		return -1;
	}

//	if (cwirc_extension_mode)
//		return cwirc_sound_read(buf, count);
//
//	if (config.flags & SND_FLAG_STEREO)
//		count *= 2;
//
//	if (config.flags & SND_FLAG_8BIT) {
//		count *= sizeof(guint8);
//
//		if ((len = read(snd_fd, snd_b_buffer, count)) < 0)
//			goto error;
//
//		len /= sizeof(guint8);
//
//		if (config.flags & SND_FLAG_STEREO)
//			len /= 2;
//
//		for (i = j = 0; i < len; i++) {
//			buf[i] = (snd_b_buffer[j++] - 128) / 128.0;
//			if (config.flags & SND_FLAG_STEREO)
//				j++;
//		}
//	} else {
//		count *= sizeof(gint16);
//
//		if ((len = read(snd_fd, snd_w_buffer, count)) < 0)
//			goto error;
//
//	}
// To remove:

	err = snd_pcm_readi(alsa_dev, snd_w_buffer, count);
	len = count;

	if (err == -EPIPE) {
		snderr(_("Overrun"));
		snd_pcm_prepare(alsa_dev);
	} else if (err < 0) {
		snderr(_("Read error"));
	} else if (err != count) {
		snderr(_("Short read, read %d frames"), err);
	} else {
		/*hlog(LOG_DEBUG, LOGPREFIX "Read %d samples", err); */
	}

//		len /= sizeof(gint16);
//
//		if (config.flags & SND_FLAG_STEREO)
//			len /= 2;
//
		for (i = j = 0; i < len; i++) {
			buf[i] = snd_w_buffer[j++] / 32768.0;
//			if (config.flags & SND_FLAG_STEREO)
//				j++;
		}

	return len;

error:
	if (errno != EAGAIN)
		snderr(_("sound_read: read: %m"));
	return len;
}

/* ---------------------------------------------------------------------- */
