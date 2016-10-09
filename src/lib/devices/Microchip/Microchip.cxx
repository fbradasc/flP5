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
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "Microchip.h"
#include "Util.h"

Device *Microchip::load(char *name)
{
Device *d = NULL;
char *spec, *device;

    spec = name;
    device = strchr(name,'/');
    if (device) {
        *device='\0';
        device++;
        if (strncasecmp(spec,"PIC",sizeof("PIC")) == 0) { 
            d = Pic::load(device);
        } else {
            throw runtime_error (
                (const char *)Preferences::Name (
                    "Unknown Microchip family %s",
                    spec
                )
            );
        }
    } else {
        throw runtime_error("Unknown Microchip device.");
    }
    return d;
}


Microchip::Microchip(char *name) : Device(name)
{
}

Microchip::~Microchip()
{
}
