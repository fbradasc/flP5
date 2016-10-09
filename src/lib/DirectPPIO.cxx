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
 * $Id: DirectPPIO.cxx,v 1.4 2002/10/11 16:26:56 marka Exp $
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifndef WIN32
#  include <unistd.h>
#  include <sys/io.h>
#endif
#include <sys/types.h>
#include <stdexcept>

using namespace std;

#include "DirectPPIO.h"
#include "Util.h"

#ifdef  WIN32

#  include <DlPortIO.h>
#  include <windows.h>

DlPortDriver DirectPPIO::dlPortDriver;

#  define outb(val, id)   DlPortWritePortUchar(id, val)
#  define inb(id)         DlPortReadPortUchar(id)

#endif

DirectPPIO::DirectPPIO(int port) : ParallelPort(port)
{
    if ((port > ports.count)                    ||
        (port < 0)                              ||
        (ports[port].access != LptPort::DIRECT) ||
        !ports[port].address                    ||
        !ports[port].device
    ) {
        throw runtime_error("Invalid DirectPP port number");
    }
#ifdef WIN32
    //Test if port is already in use
    this->hCom = CreateFile (
        ports[port].device,
        GENERIC_READ | GENERIC_WRITE,
        0,             /* comm devices must be opened w/exclusive-access */
        NULL,          /* no security attrs                              */
        OPEN_EXISTING, /* comm devices must use OPEN_EXISTING            */
        0,             /* not overlapped I/O                             */
        NULL           /* hTemplate must be NULL for comm devices        */
    );
    if (this->hCom == INVALID_HANDLE_VALUE) {
        throw runtime_error("DirectPP access error");
    }
#endif
    /* Turn port access on */
    this->ioport = ports[port].address;
    this->regs   = ports[port].regs;
#ifndef WIN32
    /* Set UID to root if running setuid */
    Util::setUser(0);

    int error = 0;

    if (ioperm(this->ioport, this->regs, 1) < 0) {
        error = errno;
    } 

    /* Set UID back to user if running setuid */
    Util::setUser(getuid());

    if (error != 0) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "ioperm: %s",
                strerror(error)
            )
        );
    }
#endif
    /* Turn stuff off */
    this->off();

    fprintf (
        stderr,
        "DirectPPIO::DirectPPIO instantiated\n"
    );
}

DirectPPIO::~DirectPPIO()
{
    /* Turn everything off */
    this->off();

#ifdef  WIN32
    if (this->hCom != INVALID_HANDLE_VALUE) {
        CloseHandle(this->hCom);
        this->hCom = INVALID_HANDLE_VALUE;
    }
#else
    /* Set UID to root if running setuid */
    Util::setUser(0);

    /* Turn port access off */
    ioperm(this->ioport, this->regs, 0);

    /* Set UID back to user if running setuid */
    Util::setUser(getuid());
#endif
    fprintf (
        stderr,
        "DirectPPIO::DirectPPIO deleted\n"
    );
}

void DirectPPIO::set_pin_state (
    const char *name,
    short reg,
    short bit,
    short invert,
    bool state
) {
unsigned int val;

    if (invert) {
        state ^= 0x01;
    }
    val = inb(this->ioport + reg);

    if (state) {
        val |= (1 << bit);
    } else {
        val &= ~(1 << bit);
    }
    outb(val, this->ioport + reg);
}

bool DirectPPIO::get_pin_state (
    const char *name, 
    short reg,
    short bit,
    short invert
) {
unsigned int val;

    val = (inb(this->ioport + reg) >> bit) & 0x01;
    if (invert) {
        val ^= 0x01;
    }
    return val;
}

bool DirectPPIO::probe(LptPort &port)
{
#ifdef WIN32

    if (NULL == port.device) {
        return false;
    }
    //Test if port is already in use
    HANDLE hCom = CreateFile (
        port.device,
        GENERIC_READ | GENERIC_WRITE,
        0,             /* comm devices must be opened w/exclusive-access */
        NULL,          /* no security attrs                              */
        OPEN_EXISTING, /* comm devices must use OPEN_EXISTING            */
        0,             /* not overlapped I/O                             */
        NULL           /* hTemplate must be NULL for comm devices        */
    );
    if (hCom == INVALID_HANDLE_VALUE) {
        return false;
    }
    CloseHandle(hCom);

#else

    if ((0==port.address) || (0==port.regs)) {
        return false;
    }

    /* Set UID to root if running setuid */
    Util::setUser(0);

    int error = 0;

    if (ioperm(port.address, port.regs, 1) < 0) {
        error = errno;
    } 

    /* Set UID back to user if running setuid */
    Util::setUser(getuid());

    if (error != 0) {
        return false;
    }

    /* Set UID to root if running setuid */
    Util::setUser(0);

    /* Turn port access off */
    ioperm(port.address, port.regs, 0);

    /* Set UID back to user if running setuid */
    Util::setUser(getuid());
#endif

    return true;
}
