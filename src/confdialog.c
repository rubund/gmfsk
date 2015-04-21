/*
 *    confdialog.c  --  Configuration dialog
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

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "main.h"
#include "confdialog.h"
#include "conf.h"
#include "hamlib.h"

static GtkWidget *ConfDialog = NULL;

/* ---------------------------------------------------------------------- */

enum {
	CATEGORY_COLUMN = 0,
	PAGE_NUM_COLUMN,
	NUM_COLUMNS
};

typedef struct _TreeItem TreeItem;

struct _TreeItem {
	const gchar	*label;
	gint		number;
	TreeItem	*children;
};

/* tree data */
static TreeItem treeitem_general[] = {
	{N_("Station info"), 1,  NULL },
	{N_("Date & Time"),  2,  NULL },
	{N_("Bands"),        3,  NULL },
	{N_("Files & Dirs"), 4,  NULL },
	{ NULL }
};

static TreeItem treeitem_appearance[] = {
	{N_("Colors"),       5,  NULL },
	{N_("Fonts"),        6,  NULL },
	{ NULL }
};

static TreeItem treeitem_modems[] = {
	{N_("All"),          7,  NULL },
	{N_("MFSK"),         8,  NULL },
	{N_("OLIVIA"),       9,  NULL },
	{N_("RTTY"),         10, NULL },
	{N_("THROB"),        11, NULL },
	{N_("PSK31"),        12, NULL },
	{N_("MT63"),         13, NULL },
	{N_("HELL"),         14, NULL },
	{N_("CW"),           15, NULL },
	{ NULL }
};

static TreeItem treeitem_waterfall[] = {
	{N_("Settings"),     16, NULL },
	{ NULL }
};

static TreeItem treeitem_devices[] = {
	{N_("Sound"),        17, NULL },
	{N_("PTT"),          18, NULL },
	{ NULL }
};

#if WANT_HAMLIB
static TreeItem treeitem_hamlib[] = {
	{N_("Settings"),     19, NULL },
	{N_("Features"),     20, NULL },
	{ NULL }
};
#endif

static TreeItem treeitem_toplevel[] = {
	{N_("General"),      0,  treeitem_general    },
	{N_("Appearance"),   0,  treeitem_appearance },
	{N_("Modems"),       0,  treeitem_modems     },
	{N_("Waterfall"),    0,  treeitem_waterfall  },
	{N_("Devices"),      0,  treeitem_devices    },
#if WANT_HAMLIB
	{N_("Hamlib"),       0,  treeitem_hamlib     },
#endif
	{ NULL }
};

static GtkTreeStore *confdialog_build_tree_store(void)
{
	GtkTreeStore *store;
	GtkTreeIter iter, childiter;
	TreeItem *toplevel, *child;

	store = gtk_tree_store_new(NUM_COLUMNS,
				   G_TYPE_STRING,
				   G_TYPE_INT);

	toplevel = treeitem_toplevel;

	while (toplevel->label) {
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter,
				   CATEGORY_COLUMN, _(toplevel->label),
				   PAGE_NUM_COLUMN, toplevel->number, -1);

		child = toplevel->children;

		while (child->label) {
			gtk_tree_store_append(store, &childiter, &iter);
			gtk_tree_store_set(store, &childiter,
					   CATEGORY_COLUMN, _(child->label),
					   PAGE_NUM_COLUMN, child->number, -1);

			child++;
		}

		toplevel++;
	}

	return store;
}

/* ---------------------------------------------------------------------- */

typedef enum {
	T_ENTRY,
	T_ENTRY_FLOAT,
	T_COLORPICKER,
	T_FONTPICKER,
	T_OPTIONMENU,
	T_SPINBUTTON,
	T_TOGGLEBUTTON,
	T_RADIOBUTTON,
	T_SCALE
} conf_type_t;

#define	CONFITEM(c)	((ConfItem *) (c))

typedef struct _ConfItem ConfItem;

struct _ConfItem {
	const gchar	*path;
	conf_type_t	type;
	const gchar	*widgetname;
	gint		radiodata;
};

static ConfItem confitems[] = {
	{"info/mycall",     T_ENTRY,        "callentry"},
	{"info/myname",     T_ENTRY,        "nameentry"},
	{"info/myqth",      T_ENTRY,        "qthentry"},
	{"info/myloc",      T_ENTRY,        "locentry"},
	{"info/myemail",    T_ENTRY,        "emailentry"},

	{"colors/tx",       T_COLORPICKER,  "txcolorpicker"},
	{"colors/rx",       T_COLORPICKER,  "rxcolorpicker"},
	{"colors/hl",       T_COLORPICKER,  "hlcolorpicker"},
	{"colors/txwin",    T_COLORPICKER,  "txwincolorpicker"},
	{"colors/rxwin",    T_COLORPICKER,  "rxwincolorpicker"},

	{"fonts/txfont",    T_FONTPICKER,   "txfontpicker"},
	{"fonts/rxfont",    T_FONTPICKER,   "rxfontpicker"},

	{"sound/dev",       T_ENTRY,        "soundentry"},
	{"sound/8bit",      T_TOGGLEBUTTON, "eightbitcheckbutton"},
	{"sound/stereo",    T_TOGGLEBUTTON, "stereocheckbutton"},
	{"sound/fulldup",   T_TOGGLEBUTTON, "rwcheckbutton"},
	{"sound/srate",     T_SPINBUTTON,   "ratespinbutton"},
	{"sound/txoffset",  T_SPINBUTTON,   "txppmspinbutton"},
	{"sound/rxoffset",  T_SPINBUTTON,   "rxppmspinbutton"},

	{"ptt/dev",         T_ENTRY,        "pttentry"},
	{"ptt/inverted",    T_TOGGLEBUTTON, "pttinvcheckbutton"},
	{"ptt/mode",        T_RADIOBUTTON,  "pttrtsradiobutton",  0},
	{"ptt/mode",        T_RADIOBUTTON,  "pttdtrradiobutton",  1},
	{"ptt/mode",        T_RADIOBUTTON,  "pttbothradiobutton", 2},

	{"wf/reflevel",     T_SCALE,        "wfrefvscale"},
	{"wf/ampspan",      T_SCALE,        "wfampvscale"},
	{"wf/mode",         T_OPTIONMENU,   "wfmodeoptionmenu"},
	{"wf/zoom",         T_OPTIONMENU,   "wfzoomoptionmenu"},
	{"wf/speed",        T_OPTIONMENU,   "wfspeedoptionmenu"},
	{"wf/window",       T_OPTIONMENU,   "wfwindowoptionmenu"},
	{"wf/direction",    T_TOGGLEBUTTON, "wfdircheckbutton"},

	{"misc/datefmt",    T_ENTRY,        "dateentry"},
	{"misc/timefmt",    T_ENTRY,        "timeentry"},
	{"misc/bands",      T_ENTRY,        "qsobandsentry"},
	{"misc/logfile",    T_ENTRY,        "logfileentry"},
	{"misc/pictxdir",   T_ENTRY,        "txpicdirentry"},
	{"misc/picrxdir",   T_ENTRY,        "rxpicdirentry"},
	{"misc/txoffset",   T_SPINBUTTON,   "txoffsetspinbutton"},

	{"mfsk/squelch",    T_SCALE,        "mfsksqlvscale"},

	{"olivia/tones",    T_OPTIONMENU,   "oliviatonesoptionmenu"},
	{"olivia/bw",       T_OPTIONMENU,   "oliviabwoptionmenu"},
	{"olivia/escape",   T_TOGGLEBUTTON, "oliviaesccheckbutton"},
	{"olivia/squelch",  T_SCALE,        "oliviasqlvscale"},
	{"olivia/smargin",  T_SPINBUTTON,   "oliviasmargspinbutton"},
	{"olivia/sinteg",   T_SPINBUTTON,   "oliviasintegspinbutton"},

	{"rtty/shift",      T_ENTRY_FLOAT,  "rttyshiftentry"},
	{"rtty/baud",       T_ENTRY_FLOAT,  "rttybaudentry"},
	{"rtty/bits",       T_OPTIONMENU,   "bitsoptionmenu"},
	{"rtty/parity",     T_OPTIONMENU,   "parityoptionmenu"},
	{"rtty/stop",       T_OPTIONMENU,   "stopoptionmenu"},
	{"rtty/reverse",    T_TOGGLEBUTTON, "reversecheckbutton"},
	{"rtty/msbfirst",   T_TOGGLEBUTTON, "msbcheckbutton"},
	{"rtty/squelch",    T_SCALE,        "rttysqlvscale"},

	{"throb/squelch",   T_SCALE,        "throbsqlvscale"},

	{"psk31/squelch",   T_SCALE,        "psk31sqlvscale"},

	{"mt63/bandwidth",  T_OPTIONMENU,   "bwoptionmenu"},
	{"mt63/interleave", T_OPTIONMENU,   "ileaveoptionmenu"},
	{"mt63/cwid",       T_TOGGLEBUTTON, "cwidcheckbutton"},
	{"mt63/escape",     T_TOGGLEBUTTON, "mt63esccheckbutton"},
	{"mt63/squelch",    T_SCALE,        "mt63sqlvscale"},

	{"hell/uppercase",  T_TOGGLEBUTTON, "hellcheckbutton"},
	{"hell/font",       T_FONTPICKER,   "hellfontpicker"},
	{"hell/bandwidth",  T_SCALE,        "hellfiltvscale"},
	{"hell/agcattack",  T_SCALE,        "hellagc1vscale"},
	{"hell/agcdecay",   T_SCALE,        "hellagc2vscale"},

	{"cw/squelch",      T_SCALE,        "cwsqlvscale"},
	{"cw/speed",        T_SCALE,        "cwspeedvscale"},
	{"cw/bandwidth",    T_SCALE,        "cwbwvscale"},

	{"hamlib/enable",   T_TOGGLEBUTTON, "hamlibcheckbutton"},
	{"hamlib/port",     T_ENTRY,        "hamlibentry"},
	{"hamlib/conf",     T_ENTRY,        "hamlibconfentry"},
	{"hamlib/wf",       T_TOGGLEBUTTON, "hlwfcheckbutton"},
	{"hamlib/qsodata",  T_TOGGLEBUTTON, "hlqsocheckbutton"},
	{"hamlib/ptt",      T_TOGGLEBUTTON, "hlpttcheckbutton"},
	{"hamlib/wf_res",   T_OPTIONMENU,   "hlresoptionmenu"},
	{"hamlib/cfreq",    T_SPINBUTTON,   "qsyspinbutton"},

	{ NULL }
};

static void confdialog_entry_changed(GtkEditable *editable, gpointer data)
{
	gchar *s;
	gdouble f;

	s = gtk_editable_get_chars(editable, 0, -1);
	f = atof(s);

	if (CONFITEM(data)->type == T_ENTRY_FLOAT)
		conf_set_float(CONFITEM(data)->path, f);
	else
		conf_set_string(CONFITEM(data)->path, s);
}

static void confdialog_spinbutton_changed(GtkSpinButton *spin, gpointer data)
{
	gdouble f;

	f = gtk_spin_button_get_value_as_float(spin);
	conf_set_float(CONFITEM(data)->path, f);
}

static void confdialog_colorpicker_set(GnomeColorPicker *colorpicker,
                                       guint arg1,
                                       guint arg2,
                                       guint arg3,
                                       guint arg4,
                                       gpointer data)
{
	gchar *s = g_strdup_printf("#%02X%02X%02X",
				   (arg1 & 0xff00) >> 8,
				   (arg2 & 0xff00) >> 8,
				   (arg3 & 0xff00) >> 8);

	conf_set_string(CONFITEM(data)->path, s);
	g_free(s);
}

static void color_picker_set_string(GnomeColorPicker *cp, const gchar *str)
{
	GdkColor clr;

	gdk_color_parse(str, &clr);
	gnome_color_picker_set_i16(cp, clr.red, clr.green, clr.blue, 0);
}

static void confdialog_fontpicker_set(GnomeFontPicker *fontpicker,
                                      gchar *arg,
                                      gpointer data)
{
	conf_set_string(CONFITEM(data)->path, arg);
}

static void confdialog_menu_selected(GtkMenuShell *menushell, gpointer data)
{
	GtkWidget *activeitem;
	gint index;

	activeitem = gtk_menu_get_active(GTK_MENU(menushell));
	index = g_list_index(menushell->children, activeitem);

	conf_set_int(CONFITEM(data)->path, index);
}

static void confdialog_button_toggled(GtkToggleButton *button, gpointer data)
{
	conf_set_bool(CONFITEM(data)->path, button->active);
}

static void confdialog_radio_button_toggled(GtkToggleButton *button, gpointer data)
{
	if (button->active)
		conf_set_int(CONFITEM(data)->path, CONFITEM(data)->radiodata);
}

static void confdialog_scale_changed(GtkAdjustment *adjustment, gpointer data)
{
	conf_set_float(CONFITEM(data)->path, adjustment->value);
}

/* ---------------------------------------------------------------------- */

#if WANT_HAMLIB

static void confdialog_speed_selected(GtkMenuShell *menushell, gpointer data)
{
	GtkWidget *activeitem;
	gint i;

	activeitem = gtk_menu_get_active(GTK_MENU(menushell));
	i = g_list_index(menushell->children, activeitem);
	i = riglist_get_speed_from_index(conf_get_int("hamlib/rig"), i);
	conf_set_int("hamlib/speed", i);
}

static void fill_speeds_menu(GtkWidget *dialog)
{
	GtkWidget *menu, *menuitem, *optionmenu;
	gint i, s, rigid, len, *speeds;
	gchar *string;

	rigid = conf_get_int("hamlib/rig");

	len = riglist_get_speeds(rigid, &speeds);

	optionmenu = lookup_widget(dialog, "hamlibspeedoptionmenu");
	gtk_option_menu_remove_menu(GTK_OPTION_MENU(optionmenu));

	menu = gtk_menu_new();

	for (i = 0; i < len; i++) {
		string = g_strdup_printf("%d", speeds[i]);
		menuitem = gtk_menu_item_new_with_label(string);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		gtk_widget_show(menuitem);
		g_free(string);
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);

	s = conf_get_int("hamlib/speed");
	i = riglist_get_index_from_speed(rigid, s);
	s = riglist_get_speed_from_index(rigid, i);
	conf_set_int("hamlib/speed", s);

	gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), i);

	g_signal_connect((gpointer) (GTK_OPTION_MENU(optionmenu)->menu),
			 "deactivate",
			 G_CALLBACK(confdialog_speed_selected),
			 NULL);

	g_free(speeds);
}

static void confdialog_rig_selected(GtkMenuShell *menushell, gpointer data)
{
	GtkWidget *activeitem;
	gint i;

	activeitem = gtk_menu_get_active(GTK_MENU(menushell));
	i = g_list_index(menushell->children, activeitem);
	i = riglist_get_id_from_index(i);
	conf_set_int("hamlib/rig", i);

	fill_speeds_menu(GTK_WIDGET(data));
}

static void fill_rigs_menu(GtkWidget *dialog)
{
	GtkWidget *menu, *menuitem, *optionmenu;
	GPtrArray *list;
	gchar *string;
	gint i, len;

	len = riglist_get_names(&list);

	menu = gtk_menu_new();

	for (i = 0; i < len; i++) {
		string = g_ptr_array_index(list, i);
		menuitem = gtk_menu_item_new_with_label(string);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		gtk_widget_show(menuitem);
	}

	optionmenu = lookup_widget(dialog, "hamliboptionmenu");
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);

	i = riglist_get_index_from_id(conf_get_int("hamlib/rig"));
	gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), i);

	g_signal_connect((gpointer) (GTK_OPTION_MENU(optionmenu)->menu),
			 "deactivate",
			 G_CALLBACK(confdialog_rig_selected),
			 (gpointer) dialog);
}

#endif

/* ---------------------------------------------------------------------- */

static void confdialog_fill(GtkWidget *dialog)
{
	ConfItem *item = confitems;
	GtkWidget *w;
	GtkAdjustment *a;
	gpointer obj;
	gchar *sig;
	GCallback cb;
	gchar *s;
	gint i;
	gdouble f;
	gboolean b;

#if WANT_HAMLIB
	fill_rigs_menu(dialog);
	fill_speeds_menu(dialog);
#endif

	for (item = confitems; item->path; item++) {
		obj = NULL;
		sig = NULL;
		cb = NULL;

		w = lookup_widget(dialog, item->widgetname);

		switch (item->type) {
		case T_ENTRY:
			s = conf_get_string(item->path);
			gtk_entry_set_text(GTK_ENTRY(w), s);
			g_free(s);

			obj = (gpointer) w;
			sig = "changed";
			cb = G_CALLBACK(confdialog_entry_changed);
			break;

		case T_ENTRY_FLOAT:
			f = conf_get_float(item->path);
			s = g_strdup_printf("%g", f);
			gtk_entry_set_text(GTK_ENTRY(w), s);
			g_free(s);

			obj = (gpointer) w;
			sig = "changed";
			cb = G_CALLBACK(confdialog_entry_changed);
			break;

		case T_COLORPICKER:
			s = conf_get_string(item->path);
			color_picker_set_string(GNOME_COLOR_PICKER(w), s);
			g_free(s);

			obj = (gpointer) w;
			sig = "color_set";
			cb = G_CALLBACK(confdialog_colorpicker_set);
			break;

		case T_FONTPICKER:
			s = conf_get_string(item->path);
			gnome_font_picker_set_font_name(GNOME_FONT_PICKER(w), s);
			gnome_font_picker_set_mode(GNOME_FONT_PICKER(w), GNOME_FONT_PICKER_MODE_FONT_INFO);
			g_free(s);

			obj = (gpointer) w;
			sig = "font_set";
			cb = G_CALLBACK(confdialog_fontpicker_set);
			break;

		case T_SPINBUTTON:
			f = conf_get_float(item->path);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), f);

			obj = (gpointer) w;
			sig = "value-changed";
			cb = G_CALLBACK(confdialog_spinbutton_changed);
			break;

		case T_OPTIONMENU:
			i = conf_get_int(item->path);
			gtk_option_menu_set_history(GTK_OPTION_MENU(w), i);

			obj = (gpointer) (GTK_OPTION_MENU(w)->menu);
			sig = "deactivate";
			cb = G_CALLBACK(confdialog_menu_selected);
			break;

		case T_TOGGLEBUTTON:
			b = conf_get_bool(item->path);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), b);

			obj = (gpointer) w;
			sig = "toggled";
			cb = G_CALLBACK(confdialog_button_toggled);
			break;

		case T_RADIOBUTTON:
			if (item->radiodata == conf_get_int(item->path))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);

			obj = (gpointer) w;
			sig = "toggled";
			cb = G_CALLBACK(confdialog_radio_button_toggled);
			break;

		case T_SCALE:
			f = conf_get_float(item->path);
			a = gtk_range_get_adjustment(GTK_RANGE(w));
			gtk_adjustment_set_value(a, f);

			obj = (gpointer) a;
			sig = "value-changed";
			cb = G_CALLBACK(confdialog_scale_changed);
			break;
		}

		g_signal_connect(obj, sig, cb, (gpointer) item);
	}
}


/* ---------------------------------------------------------------------- */

static void confdialog_respose_callback(GtkDialog *dialog,
					gint id,
					gpointer data)
{
	GError *error = NULL;

	if (id == GTK_RESPONSE_HELP) {
		gnome_help_display("gmfsk.xml", "gmfsk-prefs", &error);

		if (error != NULL) {
			g_warning(error->message);
			g_error_free(error);
		}

		return;
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_set_data(G_OBJECT(appwindow), "configdialog", NULL);
	ConfDialog = NULL;
}

static void confdialog_selection_changed_callback(GtkTreeSelection *selection,
						  gpointer data)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkWidget *notebook;
	GValue value = { 0 };
	gint page;

	view = gtk_tree_selection_get_tree_view(selection);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	gtk_tree_model_get_value(model, &iter, PAGE_NUM_COLUMN, &value);
	page = g_value_get_int(&value);
	g_value_unset(&value);

	notebook = lookup_widget(ConfDialog, "confnotebook");
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);

	if (page == 0) {
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_expand_row(view, path, FALSE);
	}
}

GtkWidget *confdialog_init(void)
{
	GtkTreeView *treeview;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	gint offset;

	ConfDialog = create_confdialog();

	confdialog_fill(ConfDialog);

	g_signal_connect(G_OBJECT(ConfDialog), "response",
			 G_CALLBACK(confdialog_respose_callback),
			 NULL);

	model = GTK_TREE_MODEL(confdialog_build_tree_store());

	treeview = GTK_TREE_VIEW(lookup_widget(ConfDialog, "conftreeview"));
	gtk_tree_view_set_model(treeview, model);

	g_object_unref(G_OBJECT(model));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "xalign", 0.0, NULL);

	offset = gtk_tree_view_insert_column_with_attributes(treeview,
							     -1,
							     _("Categories"),
							     renderer,
							     "text",
							     CATEGORY_COLUMN,
							     NULL);

	column = gtk_tree_view_get_column(treeview, offset - 1);
	gtk_tree_view_column_set_clickable(column, FALSE);

	g_signal_connect(G_OBJECT(selection), "changed",
			 G_CALLBACK(confdialog_selection_changed_callback),
			 NULL);

	return ConfDialog;
}

void confdialog_select_node(const char *confpath)
{
	TreeItem *item;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar **confpathv, **v, pathstr[8] = "";

	confpathv = g_strsplit(confpath, ":", 0);

	item = treeitem_toplevel;
	v = confpathv;

	while (item && *v) {
		gint idx = 0;

		while (item->label && strcmp(item->label, *v))
			item++, idx++;

		g_snprintf(pathstr + strlen(pathstr),
			   sizeof(pathstr) - strlen(pathstr),
			   "%d:",
			   idx);

		item = item->children;
		v++;
	}

	if (pathstr[strlen(pathstr) - 1] == ':')
		pathstr[strlen(pathstr) - 1] = 0;

	g_strfreev(confpathv);

	if (!ConfDialog && !confdialog_init()) {
		g_warning(_("confdialog_select_node: ConfDialog init failed\n"));
		return;
	}

	gtk_window_present(GTK_WINDOW(ConfDialog));

	view = GTK_TREE_VIEW(lookup_widget(ConfDialog, "conftreeview"));
	model = gtk_tree_view_get_model(view);
	selection = gtk_tree_view_get_selection(view);

	gtk_tree_view_collapse_all(view);

	if (!gtk_tree_model_get_iter_from_string(model, &iter, pathstr)) {
		g_warning(_("confdialog_select_node: tree path '%s' not found (%s)\n"), confpath, pathstr);
		return;
	}

	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_view_expand_to_path(view, path);
	gtk_tree_path_free(path);

	gtk_tree_selection_select_iter(selection, &iter);
}

/* ---------------------------------------------------------------------- */
