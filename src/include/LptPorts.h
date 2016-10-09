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
#ifndef LptPorts_H
#define LptPorts_H

#define MAX_USERS_PARPORTS  8
#define MAX_DEVFS_PARPORTS  8
#define MAX_DIRECT_PARPORTS 3
#define MAX_LPTPORTS MAX_USERS_PARPORTS+MAX_DEVFS_PARPORTS+MAX_DIRECT_PARPORTS

class LptPort
{
public:
    char* device ;
    int   address;
    int   regs   ;
    int   access ;

    enum { DIRECT, PPDEV };

    LptPort();
    LptPort(const char* dev, int acc=DIRECT, int adr=0, int reg=0);
    ~LptPort();
    LptPort& operator=(const LptPort& ref);
    void clear();
};

class LptPorts
{
public:
    LptPorts();
    ~LptPorts();

    LptPort ports[MAX_LPTPORTS];
    int     count;

    inline LptPort& operator[](int i) { return ports[i]; }

private:

    void detectPorts();

#ifdef WIN32
    void detectPorts9x(); // Win9x version
    void detectPortsNT(); // WinNT version
#endif
};

#endif // LptPorts_H
