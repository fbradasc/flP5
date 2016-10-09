/* Copyright (C) 2002-2004  Mark Andrew Aikens <marka@desert.cx>
 * Copyright (C) 2005       Pierre Gaufillet   <pierre.gaufillet@magic.fr>
 * Copyright (C) 2006       Edward Hildum      <ehildum@comcast.net>
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
 * $Id$
 */
using namespace std;

#include <stdexcept>
#include "Microchip.h"
#include "Util.h"
#include <iostream>

/* ID locations: 0x200000 - 0x200007 (byte address)
 * Config words: 0x300000 - 0x30000D (byte address)
 * Device ID:    0x3ffffe - 0x3fffff (byte address)
 * Data EEPROM:  0xf00000 (byte address)
 */
#define ID_LOC_WRDS    (8/2)
#define CFG_WORDS_WRDS (14/2)

Pic18fxx20::Pic18fxx20(char *name) : Pic18(name)
{
}

Pic18fxx20::~Pic18fxx20()
{
}

void Pic18fxx20::erase(void)
{
    if (this->memtype != MEMTYPE_FLASH) {
        throw runtime_error("Operation not supported by device");
    }
    try {
        set_program_mode();

        set_tblptr(0x3c0004);
        write_command(COMMAND_TABLE_WRITE, 0x0080);    /* "Chip Erase" */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
        this->io->usleep(this->erase_time);

        pic_off();
    } catch (std::exception& e) {
        pic_off();
        throw;
    }
}

void Pic18fxx20::write_program_memory(DataBuffer& buf, bool verify)
{
unsigned int offset;    /* byte offset */

    offset = 0;
    try {
        /* Step 1: Direct access to code memory and enable writes */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset < (codesize*2); offset+=write_buffer_size) {
            /* Step 2: load table pointer with offset of current write block */
            set_tblptr(offset);
            /* Step 3,4: Load write buffer w/ wbuf_size bytes and start write */
            if (load_write_buffer(buf, offset, write_buffer_size)) {
                program_wait();
            }
            if (verify) {
                /* Verify the memory just written */
                read_memory(buf, offset, write_buffer_size/2, true);
            }
            progress(offset);
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name(
                "Couldn't write program memory at address 0x%06lx",
                (unsigned long)offset
            )
        );
    }
}

void Pic18fxx20::write_id_memory(DataBuffer& buf, unsigned long addr, bool verify)
{
    progress(addr);
    try {
        /* Step 1: Direct access to id memory */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        /* Step 2: Load table pointer with address to program */
        set_tblptr(addr);

        /* Load 8 bytes into write buffer and begin write    */
        if (load_write_buffer(buf, addr, 2 * ID_LOC_WRDS)) {
            program_wait();
        }
        if (verify) {
            /* Verify the memory just written */
            read_memory(buf, addr, ID_LOC_WRDS, true);
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write ID memory at address 0x%06lx",
                addr
            )
        );
    }
}

void Pic18fxx20::write_data_memory(DataBuffer& buf, unsigned long addr, bool verify)
{
    uint32_t     ins;
    uint8_t      data;
    unsigned int offset = 0;    /* word offset */

    try {
        /* Step 1: Direct access to data EEPROM */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset < this->eesize; offset++) {
            progress(addr+(2*offset));

            /* Step 2: Set the data EEPROM address pointer */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADR));

            if ((offset & 1) == 0) {
                data = buf[(addr+offset)/2] & 0xff;
            } else {
                data = (buf[(addr+offset)/2] >> 8) & 0xff;
            }
            if (data != 0xff) {
                /* Step 3: Load the data to be written */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(data));
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEDATA));

                /* Step 4: Enable memory writes */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WREN);
                
                /* Step 5: Unlock sequence */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(0x55));
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF_EECON2);
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(0xAA));
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF_EECON2);

                /* Step 6: Initiate write */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WR);
                write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
                write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
                
                /* Step 7: Hold PGC low for time P11 */
                this->io->usleep(this->erase_time);
                
                /* Then hold PGC low for time P10 */
                this->io->usleep(5);    /* High-voltage discharge time P10 */
                
                /* Disable writes */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_WREN);
            }

            this->progress_count++;

            if (verify) {
                /* Initiate a memory read */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_RD);

                /* Load data into the serial data holding register */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EEDATA_W_0);
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));
                write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);

                /* Shift out data and check */
                ins = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
                if (ins != data) {
                    throw runtime_error("");
                }
                this->progress_count++;
            }
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write data memory at location %d",
                offset
            )
        );
    }
}

void Pic18fxx20::write_config_memory(DataBuffer& buf, unsigned long addr, bool verify)
{
    int           i = 0;
    unsigned long skipd_addr;

    try {
        /* Step 1: Direct access to config memory */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_CFGS);

        for (i=0; i<CFG_WORDS_WRDS+1; i++, addr += 2) {
            if (i == 5) {
                skipd_addr = addr;
                continue;
            }
            /* Give byte addresses to the callback to match datasheet. */
            progress(addr);

            /* Step 2: Set Table Pointer for config byte to be written. *
             * Write even/odd addresses                                 */
            set_tblptr(addr);
            write_command(COMMAND_TABLE_WRITE_START, buf[(addr/2)] & 0xff);
            program_wait();

            write_command(COMMAND_CORE_INSTRUCTION, ASM_INCF_TBLPTR);
            write_command(COMMAND_TABLE_WRITE_START, buf[(addr/2)] & 0xff00);
            program_wait();

            this->progress_count++;

            if (verify) {
                /* Verify the config word just written */
                read_config_memory(buf, addr, 1, true);
            }
        }
        /* Program word 6 last: if we protect the configuration words, */
        /* we won't be able to program word 7.                         */
        set_tblptr(skipd_addr);
        write_command(COMMAND_TABLE_WRITE_START, buf[(skipd_addr/2)] & 0xff);
        program_wait();
        write_command(COMMAND_CORE_INSTRUCTION, ASM_INCF_TBLPTR);
        write_command(COMMAND_TABLE_WRITE_START, buf[(skipd_addr/2)] & 0xff00);
        program_wait();

        this->progress_count++;
        if (verify) {
            /* Verify the config word just written */
            read_config_memory(buf, skipd_addr, 1, true);
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write configuration word #%d",
                i
            )
        );
    }
}

void Pic18fxx20::read_data_memory(DataBuffer& buf, unsigned long addr, bool verify)
{
    uint32_t     ins;
    unsigned int offset; /* Byte offset */
    unsigned int data;

    offset = 0;
    try {
        /* Direct address to data EEPROM */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset<this->eesize; offset++) {
            /* Give byte addresses to progress() to match datasheet. */
            progress(addr+offset);

            /* Set the data EEPROM address pointer */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADR));

            /* Initiate a memory read */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_RD);

            /* Load data into the serial data holding register */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EEDATA_W_0);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);

            /* Shift out data */
            data = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
            if (verify) {
                /* The data is packed 2 bytes per word, little endian */
                if ((offset & 1) == 0) {
                    if (data != (buf[(addr + offset)/2] & 0xff)) {
                        throw runtime_error("");
                    }
                } else {
                    if (data != ((buf[(addr + offset)/2] >> 8) & 0xff)) {
                        throw runtime_error("");
                    }
                }
            } else {
                /* The data is packed 2 bytes per word, little endian */
                if ((offset & 1) == 0) {
                    buf[(addr + offset)/2] &= ~0x00ff;
                    buf[(addr + offset)/2] |= data;
                } else {
                    buf[(addr + offset)/2] &= ~0xff00;
                    buf[(addr + offset)/2] |= (data << 8);
                }
            }
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at location %d",
                verify ? "Data memory verification failed"
                       : "Couldn't read data memory",
                offset
            )
        );
    }
}

bool Pic18fxx20::load_write_buffer (
    DataBuffer&   buf,
    unsigned long addr,
    unsigned long count
) {
    unsigned int    i;
    unsigned int    a = 0xffff;

    for ( i = 0; i < (count / 2); i++ ) {
        a &= buf[(addr/2) + i];
    }
    if ( a != 0xffff ) {
        /* Fill all but the last word in the write buffer */
        for (i = 0; i < (count / 2) - 1; i++) {
            write_command(COMMAND_TABLE_WRITE_POSTINC, buf[(addr/2) + i]);
        }
        /* Put the last word in the write buffer and start programming */
        write_command(COMMAND_TABLE_WRITE_START, buf[(addr/2) + i]);
    }
    this->progress_count += count / 2;    /* Increment by # words */
    
    return (a != 0xffff);
}

void Pic18fxx20::program_wait(void)
{
    this->io->shift_bits_out(0x00, 3);
    this->io->data(false);
    this->io->clock(true);                /* Hold clk high                   */

    this->io->usleep(program_time);       /* P9                              */

    this->io->clock(false);
    this->io->usleep(100);                /* High-voltage discharge time P10 */
    this->io->shift_bits_out(0x0000, 16); /* 16-bit payload (NOP)            */
}
