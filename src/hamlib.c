/*
 *    hamlib.c  --  Hamlib (rig control) interface
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

#if WANT_HAMLIB

#include <gnome.h>
#include <pthread.h>
#include <hamlib/rig.h>

#include "support.h"

#include "hamlib.h"
#include "main.h"
#include "conf.h"
#include "qsodata.h"
#include "trx.h"

static GSList *riglist = NULL;
static GPtrArray *rignames = NULL;

static gint riglist_compare_func(gconstpointer a, gconstpointer b);
static gint riglist_make_list(const struct rig_caps *caps, gpointer data);

/* ---------------------------------------------------------------------- */

GSList *riglist_get_list(void)
{
	if (riglist == NULL) {
		rig_set_debug(RIG_DEBUG_NONE);
		rig_load_all_backends();
		rig_set_debug(RIG_DEBUG_ERR);

		rig_list_foreach(riglist_make_list, NULL);
	}

	return riglist;
}

gint riglist_get_names(GPtrArray **list)
{
	gint i, len;

	if (riglist == NULL)
		riglist_get_list();

	if (rignames != NULL) {
		*list = rignames;
		return g_slist_length(riglist);
	}

	len = g_slist_length(riglist);

	rignames = g_ptr_array_sized_new(len);

	for (i = 0; i < len; i++) {
		struct rig_caps *rig = g_slist_nth_data(riglist, i);
		gchar *name;

		name = g_strdup_printf("%s %s",
				       rig->mfg_name,
				       rig->model_name);

		g_ptr_array_add(rignames, (gpointer) name);
	}

	*list = rignames;
	return len;
}

gint riglist_get_id_from_index(gint idx)
{
	struct rig_caps *rig;

	g_return_val_if_fail(riglist != NULL, 0);

	if ((rig = g_slist_nth_data(riglist, idx)) == NULL)
		return 0;

	return rig->rig_model;
}

gint riglist_get_index_from_id(gint id)
{
	gint i, len;

	g_return_val_if_fail(riglist != NULL, 0);

	len = g_slist_length(riglist);

	for (i = 0; i < len; i++) {
		struct rig_caps *rig = g_slist_nth_data(riglist, i);

		if (rig->rig_model == id)
			return i;
	}

	return 0;
}

/* ---------------------------------------------------------------------- */

static const gint speedlist[] = {
	300,	600,	1200,	2400,
	4800,	9600,	19200,	38400,
	57600,	115200,	230400,	460800,
	-1
};

gint riglist_get_speeds(gint rigid, gint **speeds)
{
	struct rig_caps *rig;
	gint i, n, len, *list;

	if (riglist == NULL)
		riglist_get_list();

	rig = g_slist_nth_data(riglist, riglist_get_index_from_id(rigid));

	for (len = i = 0; speedlist[i] > 0; i++) {
		if (speedlist[i] > rig->serial_rate_max)
			break;
		if (speedlist[i] >= rig->serial_rate_min)
			len++;
	}

	list = g_new(gint, len);

	for (n = i = 0; speedlist[i] > 0; i++) {
		if (speedlist[i] > rig->serial_rate_max)
			break;
		if (speedlist[i] >= rig->serial_rate_min)
			list[n++] = speedlist[i];
	}

	*speeds = list;
	return len;
}

gint riglist_get_index_from_speed(gint rigid, gint speed)
{
	gint i, len, *speeds;

	if ((len = riglist_get_speeds(rigid, &speeds)) == 0)
		return 0;

	for (i = 0; i < len; i++)
		if (speeds[i] == speed)
			break;

	/* default to the largest speed */
	if (i == len)
		i = len - 1;

	g_free(speeds);
	return i;
}

gint riglist_get_speed_from_index(gint rigid, gint idx)
{
	gint i, len, *speeds;

	if ((len = riglist_get_speeds(rigid, &speeds)) == 0)
		return 0;

	if (idx >= 0 && idx < len)
		i = speeds[idx];
	else
		i = 0;

	g_free(speeds);
	return i;
}

/* ---------------------------------------------------------------------- */

static gint riglist_make_list(const struct rig_caps *caps, gpointer data)
{
	riglist = g_slist_insert_sorted(riglist,
					(gpointer) caps,
					riglist_compare_func);
	return 1;
}

static gint riglist_compare_func(gconstpointer a, gconstpointer b)
{
	const struct rig_caps *rig1 = a;
	const struct rig_caps *rig2 = b;
	gint ret;

	if ((ret = strcmp(rig1->mfg_name, rig2->mfg_name)) != 0)
		return ret;

	if ((ret = strcmp(rig1->model_name, rig2->model_name)) != 0)
		return ret;

	return rig1->rig_model - rig2->rig_model;
}

/* ---------------------------------------------------------------------- */

G_LOCK_DEFINE_STATIC(hamlib_mutex);

static RIG *rig = NULL;
static pthread_t hamlib_thread;
static gboolean hamlib_exit = FALSE;

static gboolean hamlib_waterfall;
static gboolean hamlib_qsodata;
static gboolean hamlib_ptt;
static gboolean hamlib_qsy;
static gint hamlib_res;

static gboolean need_freq;
static gboolean need_mode;

static void *hamlib_loop(void *args);

void hamlib_set_conf(gboolean wf, gboolean qd, gboolean ptt, gint res)
{
	hamlib_waterfall = wf;
	hamlib_qsodata = qd;
	hamlib_ptt = ptt;
	hamlib_res = res;

	need_freq = (wf == TRUE || qd == TRUE) ? TRUE : FALSE;
	need_mode = need_freq;

	if (!hamlib_waterfall) {
		waterfall_set_carrier_frequency(waterfall, 0.0);
		waterfall_set_lsb(waterfall, FALSE);
	}

	if (!hamlib_qsodata)
		qsodata_set_band_mode(QSODATA_BAND_COMBO);
}

static gboolean hamlib_set_param(const gchar *par, const gchar *val)
{
	token_t t;
	gint ret;

	g_return_val_if_fail(rig != NULL, FALSE);

	if ((t = rig_token_lookup(rig, par)) == RIG_CONF_END) {
		errmsg(_("Hamlib init: Bad rig config parameter: '%s'"), par);
		return FALSE;
	}

	if ((ret = rig_set_conf(rig, t, val)) != RIG_OK) {
		errmsg(_("Hamlib init: rig_set_conf failed (%s=%s): %s"), par, val, rigerror(ret));
		return FALSE;
	}

	return TRUE;
}

void hamlib_init(void)
{
	rig_model_t model;
	struct timespec sleep;
	freq_t freq;
	rmode_t mode;
	pbwidth_t width;
	gboolean enable;
	gchar *port, *conf, *spd;
	gint ret, speed;

	if (rig != NULL)
		return;

	enable = conf_get_bool("hamlib/enable");
	model = conf_get_int("hamlib/rig");
	port = conf_get_filename("hamlib/port");
	speed = conf_get_int("hamlib/speed");
	conf = conf_get_string("hamlib/conf");

	if (!enable || !model || port[0] == 0)
		return;

	rig_set_debug(RIG_DEBUG_ERR);

	rig = rig_init(model);

	if (rig == NULL) {
		errmsg(_("Hamlib init: rig_init failed (model=%d)"), model);
		return;
	}

	g_strstrip(conf);
	if (conf[0]) {
		gchar **v, **p, *q;

		v = g_strsplit(conf, ",", 0);

		for (p = v; *p; p++) {
			if ((q = strchr(*p, '=')) == NULL) {
				errmsg(_("Hamlib init: Bad param=value pair: '%s'"), *p);
				break;
			}
			*q++ = 0;

			g_strstrip(*p);
			g_strstrip(q);

			if (hamlib_set_param(*p, q) == FALSE)
				break;
		}

		g_strfreev(v);
	}
	g_free(conf);

	hamlib_set_param("rig_pathname", port);
	g_free(port);

	spd = g_strdup_printf("%d", speed);
	hamlib_set_param("serial_speed", spd);
	g_free(spd);

	ret = rig_open(rig);

	if (ret != RIG_OK) {
		errmsg(_("Hamlib init: rig_open failed: %s"), rigerror(ret));
		rig_cleanup(rig);
		rig = NULL;
		return;
	}

	/* Polling the rig sometimes fails right after opening it */
	sleep.tv_sec = 0;
	sleep.tv_nsec = 100000000L;	/* 100ms */
	nanosleep(&sleep, NULL);

	if (need_freq == TRUE && \
	    (ret = rig_get_freq(rig, RIG_VFO_CURR, &freq)) != RIG_OK) {
		errmsg(_("Hamlib init: rig_get_freq failed: %s"), rigerror(ret));

		hamlib_waterfall = FALSE;
		hamlib_qsodata = FALSE;

		need_freq = FALSE;
		need_mode = FALSE;
	}

	if (need_mode == TRUE &&
	    (ret = rig_get_mode(rig, RIG_VFO_CURR, &mode, &width)) != RIG_OK) {
		errmsg(_("Hamlib init: rig_get_mode failed: %s.\nAssuming USB mode."), rigerror(ret));

		need_mode = FALSE;
	}

	if (hamlib_ptt == TRUE && 
	    (ret = rig_set_ptt(rig, RIG_VFO_CURR, RIG_PTT_OFF)) != RIG_OK) {
		errmsg(_("Hamlib init: rig_set_ptt failed: %s.\nHamlib PTT disabled"), rigerror(ret));

		hamlib_ptt = FALSE;
	}

	/* Don't create the thread if frequency data is not needed */
	if (need_freq == FALSE) {
		/* If PTT isn't needed either then close everything */
		if (hamlib_ptt == FALSE) {
			rig_close(rig);
			rig_cleanup(rig);
			rig = NULL;
		}
		return;
	}

	if (pthread_create(&hamlib_thread, NULL, hamlib_loop, NULL) < 0) {
		errmsg(_("Hamlib init: pthread_create: %m"));
		rig_close(rig);
		rig_cleanup(rig);
		rig = NULL;
	}
}

void hamlib_close(void)
{
	if (rig == NULL)
		return;

	/* tell the hamlib thread to kill it self */
	hamlib_exit = TRUE;

	/* and the wait for it to die */
	pthread_join(hamlib_thread, NULL);

	hamlib_exit = FALSE;

	G_LOCK(hamlib_mutex);
	rig_close(rig);
	rig_cleanup(rig);
	rig = NULL;
	G_UNLOCK(hamlib_mutex);
}

gboolean hamlib_active(void)
{
	return (rig != NULL);
}

void hamlib_set_ptt(gint ptt)
{
	gint ret;

	if (!rig || !hamlib_ptt)
		return;

	G_LOCK(hamlib_mutex);

	ret = rig_set_ptt(rig, RIG_VFO_CURR, ptt ? RIG_PTT_ON : RIG_PTT_OFF);

	if (ret != RIG_OK) {
		errmsg(_("rig_set_ptt failed: %s.\nHamlib PTT disabled.\n"), rigerror(ret));
		hamlib_ptt = FALSE;
	}

	G_UNLOCK(hamlib_mutex);
}

void hamlib_set_qsy(void)
{
	if (rig == NULL)
		return;

	hamlib_qsy = TRUE;
}

static void *hamlib_loop(void *args)
{
	struct timespec sleep;
	gdouble freq = 0.0;
	rmode_t mode = RIG_MODE_USB;
	gint ret;
	gchar *str;

	sleep.tv_sec = 0;
	sleep.tv_nsec = 100000000L;	/* 100ms */

	for (;;) {
		nanosleep(&sleep, NULL);

//		need_freq = (hamlib_waterfall || hamlib_qsodata || hamlib_qsy);
//		need_mode = need_freq;

		/* see if we are being canceled */
		if (hamlib_exit)
			break;

		if (need_freq) {
			freq_t f;

			G_LOCK(hamlib_mutex);
			ret = rig_get_freq(rig, RIG_VFO_CURR, &f);
			G_UNLOCK(hamlib_mutex);

			if (ret != RIG_OK) {
				str = g_strdup_printf(_("rig_get_freq failed: %s"),
						      rigerror(ret));
				statusbar_set_main(str);
				g_free(str);
				continue;
			}

			freq = (gdouble) f;
		}

		/* see if we are being canceled */
		if (hamlib_exit)
			break;

		if (need_mode) {
			pbwidth_t width;

			G_LOCK(hamlib_mutex);
			ret = rig_get_mode(rig, RIG_VFO_CURR, &mode, &width);
			G_UNLOCK(hamlib_mutex);

			if (ret != RIG_OK) {
				str = g_strdup_printf(_("rig_get_mode failed: %s"),
						      rigerror(ret));
				statusbar_set_main(str);
				g_free(str);
				continue;
			}
		}

		if (hamlib_qsy) {
			gfloat f = conf_get_float("hamlib/cfreq");

			hamlib_qsy = FALSE;

			G_LOCK(hamlib_mutex);
			ret = rig_set_freq(rig, RIG_VFO_CURR, ((gdouble) freq) + trx_get_freq() - f);
			G_UNLOCK(hamlib_mutex);

			if (ret != RIG_OK) {
				str = g_strdup_printf(_("rig_set_freq failed: %s"),
						      rigerror(ret));
				statusbar_set_main(str);
				g_free(str);
				continue;
			}

			waterfall_set_frequency(waterfall, f);
		}

		if (hamlib_waterfall) {
			waterfall_set_carrier_frequency(waterfall, freq);

			if (mode == RIG_MODE_LSB)
				waterfall_set_lsb(waterfall, TRUE);
			else
				waterfall_set_lsb(waterfall, FALSE);
		}

		if (hamlib_qsodata) {
			gchar *str, *fmt;

			if (mode == RIG_MODE_LSB)
				freq -= trx_get_freq();
			else
				freq += trx_get_freq();

			switch (hamlib_res) {
			case 2:
				fmt = "%.6f";
				break;
			case 1:
				fmt = "%.3f";
				break;
			case 0:
			default:
				fmt = "%.0f";
				break;
			}

			str = g_strdup_printf(fmt, freq / 1000000.0);

			gdk_threads_enter();
			qsodata_set_band_mode(QSODATA_BAND_ENTRY);
			qsodata_set_freq(str);
			gdk_threads_leave();

			g_free(str);
		}

		/* see if we are being canceled */
		if (hamlib_exit)
			break;
	}

	if (hamlib_waterfall) {
		waterfall_set_carrier_frequency(waterfall, 0.0);
		waterfall_set_lsb(waterfall, FALSE);
	}

	if (hamlib_qsodata) {
		gdk_threads_enter();
		qsodata_set_band_mode(QSODATA_BAND_COMBO);
		qsodata_set_freq("");
		gdk_threads_leave();
	}

	/* this will exit the hamlib thread */
	return NULL;
}

/* ---------------------------------------------------------------------- */

#endif
