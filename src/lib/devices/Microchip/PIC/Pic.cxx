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

Device *Pic::load(char *vendor, char *spec, char *device)
{
    if(
        Util::regexMatch("^PIC16F87[3467]A", device)
    ) {
        return new Pic16f87xA(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC16F8", device) ||
        (strcmp(device, "PIC16C84") == 0)
    ) {
        return new Pic16f8xx(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC16F7[3467]$", device)
    ) {
        return new Pic16f7x(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC16F6.*", device)
    ) {
        return new Pic16f6xx(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC12F6.*", device)
    ) {
        return new Pic12f6xx(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC16.*", device)
    ) {
        return new Pic16(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC18F([124][23]20)", device)
    ) {
        return new Pic18fxx20(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC18F((2..[05])|(2.21)|(4..[05])|(4.21))", device)
    ) {
        return new Pic18f2xx0(vendor, spec, device);
    } else if (
        Util::regexMatch("^PIC18.*", device)
    ) {
        return new Pic18(vendor, spec, device);
    } else {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Unknown PIC device %s",
                device
            )
        );
    }
    return NULL;
}

Pic::Pic(char *vendor, char *spec, char *device) : Microchip(vendor, spec, device)
{
char memtypebuf[10];

    this->popcodes = NULL;

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
    /* Get the device ID value if this device supports it. */
    if (
        config->getHex (
            Preferences::Name("deviceID"), 
            (int &)this->deviceid, 
            0xffff
        ) &&
        (this->deviceid>0)
    ) {
        if (
            config->getHex (
                Preferences::Name("deviceIDMask"), 
                (int &)this->deviceidmask, 
                0xffff
            ) &&
            (this->deviceidmask>0)
        ) {
            this->flags |= PIC_HAS_DEVICEID;
        }
    }
    /* Read programming parameters with defaults for an EPROM device */
    config->get("progCount"      ,(int &)program_count     , 25);
    config->get("progMult"       ,(int &)program_multiplier,  3);
    config->get("progTime"       ,(int &)program_time      ,100);
    config->get("eraseTime"      ,(int &)erase_time        ,  0);

    /* read the write/erase buffer sizes with default for P18F4550 */
    config->get("writeBufferSize",(int &)write_buffer_size , 32);
    config->get("eraseBufferSize",(int &)erase_buffer_size , 64);

    // set_flash_size (this->codesize);
    // set_eeprom_size(this->eesize  );
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

void Pic::off(void)
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

int start, len, addr, bytes, buffer, data, i, j, area, num_words;
bool writable, put_separator;
int bufwordsize = ((buf.get_wordsize() + 7) & ~7) / 8;
static char line[256], disasm[256], ascii[9], title[256];
int byte_address = 0;

IntPairVector::iterator n = memmap.begin();

    if ( this->wordsize == 16 ) {
        byte_address = 1;
    }
    this->dump_cb(this->dump_cb_data,(const char *)0,-1);
    for (area=0; n != memmap.end(); n++, area++) {
        start = n->first;
        len   = n->second;
        addr  = start;
        put_separator = true;
        num_words = 0;
        while (addr < (uint32_t)start+len) {
            writable = false;
            if (area==0) {
                /* code area, need to disassemble */
                if (!buf.isblank(addr,this->get_clearvalue(addr))) {
                    disasm[0]='\0';
                    if (num_words == 0) {
                        num_words = this->mem2asm(addr,buf,disasm,sizeof(disasm));
                    }
                    if (num_words) {
                        writable = true;
                        sprintf (
                            line,
                            "%07x):  %04x  %s",
                            addr << byte_address,
                            buf[addr] & 0xffff,
                            disasm
                        );
                        num_words--;
                    }
                }
                addr++;
            } else {
                /* data area, do not disassemble */
                sprintf(line,"%07x)", addr << byte_address);
                for (i=0, bytes=0; bytes<8; bytes+=bufwordsize) {
                    if (area>0 || !buf.isblank(addr,this->get_clearvalue(addr))) {
                        writable = true;
                    }
                    if (addr < (uint32_t)start+len) {
                        buffer = buf[addr];
                        for (j=0; j<bufwordsize; j++) {
                            data = buffer & 0xff;
                            sprintf(line,"%s %02x", line, data);
                            ascii[i++] = (isprint(data)) ? data : '.';
                            buffer >>= 8;
                        }
                        addr++;
                    } else {
                        for (j=0; j<bufwordsize; j++) {
                            sprintf(line,"%s   ", line);
                            ascii[i++] = ' ';
                        }
                    }
                }
                ascii[i] = '\0';
                sprintf(line,"%s |%s|", line, ascii);
            }
            if (put_separator) {
                if (area>0) {
                    this->dump_cb(this->dump_cb_data,"",-1);
                }
                title[0] = '\0';
                switch (area) {
                    case 0:
                        snprintf (
                            title, sizeof(title),
                            "@C255@_@b@C0Program memory (%s address)",
                            (byte_address) ? "byte" : "word"
                        );
                        break;                            
                    case 1:                               
                        snprintf (
                            title, sizeof(title),
                            "@C255@_@b@C0ID locations (%s address)",
                            (byte_address) ? "byte" : "word"
                        );
                        break;                            
                    case 2:                               
                        snprintf (
                            title, sizeof(title),
                            "@C255@_@b@C0Config words (%s address)",
                            (byte_address) ? "byte" : "word"
                        );
                        break;                            
                    default:                              
                        snprintf (
                            title, sizeof(title),
                            "@C255@_@b@C0EEPROM (%s address)",
                            (byte_address) ? "byte" : "word"
                        );
                        break;
                }
                if (title[0]) {
                    this->dump_cb(this->dump_cb_data,title,-1);
                }
                this->dump_cb(this->dump_cb_data,"@-",-1);
                this->dump_cb(this->dump_cb_data,"@s",-1);
                put_separator = false;
            }
            if (writable) {
                this->dump_cb (
                    this->dump_cb_data,
                    (const char *)line,
                    -1
                );
            }
        }
    }
}

#define UNKNOWN_OPCODE -1

#define DECODE_ARG0                   snprintf(buffer, sizeof_buffer, \
                                               "%s", \
                                               this->popcodes[instruction].name)
                                     
#define DECODE_ARG1(ARG1)             snprintf(buffer, sizeof_buffer, \
                                               "%s\t%#lx", \
                                               this->popcodes[instruction].name,\
                                               ARG1)
                                     
#define DECODE_ARG1WF(ARG1, ARG2)     snprintf(buffer, sizeof_buffer, \
                                               "%s\t%#lx, %s", \
                                               this->popcodes[instruction].name,\
                                               ARG1, \
                                               (ARG2 ? "f" : "w"))
                                     
#define DECODE_ARG2(ARG1, ARG2)       snprintf(buffer, sizeof_buffer, \
                                               "%s\t%#lx, %#lx", \
                                               this->popcodes[instruction].name,\
                                               ARG1, \
                                               ARG2)

#define DECODE_ARG3(ARG1, ARG2, ARG3) snprintf(buffer, sizeof_buffer, \
                                               "%s\t%#lx, %#lx, %#lx", \
                                               this->popcodes[instruction].name, \
                                               ARG1, \
                                               ARG2, \
                                               ARG3)

int Pic::mem2asm(int addr, DataBuffer& buf, char *buffer, size_t sizeof_buffer)
{
int instruction = UNKNOWN_OPCODE;
int num_words = 1;
int i;
int value;
long opcode;

    if (this->popcodes) {
        opcode = buf[addr] & 0xffff;
        for (i=0; this->popcodes[i].name; i++) {
            if ((opcode & this->popcodes[i].mask) == this->popcodes[i].opcode) {
                instruction = i;
                break;
            }
        }
    }
    if (instruction == UNKNOWN_OPCODE)  {
        snprintf(buffer, sizeof_buffer, "dw\t%#lx  ; '%c'", opcode, isprint(opcode)?opcode:'.');
        return num_words;
    }
    switch (this->popcodes[instruction].type) {
        case INSN_CLASS_LIT3_BANK:
            DECODE_ARG1((opcode & 0x7) << 5); 
            break;
        case INSN_CLASS_LIT3_PAGE:
            DECODE_ARG1((opcode & 0x7) << 9); 
            break;
        case INSN_CLASS_LIT1:
            DECODE_ARG1(opcode & 1); 
            break;
        case INSN_CLASS_LIT4:
            DECODE_ARG1(opcode & 0xf); 
            break;
        case INSN_CLASS_LIT4S:
            DECODE_ARG1((opcode & 0xf0) >> 4); 
            break;
        case INSN_CLASS_LIT6:
            DECODE_ARG1(opcode & 0x3f); 
            break;
        case INSN_CLASS_LIT8:
        case INSN_CLASS_LIT8C12:
        case INSN_CLASS_LIT8C16:
            DECODE_ARG1(opcode & 0xff); 
            break;
        case INSN_CLASS_LIT9:
            DECODE_ARG1(opcode & 0x1ff); 
            break;
        case INSN_CLASS_LIT11:
            DECODE_ARG1(opcode & 0x7ff); 
            break;
        case INSN_CLASS_LIT13:
            DECODE_ARG1(opcode & 0x1fff); 
            break;
        case INSN_CLASS_LITFSR:
            DECODE_ARG2(((opcode >> 6) & 0x3), (opcode & 0x3f));
            break;
        case INSN_CLASS_RBRA8:
            value = opcode & 0xff;
            /* twos complement number */
            if (value & 0x80) {
                value = -((value ^ 0xff) + 1);
            }
            DECODE_ARG1((uint32_t)(addr + value + 1) * 2); 
            break;
        case INSN_CLASS_RBRA11:
            value = opcode  & 0x7ff;
            /* twos complement number */
            if (value & 0x400) {
                value = -((value ^ 0x7ff) + 1);
            }      
            DECODE_ARG1((uint32_t)(addr + value + 1) * 2); 
            break;
        case INSN_CLASS_LIT20:
            {
            long int dest;

                num_words = 2;
                dest = (buf[addr+1] & 0xfff) << 8;
                dest |= opcode & 0xff;      
                DECODE_ARG1(dest * 2); 
            }
            break;
        case INSN_CLASS_CALL20:
            {
            long int dest;

                num_words = 2;
                dest = (buf[addr+1] & 0xfff) << 8;
                dest |= opcode & 0xff;      
                snprintf(buffer, sizeof_buffer, "%s\t%#lx, %#lx",
                         this->popcodes[instruction].name,
                         dest * 2,
                         (opcode >> 8) & 1);
            }
            break;
        case INSN_CLASS_FLIT12:
            {
            long int k;
            long int file;

                num_words = 2;
                k = buf[addr+1] & 0xff;
                k |= ((opcode & 0xf) << 8);
                file = (opcode >> 4) & 0x3;
                DECODE_ARG2(file, k);
            }
            break;
        case INSN_CLASS_FF:
            {
            long int file1;
            long int file2;

                num_words = 2;
                file1 = opcode & 0xfff;
                file2 = buf[addr+1] & 0xfff;
                DECODE_ARG2(file1, file2);
            }
            break;
        case INSN_CLASS_FP:
            DECODE_ARG2((opcode & 0xff), ((opcode >> 8) & 0x1f));
            break;
        case INSN_CLASS_PF:
            DECODE_ARG2(((opcode >> 8) & 0x1f), (opcode & 0xff));
            break;
        case INSN_CLASS_SF:
            {
            long int offset;
            long int file;

                num_words = 2;
                offset = opcode & 0x7f;
                file = buf[addr+1] & 0xfff;
                DECODE_ARG2(offset, file);
            }
            break;
        case INSN_CLASS_SS:
            {
            long int offset1;
            long int offset2;

                num_words = 2;
                offset1 = opcode & 0x7f;
                offset2 = buf[addr+1] & 0x7f;
                DECODE_ARG2(offset1, offset2);
            }
            break;
        case INSN_CLASS_OPF5:
            DECODE_ARG1(opcode & 0x1f);
            break;
        case INSN_CLASS_OPWF5:
            DECODE_ARG1WF((opcode & 0x1f), ((opcode >> 5) & 1));
            break;
        case INSN_CLASS_B5:
            DECODE_ARG2((opcode & 0x1f), ((opcode >> 5) & 7));
            break;
        case INSN_CLASS_B8:
            DECODE_ARG2((opcode & 0xff), ((opcode >> 8) & 7));
            break;
        case INSN_CLASS_OPF7:
            DECODE_ARG1(opcode & 0x7f);
            break;
        case INSN_CLASS_OPF8:
            DECODE_ARG1(opcode & 0xff);
            break;
        case INSN_CLASS_OPWF7:
            DECODE_ARG1WF((opcode & 0x7f), ((opcode >> 7) & 1));
            break;
        case INSN_CLASS_OPWF8:
            DECODE_ARG1WF((opcode & 0xff), ((opcode >> 8) & 1));
            break;
        case INSN_CLASS_B7:
            DECODE_ARG2((opcode & 0x7f), ((opcode >> 7) & 7));
            break;
        case INSN_CLASS_OPFA8:
            DECODE_ARG2((opcode & 0xff), ((opcode >> 8) & 1));
            break;
        case INSN_CLASS_BA8:
            DECODE_ARG3((opcode & 0xff), ((opcode >> 9) & 7), ((opcode >> 8) & 1));
            break;
        case INSN_CLASS_OPWFA8:
            DECODE_ARG3((opcode & 0xff), ((opcode >> 9) & 1), ((opcode >> 8) & 1));
            break;
        case INSN_CLASS_IMPLICIT:
            DECODE_ARG0;
            break;
        case INSN_CLASS_TBL:
            {
            char stroperator[5]; 

                switch (opcode & 0x3) {
                    case 0:
                        strncpy(stroperator, "*", sizeof(stroperator));
                        break;
                    case 1:
                        strncpy(stroperator, "*+", sizeof(stroperator));
                        break;
                    case 2:
                        strncpy(stroperator, "*-", sizeof(stroperator));
                        break;
                    case 3:
                        strncpy(stroperator, "+*", sizeof(stroperator));
                        break;
                    default:
                        num_words = 0;
                        break;
                }
                if (num_words) {
                    snprintf(buffer,
                             sizeof_buffer,
                             "%s\t%s",
                             this->popcodes[instruction].name,
                             stroperator);
                }
            }
            break;
        case INSN_CLASS_TBL2:
            DECODE_ARG2(((opcode >> 9) & 1), (opcode & 0xff));
            break;
        case INSN_CLASS_TBL3:
            DECODE_ARG3(((opcode >> 9) & 1), 
                        ((opcode >> 8) & 1), 
                        (opcode & 0xff));
            break;
        default:
            num_words = 0;
            break;
    }
    return num_words;
}

bool Pic::check(void)
{
uint32_t devid;

    if (!(this->flags & PIC_HAS_DEVICEID)) {
        /* Device doesn't have a device ID to check */
        return true;
    }
    try {
        this->set_program_mode();
        devid = read_deviceid();
        this->off();

        /* Wait a bit to make sure program mode is off before continuing
         * with other operations on the device. */
        this->io->usleep(1000);
    } catch (std::exception& e) {
        this->off();
        throw;
    }
    if ((devid & this->deviceidmask) == (this->deviceid & this->deviceidmask)) {
        /* Keep the chip rev in case it's needed later */
        this->deviceid = devid;
        printf (
            "Chip Rev: 0x%lx\n",
            (uint32_t)(devid & ~this->deviceidmask)
        );
        return true;
    }
    /* XXX: there's no way to pass back an error string without using
     * exceptions! */
    throw runtime_error (
        (const char *)Preferences::Name (
            "Device ID 0x%lx is wrong (expected 0x%lx, mask 0x%lx)",
            (uint32_t)devid,
            (uint32_t)this->deviceid,
            (uint32_t)this->deviceidmask
        )
    );
    return false;
}

uint32_t Pic::read_deviceid(void)
{
    fprintf (
        stderr,
        "Warning: read_deviceid is not implemented for this device.\n"
    );
    /* Fake it out. */
    return (this->deviceid & this->deviceidmask);
}

void Pic::set_config_default(DataBuffer& buf)
{
    /* On most PICs, the configuration memory is only = 0xffff but it is not always the case */
    /* TODO should be done through the pic.conf configuration */
}
