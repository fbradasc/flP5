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
#include "AvrUI.h"
#include "PicUI.h"

bool DeviceUI::verifyConfig(const char *device, bool verbose)
{
    return PicUI::verifyConfig(device,verbose) ||
           AvrUI::verifyConfig(device,verbose) ||
           false;
}

bool DeviceUI::initDevicesSettings (
    const char  *vendor,
    const char  *spec  ,
    Preferences &device
) {
    return PicUI::initDevicesSettings(vendor, spec, device) ||
           AvrUI::initDevicesSettings(vendor, spec, device) ||
           false;
}

bool DeviceUI::loadSettings (
    const char  *vendor,
    const char  *spec  ,
    char        *path  ,
    Preferences &from
)
{
    return PicUI::loadSettings(vendor, spec, path, from) ||
           AvrUI::loadSettings(vendor, spec, path, from) ||
           false;
}

void DeviceUI::cleanConfigFields()
{
    PicUI::cleanConfigFields();
    AvrUI::cleanConfigFields();
}

bool DeviceUI::resetConfigFields(Preferences &device, const char *deviceSpec, const char *path)
{
    return PicUI::resetConfigFields(device, deviceSpec, path) ||
           AvrUI::resetConfigFields(device, deviceSpec, path) ||
           false;
}

bool DeviceUI::saveConfig(const char *deviceSpec, const char *deviceName)
{
    return PicUI::saveConfig(deviceSpec, deviceName) ||
           AvrUI::saveConfig(deviceSpec, deviceName) ||
           false;
}

bool DeviceUI::isExperimental(const char *deviceSpec)
{
     return PicUI::isExperimental(deviceSpec) ||
            AvrUI::isExperimental(deviceSpec) ||
            false;
}

bool DeviceUI::getVoltageRanges (
    const char *deviceSpec,
    double &vpp_min , double &vpp_max ,
    double &vddp_min, double &vddp_max,
    double &vdd_min , double &vdd_max
) {
    return PicUI::getVoltageRanges (
               deviceSpec,
               vpp_min , vpp_max ,
               vddp_min, vddp_max,
               vdd_min , vdd_max
           ) ||
           AvrUI::getVoltageRanges (
               deviceSpec,
               vpp_min , vpp_max ,
               vddp_min, vddp_max,
               vdd_min , vdd_max
           ) ||
           false;
}
