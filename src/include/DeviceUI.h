#ifndef DEVICE_UI_H
#define DEVICE_UI_H

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
#include "Preferences.h"

class DeviceUI
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
    static bool cfgWordsCB(CfgOper oper);
    static bool isExperimental(const char *deviceSpec);
    static bool getVoltageRanges (
        const char *deviceSpec,
        double &vpp_min , double &vpp_max ,
        double &vddp_min, double &vddp_max,
        double &vdd_min , double &vdd_max
    );
};

#endif // DEVICE_UI_H
