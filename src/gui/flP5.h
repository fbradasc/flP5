// generated by Fast Light User Interface Designer (fluid) version 1.0104

#ifndef flP5_h
#define flP5_h
#include <FL/Fl.H>
/* Copyright (C) 2003  Francesco Bradascio <fbradasc@yahoo.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "proto_flP5.h"
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/fl_ask.H>
extern Fl_Choice *ch_pinNumber[9];
extern Fl_Check_Button *tb_pinInvert[9];
extern Fl_Box *bx_pinName[9];
extern Fl_Input *tx_devParam[13];
#include <FL/Fl_RaiseButton.H>
#include <FL/Fl_Double_Window.H>
extern Fl_Double_Window *flP5;
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
extern Fl_Group *t_devcfg;
extern Fl_Group *g_devCfgNewEditCopy;
#include <FL/Fl_Box.H>
extern Fl_Group *g_devcfg;
#include <FL/Fl_Input.H>
extern Fl_Input *tx_devName;
#include <FL/Fl_Sorted_Choice.H>
extern Fl_Sorted_Choice *ch_devProgSpec;
extern Fl_Group *g_devgencfg;
extern Fl_Sorted_Choice *ch_devMemType;
extern Fl_Group *g_devcfgwords;
extern Fl_Group *g_devConfigWordsEdit;
extern Fl_Input *tx_devCfgWord[11];
extern Fl_Group *g_devConfigWordsToolBar;
extern Fl_Group *g_devConfigWordsNewEditCopy;
#include <FL/Fl_Hold_Browser.H>
extern Fl_Hold_Browser *ls_devConfigWords;
extern Fl_Group *g_devmiscellanea;
#include <FL/Fl_Check_Button.H>
extern Fl_Check_Button *tb_devExperimental;
extern Fl_Group *t_progcfg;
extern Fl_Group *g_progCfgNewEditCopy;
extern Fl_Group *g_progcfg;
extern Fl_Input *tx_programmerName;
extern Fl_Check_Button *tb_vddMinCond[3];
extern Fl_Check_Button *tb_vddProgCond[3];
extern Fl_Check_Button *tb_vddMaxCond[3];
extern Fl_Check_Button *tb_vppOffCond;
extern Fl_Check_Button *tb_saVddVppControl;
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Progress.H>
extern Fl_Progress *p_progress;
extern Fl_Sorted_Choice *ch_devices;
extern Fl_Sorted_Choice *ch_programmers;
#include <FL/Fl_Browser.H>
extern Fl_Browser *ls_memdump;
#include <FL/Fl_Pack.H>
extern Fl_Pack *p_toolbar;
extern Fl_RaiseButton *pb_operation[7];
Fl_Double_Window* make_flP5();
extern Fl_Menu_Item menu_ch_devProgSpec[];
extern Fl_Menu_Item menu_ch_devMemType[];
extern Fl_Menu_Item menu_[];
extern Fl_Menu_Item menu_1[];
extern Fl_Menu_Item menu_2[];
extern Fl_Menu_Item menu_3[];
extern Fl_Menu_Item menu_4[];
extern Fl_Menu_Item menu_5[];
extern Fl_Menu_Item menu_6[];
extern Fl_Menu_Item menu_7[];
extern Fl_Menu_Item menu_8[];
extern Fl_Menu_Item menu_mb_menuBar[];
#define mi_open (menu_mb_menuBar+1)
#define mi_save (menu_mb_menuBar+2)
#define sm_ppNumber (menu_mb_menuBar+6)
#define sm_ppAccessMethod (menu_mb_menuBar+13)
#define linux_pp_dev (menu_mb_menuBar+15)
#define sm_operations (menu_mb_menuBar+18)
extern Fl_Double_Window *w_calibration;
#include <FL/Fl_Wizard.H>
extern Fl_Wizard *wz_calibration;
extern Fl_Group *g_warning;
extern Fl_Group *g_vpp;
extern Fl_Group *g_vddmax;
extern Fl_Group *g_vddp;
extern Fl_Group *g_vddmin;
extern Fl_Group *g_calFinish;
#include <FL/Fl_Button.H>
extern Fl_Button *pb_calNext;
extern Fl_Button *pb_calCancel;
extern Fl_Button *pb_calSkip;
extern Fl_Button *pb_calPrec;
bool make_calibration_window(bool force,const char *devname,double vppmin,double vppmax,double vddpmin,double vddpmax,double vddmin,double vddmax);
extern Fl_Double_Window *w_report;
extern Fl_Button *pb_discard;
#include <FL/Fl_Return_Button.H>
extern Fl_Return_Button *pb_load;
extern Fl_Browser *ls_report;
Fl_Double_Window* make_report_window(void);
extern Fl_Double_Window *w_copyright;
#include <FL/Fl_Help_View.H>
Fl_Double_Window* make_copyright_window(char *copyright);
bool show_report_window(const char *title);
extern Fl_Double_Window *w_regcheck;
extern Fl_Group *g_regform;
extern Fl_Button *pb_reglater;
extern Fl_Return_Button *pb_regnow;
extern Fl_Input *tx_paypal_code;
extern Fl_Input *tx_reg_key;
extern Fl_Box *bx_countdown;
void make_regcheck_window(void);
void registrationCountDown(void *count);
#endif
