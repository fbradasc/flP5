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
 * $Id: Pic.cxx,v 1.20 2002/10/19 19:09:00 marka Exp $
 */
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "Microchip.h"
#include "IO.h"
#include "Util.h"

Device *Pic::load(char *name)
{
    if(
        Util::regexMatch("^PIC16F87[3467]A", name)
    ) {
        return new Pic16f87xA(name);
    } else if (
        Util::regexMatch("^PIC16F8", name) ||
        (strcmp(name, "PIC16C84") == 0)
    ) {
        return new Pic16f8xx(name);
    } else if (
        Util::regexMatch("^PIC16F7[3467]$", name)
    ) {
        return new Pic16f7x(name);
    } else if (
        Util::regexMatch("^PIC16F6.*", name)
    ) {
        return new Pic16f6xx(name);
    } else if (
        Util::regexMatch("^PIC12F6.*", name)
    ) {
        return new Pic12f6xx(name);
    } else if (
        Util::regexMatch("^PIC16.*", name)
    ) {
        return new Pic16(name);
    } else if (
        Util::regexMatch("^PIC18.*", name)
    ) {
        return new Pic18(name);
    } else {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Unknown PIC device %s",
                name
            )
        );
    }
    return NULL;
}


Pic::Pic(char *name) : Microchip(name)
{
char memtypebuf[10];

    this->flags = 0;
    /* Fill in this PIC's common attributes */
    if (!config->get("memType",memtypebuf,"rom",10)) {
        throw runtime_error (
            "PIC device is missing memtype configuration parameter"
        );
    }
    if (strcmp(memtypebuf, "eprom") == 0) {
        this->memtype = MEMTYPE_EPROM;
        this->flags |= PIC_REQUIRE_EPROG;
    } else if (strcmp(memtypebuf, "flash") == 0) {
        this->memtype = MEMTYPE_FLASH;
    } else if (strcmp(memtypebuf, "flashe") == 0) {
        this->memtype = MEMTYPE_FLASH;
        this->flags |= PIC_REQUIRE_EPROG;
    } else {
        throw runtime_error("PIC device has an unknown memory type");
    }
    if (!config->get("wordSize",(int &)this->wordsize,0)) {
        throw runtime_error("PIC device has incomplete configuration entry");
    }
    if (!config->get("codeSize",(int &)this->codesize,0)) {
        throw runtime_error("PIC device has incomplete configuration entry");
    }
    if (config->get("eepromSize",(int &)this->eesize,0) && (this->eesize>0)) {
        this->flags |= PIC_FEATURE_EEPROM;
    } else {
        this->eesize=0;
    }
    /* Calculate the word mask */
    this->wordmask = 0;
    for (int i=0; i<this->wordsize; i++) {
        this->wordmask <<= 1;
        this->wordmask |= 0x0001;
    }
    /* Read programming parameters with defaults for an EPROM device */
    config->get("progCount",(int &)program_count     , 25);
    config->get("progMult" ,(int &)program_multiplier,  3);
    config->get("progTime" ,(int &)program_time      ,100);
    config->get("eraseTime",(int &)erase_time        ,  0);
}

Pic::~Pic()
{
}

bool Pic::program_cycle(uint32_t data, uint32_t mask)
{
    this->write_prog_data(data);
    this->write_command(COMMAND_BEGIN_PROG);
    this->io->usleep(this->program_time);
    if (this->flags & PIC_REQUIRE_EPROG) {
        this->write_command(COMMAND_END_PROG);
    }
    if ((read_prog_data() & mask) == (data & mask)) {
        return true;
    }
    return false;
}

void Pic::set_program_mode(void)
{
    /* Power up the PIC and put it in program/verify mode */
    this->io->clock(false);        /* Set RB6 low */
    this->io->data(false);         /* Set RB7 low */
    this->io->vdd(IO::VDD_TO_ON);  /* Raise Vdd while RB6 & RB7 are low */
    this->io->usleep(1000);        /* Wait a bit */
    this->io->vpp(IO::VPP_TO_VIH); /* Raise Vpp while RB6 & RB7 are low */
    this->io->usleep(1000);        /* Wait a bit */
}

void Pic::pic_off(void)
{
    /* Shut everything down */
    this->io->clock(false);
    this->io->data(false);
    this->io->vpp(IO::VPP_TO_GND);
    this->io->vdd(IO::VDD_TO_OFF);
}

void Pic::write_command(uint32_t command)
{
    this->io->shift_bits_out(command, 6, 1, 1);
    this->io->usleep(1);
}

void Pic::write_prog_data(uint32_t data)
{
    data = (data & this->wordmask) << 1;
    this->write_command(COMMAND_LOAD_PROG_DATA);
    this->io->shift_bits_out(data, 16, 1);
    this->io->usleep(1);
}

uint32_t Pic::read_prog_data(void)
{
uint32_t data;

    this->write_command(COMMAND_READ_PROG_DATA);
    data = this->io->shift_bits_in(16, 1);
    this->io->usleep(1);
    return (data >> 1) & this->wordmask;
}

void Pic::dump(DataBuffer& buf)
{
    if (!this->dump_cb) {
        return;
    }

int start, len, addr, bytes, data, i;
bool writable;
int bufwordsize = ((buf.get_wordsize() + 7) & ~7) / 8;
static char line[256], disasm[256], ascii[9];

IntPairVector::iterator n = memmap.begin();

    if (wordsize==12 || wordsize==14) {
        this->dump_cb(this->dump_cb_data,(const char *)0,-1);
        for (; n != memmap.end(); n++) {
            start = n->first;
            len   = n->second;
            addr  = start;
            while (addr < (unsigned long)start+len) {
                disasm[0]='\0';
                if (!buf.isblank(addr,this->get_clearvalue(addr))) {
                    if (this->wordsize==12) {
                        this->mem2asm12(buf[addr],disasm);
                    } else {
                        this->mem2asm14(buf[addr],disasm);
                    }
                    sprintf (
                        line,
                        "%07x):  %04x  %s",
                        addr,
                        buf[addr] & 0xffff,
                        disasm
                    );
                    this->dump_cb (
                        this->dump_cb_data,
                        (const char *)line,
                        -1
                    );
                }
                addr++;
            }
        }
    } else {
        this->Device::dump(buf);
    }
}

#define byte_op  0
#define bit_op   1
#define branch   2
#define literals 3

/* splices off a section of bits from the insn word */
/* the least significant bit is at '0', s1 >= s2 */
#define slice(s1, s2)  ((insn >> s2) & ((1<<(s1-s2+1))-1))

char *Pic::byte_op_names12[] = {
  "movwf",
  "GPDASM ERROR",  /* this position isn't unique */ 
  "subwf", 
  "decf", 
  "iorwf", 
  "andwf", 
  "xorwf", 
  "addwf", 
  "movf", 
  "comf", 
  "incf", 
  "decfsz", 
  "rrf", 
  "rlf", 
  "swapf", 
  "incfsz" 
};

char *Pic::bit_op_names12[] = {
  "bcf", 
  "bsf", 
  "btfsc", 
  "btfss"
};

char *Pic::lit_op_names12[] = {
  "movlw", 
  "iorlw", 
  "andlw", 
  "xorlw"
};

char *Pic::byte_op_names14[] = {
  "GPDASM ERROR",  /* this position isn't unique */ 
  "GPDASM ERROR",  /* this position isn't unique */ 
  "subwf", 
  "decf", 
  "iorwf", 
  "andwf", 
  "xorwf", 
  "addwf", 
  "movf", 
  "comf", 
  "incf", 
  "decfsz", 
  "rrf", 
  "rlf", 
  "swapf", 
  "incfsz" 
};

char *Pic::bit_op_names14[] = {
  "bcf", 
  "bsf", 
  "btfsc", 
  "btfss"
};

char *Pic::lit_op_names14[] = {
  "movlw", 
  "movlw", 
  "movlw", 
  "movlw", 
  "retlw",
  "retlw",
  "retlw",
  "retlw",
  "iorlw",
  "andlw",
  "xorlw",
  "GPDASM ERROR",  /* this position is unused, shouldn't ever get here */
  "sublw",
  "sublw",
  "addlw",
  "addlw"
};

void Pic::mem2asm12(int insn, char *buffer)
{
char *pointer = buffer;

    insn &= 0xfff;
    switch (slice(11,10)) {
        case byte_op:
            if (slice(9,6) == 0) {
                if (slice(5,5)) {
                    sprintf(pointer, "movwf\t0x%02x", slice(4,0));
                } else {
                    switch (slice(7,0)) {
                        case 0x00:
                            sprintf(pointer, "nop");
                        break;        
                        case 0x02:
                            sprintf(pointer, "option");
                        break;
                        case 0x03:
                            sprintf(pointer, "sleep");
                        break;        
                        case 0x04:
                            sprintf(pointer, "clrwdt");
                        break;        
                        case 0x05: 
                        case 0x06: 
                        case 0x07:
                            sprintf(pointer, "tris\t%01x", insn & 7);
                        break;
                        default:
                            sprintf(pointer, "dw\t0x%04x", insn);
                    }
                }
            } else if (slice(9,6) == 1) {
                if (slice(7,0) == 0x40) {
                    sprintf(pointer, "clrw");
                } else if (slice(5,5)) {
                    sprintf(pointer, "clrf\t0x%02x", slice(4,0));
                } else {
                    sprintf(pointer, "dw\t0x%03x", insn);
                }
            } else {
                sprintf (
                    pointer,
                    "%s\t0x%02x,%s",
                    byte_op_names12[slice(9,6)],
                    slice(4,0),
                    (insn & 0x20 ? "f" : "w")
                );
            }
        break;
        case bit_op:
            sprintf(
                pointer,
                "%s\t0x%02x,%d",
                bit_op_names12[slice(9,8)],
                slice(4,0),
                slice(7,5)
            );
        break;
        case branch:
            if (insn & 0x200) {
                sprintf(pointer, "goto\t0x%03x", slice(8,0));
            } else if (slice(9,8) == 1) {
                sprintf(pointer, "call\t0x%02x", slice(7,0));
            } else {
                sprintf(pointer, "retlw\t0x%02x", slice(7,0));
            }
        break;
        case literals:
            sprintf (
                pointer,
                "%s\t0x%02x",
                lit_op_names12[slice(9,8)],
                slice(7,0)
            );
        break;
        default:
            sprintf(pointer, "dw\t0x%04x", insn);
    }
}

void Pic::mem2asm14(int insn, char *buffer)
{
char *pointer = buffer;

    insn &= 0x3fff;
    switch (slice (13, 12)) {
        case byte_op:
            if (slice(11,8) == 0) {
                if (slice(7,7)) {
                    sprintf(pointer, "movwf\t0x%02x", slice(6,0));
                } else {
                    switch (slice(7,0)) {
                        case 0x08:
                            sprintf(pointer, "return");
                        break;        
                        case 0x09:
                            sprintf(pointer, "retfie");
                        break;        
                        case 0x62:
                            sprintf(pointer, "option");
                        break;
                        case 0x63:
                            sprintf(pointer, "sleep");
                        break;        
                        case 0x64:
                            sprintf(pointer, "clrwdt");
                        break;        
                        case 0x65: 
                        case 0x66: 
                        case 0x67:
                            sprintf(pointer, "tris\t%01x", insn & 7);
                        break;
                        case 0x00:
                        case 0x20:
                        case 0x40:
                        case 0x60:
                            sprintf(pointer, "nop");
                        break;        
                        default:
                            sprintf(pointer, "dw\t0x%04x", insn);
                    }
                }
            } else if (slice(11,8) == 1) {
                if (slice(7,0) == 0x03) {
                    sprintf(pointer, "clrw");
                } else if (slice(7,7)) {
                    sprintf(pointer, "clrf\t0x%02x", slice(6,0));
                } else {
                    sprintf(pointer, "dw\t0x%04x", insn);
                }
            } else {
                sprintf (
                    pointer,
                    "%s\t0x%02x,%s",
                    byte_op_names14[slice(11,8)],
                    slice(6,0),
                    (insn & 0x80 ? "f" : "w")
                );
            }
        break;
        case bit_op:
            sprintf (
                pointer,
                "%s\t0x%02x,%d",
                bit_op_names14[slice(11,10)],
                slice(6,0),
                slice(9,7)
            );
        break;
        case branch:
            if (insn & 0x800) {
                sprintf(pointer, "goto\t0x%04x", slice(10,0));
            } else {
                sprintf(pointer, "call\t0x%04x", slice(10,0));
            }
        break;
        case literals:
            if (slice(11,8) == 0x1011) {
                sprintf(pointer, "dw\t0x%04x", insn);
            } else {
                sprintf (
                    pointer,
                    "%s\t0x%02x",
                    lit_op_names14[slice(11,8)],
                    slice(7,0)
                );
            }
        break;
        default:
            sprintf(pointer, "dw\t0x%04x", insn);
    }
}
