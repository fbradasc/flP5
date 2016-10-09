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

#if defined(WIN32) && !defined(__MINGW32__)
typedef unsigned long uint32_t;
#  else
#  include <stdint.h>
#endif

#include "Preferences.h"

/** \file */

#define BITS_AND_CLK(numbits,hold_clock) ((numbits) * ((hold_clock) ? -1 : 1))

/** A time value represented in microseconds */
typedef unsigned long microtime_t;

/** A time value represented in nanoseconds */
typedef unsigned long nanotime_t;


/** This contains the various signal propagation delays added by the
 * programming hardware and cable. */
struct signal_delays {
    /** The propagation delay when setting the signal from high to low */
    nanotime_t high_to_low;

    /** The propagation delay when setting the signal from low to high */
    nanotime_t low_to_high;

    /** The propagation delay for reads of the signal */
    nanotime_t read;
};

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
    static IO *acquire(Preferences *config, const char *name, int port = 0);

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

    /** Turns everything off - IO::acquire does this at the
     * beginning and the individual IO destructors do it at the end. */
    virtual void off(void);

    /** Sets the state of the reset signal.
     * \param state The new state of the reset signal.
     */
    virtual void reset(bool state) = 0;

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
    virtual void usleep(microtime_t us);

    /** Get the current time
     * \return the current time in microseconds
     */
    virtual microtime_t now();

    /**
     * Sends a stream of up to 32 bits on the data signal, clocked by the
     * clock signal. The bits are sent LSB first and it is assumed that
     * the device will latch the data on the falling edge of the clock.
     *
     *           __________               ____________             
     *          /          \             /            \            
     * clock   /            \___________/              \___________
     *
     *             <-tset->  <-thold->        <-tset->  <-thold->
     *        ...  ___________________......  ___________________..
     *  data     \/                   ......\/                   ..
     *        .../\___________________....../\___________________..
     *
     *           ^ shift_out is entered here    and returns here ^
     *
     * \param bits The data bits to send out, starting at the LSB.
     * \param numbits The number of bits to send out.
     * \param tset Setup time for the data before the clock's falling edge.
     * \param thold The data hold time after the clock's falling edge.
     */
    virtual void shift_bits_out (
        uint32_t bits,
        int numbits,
        microtime_t tset  = 1,
        microtime_t thold = 1
    );

    /**
     * Reads a stream of up to 32 bits from the data signal, clocked by
     * us controlling the clock signal. The bits are received LSB first
     * and are read no earlier than tdly after the rising edge of the clock.
     *
     *          _________           _________
     *         /         \         /         \
     * clock  /           \_______/           \_______
     *
     *          <-tdly->  <-tlow->  <-tdly->  <-tlow->
     *        ........  _.................  _.........
     *  data  ....... \/ ................ \/ .........
     *        ......../\_................./\_.........
     *
     *                  ^data must be stable and is read
     *        ^ shift_in entered here   returns here ^
     *
     * \param numbits The number of bits to read in and return.
     * \param tdly The delay time from the clock's rising edge to data
     *             valid (from the device).
     * \param tlow The minimum clock low time
     * \returns The bits which were read.
     */
    virtual uint32_t shift_bits_in (
        int numbits,
        microtime_t tdly = 1,
        microtime_t tlow = 1
    );

    virtual uint32_t shift_bits_out_in (
        uint32_t bits,
        int numbits,
        microtime_t tset  = 1,
        microtime_t thold = 1
    );

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

protected:
    /** Constructor
     * \param port A subclass specific port number.
     */
    IO(int port);

    /** To initialize the status of the parallel port pin state 
     */
    virtual void initialize() = 0;

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

    /** Reads the signal propagation delays from the configuration file.
     * \param name The name of the signal
     * \param delays The signal_delay structure to fill in
     * \param default_delay The default delay time to use if an entry in the
     *     config file is not found.
     * \param additional_delay Additional delay that will be added to the
     *     delay that is read from the config file.
     */
    void read_signal_delay (
        const char *name,
        struct signal_delays &delays,
        nanotime_t default_delay,
        nanotime_t additional_delay
    );

    /** Executes any delays required before reading the value of a signal.
     * \param delays Pointer to the delay information for the signal
     */
    void pre_read_delay(struct signal_delays &delays);

    /** Executes any delays required after changing the state of a signal.
     * \param delays Pointer to the delay information for the signal
     * \param prev The previous state of the signal
     * \param current The current state of the signal
     */
    void post_set_delay (
        struct signal_delays &delays,
        bool prev,
        bool current
    );

    /** The propagation delays on the clock signal */
    struct signal_delays clk_delays_;

    /** The propagation delays on the data read and write signals */
    struct signal_delays data_delays_;

    /** The propagation delays on the Pwr signal */
    struct signal_delays vdd_delays_;

    /** The propagation delays on the Vpp signal */
    struct signal_delays vpp_delays_;
};


#endif
