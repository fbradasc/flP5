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

IO *IO::acquire(Preferences *cfg, const char *name, int port)
{
IO *io;

    IO::config = cfg;

#if defined(linux) && defined(ENABLE_LINUX_PPDEV)
    if (strcasecmp(name, "LinuxPPDev") == 0) {
        io = new LinuxPPDevIO(port);
    } else
#endif
    if (strcasecmp(name, "DirectPP") == 0) {
        io = new DirectPPIO(port);
    } else {
        throw runtime_error("Unknown IO driver selected");
    }
    io->initialize();
    io->off();

    return io;
}

IO::IO(int port)
{
long default_delay, additional_delay;
struct signal_delays tmp_delays;

    production_   = false;

    /* Read the signal delay values */
    config->get("signalDelay.default",    (int &)default_delay,    0);
    config->get("signalDelay.additional", (int &)additional_delay, 0);

    read_signal_delay (
        "clk", this->clk_delays_, default_delay, additional_delay
    );
    read_signal_delay (
        "data", this->data_delays_, default_delay, additional_delay
    );
    read_signal_delay (
        "vdd", this->vdd_delays_, default_delay, additional_delay
    );
    read_signal_delay (
        "vpp", this->vpp_delays_, default_delay, additional_delay
    );
}

void IO::read_signal_delay (
    const char *name,
    struct signal_delays &delays,
    nanotime_t default_delay,
    nanotime_t additional_delay
) {
long value;

    /* Get the input delay */
    if ( !config->get (
            Preferences::Name("signalDelay.read.%s",name),
            (int &)delays.read,
            (int)default_delay
         )
    ) {
        config->get("signalDelay.read",(int &)delays.read,default_delay);
    }
    /* Get the output delays */
    if ( !config->get (
            Preferences::Name("signalDelay.write.%s",name),
            (int &)value,
            (int)default_delay
         )
    ) {
        config->get("signalDelay.write",(int &)value,default_delay);
    }
    config->get (
        Preferences::Name("signalDelay.write.%s.high_to_low",name),
        (int &)delays.high_to_low,
        (int)value
    );
    config->get (
        Preferences::Name("signalDelay.write.%s.low_to_high",name),
        (int &)delays.low_to_high,
        (int)value
    );
    delays.read        += additional_delay;
    delays.high_to_low += additional_delay;
    delays.low_to_high += additional_delay;
}

void IO::pre_read_delay(struct signal_delays &delays)
{
microtime_t us;

    /* Convert nanoseconds to microseconds, rounding up */
    us = (delays.read + 999) / 1000;
    this->usleep(us);
}

void IO::post_set_delay(struct signal_delays &delays, bool prev, bool current)
{
microtime_t us;

    /* Convert nanoseconds to microseconds, rounding up */
    if((prev == false) && (current == true)) {
        us = (delays.low_to_high + 999) / 1000;
    } else if((prev == true) && (current == false)) {
        us = (delays.high_to_low + 999) / 1000;
    } else {
        return;
    }
    this->usleep(us);
}

IO::~IO()
{
}

#ifdef  WIN32
#  pragma optimize( "", off )
#endif

void IO::usleep(microtime_t us)
{
    if (us <= 0) {
        return;
    }

#ifdef WIN32

LARGE_INTEGER i1, i2;

    QueryPerformanceCounter(&i1);
    do {
        QueryPerformanceCounter(&i2);
    } while ( (uint32_t)(i2.QuadPart - i1.QuadPart) < us );

#else

struct timeval now, later;

    if (us >= 10000) {   /* 10ms */
        later.tv_sec = us / 1000000;
        later.tv_usec = us % 1000000;
        select(0, NULL, NULL, NULL, &later);
        return;
    }
    /* Busy-wait for SPEED */
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

microtime_t IO::now()
{
#ifdef WIN32

LARGE_INTEGER i;

    QueryPerformanceCounter(&i);

    return (microtime_t)i.QuadPart;
#else

struct timeval now;

    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000000) + now.tv_usec;
#endif
}

void IO::shift_bits_out (
    uint32_t bits,
    int numbits,
    microtime_t tset,
    microtime_t thold
) {
bool hold_clock = false;

    if (numbits<0) {
        hold_clock  = true;
        numbits    *= -1;
    }
    while (numbits > 0) {
        this->clock(true);
        this->data(bits & 0x01);

        /* Delay for data setup time */
        this->usleep(tset);

        /* Falling edge */
        this->clock(false);

        /* if hld_clock==true, keep the clock high after the last data bit */
        if (numbits>1 || !hold_clock) {
           /* Falling edge */
           this->clock(false);
        }
        /* Delay for data hold time */
        this->usleep(thold);

        bits >>= 1;
        numbits--;
    }
}

uint32_t IO::shift_bits_in(int numbits, microtime_t tdly, microtime_t tlow)
{
uint32_t data, mask;

    data = 0;
    mask = 0x00000001;
    this->data(true);
    while (numbits > 0) {
        this->clock(true);
        this->usleep(tdly);
        if (this->data()) {
            data |= mask;
        }
        this->clock(false);
        this->usleep(tlow);
        mask <<= 1;
        numbits--;
    }
    this->data(false);
    return data;
}

uint32_t IO::shift_bits_out_in (
    uint32_t bits,
    int numbits,
    microtime_t tset,
    microtime_t thold
) {
bool hold_clock = false;
uint32_t data, mask;

    data = 0;
    mask = 0x00000001;

    if (numbits<0) {
        hold_clock  = true;
        numbits    *= -1;
    }
    while (numbits > 0) {
        this->data(bits & 0x01); // set output bit

        this->clock(true);

        /* Delay for data setup time */
        this->usleep(tset);

        if (this->data()) { // get input bit
            data |= mask;
        }

        /* if hld_clock==true, keep the clock high after the last data bit */
        if (numbits>1 || !hold_clock) {
           /* Falling edge */
           this->clock(false);
        }
        /* Delay for data hold time */
        this->usleep(thold);

        mask <<= 1;
        bits >>= 1;
        numbits--;
    }
    return data;
}

void IO::off(void)
{
    vpp   (VPP_TO_VDD);
    clock (false);
    data  (false);
    vdd   (VDD_TO_OFF);
    // vdd   (VDD_TO_PRG);
}
