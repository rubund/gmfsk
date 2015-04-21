/*
 *    callbacks.h  --  GUI callbacks
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

/* ---------------------------------------------------------------------- */

#include "waterfall.h"
#include "miniscope.h"
#include "gtkdial.h"
#include "papertape.h"

/* ---------------------------------------------------------------------- */

gboolean
on_appwindow_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_send_file1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_log_to_file1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear_tx_window1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear_rx_window1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_mfsk1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_mfsk2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_rtty1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_throb1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_throb2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_throb4_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_bpsk31_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_qpsk31_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_feldhell1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig2_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig3_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig4_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig5_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig6_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig7_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig8_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig9_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig10_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig11_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macroconfig12_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pausebutton_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_rxbutton_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_txbutton_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_abortbutton_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_tunebutton_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_qsoentry_changed                    (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_logbutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_newbutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_rxtext_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_waterfall_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_freqspinbutton_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_refspinbutton_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_ampspinbutton_changed              (GtkEditable     *editable,
                                        gpointer         user_data);


void
on_afcbutton_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_squelchbutton_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_reversebutton_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);


gboolean
on_txfileselection_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_txfileselect_ok_button_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_txfileselect_cancel_button_clicked  (GtkButton       *button,
                                        gpointer         user_data);

void
on_wfpause_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_macrobutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton1_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton2_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton2_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton3_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton3_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton4_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton4_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton5_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton5_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton6_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton6_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton7_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton7_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton8_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton8_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton9_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton9_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton10_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton10_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton11_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton11_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_macrobutton12_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_macrobutton12_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_rxtext_populate_popup               (GtkTextView     *textview,
                                        GtkMenu         *menu,
                                        gpointer         user_data);

void
on_txtext_populate_popup               (GtkTextView     *textview,
                                        GtkMenu         *menu,
                                        gpointer         user_data);

void
on_mt63_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_confrestartbutton_clicked           (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_rxtext_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_wfspeed_half_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_wfspeed_normal_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_wfspeed_double_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_modeeventbox_button_press_event     (GtkWidget       *eventwidget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_contents1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_wfproperties_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_txtext_delete_from_cursor           (GtkTextView     *textview,
                                        GtkDeleteType    type,
                                        gint             count,
                                        gpointer         user_data);


void
on_waterfall1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_spectrum1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_scope1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_psk63_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cw1_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_olivia1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_qsybutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_waterfall2_color_activate 		(GtkMenuItem *menuitem, gpointer user_data);

void
on_waterfall2_grayscale_activate 	(GtkMenuItem *menuitem, gpointer user_data);

void
on_waterfall3x1_activate	(GtkMenuItem *menuitem, gpointer user_data);

void
on_waterfall3x2_activate	(GtkMenuItem *menuitem, gpointer user_data);

void
on_waterfall3x4_activate	(GtkMenuItem *menuitem, gpointer user_data);

