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
 * $Id: DirectPPIO.h,v 1.2 2002/09/12 03:42:06 marka Exp $
 */
#ifndef __DirectPPIO_h
#define __DirectPPIO_h

#include "ParallelPort.h"

#ifdef WIN32
#  include "DlPortDriver.h"
#endif

/** \file */

/**
 * An implementation of the IO interface for parallel ports which uses
 * direct port I/O.
 */
class DirectPPIO : public ParallelPort
{
public:
    /**
     * This constructor will set I/O permissions and then set the
     * effective UID back to the read UID (effectively dropping root
     * permissions on SUID executables).
     *
     * \param port The number of the parallel port to use. 
     * \throws runtime_error Contains a textual description of the error.
     */
    DirectPPIO(int port);

    /** Destructor */
    ~DirectPPIO();

    void set_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert,
        bool state
    );

    bool get_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert
    );

private:
    int ioport;
    int regs;

#ifdef WIN32
    HANDLE  hCom;

    static DlPortDriver dlPortDriver;
#endif
};

#endif
