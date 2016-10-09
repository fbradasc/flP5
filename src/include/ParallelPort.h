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

#define PINDECL(prefix) short prefix##Reg, prefix##Bit, prefix##Invert

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

protected:
    /** The parallel port number this object is using */
    int port;

    PINDECL(icspDataIn);  /** Par. port pin data for the read data signal    */
    PINDECL(icspDataOut); /** Par. port pin data for the write data signal   */
    PINDECL(icspClock);   /** Par. port pin data for the clock signal        */
    PINDECL(icspVppOn);   /** Par. port pin data for the Vpp enable signal   */
    PINDECL(icspVddOn);   /** Par. port pin data for the Vdd enable signal   */
    PINDECL(selMinVdd);   /** Par. port pin data for the Vdd Min sel. signal */
    PINDECL(selProgVdd);  /** Par. port pin data for the Vdd Prg sel. signal */
    PINDECL(selMaxVdd);   /** Par. port pin data for the Vdd Max sel. signal */
    PINDECL(selVihhVpp);  /** Par. port pin data for the Vpp VIH sel. signal */

    int vddMinCond;       /** Pins to activate to select Vdd Min value  */
    int vddProgCond;      /** Pins to activate to select Vdd Prog value */
    int vddMaxCond;       /** Pins to activate to select Vdd Max value  */
    int vppOffCond;       /** True if the selVihhVpp pin has to be off before
                           *  setting off the icspVppOn pin */

    virtual void set_pin_state (
        char *name,
        short reg,
        short bit,
        short invert,
        bool state
    ) = 0;

    virtual bool get_pin_state (
        char *name,
        short reg,
        short bit,
        short invert
    ) = 0;

    void set_pin_state (
        char *name,
        short reg,
        short bit,
        short invert,
        bool state,
        struct signal_delays *delays
    );

    bool get_pin_state (
        char *name,
        short reg,
        short bit,
        short invert,
        struct signal_delays *delays
    );

};

#endif
