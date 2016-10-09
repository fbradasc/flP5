/* Copyright (C) 2002  Mark Andrew Aikens <marka@desert.cx>
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
 * $Id: LinuxPPDevIO.cxx,v 1.6 2002/10/11 16:26:56 marka Exp $
 */
#ifdef linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

using namespace std;


#include <linux/parport.h>
#include <linux/ppdev.h>

#include "Util.h"
#include "LinuxPPDevIO.h"


LinuxPPDevIO::LinuxPPDevIO(int port) : ParallelPort(port)
{
struct stat fdata;
char devname[20];
int arg;

    if (stat("/dev/parports", &fdata) == 0) {
        /* We're using DevFS */
        sprintf(devname, "/dev/parports/%d", port);
    } else {
        sprintf(devname, "/dev/parport%d", port);
    }
    try {
        if ((this->fd = open(devname, O_RDWR)) < 0) {
            throw errno;
        }
        // if (ioctl(this->fd, PPEXCL) < 0) {
        //     throw errno;
        // }
        if (ioctl(this->fd, PPCLAIM) < 0) {
            throw errno;
        }
        arg = IEEE1284_MODE_BYTE; // PARPORT_MODE_PCSPP;

        if (ioctl(this->fd, PPSETMODE, &arg) < 0) {
            throw errno;
        }
        vpp   (VPP_TO_VDD);
        clock (false);
        data  (false);
        vdd   (VDD_TO_OFF);
        vdd   (VDD_TO_PRG);
    } catch (int err) {
        close(this->fd);
        throw runtime_error(strerror(err));
    }
    fprintf (
        stderr,
        "LinuxPPDevIO::LinuxPPDevIO instantiated\n"
    );
}

LinuxPPDevIO::~LinuxPPDevIO()
{
int arg;

    /* Turn things off */
    vpp   (VPP_TO_VDD);
    clock (false);
    data  (false);
    vdd   (VDD_TO_OFF);
    vdd   (VDD_TO_PRG);

    arg = IEEE1284_MODE_COMPAT;
    ioctl(this->fd, PPSETMODE, &arg);
    ioctl(this->fd, PPRELEASE);     /* Ignore errors */
    close(this->fd);

    fprintf (
        stderr,
        "LinuxPPDevIO::LinuxPPDevIO deleted\n"
    );
}

void LinuxPPDevIO::set_bit_common (
    char *name,
    short reg,
    short bit,
    short invert,
    bool state 
) {
int parm1, parm2, arg;

    parm1 = 0;
    parm2 = 0;
    switch (reg) {
        case 0:
            parm1 = PPRDATA;
            parm2 = PPWDATA;
        break;
        case 2:
            parm1 = PPRCONTROL;
            parm2 = PPWCONTROL;
        break;
        default:
            throw runtime_error (
                (const char *)Preferences::Name (
                    "setBitCommon(%s): unknown register",
                    name
                )
            );
        break;
    }
    if (ioctl(this->fd, parm1, &arg) < 0) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "setBitCommon(%s): read",
                name
            )
        );
    }
    if (invert) {
        state ^= 0x01;
    }
    if (state) {
        arg |= (1 << bit);
    } else {
        arg &= ~(1 << bit);
    }
    if (ioctl(this->fd, parm2, &arg) < 0) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "setBitCommon(%s): write",
                name
            )
        );
    }
}

bool LinuxPPDevIO::get_bit_common(char *name,short reg,short bit,short invert)
{
unsigned int parm, arg;

    switch (reg) {
        case 0:
            parm = PPRDATA;
        break;
        case 1:
            parm = PPRSTATUS;
        break;
        case 2:
            parm = PPRCONTROL;
        break;
        default:
            throw runtime_error("getBitCommon: unknown register");
        break;
    }
    if (ioctl(this->fd, parm, &arg) < 0) {
        throw runtime_error("getBitCommon: read");
    }
    parm = (arg >> bit) & 0x01;

    if (invert) {
        parm ^= 0x01;
    }
    return parm;
}

#endif // linux
