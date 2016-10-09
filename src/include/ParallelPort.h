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

#include "IO.h"
#include "LptPorts.h"


/** \file */

#define PARPORT_PINS 25

/** This class contains methods and data elements that are common to all IO
 * implementation attached to a parallel port.
 */
class ParallelPort : public IO
{
public:
    typedef enum PP_PINS {
        OFF=0,
        SCK,
        SDI_MISO,
        SDO_MOSI,
        VDD_VCC,
        VPP_RESET,
        SEL_MIN_VDD,
        SEL_PRG_VDD,
        SEL_MAX_VDD,
        SEL_VIHH_VPP,
        LAST_PIN
    } PpPins;

    /** This constructor is called right before a parallel port IO class is
     * created. It the responsibility of this function to read in the
     * configuration variables specific to parallel port programmers.
     * \param port The parallel port number that is being opened.
     */
    ParallelPort(int port);

    /** Frees all memory and resources associated with this object */
    virtual ~ParallelPort();

    virtual void reset(bool state); // vpp((state) ? VPP_TO_GND : VPP_TO_VDD);
    virtual void clock(bool state);
    virtual void data(bool state);
    virtual bool data(void);
    virtual void vpp(VppMode mode);
    virtual void vdd(VddMode mode);

    static  char getReg(int pin);
    static  char getBit(int pin);
    static  bool hwInvert(int pin);

    static  LptPorts ports;

protected:
    void initialize();

    /** The parallel port number this object is using */
    int port;

    int pins[PARPORT_PINS]; /** parport pins settings                     */
    int vddMinCond;         /** Pins to activate to select Vdd Min value  */
    int vddProgCond;        /** Pins to activate to select Vdd Prog value */
    int vddMaxCond;         /** Pins to activate to select Vdd Max value  */
    int vppOffCond;         /** True if the selVihhVpp pin has to be off  *
                             ** before setting off the icspVppOn pin      */

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
        PpPins connection,
        bool state,
        struct signal_delays *delays
    );

    bool get_pin_state (
        const char *name,
        PpPins connection,
        struct signal_delays *delays
    );
};

#endif
