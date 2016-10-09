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
 * $Id: IO.cxx,v 1.6 2002/10/18 15:22:58 marka Exp $
 */
#include <stdio.h>
#include <string.h>
#ifdef linux
#  include <sys/types.h>
#  include <sys/select.h>
#  include <sys/time.h>
#  include <unistd.h>
#endif
#include <stdexcept>

using namespace std;

#include "IO.h"
#include "Util.h"

#if defined(linux) && defined(ENABLE_LINUX_PPDEV)
#  include "LinuxPPDevIO.h"
#endif

#include "DirectPPIO.h"

Preferences *IO::config = NULL;

IO *IO::acquire(Preferences *cfg, char *name, int port)
{
    IO::config = cfg;

#if defined(linux) && defined(ENABLE_LINUX_PPDEV)
    if (strcasecmp(name, "LinuxPPDev") == 0) {
        return new LinuxPPDevIO(port);
    }
#endif
    if (strcasecmp(name, "DirectPP") == 0) {
        return new DirectPPIO(port);
    }
    throw runtime_error("Unknown IO driver selected");
}

IO::IO(int port)
{
    production_   = false;
    tdly_stretch_ = 0;
    tset_stretch_ = 0;
    thld_stretch_ = 0;
    gen_stretch_  = 0;
}

IO::~IO()
{
}

#ifdef  WIN32
#  pragma optimize( "", off )
#endif

void IO::usleep(unsigned long us)
{
    us += this->gen_stretch_;

    if (us <= 0) {
        return;
    }

#ifdef WIN32

LARGE_INTEGER i1, i2;

    QueryPerformanceCounter(&i1);
    do {
        QueryPerformanceCounter(&i2);
    } while ( (unsigned long)(i2.QuadPart - i1.QuadPart) < us );

#else

struct timeval now, later;

    if (us >= 10000) {   /* 10ms */
        later.tv_sec = us / 1000000;
        later.tv_usec = us % 1000000;
        select(0, NULL, NULL, NULL, &later);
        return;
    }
    gettimeofday(&later, NULL);
    later.tv_sec += us / 1000000;
    later.tv_usec += us % 1000000;

    while (later.tv_usec > 1000000) {
        later.tv_usec -= 1000000;
        later.tv_sec += 1;
    }
    while (1) {
        gettimeofday(&now, NULL);
        if (
            (now.tv_sec > later.tv_sec) ||
            ((now.tv_sec == later.tv_sec) && (now.tv_usec > later.tv_usec))
        ) {
            break;
        }
    }

#endif
}

void IO::shift_bits_out(uint32_t bits, int numbits, int tset, int thold)
{
    while (numbits > 0) {
        this->clock(true);
        this->data(bits & 0x01);

        /* Delay for data setup time */
        this->usleep(tset + this->tset_stretch_);

        /* Falling edge */
        this->clock(false);

        /* Delay for data hold time */
        this->usleep(thold + this->thld_stretch_);

        bits >>= 1;
        numbits--;
    }
}

uint32_t IO::shift_bits_in(int numbits, int tdly)
{
uint32_t data, mask;

    data = 0;
    mask = 0x00000001;
    this->data(true);
    while (numbits > 0) {
        this->clock(true);
        this->usleep(tdly + this->tdly_stretch_);
        if (this->data()) {
            data |= mask;
        }
        this->clock(false);
        this->usleep(tdly + this->tdly_stretch_);
        mask <<= 1;
        numbits--;
    }
    this->data(false);
    return data;
}
