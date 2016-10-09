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
#ifndef LptPorts_H
#define LptPorts_H

#define MAX_LPTPORTS 5

class LptPorts
{
public:
    LptPorts();

    static int address[MAX_LPTPORTS];
    static int regs[MAX_LPTPORTS];
    static int count;

private:

#ifdef WIN32
    void detectPorts();
    void detectPorts9x(); // Win9x version
    void detectPortsNT(); // WinNT version
#endif
};

#endif // LptPorts_H
