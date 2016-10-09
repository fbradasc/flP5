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

#define SET_PIN_STATE(function,name,prefix) this->set_pin_state (             \
        name,                                                                 \
        ParallelPort::controls[name].getReg(),                                \
        ParallelPort::controls[name].getBit(),                                \
        ( ParallelPort::controls[name].getMode() & PIN_NEG ) ? 1 : 0,         \
        state,                                                                \
        &this->function##_delays_                                             \
    );                                                                        \

#define GET_PIN_STATE(function,name,prefix) this->get_pin_state (             \
        name,                                                                 \
        ParallelPort::controls[name].getReg(),                                \
        ParallelPort::controls[name].getBit(),                                \
        ( ParallelPort::controls[name].getMode() & PIN_NEG ) ? 1 : 0,         \
        &this->function##_delays_                                             \
    )

const IOPin ParallelPort::pins[] =
{
#if defined(ENABLE_LINUX_GPIO)
    { 0, -1, -1, PIN_3V3                     },
    { 0, -1, -1, PIN_5V0                     },
    { 0,  0,  2,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_5V0                     },
    { 0,  0,  3,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0,  4,           PIN_INP | PIN_OUT },
    { 0,  0, 14,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0, 15,           PIN_INP | PIN_OUT },
    { 0,  0, 17,           PIN_INP | PIN_OUT },
    { 0,  0, 18,           PIN_INP | PIN_OUT },
    { 0,  0, 27,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0, 22,           PIN_INP | PIN_OUT },
    { 0,  0, 23,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_3V3                     },
    { 0,  0, 24,           PIN_INP | PIN_OUT },
    { 0,  0, 10,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0,  9,           PIN_INP | PIN_OUT },
    { 0,  0, 25,           PIN_INP | PIN_OUT },
    { 0,  0, 11,           PIN_INP | PIN_OUT },
    { 0,  0,  8,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0,  7,           PIN_INP | PIN_OUT }
#if defined(RPI_MODEL_PLUS)
    ,
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0,  5,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0,  6,           PIN_INP | PIN_OUT },
    { 0,  0, 12,           PIN_INP | PIN_OUT },
    { 0,  0, 13,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0, 19,           PIN_INP | PIN_OUT },
    { 0,  0, 16,           PIN_INP | PIN_OUT },
    { 0,  0, 26,           PIN_INP | PIN_OUT },
    { 0,  0, 20,           PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0,  0, 21,           PIN_INP | PIN_OUT }
#endif
#else
    { 0,  2,  0, PIN_NEG | PIN_INP | PIN_OUT },
    { 0,  0,  0,                     PIN_OUT },
    { 0,  0,  1,                     PIN_OUT },
    { 0,  0,  2,                     PIN_OUT },
    { 0,  0,  3,                     PIN_OUT },
    { 0,  0,  4,                     PIN_OUT },
    { 0,  0,  5,                     PIN_OUT },
    { 0,  0,  6,                     PIN_OUT },
    { 0,  0,  7,                     PIN_OUT },
    { 0,  1,  6,           PIN_INP           },
    { 0,  1,  7, PIN_NEG | PIN_INP           },
    { 0,  1,  5,           PIN_INP           },
    { 0,  1,  4,           PIN_INP           },
    { 0,  2,  1, PIN_NEG | PIN_INP | PIN_OUT },
    { 0,  1,  3,           PIN_INP           },
    { 0,  2,  2,           PIN_INP | PIN_OUT },
    { 0,  2,  3, PIN_NEG | PIN_INP | PIN_OUT },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     },
    { 0, -1, -1, PIN_GND                     }
#endif
};

IOControls ParallelPort::controls = ParallelPort::init_controls();

ParallelPort::ParallelPort(int port) : IO(port)
{
char programmer[20];
int vregs, val;
int index = 0;
const char *message = "Either none or at least two of:\n\n" \
                      "\t%s.vminpin\n" \
                      "\t%s.vprgpin\n" \
                      "\t%s.vmaxpin\n\n" \
                      "need to be set.";

    controls = init_controls(); // reload controls

    config->get("vddMinCond"        ,vddMinCond ,0);
    config->get("vddProgCond"       ,vddProgCond,0);
    config->get("vddMaxCond"        ,vddMaxCond ,0);
    config->get("vppOffCond"        ,vppOffCond ,0);
    config->get("hasSAVddVppControl",val        ,0);

    hasSAVddVppControl((val!=0));
 
    vregs = 0;
    vregs += (GET_PIN_BIT("selMaxVdd" )>=0) ? 1 : 0;
    vregs += (GET_PIN_BIT("selProgVdd")>=0) ? 1 : 0;
    vregs += (GET_PIN_BIT("selMinVdd" )>=0) ? 1 : 0;

    this->production( (vregs>0) ? true : false );
}

ParallelPort::~ParallelPort()
{
}

IOControls ParallelPort::init_controls()
{
IOControls controls;

    add_control(controls);                         /** initialize */
    add_control(controls, "icspClock"  , PIN_OUT); /** Par. port pin data for the read data signal    */
    add_control(controls, "icspDataIn" , PIN_INP); /** Par. port pin data for the write data signal   */
    add_control(controls, "icspDataOut", PIN_OUT); /** Par. port pin data for the clock signal        */
    add_control(controls, "icspVddOn"  , PIN_OUT); /** Par. port pin data for the Vpp enable signal   */
    add_control(controls, "icspVppOn"  , PIN_OUT); /** Par. port pin data for the Vdd enable signal   */
    add_control(controls, "selMinVdd"  , PIN_OUT); /** Par. port pin data for the Vdd Min sel. signal */
    add_control(controls, "selProgVdd" , PIN_OUT); /** Par. port pin data for the Vdd Prg sel. signal */
    add_control(controls, "selMaxVdd"  , PIN_OUT); /** Par. port pin data for the Vdd Max sel. signal */
    add_control(controls, "selVihhVpp" , PIN_OUT); /** Par. port pin data for the Vpp VIH sel. signal */

    return controls;
}

void ParallelPort::add_control(IOControls &cl, const char *name, unsigned short dir)
{
int pin=0;
IOPin iopin;
static int index=0;

    if (NULL == name) {
        index=0;
    } else {
        if (NULL != config) {
            config->get(name,pin,0);
        }

        iopin.mode = ( dir == PIN_INP ) ? PIN_INP : PIN_OUT;

        if (pin < 0) {
            iopin.mode |= PIN_NEG;
            pin *= -1;
        }
        if (pin == 0) { /* signal not used by the programmer */
            iopin.reg = -1;
            iopin.bit = -1;
        } else if ((pin-1 > NIOP) || (ParallelPort::pins[pin-1].getReg() == -1)) {
            throw runtime_error (
                (const char *)Preferences::Name (
                    "Invalid value for configuration parameter %s Pin",
                    name
                )
            );
        } else if ((ParallelPort::pins[pin-1].getMode() & dir) == 0) {
            throw runtime_error (
                (const char *)Preferences::Name (
                    "Mismatch direction for configuration parameter %s Pin",
                    name
                )
            );
        } else {
            if ((ParallelPort::pins[pin-1].getMode() & PIN_NEG)) {
                iopin.mode ^= PIN_NEG;
            }
            iopin.reg = ParallelPort::pins[pin-1].getReg();
            iopin.bit = ParallelPort::pins[pin-1].getBit();
        }

        iopin.idx = index++;

        cl[name] = iopin;
    }
}

void ParallelPort::clock(bool state)
{
    SET_PIN_STATE(clk, "icspClock", icspClock);
}   
    
void ParallelPort::data(bool state)
{
    SET_PIN_STATE(data, "icspDataOut", icspDataOut);
}  

bool ParallelPort::data(void)
{
    return GET_PIN_STATE(data, "icspDataIn", icspDataIn);
}

void ParallelPort::vpp(VppMode mode)
{
bool state;

    switch (mode) {
        case VPP_TO_VIH:
            if (this->vppOffCond) {
                state = true; SET_PIN_STATE(vpp, "icspVppOn", icspVppOn);
            }
            if (GET_PIN_BIT("selVihhVpp")>=0) {
                state = true; SET_PIN_STATE(vpp, "selVihhVpp", selVihhVpp);
            }
            if (!this->vppOffCond) {
                state = true; SET_PIN_STATE(vpp, "icspVppOn", icspVppOn);
            }
        break;
        case VPP_TO_GND:
            if (this->vppOffCond && GET_PIN_BIT("selVihhVpp")>=0) {
                state = false; SET_PIN_STATE(vpp, "selVihhVpp", selVihhVpp);
            }
            state = false; SET_PIN_STATE(vpp, "icspVppOn", icspVppOn);
        break;
        case VPP_TO_VDD:
            if (GET_PIN_BIT("selVihhVpp")>=0) {
                state = false; SET_PIN_STATE(vpp, "selVihhVpp", selVihhVpp);
            }
            state = true; SET_PIN_STATE(vpp, "icspVppOn", icspVppOn);
        break;
    }
}

void ParallelPort::vdd(VddMode mode)
{
bool state;

    if (mode==VDD_TO_OFF) {
        state = false; SET_PIN_STATE(vdd, "icspVddOn", icspVddOn);
    } else if (mode==VDD_TO_ON) {
        state = true; SET_PIN_STATE(vdd, "icspVddOn", icspVddOn);
    } else if (this->production()) {
        switch (mode) {
            case VDD_TO_ON:
            case VDD_TO_OFF:
                /* hadled in the above if ... */
            break;
            case VDD_TO_MIN:
                if (GET_PIN_BIT("selMaxVdd")>=0) {
                    state = (vddMinCond & 4);
                    SET_PIN_STATE(vdd, "selMaxVdd", selMaxVdd);
                }
                if (GET_PIN_BIT("selProgVdd")>=0) {
                    state = (vddMinCond & 2);
                    SET_PIN_STATE(vdd, "selProgVdd", selProgVdd);
                }
                if (GET_PIN_BIT("selMinVdd")>=0) {
                    state = (vddMinCond & 1);
                    SET_PIN_STATE(vdd, "selMinVdd", selMinVdd);
                }
            break;
            case VDD_TO_PRG:
                if (GET_PIN_BIT("selMaxVdd")>=0) {
                    state = (vddProgCond & 4);
                    SET_PIN_STATE(vdd, "selMaxVdd", selMaxVdd);
                }
                if (GET_PIN_BIT("selMinVdd")>=0) {
                    state = (vddProgCond & 1);
                    SET_PIN_STATE(vdd, "selMinVdd", selMinVdd);
                }
                if (GET_PIN_BIT("selProgVdd")>=0) {
                    state = (vddProgCond & 2);
                    SET_PIN_STATE(vdd, "selProgVdd", selProgVdd);
                }
            break;
            case VDD_TO_MAX:
                if (GET_PIN_BIT("selMinVdd")>=0) {
                    state = (vddMaxCond & 1);
                    SET_PIN_STATE(vdd, "selMinVdd", selMinVdd);
                }
                if (GET_PIN_BIT("selProgVdd")>=0) {
                    state = (vddMaxCond & 2);
                    SET_PIN_STATE(vdd, "selProgVdd", selProgVdd);
                }
                if (GET_PIN_BIT("selMaxVdd")>=0) {
                    state = (vddMaxCond & 4);
                    SET_PIN_STATE(vdd, "selMaxVdd", selMaxVdd);
                }
            break;
        }
    }
}

void ParallelPort::set_pin_state (
    const char *name,
    short reg,
    short bit,
    short invert,
    bool state,
    struct signal_delays *delays
) {
bool old_state;

    if (reg <0 || bit<0) {
        return;
    }
    if (delays) {
        old_state = this->get_pin_state(name, reg, bit, invert);
    }
    this->set_pin_state(name, reg, bit, invert, state);

    if (delays) {
        this->post_set_delay(*delays, old_state, state);
    }
}

bool ParallelPort::get_pin_state (
    const char *name,
    short reg,
    short bit,
    short invert,
    struct signal_delays *delays
) {
    if (reg <0 || bit<0) {
        return false;
    }
    if (delays) {
        this->pre_read_delay(*delays);
    }
    return this->get_pin_state(name, reg, bit, invert);
}
