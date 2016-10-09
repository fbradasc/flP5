#ifndef PIC_UI_H
#define PIC_UI_H

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
#include "DeviceUI.h"

#include <FL/Fl_Browser.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Tabs.H>

class PicUI: public DeviceUI
{
public:
    enum DEV_PARAM {
        PAR_WORD_SIZE=0,
        PAR_CODE_SIZE,
        PAR_EEPROM_SIZE,
        PAR_PROG_COUNT,
        PAR_PROG_MULT,
        PAR_PROG_TIME,
        PAR_ERASE_TIME,
        PAR_WRITE_BUF_SIZE,
        PAR_ERASE_BUF_SIZE,
        PAR_DEVICE_ID,
        PAR_DEVICE_ID_MASK,
        PAR_VPP_MIN,
        PAR_VPP_MAX,
        PAR_VDD_MIN,
        PAR_VDD_MAX,
        PAR_VDDP_MIN,
        PAR_VDDP_MAX,
        LAST_PARAM
    };

    enum CONFIG_WORDS {
        CW_MASK=0,
        CW_SAVE,
        CW_DEFV,
        CW_CP_MASK,
        CW_CP_ALL,
        CW_CP_NONE,
        CW_DP_MASK,
        CW_DP_ON,
        CW_DP_OFF,
        CW_BD_MASK,
        CW_BD_ON,
        CW_BD_OFF,
        LAST_CONFIG_WORD
    };


    static bool verifyConfig(const char *device, bool verbose);
    static bool initDevicesSettings (
        const char  *vendor,
        const char  *spec  ,
        Preferences &device
    );
    static bool loadSettings (
        const char  *vendor,
        const char  *spec  ,
        char        *path  ,
        Preferences &from
    );
    static void cleanConfigFields();
    static bool resetConfigFields (
        Preferences &device,
        const char *deviceSpec,
        const char *path
    );
    static bool saveConfig(const char *deviceSpec, const char *deviceName);
    static bool cfgWordsCB(CfgOper oper);
    static bool isExperimental(const char *deviceSpec);
    static bool getVoltageRanges (
        const char *deviceSpec,
        double &vpp_min , double &vpp_max ,
        double &vddp_min, double &vddp_max,
        double &vdd_min , double &vdd_max
    );

    static void activate();
    static void deactivate();

    static bool match(const char *deviceSpec);
    static bool match(const char *vendor, const char *spec);

    static Fl_Input        *tx_Param[PicUI::LAST_PARAM];
    static Fl_Input        *tx_CfgWord[PicUI::LAST_CONFIG_WORD];
    static Fl_Tabs         *g_Settings;
    static Fl_Group        *g_gencfg;
    static Fl_Choice       *ch_MemType;
    static Fl_Group        *g_cfgwords;
    static Fl_Group        *g_ConfigWordsEdit;
    static Fl_Group        *g_ConfigWordsToolBar;
    static Fl_Group        *g_ConfigWordsNewEditCopy;
    static Fl_Browser      *ls_ConfigWords;
    static Fl_Group        *g_miscellanea;
    static Fl_Check_Button *tb_Experimental;
};

#endif // PIC_UI_H
