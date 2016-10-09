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
// DlPortDriver.c - Dynamic install/start/stop/remove DLPortIO driver 

#if defined(WIN32) && !defined(ENABLE_LINUX_GPIO)

#include <windows.h>
#include <stdio.h>

#include "DlPortDriver.h"

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

const char DlPortDriver::LIBRARY_FILENAME[] = "DLPortIO.dll";

const char DlPortDriver::DRIVER_NAME[]  = "DLPortIO";
const char DlPortDriver::DISPLAY_NAME[] = "DriverLINX Port I/O Driver";
const char DlPortDriver::DRIVER_GROUP[] = "SST miniport drivers";

SC_HANDLE DlPortDriver::hSCMan = NULL;

bool DlPortDriver::fActiveHW = false;
bool DlPortDriver::fHardAccess = true;
bool DlPortDriver::fRunningWinNT;
bool DlPortDriver::fDrvPrevInst;
bool DlPortDriver::fDrvPrevStart;
char DlPortDriver::fDriverPath[512] = "";
char DlPortDriver::fDLLPath[512] = "";
char DlPortDriver::fLastError[512] = "";

char *DlPortDriver::getLastErrorMsg()
{
char *sp;

    sp = fLastError;
    strcpy(fLastError,"");
    return sp;
}

//---------------------------------------------------------------------------
// DlPortDriver::connectSCM()
//    Connects to the WinNT Service Control Manager
//---------------------------------------------------------------------------
bool DlPortDriver::connectSCM()
{
DWORD dwStatus = 0; // Assume success, until we prove otherwise
DWORD scAccess;     // Access mode when connecting to SCM

    // Try and connect as administrator
    scAccess = SC_MANAGER_CONNECT |
               SC_MANAGER_QUERY_LOCK_STATUS |
               SC_MANAGER_ENUMERATE_SERVICE |
               SC_MANAGER_CREATE_SERVICE;      // Admin only

    // Connect to the SCM
    hSCMan = OpenSCManager(NULL, NULL, scAccess);

    // If we're not in administrator mode, try and reconnect
    if (hSCMan==NULL && GetLastError()==ERROR_ACCESS_DENIED) {
        scAccess = SC_MANAGER_CONNECT |
                   SC_MANAGER_QUERY_LOCK_STATUS |
                   SC_MANAGER_ENUMERATE_SERVICE;

        // Connect to the SCM
        hSCMan = OpenSCManager(NULL, NULL, scAccess);
    }

    // Did it succeed?
    if (hSCMan==NULL) {
        // Failed, save error information
        dwStatus=GetLastError();
        sprintf(fLastError,"connectSCM: Error #%lu", dwStatus);
    }
    return dwStatus==0; // Success == 0
}


//---------------------------------------------------------------------------
// DlPortDriver::disconnectSCM()
//    Disconnects from the WinNT Service Control Manager
//---------------------------------------------------------------------------
bool DlPortDriver::disconnectSCM()
{
    if (hSCMan != NULL) {
        // Disconnect from our local Service Control Manager
        CloseServiceHandle(hSCMan);
        hSCMan=NULL;
    }
    return true;
}

//---------------------------------------------------------------------------
// DlPortDriver::driverInstall()
//    Installs the DriverLINX driver into Windows NT's registry
//---------------------------------------------------------------------------
bool DlPortDriver::driverInstall()
{
SC_HANDLE hService;           // Handle to the new service
DWORD dwStatus = 0;           // Assume success, until we prove otherwise
static char DriverPath[1024]; // Path including filename

    fDrvPrevInst=false; // Assume the driver wasn't installed previously

    
    sprintf(DriverPath, "%s\\%s.SYS", fDriverPath, DRIVER_NAME);

    // Is the DriverLINX driver already in the SCM? If so,
    // indicate success and set fDrvPrevInst to true.
    hService=OpenService(hSCMan, DRIVER_NAME, SERVICE_QUERY_STATUS);
    if (hService!=NULL) {
        fDrvPrevInst=true;            // Driver previously installed,
                                      // don't remove
        CloseServiceHandle(hService); // Close the service
        return true;                  // Success
    }
    // Add to our Service Control Manager's database
    hService=CreateService (
        hSCMan,
        DRIVER_NAME,
        DISPLAY_NAME,
        SERVICE_START | SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DriverPath,
        DRIVER_GROUP,
        NULL, NULL, NULL, NULL
    );
    if (hService==NULL) {
       dwStatus=GetLastError();
    } else {
       // Close the service for now...
       CloseServiceHandle(hService);
    }
    if (dwStatus!=0) {
        sprintf(fLastError, "driverInstall: Error #%lu", dwStatus);
    }
    return dwStatus==0; // Success == 0
}

//---------------------------------------------------------------------------
// DlPortDriver::driverStart()
//    Starts the Windows NT DriverLINX driver for use
//---------------------------------------------------------------------------
bool DlPortDriver::driverStart()
{
SC_HANDLE hService; // Handle to the service to start
DWORD dwStatus = 0; // Assume success, until we prove otherwise
SERVICE_STATUS sStatus;

    fDrvPrevStart=false; // Assume the driver was not already running

    hService = OpenService(hSCMan, DRIVER_NAME, SERVICE_QUERY_STATUS);
    if (hService!=NULL && QueryServiceStatus(hService, &sStatus)) {
        // Got the service status, now check it
        if (sStatus.dwCurrentState == SERVICE_RUNNING) {
            fDrvPrevStart=true;           // Driver was previously started
            CloseServiceHandle(hService); // Close service
            return true;                  // Success
        } else if (sStatus.dwCurrentState==SERVICE_STOPPED) {
            // Driver was stopped. Start the driver.
            CloseServiceHandle(hService);
            hService = OpenService(hSCMan, DRIVER_NAME, SERVICE_START);
            if (!StartService(hService, 0, NULL)) {
                dwStatus=GetLastError();
            }
            CloseServiceHandle(hService); // Close service
        } else {
            dwStatus=-1; // Can't run the service
        }
    } else {
        dwStatus=GetLastError();
    }
    if (dwStatus!=0) {
        sprintf(fLastError,"driverStart: Error #%lu", dwStatus);
    }
    return dwStatus==0; // Success == 0
}

//---------------------------------------------------------------------------
// DlPortDriver::driverStop()
//    Stops a Windows NT driver from use
//---------------------------------------------------------------------------
bool DlPortDriver::driverStop()
{
SC_HANDLE hService; // Handle to the service to start
DWORD dwStatus = 0; // Assume success, until we prove otherwise

    // If we didn't start the driver, then don't stop it.
    // Pretend we stopped it, by indicating success.
    if (fDrvPrevStart) {
        return true;
    }
    // Get a handle to the service to stop
    hService = OpenService (
        hSCMan,
        DRIVER_NAME,
        SERVICE_STOP | SERVICE_QUERY_STATUS
    );
    if (hService!=NULL) {
        // Stop the driver, then close the service
        SERVICE_STATUS sStatus;

        if (!ControlService(hService, SERVICE_CONTROL_STOP, &sStatus)) {
            dwStatus = GetLastError();
        }
        // Close the service
        CloseServiceHandle(hService);
    } else {
        dwStatus = GetLastError();
    }
    if (dwStatus!=0) {
        sprintf(fLastError,"driverStop: Error #%lu", dwStatus);
    }
    return dwStatus==0; // Success == 0
}

//---------------------------------------------------------------------------
// DlPortDriver::driverRemove()
//    Removes a driver from the Windows NT system (service manager)
//---------------------------------------------------------------------------
bool DlPortDriver::driverRemove()
{
SC_HANDLE hService; // Handle to the service to start
DWORD dwStatus = 0; // Assume success, until we prove otherwise

    // If we didn't install the driver, then don't remove it.
    // Pretend we removed it, by indicating success.
    if (fDrvPrevInst) {
        return true;
    }
    // Get a handle to the service to remove
    hService = OpenService (
        hSCMan,
        DRIVER_NAME,
        DELETE
    );
   
    if (hService!=NULL) {
       // Remove the driver then close the service again
        if (!DeleteService(hService)) {
            dwStatus = GetLastError();
        }
        // Close the service
        CloseServiceHandle(hService);
    } else {
        dwStatus = GetLastError();
    }
    if (dwStatus!=0) {
        sprintf(fLastError, "driverRemove: Error #%lu", dwStatus);
    }
    return dwStatus==0; // Success == 0
}

bool DlPortDriver::isLoaded()
{
    return fActiveHW;
}

//---------------------------------------------------------------------------
// DlPortDriver constructor
//    Opens the DLL dynamically
//---------------------------------------------------------------------------
DlPortDriver::DlPortDriver()
{
OSVERSIONINFO os;

    // If the DLL/driver is already open, then forget it!
    if (isLoaded()) {
        return;
    }
    // Are we running Windows NT?
    memset(&os, NULL, sizeof(OSVERSIONINFO));
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    fRunningWinNT=(os.dwPlatformId==VER_PLATFORM_WIN32_NT);

    // Set default WinNT driver path
    char Buffer[MAX_PATH];
    GetSystemDirectory(Buffer, MAX_PATH);
    strcpy(fDriverPath, Buffer);
    strcat(fDriverPath, "\\DRIVERS");

    // Set the default DLL path
    strcpy(fDLLPath,"");

    fActiveHW=false;  // DLL/Driver not loaded
    fHardAccess=true; // Not used, default true

    strcpy(fLastError,"");    // No errors yet

    // If we're running Windows NT, install the driver then start it
    if (fRunningWinNT) {
        // Connect to the Service Control Manager
        if (!connectSCM()) {
            return;
        }
        // Install the driver
        if (!driverInstall()) {
            // Driver install failed, so disconnect from the SCM
            disconnectSCM();
            return;
        }
        // Start the driver
        if (!driverStart()) {
            // Driver start failed, so remove it then disconnect from SCM
            driverRemove();
            disconnectSCM();
            return;
        }
    }
    fActiveHW=true; // Success
}

//---------------------------------------------------------------------------
// DlPortDriver destructor
//    Closes the dynamically opened DLL
//---------------------------------------------------------------------------
DlPortDriver::~DlPortDriver()
{
   // Don't close anything if it wasn't opened previously
   if (!isLoaded()) {
       return;
   }
   // If we're running Windows NT, stop the driver then remove it
   if (fRunningWinNT) {
       if (!driverStop()) {
           return;
       }
       if (!driverRemove()) {
           return;
       }
       disconnectSCM();
   }
   fActiveHW=false; // Success
}

#endif // WIN32 && !ENABLE_LINUX_GPIO
