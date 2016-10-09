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
    } else if ((svalue > 25) || (getReg(svalue-1) == -1)) {                   \
        throw runtime_error (                                                 \
            "Invalid value for configuration parameter "                      \
            name "Pin"                                                        \
        );                                                                    \
    } else {                                                                  \
        if (hwInvert(svalue-1)) {                                             \
            this->prefix##Invert ^= 1;                                        \
        }                                                                     \
        this->prefix##Reg = getReg(svalue-1);                                 \
        this->prefix##Bit = getBit(svalue-1);                                 \
    }                                                                         \
}

#define SET_PIN_STATE(function,name,connection) this->set_pin_state (         \
        name,                                                                 \
        connection,                                                           \
        state,                                                                \
        &this->function##_delays_                                             \
    );                                                                        \

#define GET_PIN_STATE(function,name,connection) this->get_pin_state (         \
        name,                                                                 \
        connection,                                                           \
        &this->function##_delays_                                             \
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
int vregs, val;

    vregs = 0;
    for (int i=0;i<PARPORT_PINS;i++) {
        config->get(Preferences::Name("parportPin_%02d",i),val,OFF);

        if ((val != OFF) && (val != (OFF+LAST_PIN)) && (getReg(i) == -1)) {
            throw runtime_error (
                (const char *)Preferences::Name (
                    "Invalid value for configuration parameter parportPin_%02d", i
                )
            );
        } else {
            if (hwInvert(i)) {
                if (val>=LAST_PIN) {
                    val -= LAST_PIN;
                } else {
                    val += LAST_PIN;
                }
            }
            pins[i] = val;
            if ((pins[i]==SEL_MIN_VDD) ||
                (pins[i]==SEL_PRG_VDD) ||
                (pins[i]==SEL_MAX_VDD) ||
                (pins[i]==SEL_MIN_VDD+LAST_PIN) ||
                (pins[i]==SEL_PRG_VDD+LAST_PIN) ||
                (pins[i]==SEL_MAX_VDD+LAST_PIN)) {
                vregs++;
            }
        }
    }

    config->get("vddMinCond"        ,vddMinCond ,0);
    config->get("vddProgCond"       ,vddProgCond,0);
    config->get("vddMaxCond"        ,vddMaxCond ,0);
    config->get("vppOffCond"        ,vppOffCond ,0);
    config->get("hasSAVddVppControl",val        ,0);

    hasSAVddVppControl((val!=0));
 
    this->production( (vregs>0) ? true : false );
}

ParallelPort::~ParallelPort()
{
}

char ParallelPort::getReg(int pin)
{
    return ( (pin >= 0) && (pin < 25) ) ? pin2reg[pin] : -1;
}

char ParallelPort::getBit(int pin)
{
    return ( (pin >= 0) && (pin < 25) ) ? pin2bit[pin] : -1;
}

bool ParallelPort::hwInvert(int pin)
{
    return ( (pin >= 0) && (pin < 25) ) ? hw_invert[pin] : false;
}

void ParallelPort::reset(bool state)
{
    vpp((state) ? VPP_TO_GND : VPP_TO_VDD);
}

void ParallelPort::clock(bool state)
{
    SET_PIN_STATE(clk, "SCK", SCK);
}   
    
void ParallelPort::data(bool state)
{
    SET_PIN_STATE(data, "SDO/MOSI", SDO_MOSI);
}  

bool ParallelPort::data(void)
{
    return GET_PIN_STATE(data, "SDI/MISO", SDI_MISO);
}

void ParallelPort::vpp(VppMode mode)
{
bool state;

    switch (mode) {
        case VPP_TO_VIH:
            if (this->vppOffCond) {
                state = true; SET_PIN_STATE(vpp, "VPP/RESET", VPP_RESET);
            }
            state = true; SET_PIN_STATE(vpp, "Select Vihh", SEL_VIHH_VPP);
            if (!this->vppOffCond) {
                state = true; SET_PIN_STATE(vpp, "VPP/RESET", VPP_RESET);
            }
        break;
        case VPP_TO_GND:
            if (this->vppOffCond) {
                state = false; SET_PIN_STATE(vpp, "Select Vihh", SEL_VIHH_VPP);
            }
            state = false; SET_PIN_STATE(vpp, "VPP/RESET", VPP_RESET);
        break;
        case VPP_TO_VDD:
            state = false; SET_PIN_STATE(vpp, "Select Vihh", SEL_VIHH_VPP);
            state = true; SET_PIN_STATE(vpp, "VPP/RESET", VPP_RESET);
        break;
    }
}

void ParallelPort::vdd(VddMode mode)
{
bool state;

    if (mode==VDD_TO_OFF) {
        state = false; SET_PIN_STATE(vdd, "VDD/VCC", VDD_VCC);
    } else if (mode==VDD_TO_ON) {
        state = true; SET_PIN_STATE(vdd, "VDD/VCC", VDD_VCC);
    } else if (this->production()) {
        switch (mode) {
            case VDD_TO_ON:
            case VDD_TO_OFF:
                /* hadled in the above if ... */
            break;
            case VDD_TO_MIN:
                state = (vddMinCond & 4);
                SET_PIN_STATE(vdd, "Select Vdd Max", SEL_MAX_VDD);
                state = (vddMinCond & 2);
                SET_PIN_STATE(vdd, "Select Vdd Programming", SEL_PRG_VDD);
                state = (vddMinCond & 1);
                SET_PIN_STATE(vdd, "Select Vdd Min", SEL_MIN_VDD);
            break;
            case VDD_TO_PRG:
                state = (vddProgCond & 4);
                SET_PIN_STATE(vdd, "Select Vdd Max", SEL_MAX_VDD);
                state = (vddProgCond & 1);
                SET_PIN_STATE(vdd, "Select Vdd Min", SEL_MIN_VDD);
                state = (vddProgCond & 2);
                SET_PIN_STATE(vdd, "Select Vdd Programming", SEL_PRG_VDD);
            break;
            case VDD_TO_MAX:
                state = (vddMaxCond & 1);
                SET_PIN_STATE(vdd, "Select Vdd Min", SEL_MIN_VDD);
                state = (vddMaxCond & 2);
                SET_PIN_STATE(vdd, "Select Vdd Programming", SEL_PRG_VDD);
                state = (vddMaxCond & 4);
                SET_PIN_STATE(vdd, "Select Vdd Max", SEL_MAX_VDD);
            break;
        }
    }
}

void ParallelPort::initialize()
{
    this->set_pin_state("OFF", OFF, false, NULL);
}

void ParallelPort::set_pin_state (
    const char *name,
    PpPins connection,
    bool state,
    struct signal_delays *delays
) {
bool old_state;

    for (int i=0; i<PARPORT_PINS; i++) {
        if (((pins[i]==connection) || ((pins[i]-LAST_PIN)==connection)) &&
            (getReg(i) != 1) && (getReg(i) != -1)) {
            if (delays) {
                this->pre_read_delay(*delays);
                old_state = this->get_pin_state (
                    name,
                    getReg(i),
                    getBit(i),
                    (pins[i]>=LAST_PIN)? 1 : 0
                );
            }
            this->set_pin_state (
                name,
                getReg(i),
                getBit(i),
                (pins[i]>=LAST_PIN)? 1 : 0,
                state
            );
            if (delays) {
                this->post_set_delay(*delays, old_state, state);
            }
        }
    }
}

bool ParallelPort::get_pin_state (
    const char *name,
    PpPins connection,
    struct signal_delays *delays
) {
    if (delays) {
        this->pre_read_delay(*delays);
    }
    bool state=false;
    bool first=true;
    for (int i=0; i<PARPORT_PINS; i++) {
        if ((pins[i]==connection) || ((pins[i]-LAST_PIN)==connection)) {
            bool pin_state = this->get_pin_state (
                name,
                getReg(i),
                getBit(i),
                (pins[i]>=LAST_PIN)? 1 : 0
            );
            if ( first ) {
                state = pin_state;
                first = false;
            } else if ( pin_state != state ) {
                state = false;
                break;
            }
        }
    }
    return state;
}
