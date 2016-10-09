/* Copyright (C) 2002-2004  Mark Andrew Aikens <marka@desert.cx>
 * Copyright (C) 2005       Pierre Gaufillet   <pierre.gaufillet@magic.fr>
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
#include <stdexcept>
#include <iostream>

using namespace std;

#include "Microchip.h"
#include "Util.h"

/* ID locations: 0x200000 - 0x200007 (byte address)
 * Config words: 0x300000 - 0x30000D (byte address)
 * Device ID:    0x3ffffe - 0x3fffff (byte address)
 * Data EEPROM:  0xf00000 (byte address)
 */
#define ID_LOC_WRDS    (8/2)
#define CFG_WORDS_WRDS (14/2)

Pic18f2xx0::Pic18f2xx0(char *name) : Pic18fxx20(name)
{
}

Pic18f2xx0::~Pic18f2xx0()
{
}

void Pic18f2xx0::erase(void)
{
    if (this->memtype != MEMTYPE_FLASH) {
        throw runtime_error("Operation not supported by device");
    }
    try {
        set_program_mode();

        set_tblptr(0x3c0005);
        write_command(COMMAND_TABLE_WRITE, 0x3f3f);    /* "Chip Erase" */
        set_tblptr(0x3c0004);
        write_command(COMMAND_TABLE_WRITE, 0x8787);    /* "Chip Erase" */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
        this->io->usleep(this->erase_time);

        pic_off();
    } catch (std::exception& e) {
        pic_off();
        throw;
    }
}

void Pic18f2xx0::write_data_memory(DataBuffer& buf, unsigned long addr, bool verify)
{
    uint32_t        ins;
    uint8_t            data;
    unsigned int    offset = 0;    /* word offset    */

    try {
        /* Step 1: Direct access to data EEPROM */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset < this->eesize; offset++) {
            progress(addr+(2*offset));

            /* Step 2: Set the data EEPROM address pointer */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADR));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset >> 8));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADRH));

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
    
                /* Step 5: Initiate write */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WR);
    
                /* Step 6: Poll WR bit, repeat until the bit is clear */
                do {
                    write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EECON1_W_0);
                    write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));
                    write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
                    ins = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
                } while (ins & 0x02);

                /* Step 7: Hold PGC low for time P10 */
                this->io->usleep(100);    /* High-voltage discharge time P10 */
                
                /* Step 8: Disable writes */
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

void Pic18f2xx0::read_data_memory(DataBuffer& buf, unsigned long addr, bool verify)
{
    uint32_t     ins;
    unsigned int offset;    /* Byte offset */
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
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset >> 8));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADRH));

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
                    if (data != (buf[(addr + offset)/2] & 0xff))
                        throw runtime_error("");
                } else {
                    if (data != ((buf[(addr + offset)/2] >> 8) & 0xff))
                        throw runtime_error("");
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
