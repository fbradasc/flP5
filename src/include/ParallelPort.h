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
 * $Id: ParallelPort.h,v 1.3 2002/09/12 02:11:07 marka Exp $
 */
#ifndef __ParallelPort_h
#define __ParallelPort_h

#include <map>

#include "IO.h"
#include "LptPorts.h"

/** \file */

#if defined(ENABLE_LINUX_GPIO)
#  if defined(RPI_MODEL_PLUS)
#    define NIOP 40
#  else
#    define NIOP 26
#  endif
#else
#  define NIOP 25
#endif

#define PIN_INP ( 1 << 0 )
#define PIN_OUT ( 1 << 1 )
#define PIN_5V0 ( 1 << 2 )
#define PIN_3V3 ( 1 << 3 )
#define PIN_GND ( 1 << 4 )
#define PIN_NEG ( 1 << 5 )

class IOPin {
    public:
        char           getIdx () const { return idx ; }
        char           getReg () const { return reg ; }
        char           getBit () const { return bit ; }
        unsigned short getMode() const { return mode; }

        char           idx;
        char           reg;
        char           bit;
        unsigned short mode;
};

typedef std::map<std::string, IOPin> IOControls;

#define GET_PIN_IDX(name) ParallelPort::controls[name].getIdx()
#define GET_PIN_REG(name) ParallelPort::controls[name].getReg()
#define GET_PIN_BIT(name) ParallelPort::controls[name].getBit()
#define GET_PIN_NEG(name) ((ParallelPort::controls[name].getMode() & PIN_NEG) ? true : false)
#define GET_PIN_DIR(name) ((ParallelPort::controls[name].getMode() & PIN_INP) ? true : false)
#define LAST_PIN          ParallelPort::controls.size()

/** This class contains methods and data elements that are common to all IO
 * implementation attached to a parallel port.
 */
class ParallelPort : public IO
{
public:
    /** This constructor is called right before a parallel port IO class is
     * created. It the responsibility of this function to read in the
     * configuration variables specific to parallel port programmers.
     * \param port The parallel port number that is being opened.
     */
    ParallelPort(int port);

    /** Frees all memory and resources associated with this object */
    virtual ~ParallelPort();

    virtual void clock(bool state);
    virtual void data(bool state);
    virtual bool data(void);
    virtual void vpp(VppMode mode);
    virtual void vdd(VddMode mode);

    static LptPorts ports;

    static const IOPin pins[NIOP];

    static IOControls controls;

protected:
    /** The parallel port number this object is using */
    int port;

    int vddMinCond;       /** Pins to activate to select Vdd Min value  */
    int vddProgCond;      /** Pins to activate to select Vdd Prog value */
    int vddMaxCond;       /** Pins to activate to select Vdd Max value  */
    int vppOffCond;       /** True if the selVihhVpp pin has to be off before
                           *  setting off the icspVppOn pin */

    virtual void set_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert,
        bool state
    ) = 0;

    virtual bool get_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert
    ) = 0;

    void set_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert,
        bool state,
        struct signal_delays *delays
    );

    bool get_pin_state (
        const char *name,
        short reg,
        short bit,
        short invert,
        struct signal_delays *delays
    );

private:
    static IOControls init_controls();
    static void       add_control  (IOControls &cl, const char *name=NULL, unsigned short dir=PIN_INP);
};

#endif
