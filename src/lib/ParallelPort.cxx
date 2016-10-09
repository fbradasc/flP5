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
 * $Id: ParallelPort.cxx,v 1.6 2002/10/11 16:26:56 marka Exp $
 */
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "ParallelPort.h"
#include "Util.h"

LptPorts ParallelPort::ports;

#define SETPINDATA(name, prefix, pin)                                         \
{                                                                             \
int svalue=0;                                                                 \
                                                                              \
    svalue = pin;                                                             \
    this->prefix##Invert = 0;                                                 \
    if (svalue < 0) {                                                         \
        this->prefix##Invert = 1;                                             \
        svalue *= -1;                                                         \
    }                                                                         \
    if (svalue == 0) { /* signal not used by the programmer */                \
        this->prefix##Reg = -1;                                               \
        this->prefix##Bit = -1;                                               \
    } else if ((svalue > 25) || (pin2reg[svalue-1] == -1)) {                  \
        throw runtime_error (                                                 \
            "Invalid value for configuration parameter "                      \
            name "Pin"                                                        \
        );                                                                    \
    } else {                                                                  \
        if (hw_invert[svalue-1]) {                                            \
            this->prefix##Invert ^= 1;                                        \
        }                                                                     \
        this->prefix##Reg = pin2reg[svalue-1];                                \
        this->prefix##Bit = pin2bit[svalue-1];                                \
    }                                                                         \
}

#define READPINDATA(name,prefix)                                              \
{                                                                             \
int value=0;                                                                  \
                                                                              \
    config->get(name,value,0);                                                \
    SETPINDATA(name,prefix,value);                                            \
}

#define SET_BIT_COMMON(name,prefix) this->set_bit_common (                    \
        name,                                                                 \
        this->prefix##Reg,                                                    \
        this->prefix##Bit,                                                    \
        this->prefix##Invert,                                                 \
        state                                                                 \
    )

#define GET_BIT_COMMON(name,prefix) this->get_bit_common (                    \
        name,                                                                 \
        this->prefix##Reg,                                                    \
        this->prefix##Bit,                                                    \
        this->prefix##Invert                                                  \
    )

/* Mapping of parallel port pin #'s to I/O register offset. */
static char pin2reg[25] = {
    2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 1, 2, 2,
    -1, -1, -1, -1, -1, -1, -1, -1
};

/* Mapping of parallel port pin #'s to I/O register bit positions. */
static char pin2bit[25] = {
    0, 0, 1, 2, 3, 4, 5, 6, 7, 6, 7, 5, 4, 1, 3, 2, 3,
    -1, -1, -1, -1, -1, -1, -1, -1
};

/* Flags for each pin, indicating if the parallel port hardware inverts the
 * value. (BSY, STB, ALF, DSL) */
static char hw_invert[25] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0
};

ParallelPort::ParallelPort(int port) : IO(port)
{
char programmer[20];
int vregs, val;
char *message = "Either none or at least two of:\n\n" \
                "\t%s.vminpin\n" \
                "\t%s.vprgpin\n" \
                "\t%s.vmaxpin\n\n" \
                "need to be set.";

    READPINDATA( "icspClock"  , icspClock   );
    READPINDATA( "icspDataIn" , icspDataIn  );
    READPINDATA( "icspDataOut", icspDataOut );
    READPINDATA( "icspVddOn"  , icspVddOn   );
    READPINDATA( "icspVppOn"  , icspVppOn   );
    READPINDATA( "selMinVdd"  , selMinVdd   );
    READPINDATA( "selProgVdd" , selProgVdd  );
    READPINDATA( "selMaxVdd"  , selMaxVdd   );
    READPINDATA( "selVihhVpp" , selVihhVpp  );

    config->get("vddMinCond"        ,vddMinCond ,0);
    config->get("vddProgCond"       ,vddProgCond,0);
    config->get("vddMaxCond"        ,vddMaxCond ,0);
    config->get("vppOffCond"        ,vppOffCond ,0);
    config->get("hasSAVddVppControl",val        ,0);

    hasSAVddVppControl((val!=0));
 
    vregs = 0;
    vregs += (selMaxVddBit >=0) ? 1 : 0;
    vregs += (selProgVddBit>=0) ? 1 : 0;
    vregs += (selMinVddBit >=0) ? 1 : 0;

    this->production( (vregs>0) ? true : false );
}

ParallelPort::~ParallelPort()
{
}

void ParallelPort::clock(bool state)
{
    SET_BIT_COMMON("icspClock", icspClock);
}   
    
void ParallelPort::data(bool state)
{
    SET_BIT_COMMON("icspDataOut", icspDataOut);
}  

bool ParallelPort::data(void)
{
    return GET_BIT_COMMON("icspDataIn", icspDataIn);
}

void ParallelPort::vpp(VppMode mode)
{
bool state;

    switch (mode) {
        case VPP_TO_VIH:
            if (this->vppOffCond) {
                state = true; SET_BIT_COMMON("icspVppOn", icspVppOn);
            }
            if (this->selVihhVppBit>=0) {
                state = true; SET_BIT_COMMON("selVihhVpp", selVihhVpp);
            }
            if (!this->vppOffCond) {
                state = true; SET_BIT_COMMON("icspVppOn", icspVppOn);
            }
        break;
        case VPP_TO_GND:
            if (this->vppOffCond && this->selVihhVppBit>=0) {
                state = false; SET_BIT_COMMON("selVihhVpp", selVihhVpp);
            }
            state = false; SET_BIT_COMMON("icspVppOn", icspVppOn);
        break;
        case VPP_TO_VDD:
            if (this->selVihhVppBit>=0) {
                state = false; SET_BIT_COMMON("selVihhVpp", selVihhVpp);
            }
            state = true; SET_BIT_COMMON("icspVppOn", icspVppOn);
        break;
    }
}

void ParallelPort::vdd(VddMode mode)
{
bool state;

    if (mode==VDD_TO_OFF) {
        state = false; SET_BIT_COMMON("icspVddOn", icspVddOn);
    } else if (mode==VDD_TO_ON) {
        state = true; SET_BIT_COMMON("icspVddOn", icspVddOn);
    } else if (this->production()) {
        switch (mode) {
            case VDD_TO_ON:
            case VDD_TO_OFF:
                /* hadled in the above if ... */
            break;
            case VDD_TO_MIN:
                if (this->selMaxVddBit>=0) {
                    state = (vddMinCond & 4);
                    SET_BIT_COMMON("selMaxVdd", selMaxVdd);
                }
                if (this->selProgVddBit>=0) {
                    state = (vddMinCond & 2);
                    SET_BIT_COMMON("selProgVdd", selProgVdd);
                }
                if (this->selMinVddBit>=0) {
                    state = (vddMinCond & 1);
                    SET_BIT_COMMON("selMinVdd", selMinVdd);
                }
            break;
            case VDD_TO_PRG:
                if (this->selMaxVddBit>=0) {
                    state = (vddProgCond & 4);
                    SET_BIT_COMMON("selMaxVdd", selMaxVdd);
                }
                if (this->selMinVddBit>=0) {
                    state = (vddProgCond & 1);
                    SET_BIT_COMMON("selMinVdd", selMinVdd);
                }
                if (this->selProgVddBit>=0) {
                    state = (vddProgCond & 2);
                    SET_BIT_COMMON("selProgVdd", selProgVdd);
                }
            break;
            case VDD_TO_MAX:
                if (this->selMinVddBit>=0) {
                    state = (vddMaxCond & 1);
                    SET_BIT_COMMON("selMinVdd", selMinVdd);
                }
                if (this->selProgVddBit>=0) {
                    state = (vddMaxCond & 2);
                    SET_BIT_COMMON("selProgVdd", selProgVdd);
                }
                if (this->selMaxVddBit>=0) {
                    state = (vddMaxCond & 4);
                    SET_BIT_COMMON("selMaxVdd", selMaxVdd);
                }
            break;
        }
    }
}
