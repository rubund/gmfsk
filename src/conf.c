/*
 *    conf.c  --  Configuration
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
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "interface.h"
#include "support.h"

#include "conf.h"
#include "main.h"
#include "trx.h"
#include "ptt.h"
#include "snd.h"
#include "hamlib.h"

static GConfClient *client = NULL;

/* ---------------------------------------------------------------------- */

static gchar *mkkey(const gchar *key)
{
	static gchar buf[256];

	g_snprintf(buf, sizeof(buf), "/apps/%s/%s", PACKAGE, key);
	buf[sizeof(buf) - 1] = 0;

	return buf;
}

/* ---------------------------------------------------------------------- */

void conf_set_string(const gchar *key, const gchar *val)
{
	g_return_if_fail(client);
	g_return_if_fail(key);

	gconf_client_set_string(client, mkkey(key), val, NULL);
}

void conf_set_bool(const gchar *key, gboolean val)
{
	g_return_if_fail(client);
	g_return_if_fail(key);

	gconf_client_set_bool(client, mkkey(key), val, NULL);
}

void conf_set_int(const gchar *key, gint val)
{
	g_return_if_fail(client);
	g_return_if_fail(key);

	gconf_client_set_int(client, mkkey(key), val, NULL);
}

void conf_set_float(const gchar *key, gdouble val)
{
	g_return_if_fail(client);
	g_return_if_fail(key);

	gconf_client_set_float(client, mkkey(key), val, NULL);
}

/* ---------------------------------------------------------------------- */

/*
 * These config defaults are only used as a last resort.
 * Normally the gconf schema system provides these defaults.
 * (see gmfsk.schemas).
 */

typedef enum {
	D_INT,
	D_BOOL,
	D_FLOAT,
	D_STRING
} conf_data_t;

struct conf_default {
	const gchar	*key;
	conf_data_t	type;
	union {
		gint		 i;
		gboolean	 b;
		gdouble		 f;
		const gchar	*s;
	} value;
};

static struct conf_default conf_defs[] = {
	{ "info/mycall",     D_STRING, { .s = ""     } },
	{ "info/myname",     D_STRING, { .s = ""     } },
	{ "info/myqth",      D_STRING, { .s = ""     } },
	{ "info/myloc",      D_STRING, { .s = ""     } },
	{ "info/myemail",    D_STRING, { .s = ""     } },
	{ "colors/tx",       D_STRING, { .s = "red"  } },
	{ "colors/rx",       D_STRING, { .s = "black" } },
	{ "colors/hl",       D_STRING, { .s = "blue" } },
	{ "colors/txwin",    D_STRING, { .s = "white" } },
	{ "colors/rxwin",    D_STRING, { .s = "white" } },
	{ "fonts/txfont",    D_STRING, { .s = "Sans 10" } },
	{ "fonts/rxfont",    D_STRING, { .s = "Sans 10" } },
	{ "sound/dev",       D_STRING, { .s = "/dev/dsp" } },
	{ "ptt/dev",         D_STRING, { .s = "none" } },
	{ "misc/datefmt",    D_STRING, { .s = "%d.%m.%Y" } },
	{ "misc/timefmt",    D_STRING, { .s = "%H:%M:%S %Z" } },
	{ "misc/bands",      D_STRING, { .s = ",1.8,3.5,7,10,14,18,21,24,28" } },
	{ "misc/logfile",    D_STRING, { .s = "~/gMFSK.log" } },
	{ "misc/picrxdir",   D_STRING, { .s = "~/gMFSK/" } },
	{ "misc/pictxdir",   D_STRING, { .s = "~/gMFSK/" } },
	{ "misc/fftwwisdom", D_STRING, { .s = "" } },
	{ "hell/font",       D_STRING, { .s = "FeldNarr 14" } },
	{ "hamlib/conf",     D_STRING, { .s = ""     } },
	{ "hamlib/port",     D_STRING, { .s = ""     } },

	{ "sound/8bit",	     D_BOOL,   { .b = FALSE  } },
	{ "sound/stereo",    D_BOOL,   { .b = FALSE  } },
	{ "sound/fulldup",   D_BOOL,   { .b = FALSE  } },
	{ "ptt/inverted",    D_BOOL,   { .b = FALSE  } },
	{ "misc/afc",        D_BOOL,   { .b = FALSE  } },
	{ "misc/squelch",    D_BOOL,   { .b = TRUE   } },
	{ "misc/reverse",    D_BOOL,   { .b = FALSE  } },
	{ "olivia/escape",   D_BOOL,   { .b = TRUE   } },
	{ "rtty/reverse",    D_BOOL,   { .b = TRUE   } },
	{ "rtty/msbfirst",   D_BOOL,   { .b = FALSE  } },
	{ "mt63/cwid",       D_BOOL,   { .b = FALSE  } },
	{ "mt63/escape",     D_BOOL,   { .b = TRUE   } },
	{ "hell/uppercase",  D_BOOL,   { .b = TRUE   } },
	{ "wf/direction",    D_BOOL,   { .b = FALSE  } },
	{ "hamlib/enable",   D_BOOL,   { .b = FALSE  } },
	{ "hamlib/wf",       D_BOOL,   { .b = TRUE   } },
	{ "hamlib/qsodata",  D_BOOL,   { .b = TRUE   } },
	{ "hamlib/ptt",      D_BOOL,   { .b = TRUE   } },

	{ "ptt/mode",        D_INT,    { .i = 2      } },
	{ "wf/mode",         D_INT,    { .i = 0      } },
	{ "wf/zoom",         D_INT,    { .i = 0      } },
	{ "wf/speed",        D_INT,    { .i = 1      } },
	{ "wf/window",       D_INT,    { .i = 1      } },
	{ "misc/druidlevel", D_INT,    { .i = 0      } },
	{ "misc/lastmode",   D_INT,    { .i = 0      } },
	{ "olivia/tones",    D_INT,    { .i = 3      } },
	{ "olivia/bw",       D_INT,    { .i = 3      } },
	{ "olivia/smargin",  D_INT,    { .i = 8      } },
	{ "olivia/sinteg",   D_INT,    { .i = 4      } },
	{ "rtty/bits",       D_INT,    { .i = 0      } },
	{ "rtty/parity",     D_INT,    { .i = 0      } },
	{ "rtty/stop",       D_INT,    { .i = 1      } },
	{ "mt63/bandwidth",  D_INT,    { .i = 1      } },
	{ "mt63/interleave", D_INT,    { .i = 1      } },
	{ "hamlib/rig",      D_INT,    { .i = 1      } },
	{ "hamlib/wf_res",   D_INT,    { .i = 1      } },
	{ "hamlib/speed",    D_INT,    { .i = 9600   } },

	{ "sound/srate",     D_FLOAT,  { .f = 8000.0 } },
	{ "sound/txoffset",  D_FLOAT,  { .f =   0.0  } },
	{ "sound/rxoffset",  D_FLOAT,  { .f =   0.0  } },
	{ "wf/reflevel",     D_FLOAT,  { .f = -10.0  } },
	{ "wf/ampspan",      D_FLOAT,  { .f = 100.0  } },
	{ "misc/txoffset",   D_FLOAT,  { .f =   0.0  } },
	{ "mfsk/squelch",    D_FLOAT,  { .f =  15.0  } },
	{ "olivia/squelch",  D_FLOAT,  { .f =   4.0  } },
	{ "rtty/shift",      D_FLOAT,  { .f = 170.0  } },
	{ "rtty/baud",       D_FLOAT,  { .f =  45.45 } },
	{ "rtty/squelch",    D_FLOAT,  { .f =  15.0  } },
	{ "throb/squelch",   D_FLOAT,  { .f =  15.0  } },
	{ "psk31/squelch",   D_FLOAT,  { .f =  15.0  } },
	{ "mt63/squelch",    D_FLOAT,  { .f =  15.0  } },
	{ "hell/bandwidth",  D_FLOAT,  { .f = 245.0  } },
	{ "hell/agcattack",  D_FLOAT,  { .f =   5.0  } },
	{ "hell/agcdecay",   D_FLOAT,  { .f = 500.0  } },
	{ "cw/squelch",      D_FLOAT,  { .f =  15.0  } },
	{ "cw/speed",        D_FLOAT,  { .f =  18.0  } },
	{ "cw/bandwidth",    D_FLOAT,  { .f =  75.0  } },
	{ "hamlib/cfreq",    D_FLOAT,  { .f = 1000.0  } },

	{ NULL }
};

/* ---------------------------------------------------------------------- */

gchar *conf_get_string(const gchar *key)
{
	struct conf_default *defs = conf_defs;
	gchar *s;

	g_return_val_if_fail(client, NULL);
	g_return_val_if_fail(key, NULL);

	s = gconf_client_get_string(client, mkkey(key), NULL);

	if (s == NULL) {
		while (defs->key) {
			if (!strcmp(defs->key, key) && defs->type == D_STRING)
				return g_strdup(defs->value.s);
			defs++;
		}
		return g_strdup("");
	}

	return g_strdup(s);
}

gboolean conf_get_bool(const gchar *key)
{
	struct conf_default *defs = conf_defs;
	GConfValue *value;

	g_return_val_if_fail(client, FALSE);
	g_return_val_if_fail(key, FALSE);

	value = gconf_client_get(client, mkkey(key), NULL);

	if (!value || value->type != GCONF_VALUE_BOOL) {
		while (defs->key) {
			if (!strcmp(defs->key, key) && defs->type == D_BOOL)
				return defs->value.b;
			defs++;
		}
		return FALSE;
	}

	return gconf_value_get_bool(value);
}

gint conf_get_int(const gchar *key)
{
	struct conf_default *defs = conf_defs;
	GConfValue *value;

	g_return_val_if_fail(client, 0);
	g_return_val_if_fail(key, 0);

	value = gconf_client_get(client, mkkey(key), NULL);

	if (!value || value->type != GCONF_VALUE_INT) {
		while (defs->key) {
			if (!strcmp(defs->key, key) && defs->type == D_INT)
				return defs->value.i;
			defs++;
		}
		return 0;
	}

	return gconf_value_get_int(value);
}

gdouble conf_get_float(const gchar *key)
{
	struct conf_default *defs = conf_defs;
	GConfValue *value;

	g_return_val_if_fail(client, 0.0);
	g_return_val_if_fail(key, 0.0);

	value = gconf_client_get(client, mkkey(key), NULL);

	if (!value || value->type != GCONF_VALUE_FLOAT) {
		while (defs->key) {
			if (!strcmp(defs->key, key) && defs->type == D_FLOAT)
				return defs->value.f;
			defs++;
		}
		return 0.0;
	}

	return gconf_value_get_float(value);
}

/* ---------------------------------------------------------------------- */

gchar *conf_get_filename(const gchar *key)
{
	GError *error = NULL;
	gchar *str, *name;

	str = conf_get_string(key);

	name = g_filename_from_utf8(str, -1, NULL, NULL, &error);

	if (name == NULL) {
		if (error != NULL) {
			errmsg(_("Error converting filename (%s=%s): %s"),
			       key,
			       str,
			       error->message);
			g_error_free(error);
		}
		return NULL;
	}

	g_free(str);

	if (name[0] == '~' && name[1] == G_DIR_SEPARATOR) {
		gchar *tmp;

		tmp = g_strdup_printf("%s%c%s",
				      g_get_home_dir(),
				      G_DIR_SEPARATOR,
				      name + 2);

		g_free(name);
		name = tmp;
	}

	return name;
}

/* ---------------------------------------------------------------------- */

void conf_set_tag_color(GtkTextTag *tag, const gchar *color)
{
	g_return_if_fail(color);

	g_object_set(G_OBJECT(tag), "foreground", color, NULL);
}

void conf_set_bg_color(GtkWidget *widget, const gchar *color)
{
	GdkColormap *map;
	GdkColor *clr;

	g_return_if_fail(color);

	clr = g_malloc(sizeof(GdkColor));	// FIXME: leaking memory!!
	gdk_color_parse(color, clr);
	map = gdk_colormap_get_system();

	if (!gdk_colormap_alloc_color(map, clr, FALSE, TRUE)) {
		g_warning(_("Couldn't allocate color"));
		return;
	}

	gtk_widget_modify_base(widget, GTK_STATE_NORMAL, clr);
}

void conf_set_font(GtkWidget *widget, const gchar *font)
{
	PangoFontDescription *font_desc;

	g_return_if_fail(font);

	font_desc = pango_font_description_from_string(font);
	gtk_widget_modify_font(widget, font_desc);
	pango_font_description_free(font_desc);
}

void conf_set_ptt(void)
{
	const gchar *dev;
	gboolean inv;
	gint mode;

	dev  = conf_get_string("ptt/dev");
	inv  = conf_get_bool("ptt/inverted");
	mode = conf_get_int("ptt/mode");

	ptt_init(dev, inv, mode);
}

void conf_set_qso_bands(const gchar *bandlist)
{
	GtkCombo *qsobandcombo;
	GList *list = NULL;
	gchar *bands, **bandp, **p;

	bands = g_strdup(bandlist);

	g_strdelimit(bands, ", \t\r\n", ',');
	bandp = g_strsplit(bands, ",", 64);

	for (p = bandp; *p; p++)
		list = g_list_append(list, (gpointer) *p);

	qsobandcombo = GTK_COMBO(lookup_widget(appwindow, "qsobandcombo"));
	gtk_combo_set_popdown_strings(qsobandcombo, list);

	g_free(bands);
	g_strfreev(bandp);
	g_list_free(list);
}

void conf_set_mfsk_config(void)
{
	trx_set_mfsk_parms(conf_get_float("mfsk/squelch"));
}

void conf_set_olivia_config(void)
{
	trx_set_olivia_parms(conf_get_float("olivia/squelch"),
			     conf_get_int("olivia/tones"),
			     conf_get_int("olivia/bw"),
			     conf_get_float("olivia/smargin"),
			     conf_get_float("olivia/sinteg"),
			     conf_get_bool("olivia/escape"));
}

void conf_set_rtty_config(void)
{
	trx_set_rtty_parms(conf_get_float("rtty/squelch"),
			   conf_get_float("rtty/shift"),
			   conf_get_float("rtty/baud"),
			   conf_get_int("rtty/bits"),
			   conf_get_int("rtty/parity"),
			   conf_get_int("rtty/stop"),
			   conf_get_bool("rtty/reverse"),
			   conf_get_bool("rtty/msbfirst"));
}

void conf_set_throb_config(void)
{
	trx_set_throb_parms(conf_get_float("throb/squelch"));
}

void conf_set_psk31_config(void)
{
	trx_set_psk31_parms(conf_get_float("psk31/squelch"));
}

void conf_set_mt63_config(void)
{
	trx_set_mt63_parms(conf_get_float("mt63/squelch"),
			   conf_get_int("mt63/bandwidth"),
			   conf_get_int("mt63/interleave"),
			   conf_get_bool("mt63/cwid"),
			   conf_get_bool("mt63/escape"));
}

void conf_set_hell_config(void)
{
	trx_set_hell_parms(conf_get_string("hell/font"),
			   conf_get_bool("hell/uppercase"),
			   conf_get_float("hell/bandwidth"),
			   conf_get_float("hell/agcattack"),
			   conf_get_float("hell/agcdecay"));
}

void conf_set_cw_config(void)
{
	trx_set_cw_parms(conf_get_float("cw/squelch"),
			 conf_get_float("cw/speed"),
			 conf_get_float("cw/bandwidth"));
}

void conf_set_waterfall_config(void)
{
	waterfall_set_ampspan(waterfall, conf_get_float("wf/ampspan"));
	waterfall_set_reflevel(waterfall, conf_get_float("wf/reflevel"));
	waterfall_set_mode(waterfall, conf_get_int("wf/mode"));
	waterfall_set_magnification(waterfall, conf_get_int("wf/zoom"));
	waterfall_set_speed(waterfall, conf_get_int("wf/speed"));
	waterfall_set_window(waterfall, conf_get_int("wf/window"));
	waterfall_set_dir(waterfall, conf_get_bool("wf/direction"));
}

void conf_set_hamlib_config(void)
{
#if WANT_HAMLIB
	hamlib_set_conf(conf_get_bool("hamlib/wf"),
			conf_get_bool("hamlib/qsodata"),
			conf_get_bool("hamlib/ptt"),
			conf_get_int("hamlib/wf_res"));
#endif
}

void conf_set_sound_config(void)
{
	snd_config_t cfg;
	guint flags;

	cfg.device = conf_get_filename("sound/dev");
	cfg.samplerate = conf_get_float("sound/srate");
	cfg.txoffset = conf_get_float("sound/txoffset");
	cfg.rxoffset = conf_get_float("sound/rxoffset");

	/* get testmode flags */
	sound_get_flags(&flags);
	flags &= SND_FLAG_TESTMODE_MASK;

	if (conf_get_bool("sound/8bit"))
		flags |= SND_FLAG_8BIT;

	if (conf_get_bool("sound/stereo"))
		flags |= SND_FLAG_STEREO;

	if (conf_get_bool("sound/fulldup"))
		flags |= SND_FLAG_FULLDUP;

	sound_set_conf(&cfg);
	sound_set_flags(flags);

	g_free(cfg.device);
}

/* ---------------------------------------------------------------------- */

static void tag_color_changed_notify(GConfClient *client,
				     guint id,
				     GConfEntry *entry,
				     gpointer data)
{
	GtkTextTag *tag = GTK_TEXT_TAG(data);
	GConfValue *value = gconf_entry_get_value(entry);

	if (value && value->type == GCONF_VALUE_STRING)
		conf_set_tag_color(tag, gconf_value_get_string(value));
}

static void bg_color_changed_notify(GConfClient *client,
				    guint id,
				    GConfEntry *entry,
				    gpointer data)
{
	GtkWidget *widget = GTK_WIDGET(data);
	GConfValue *value = gconf_entry_get_value(entry);

	if (value && value->type == GCONF_VALUE_STRING)
		conf_set_bg_color(widget, gconf_value_get_string(value));
}

static void font_changed_notify(GConfClient *client,
				guint id,
				GConfEntry *entry,
				gpointer data)
{
	GtkWidget *widget = GTK_WIDGET(data);
	GConfValue *value = gconf_entry_get_value(entry);

	if (value && value->type == GCONF_VALUE_STRING)
		conf_set_font(widget, gconf_value_get_string(value));
}

static void txoffset_changed_notify(GConfClient *client,
				    guint id,
				    GConfEntry *entry,
				    gpointer data)
{
	GConfValue *value = gconf_entry_get_value(entry);

	if (value && value->type == GCONF_VALUE_FLOAT)
		trx_set_txoffset(gconf_value_get_float(value));
}

static void qso_bands_changed_notify(GConfClient *client,
				     guint id,
				     GConfEntry *entry,
				     gpointer data)
{
	GConfValue *value = gconf_entry_get_value(entry);

	if (value && value->type == GCONF_VALUE_STRING)
		conf_set_qso_bands(gconf_value_get_string(value));
}

static void hamlib_changed_notify(GConfClient *client,
				  guint id,
				  GConfEntry *entry,
				  gpointer data)
{
#if WANT_HAMLIB
	GConfValue *value = gconf_entry_get_value(entry);

	if (value && value->type == GCONF_VALUE_BOOL) {
		if (gconf_value_get_bool(value))
			hamlib_init();
		else
			hamlib_close();
	}
#endif
}

/* ---------------------------------------------------------------------- */

static void generic_config_changed_notify(GConfClient *client,
					  guint id,
					  GConfEntry *entry,
					  gpointer data)
{
	void (*func) (void) = data;

	func();
}

/* ---------------------------------------------------------------------- */

void conf_init(void)
{
	g_return_if_fail(!client);

	client = gconf_client_get_default();

	if (!client) {
		g_warning(_("GConf client init failed!!!\n"));
		exit(1);
	}

	gconf_client_add_dir(client, "/apps/" PACKAGE,
			     GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

void conf_load(void)
{
	GtkWidget *w;
	const gchar *s;
	gboolean b;
	gfloat f;

	g_return_if_fail(client);

	s = conf_get_string("colors/hl");
	conf_set_tag_color(hltag, s);
	gconf_client_notify_add(client,
				mkkey("colors/hl"),
				tag_color_changed_notify,
				hltag,
				NULL, NULL);

	s = conf_get_string("colors/rx");
	conf_set_tag_color(rxtag, s);
	gconf_client_notify_add(client,
				mkkey("colors/rx"),
				tag_color_changed_notify,
				rxtag,
				NULL, NULL);

	s = conf_get_string("colors/tx");
	conf_set_tag_color(txtag, s);
	gconf_client_notify_add(client,
				mkkey("colors/tx"),
				tag_color_changed_notify,
				txtag,
				NULL, NULL);


	s = conf_get_string("colors/rxwin");
	w = lookup_widget(appwindow, "rxtext");
	conf_set_bg_color(w, s);
	gconf_client_notify_add(client,
				mkkey("colors/rxwin"),
				bg_color_changed_notify,
				w,
				NULL, NULL);

	s = conf_get_string("colors/txwin");
	w = lookup_widget(appwindow, "txtext");
	conf_set_bg_color(w, s);
	gconf_client_notify_add(client,
				mkkey("colors/txwin"),
				bg_color_changed_notify,
				w,
				NULL, NULL);


	s = conf_get_string("fonts/rxfont");
	w = lookup_widget(appwindow, "rxtext");
	conf_set_font(w, s);
	gconf_client_notify_add(client,
				mkkey("fonts/rxfont"),
				font_changed_notify,
				w,
				NULL, NULL);

	s = conf_get_string("hell/font");
	w = lookup_widget(appwindow, "txentry");
	conf_set_font(w, s);
	gconf_client_notify_add(client,
				mkkey("hell/font"),
				font_changed_notify,
				w,
				NULL, NULL);

	s = conf_get_string("fonts/txfont");
	w = lookup_widget(appwindow, "txtext");
	conf_set_font(w, s);
	gconf_client_notify_add(client,
				mkkey("fonts/txfont"),
				font_changed_notify,
				w,
				NULL, NULL);


	f = conf_get_float("misc/txoffset");
	trx_set_txoffset(f);
	gconf_client_notify_add(client,
				mkkey("misc/txoffset"),
				txoffset_changed_notify,
				NULL,
				NULL, NULL);


	conf_set_ptt();
	gconf_client_notify_add(client,
				mkkey("ptt"),
				generic_config_changed_notify,
				conf_set_ptt,
				NULL, NULL);


	s = conf_get_qsobands();
	conf_set_qso_bands(s);
	gconf_client_notify_add(client,
				mkkey("misc/bands"),
				qso_bands_changed_notify,
				NULL,
				NULL, NULL);


	conf_set_mfsk_config();
	gconf_client_notify_add(client,
				mkkey("mfsk"),
				generic_config_changed_notify,
				conf_set_mfsk_config,
				NULL, NULL);

	conf_set_olivia_config();
	gconf_client_notify_add(client,
				mkkey("olivia"),
				generic_config_changed_notify,
				conf_set_olivia_config,
				NULL, NULL);

	conf_set_rtty_config();
	gconf_client_notify_add(client,
				mkkey("rtty"),
				generic_config_changed_notify,
				conf_set_rtty_config,
				NULL, NULL);

	conf_set_throb_config();
	gconf_client_notify_add(client,
				mkkey("throb"),
				generic_config_changed_notify,
				conf_set_throb_config,
				NULL, NULL);

	conf_set_psk31_config();
	gconf_client_notify_add(client,
				mkkey("psk31"),
				generic_config_changed_notify,
				conf_set_psk31_config,
				NULL, NULL);

	conf_set_mt63_config();
	gconf_client_notify_add(client,
				mkkey("mt63"),
				generic_config_changed_notify,
				conf_set_mt63_config,
				NULL, NULL);

	conf_set_hell_config();
	gconf_client_notify_add(client,
				mkkey("hell"),
				generic_config_changed_notify,
				conf_set_hell_config,
				NULL, NULL);

	conf_set_cw_config();
	gconf_client_notify_add(client,
				mkkey("cw"),
				generic_config_changed_notify,
				conf_set_cw_config,
				NULL, NULL);

	conf_set_waterfall_config();
	gconf_client_notify_add(client,
				mkkey("wf"),
				generic_config_changed_notify,
				conf_set_waterfall_config,
				NULL, NULL);


	gconf_client_notify_add(client,
				mkkey("hamlib/enable"),
				hamlib_changed_notify,
				NULL,
				NULL, NULL);

	conf_set_hamlib_config();
	gconf_client_notify_add(client,
				mkkey("hamlib"),
				generic_config_changed_notify,
				conf_set_hamlib_config,
				NULL, NULL);


	conf_set_sound_config();
	gconf_client_notify_add(client,
				mkkey("sound"),
				generic_config_changed_notify,
				conf_set_sound_config,
				NULL, NULL);


	/*
	 * Load misc settings
	 */
	b = conf_get_bool("misc/afc");
	w = lookup_widget(appwindow, "afcbutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), b);

	b = conf_get_bool("misc/squelch");
	w = lookup_widget(appwindow, "squelchbutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), b);

	b = conf_get_bool("misc/reverse");
	w = lookup_widget(appwindow, "reversebutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), b);

	b = conf_get_bool("misc/log");
	w = find_menu_item("_File/_Log to file");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), b);
}

void conf_clear(void)
{
	GtkWidget *w;
	gboolean b;

	g_return_if_fail(client);

	/*
	 * Save misc settings
	 */
	w = lookup_widget(appwindow, "afcbutton");
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	gconf_client_set_bool(client, mkkey("misc/afc"), b, NULL);

	w = lookup_widget(appwindow, "squelchbutton");
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	gconf_client_set_bool(client, mkkey("misc/squelch"), b, NULL);

	w = lookup_widget(appwindow, "reversebutton");
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	gconf_client_set_bool(client, mkkey("misc/reverse"), b, NULL);

	w = find_menu_item("_File/_Log to file");
	b = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	gconf_client_set_bool(client, mkkey("misc/log"), b, NULL);

	/*
	 * Clear GConfClient
	 */
	g_object_unref(G_OBJECT(client));
	client = NULL;
}

/* ---------------------------------------------------------------------- */
