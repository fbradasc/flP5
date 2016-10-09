/* Copyright (C) 2003-2010  Francesco Bradascio <fbradasc@gmail.com>
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
#ifndef DlPortDriver_H
#define DlPortDriver_H

#include <windows.h>
#include <winsvc.h>

class DlPortDriver
{
private:
    static const char LIBRARY_FILENAME[];
    static const char DRIVER_NAME[];
    static const char DISPLAY_NAME[];
    static const char DRIVER_GROUP[];
    static SC_HANDLE hSCMan;

    //
    // Is the DLL loaded?
    //
    static bool fActiveHW;
    //
    // Not used: for compatibility only
    //
    static bool fHardAccess;
    //
    // True when we're running Windows NT
    //
    static bool fRunningWinNT;
    //
    // DriverLINX driver already installed?
    //
    static bool fDrvPrevInst;
    //
    // DriverLINX driver already running?
    //
    static bool fDrvPrevStart;
    //
    // Full path of WinNT driver
    //
    static char fDriverPath[512];
    //
    // Full path of DriverLINX DLL
    //
    static char fDLLPath[512];
    //
    // Last error which occurred in Open/CloseDriver()
    //
    static char fLastError[512];

    static bool connectSCM();
    static bool disconnectSCM();
    static bool driverInstall();
    static bool driverStart();
    static bool driverStop();
    static bool driverRemove();

public:
    DlPortDriver();
    ~DlPortDriver();

    static char *getLastErrorMsg();
    static bool isLoaded();
};

#endif // DlPortDriver_H
