#ifndef AVR_UI_H
#define AVR_UI_H

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
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Tabs.H>

#include <Atmel.h>

class AvrUI: public DeviceUI
{
public:
    static bool verifyConfig(const char *device, bool verbose);
    static bool initDevicesSettings (
        const char *vendor,
        const char *spec,
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
    static bool isExperimental(const char *deviceSpec);
    static bool getVoltageRanges (
        const char *deviceSpec,
        double &vpp_min , double &vpp_max ,
        double &vddp_min, double &vddp_max,
        double &vdd_min , double &vdd_max
    );

    static bool match(const char *deviceSpec);
    static bool match(const char *vendor, const char *spec);

    static void activate();
    static void deactivate();

    static void validateChar(Fl_Input *o, const char *range, unsigned int max_size);

    static void initFusesAndLocks(Avr *chip, DataBuffer& buf);

    static Fl_Input        *tx_Param[Avr::LAST_PARAM];
    static Fl_Input        *tx_bitNames[Avr::LAST_BIT_NAME];
    static Fl_Group        *g_Settings;
    static Fl_Group        *g_gencfg;
    static Fl_Group        *g_flashcfg;
    static Fl_Group        *g_eepromcfg;
    static Fl_Group        *g_fusecfg;
    static Fl_Group        *g_misccfg;
    static Fl_Check_Button *tb_powerOffAfterWrite;
    static Fl_Check_Button *tb_Experimental;

    static Fl_Light_Button *lb_bits[Avr::LAST_BIT_NAME][Avr::BITS_IN_FUSES];
    static Fl_Group        *g_fuses[Avr::LAST_BIT_NAME];

    static const char      *instructionsFormatTips;

private:
    class Float
    {
    public:
        static void compare(int i, Preferences &from, Preferences &to, bool &different);
        static void clone  (int i, Preferences &from, Preferences &to);
        static void reset  (int i, Preferences &device);
        static void init   (int i, Preferences &device);
        static void save   (int i, Preferences &device);
    };

    class Int
    {
    public:
        static void compare(int i, Preferences &from, Preferences &to, bool &different);
        static void clone  (int i, Preferences &from, Preferences &to);
        static void reset  (int i, Preferences &device);
        static void init   (int i, Preferences &device);
        static void save   (int i, Preferences &device);
    };

    class String
    {
    public:
        static void compare(int i, Preferences &from, Preferences &to, bool &different);
        static void clone  (int i, Preferences &from, Preferences &to);
        static void reset  (int i, Preferences &device);
        static void init   (int i, Preferences &device);
        static void save   (int i, Preferences &device);
    };
};

#endif // AVR_UI_H
