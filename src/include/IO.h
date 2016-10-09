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
 * $Id: IO.h,v 1.3 2002/09/11 02:35:58 marka Exp $
 */
#ifndef __IO_h
#define __IO_h

#ifdef WIN32
typedef unsigned long uint32_t;
#  else
#  include <stdint.h>
#endif

#include "Preferences.h"

/** \file */


/**
 * A base class for lowlevel access methods to the PIC programmer.
 */
class IO
{
public:
    typedef enum _VddMode {
        VDD_TO_OFF=0,
        VDD_TO_ON,
        VDD_TO_MIN,
        VDD_TO_PRG,
        VDD_TO_MAX
    } VddMode;

    typedef enum _VppMode {
        VPP_TO_VIH=0,
        VPP_TO_GND,
        VPP_TO_VDD
    } VppMode;

    /** Creates an instance of an IO class from it's name.
     * \param config The settings for the IO hardware to use.
     * \param name The name of the IO hardware to use.
     * \param port The port number to pass to the specific subclass
     *        constructor.
     * \returns An instance of the IO subclass that implements the
     *          hardware interface specified by \c name.
     * \throws runtime_error Contains a textual description of the error.
     */
    static IO *acquire(Preferences *config, char *name, int port = 0);

    /** Sets the type of the programmer: for production or for developement
     * \param state True if the programmer is for production
     */
    void production(bool state) { production_ = state; }

    /** Gets the type of the programmer
     * \returns true if the programmer is for production
     */
    bool production(void) { return production_; }

    /** Sets the progammer Vdd/Vpp control method
     * \param state True if the programmer can switch Vdd and Vpp on/off
     *              independently from each other
     */
    void hasSAVddVppControl(bool state) { saVddVppControl_ = state; }

    /** Gets the progammer Vdd/Vpp control method
     * \returns true if the programmer can switch Vdd and Vpp on/off
     *          independently from each other
     */
    bool hasSAVddVppControl(void) { return saVddVppControl_; }

    /** Frees all the memory and resources associated with this object */
    virtual ~IO();

    /** Sets the state of the clock signal.
     * \param state The new state of the clock signal.
     */
    virtual void clock(bool state) = 0;

    /** Sets the state of the data signal.
     * \param state The new state of the data signal.
     */
    virtual void data(bool state) = 0;

    /** Gets the state of the data signal.
     * \return The boolean state of the data signal.
     */
    virtual bool data(void) = 0;

    /** Sets the state of the programming voltage.
     * \param state The new state of the programming voltage.
     */
    virtual void vpp(VppMode mode) = 0;

    /** Sets the state of Vcc to the device.
     * \param state The new state of supply voltage.
     */
    virtual void vdd(VddMode mode) = 0;

    /** Waits for a specified number of microseconds to pass.
     * \param us The number of microseconds to delay for.
     */
    virtual void usleep(unsigned long us);

    /** Additional delay to add to Tdly. This will stretch out the length
     * of the clock signal by this many microseconds.
     */
    void          set_tdly_stretch(unsigned long s) { tdly_stretch_ = s;    }
    unsigned long get_tdly_stretch()                { return tdly_stretch_; }

    /** Additional delay to add to Tset. This will increase the data setup
     * time by this many microseconds.
     */
    void          set_tset_stretch(unsigned long s) { tset_stretch_ = s;    }
    unsigned long get_tset_stretch()                { return tset_stretch_; }

    /** Additional delay to add to Thld. This will increase the data hold
     * time by this many microseconds.
     */
    void          set_thld_stretch(unsigned long s) { thld_stretch_ = s;    }
    unsigned long get_thld_stretch()                { return thld_stretch_; }

    /** General additional delay. This extra time will be added to every
     * delay that occurs. Use this to slow down the entire programming
     * operation.
     */
    void          set_gen_stretch(unsigned long s) { gen_stretch_ = s;    }
    unsigned long get_gen_stretch()                { return gen_stretch_; }

    /**
     * Sends a stream of up to 32 bits on the data signal, clocked by the
     * clock signal. The bits are sent LSB first and it is assumed that
     * the device will latch the data on the falling edge of the clock.
     *
     * \param bits The data bits to send out, starting at the LSB.
     * \param numbits The number of bits to send out.
     * \param tset Setup time for the data before the clock's falling
     *        edge (in microseconds).
     * \param thold The data hold time after the clock's falling
     *        edge (in microseconds).
     */
    virtual void shift_bits_out (
        uint32_t bits,
        int numbits,
        int tset=0,
        int thold=0
    );

    /**
     * Reads a stream of up to 32 bits from the data signal, clocked by
     * us controlling the clock signal. The bits are received LSB first
     * and are read tdly microseconds after the rising edge of the clock.
     *
     * \param numbits The number of bits to read in and return.
     * \param tdly The delay time (microseconds) from the clock's rising
     *        edge to data valid (from the device).
     * \returns The bits which were read.
     */
    virtual uint32_t shift_bits_in(int numbits, int tdly=0);

    virtual void set_bit_common (
        char *name,
        short reg,
        short bit,
        short invert,
        bool state
    ) = 0;

    virtual bool get_bit_common (
        char *name,
        short reg,
        short bit,
        short invert
    ) = 0;

protected:
    /** Constructor
     * \param port A subclass specific port number.
     */
    IO(int port);

    /** The programmer attached to the parallel port is a production programmer
     */
    bool production_;

    /** The programmer attached to the parallel port can switch Vdd and Vpp
     *  on/off independently from each other
     */
    bool saVddVppControl_;

    /** The settings for the programmer to use
     */
    static Preferences *config;

    /** Additional delay to add to Tdly. This will stretch out the length
     * of the clock signal by this many microseconds.
     */
    unsigned long tdly_stretch_;

    /** Additional delay to add to Tset. This will increase the data setup
     * time by this many microseconds.
     */
    unsigned long tset_stretch_;

    /** Additional delay to add to Thld. This will increase the data hold
     * time by this many microseconds.
     */
    unsigned long thld_stretch_;

    /** General additional delay. This extra time will be added to every
     * delay that occurs. Use this to slow down the entire programming
     * operation.
     */
    unsigned long gen_stretch_;
};


#endif
