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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifndef WIN32
#  include <unistd.h>
#  include <sys/io.h>
#else
#  include <windows.h>
#endif
#include <sys/types.h>
#include <stdlib.h>
#include <stdexcept>
#include "LptPorts.h"
#include "Util.h"
#include "LinuxPPDevIO.h"

LptPort::LptPort()
{
    device  = NULL;
    address = 0;
    regs    = 0;
    access  = DIRECT;
}

LptPort::LptPort(const char* dev, int acc, int adr, int reg)
{
    device  = (dev) ? strdup(dev) : NULL;
    address = adr;
    regs    = reg;
    access  = acc;
}

LptPort::~LptPort()
{
    if (device) {
        delete device;
    }
}

LptPort& LptPort::operator=(const LptPort& ref)
{
    if (NULL != device) {
        free(device);
    }
    device  = (ref.device) ? strdup(ref.device) : NULL;
    address = ref.address;
    regs    = ref.regs;
    access  = ref.access;

    return *this;
}
void LptPort::clear()
{
    if (NULL != device) {
        free(device);
        device = NULL;
    }
    address = 0;
    regs    = 0;
    access  = DIRECT;
}

LptPorts::LptPorts()
{
    detectPorts();
}

LptPorts::~LptPorts()
{
    for (int i=0; i<MAX_LPTPORTS; i++) {
        ports[i].clear();
    }
}

#ifndef WIN32

void LptPorts::detectPorts()
{
    int i;

    for (i=0; i<MAX_LPTPORTS; i++) {
        ports[i].clear();
    }
    count=0;
    //
    // Linux PPDev access
    //
    for (i=0; i<MAX_USERS_PARPORTS; i++) {
        LptPort port(Preferences::Name("/dev/parport%d",i), LptPort::PPDEV);
        if (LinuxPPDevIO::probe(port)) {
            ports[count++] = port;
        }
    }
    for (i=0; i<MAX_DEVFS_PARPORTS; i++) {
        LptPort port(Preferences::Name("/dev/parports/%d",i), LptPort::PPDEV);
        if (LinuxPPDevIO::probe(port)) {
            ports[count++] = port;
        }
    }
    //
    // Direct PP access
    //
    int address[MAX_DIRECT_PARPORTS] = { 0x378, 0x278, 0x3BC };

    for (i=0; i<MAX_DIRECT_PARPORTS; i++) {
        LptPort port(Preferences::Name("Address: %0X",address[i]), LptPort::DIRECT, address[i], 3);
        if (LinuxPPDevIO::probe(port)) {
            ports[count++] = port;
        }
    }
}

#else

void LptPorts::detectPorts()
{
int RunningWinNT;
OSVERSIONINFO os;

    for (int i=0; i<MAX_LPTPORTS; i++) {
        ports[i].clear();
    }
    count=0;

    // Are we running Windows NT?
    memset(&os,'\0',sizeof(OSVERSIONINFO));
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);
    RunningWinNT=(os.dwPlatformId==VER_PLATFORM_WIN32_NT);

    // Detect the printer ports available
    if (RunningWinNT) {
        count = 3;      // No printer ports counted
        //  detectPortsNT(); // WinNT version
    } else {
        detectPorts9x(); // Win9x version
    }
    for (int i=0; i<count; i++) {
            ports[i].device = strdup(Preferences::Name("LPT%d",i));
    }
}

void LptPorts::detectPorts9x()
{
const char *BASE_KEY     = "Config Manager\\Enum";
const char *PROBLEM      = "Problem";
const char *ALLOCATION   = "Allocation";
const char *PORTNAME     = "PortName";
const char *HARDWARE_KEY = "HardwareKey";

const REGSAM KEY_PERMISSIONS = KEY_ENUMERATE_SUB_KEYS |
                               KEY_QUERY_VALUE;

HKEY CurKey;               // Current key when using the registry
char KeyName[MAX_PATH];    // A key name when using the registry

char **KeyList;            // List of keys
DWORD KeyCount;            // Count of the number of keys in KeyList

DWORD index;

    // Open the registry
    RegOpenKeyEx(HKEY_DYN_DATA, BASE_KEY, 0, KEY_PERMISSIONS, &CurKey);

    // Grab all the key names under HKEY_DYN_DATA
    //
    // Do this by first counting the number of keys,
    // then creating an array big enough to hold them
    // using the KeyList pointer.

    FILETIME DummyFileTime;
    DWORD DummyLength = MAX_PATH - 1;
    KeyCount = 0;
    while (
        RegEnumKeyEx (
            CurKey, KeyCount++, KeyName, &DummyLength,
            NULL, NULL, NULL, &DummyFileTime
        ) != ERROR_NO_MORE_ITEMS
    ) {
        DummyLength = MAX_PATH;
    }
    KeyList = new char*[KeyCount];

    KeyCount = 0;
    DummyLength = MAX_PATH;
    while (
        RegEnumKeyEx(
            CurKey, KeyCount, KeyName, &DummyLength,
            NULL, NULL, NULL, &DummyFileTime
        ) != ERROR_NO_MORE_ITEMS
    ) {
        KeyList[KeyCount] = new char[DummyLength+1];
        strcpy(KeyList[KeyCount], KeyName);
        DummyLength = MAX_PATH;
        KeyCount++;
    }
    // Close the key
    RegCloseKey(CurKey);

    // Cycle through all keys; looking for a string valued subkey called
    // 'HardWareKey' which is not NULL, and another subkey called 'Problem'
    // whose fields are all valued 0.
    for (DWORD KeyIndex=0; KeyIndex<KeyCount; KeyIndex++) {
        int HasProblem = FALSE; // Is 'Problem' non-zero? Assume it is Ok

        // Open the key
        strcpy(KeyName, BASE_KEY);
        strcat(KeyName, "\\");
        strcat(KeyName, KeyList[KeyIndex]);
        if (
            RegOpenKeyEx (
                HKEY_DYN_DATA, KeyName, 0, KEY_PERMISSIONS, &CurKey
            ) != ERROR_SUCCESS
        ) {
            continue;
        }
        // Test for a 0 valued Problem sub-key,
        // which must only consist of raw data
        DWORD DataType, DataSize;
        RegQueryValueEx(CurKey, PROBLEM, NULL, &DataType, NULL, &DataSize);

        if (DataType == REG_BINARY) {
            // We have a valid, binary "Problem" sub-key
            // Test to see if the fields are zero

            char HardwareSubKey[MAX_PATH]; // Data from the "Hardware" sub-key

            BYTE *Data = new BYTE[DataSize]; // Data from "Problem" sub-key

            // Read the data from the "Problem" sub-key
            if (
                RegQueryValueEx (
                    CurKey, PROBLEM, NULL,
                    NULL, Data, &DataSize
                ) == ERROR_SUCCESS
            ) {
                // See if it has any problems
                for (DWORD index=0; index<DataSize; index++) {
                    HasProblem |= Data[index];
                }
            } else {
                HasProblem = TRUE; // No good
            }
            delete[] Data;

            // Now try and read the Hardware sub-key
            DataSize = MAX_PATH;
            RegQueryValueEx (
                CurKey, HARDWARE_KEY, NULL, &DataType,
                (BYTE *)HardwareSubKey, &DataSize
            );
            if (DataType != REG_SZ) {
                HasProblem = TRUE; // No good
            }
            // Do we have no problem, and a non-null Hardware sub-key?
            if (!HasProblem && strlen(HardwareSubKey) > 0) {
                // Now open the key which is "pointed at" by HardwareSubKey
                RegCloseKey(CurKey);

                strcpy(KeyName, "Enum\\");
                strcat(KeyName, HardwareSubKey);
                if (
                    RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE, KeyName, 0,
                        KEY_PERMISSIONS, &CurKey
                    ) != ERROR_SUCCESS
                ) {
                    continue;
                }
                // Now read in the PortName and obtain the LPT number from it
                char PortName[MAX_PATH];
                DataSize = MAX_PATH;
                RegQueryValueEx (
                    CurKey, PORTNAME, NULL, &DataType,
                    (BYTE *)PortName, &DataSize
                );
                if (DataType != REG_SZ) {
                    strcpy(PortName, ""); // No good
                }
                //------- Search for COM ports
                // Make sure it has LPT in it
                if (strstr(PortName, "LPT") != NULL) {
                    // The nubmer of the port
                    int PortNumber;
                    // Holds the number of the port as a string
                    char PortNumberStr[MAX_PATH];
                    // Holds the registry data for the port address allocation
                    WORD Allocation[64];

                    memset(PortNumberStr, '\0', MAX_PATH);
                    strncpy (
                        PortNumberStr,
                        strstr(PortName, "LPT")+3,
                        strlen(PortName)-(strstr(PortName, "LPT")-PortName)-2
                    );
                    // Find the port number
                    PortNumber = atoi(PortNumberStr);

                    // Find the address
                    RegCloseKey(CurKey);

                    strcpy(KeyName, BASE_KEY);
                    strcat(KeyName, "\\");
                    strcat(KeyName, KeyList[KeyIndex]);
                    RegOpenKeyEx (
                        HKEY_DYN_DATA, KeyName, 0, KEY_PERMISSIONS, &CurKey
                    );
                    DataSize = sizeof(Allocation);
                    RegQueryValueEx (
                        CurKey, ALLOCATION, NULL, &DataType,
                        (unsigned char*)Allocation, &DataSize
                    );
                    if (DataType == REG_BINARY) {
                        // Decode the Allocation data: the port address is
                        // present directly after a 0x000C entry (which doesn't
                        // have 0x0000 after it).
                        for (int pos=0; pos<63; pos++) {
                            if (
                                Allocation[pos] == 0x000C &&
                                Allocation[pos+1] != 0x0000 &&
                                PortNumber<=MAX_LPTPORTS
                            ) {
                                // Found a port; add it to the list
                                ports[PortNumber-1].address = Allocation[pos+1];
                                if (
                                    Allocation[pos+2] > 0 &&
                                    Allocation[pos+2] >= Allocation[pos+1]
                                ) {
                                        ports[PortNumber-1].regs = Allocation[pos+2] -
                                                                             Allocation[pos+1] +
                                                                             1;
                                }
                                count++;
                                break;
                            }
                        }
                    }
                }
            }
        }
        RegCloseKey(CurKey);
    }
    // Destroy our key list
    for (index=0; index<KeyCount; index++) {
        delete[] KeyList[index];
    }
    delete KeyList;
}

void LptPorts::detectPortsNT()
{
const char *BASE_KEY         = "HARDWARE"
                               "\\DEVICEMAP"
                               "\\PARALLEL PORTS";

const char *LOADED_KEY       = "HARDWARE"
                               "\\RESOURCEMAP"
                               "\\LOADED PARALLEL DRIVER RESOURCES"
                               "\\Parport";

const char *DOS_DEVICES      = "\\DosDevices\\LPT";

const char *DEVICE_PARALLEL  = "\\Device\\Parallel";

const REGSAM KEY_PERMISSIONS = KEY_ENUMERATE_SUB_KEYS |
                               KEY_QUERY_VALUE;

HKEY CurKey;               // Current key when using the registry
char KeyName[MAX_PATH];    // A key name when using the registry

char **ValueList;          // List of value names
DWORD ValueCount;          // Count of the number of value names in ValueList

DWORD index;

    // Open the registry
    if (
        RegOpenKeyEx (
            HKEY_LOCAL_MACHINE, BASE_KEY, 0, KEY_PERMISSIONS, &CurKey
        ) != ERROR_SUCCESS
    ) {
        return; // Can't do anything without this BASE_KEY
    }
    // Grab all the value names under HKEY_LOCAL_MACHINE
    //
    // Do this by first counting the number of value names,
    // then creating an array big enough to hold them
    // using the ValueList pointer.

    DWORD DummyLength = MAX_PATH;
    DWORD ValueType;
    ValueCount = 0;
    while (
        RegEnumValue (
            CurKey, ValueCount++, KeyName, &DummyLength,
            NULL, &ValueType, NULL, NULL
        ) != ERROR_NO_MORE_ITEMS
    ) {
        DummyLength = MAX_PATH;
    }
    ValueList   = new char*[ValueCount];
    ValueCount  = 0;
    DummyLength = MAX_PATH;
    while (
        RegEnumValue (
            CurKey, ValueCount, KeyName, &DummyLength,
            NULL, &ValueType, NULL, NULL
        ) != ERROR_NO_MORE_ITEMS
    ) {
        ValueList[ValueCount] = new char[DummyLength+1];
        strcpy(ValueList[ValueCount], KeyName);
        DummyLength = MAX_PATH;
        ValueCount++;
    }
    // Close the key
    RegCloseKey(CurKey);

    for (index=0; index<ValueCount; index++) {
        //
        // Key value for \DosDevices\LPT
        //
        char DosDev[MAX_PATH];
        //
        // Type and size of data read from the registry
        //
        DWORD DataType, DataSize;
        //
        // Is it a \DosDevices\LPT key?
        //
        strcpy(KeyName, BASE_KEY);
        if (
            RegOpenKeyEx (
                HKEY_LOCAL_MACHINE, KeyName, 0, KEY_PERMISSIONS, &CurKey
            ) == ERROR_SUCCESS
        ) {
            DataSize = MAX_PATH;
            RegQueryValueEx (
                CurKey, ValueList[index], NULL, &DataType,
                (BYTE *)DosDev, &DataSize
            );
            RegCloseKey(CurKey);
            //
            // Make sure it was a string
            //
            if (DataType != REG_SZ) {
                strcpy(DosDev, "");
            }
        } else {
            strcpy(DosDev, "");
        }
        if (strstr(DosDev, DOS_DEVICES) != NULL) {
            int PortNumber;                  // The nubmer of the port
            char PortNumberStr[MAX_PATH];    // String version of PortNumber
            char PortIDStr[MAX_PATH];        // PortID

            memset(PortNumberStr, '\0', MAX_PATH);
            strncpy (
                PortNumberStr,
                strstr(DosDev,DOS_DEVICES) + strlen(DOS_DEVICES),
                strlen(DosDev) - (strstr(DosDev,DOS_DEVICES)-DosDev)
                               - strlen(DOS_DEVICES) + 1
            );
            // Get the Port ID
            memset(PortIDStr, '\0', MAX_PATH);
            strncpy (
                PortIDStr,
                strstr(ValueList[index],DEVICE_PARALLEL) +
                strlen(DEVICE_PARALLEL),
                strlen(ValueList[index]) - (
                    strstr(ValueList[index],DEVICE_PARALLEL) -
                    ValueList[index]
                ) - strlen(DEVICE_PARALLEL) + 1
            );
            // Get the port number
            PortNumber = atoi(PortNumberStr);

            // Get the port address
            RegOpenKeyEx (
                HKEY_LOCAL_MACHINE, LOADED_KEY,
                0, KEY_PERMISSIONS, &CurKey
            );
            strcpy(DosDev, "\\Device\\ParallelPort");
            strcat(DosDev, PortIDStr);
            strcat(DosDev, ".Raw");
          
            if (
                RegQueryValueEx (
                   CurKey, DosDev, NULL, &DataType, NULL, NULL
                ) == ERROR_SUCCESS && DataType == REG_RESOURCE_LIST
            ) {
                WORD Allocation[64]; // Binary data with port number inside
              
                // Read in the binary data
                DataSize = sizeof(Allocation);
                RegQueryValueEx (
                   CurKey, DosDev, NULL, NULL,
                   (unsigned char*)Allocation, &DataSize
                );
                // Found a port; add it to the list
                if (DataSize>0 && PortNumber<=MAX_LPTPORTS) {
                    ports[PortNumber-1].address = Allocation[12];
                    ports[PortNumber-1].regs = 8;
                    count++;
                }
            }
            RegCloseKey(CurKey);
        }
    }
    // Destroy our key value list
    // Destroy our key list
    for (index=0; index<ValueCount; index++) {
        delete[] ValueList[index];
    }
    delete ValueList;
}

#endif // WIN32
