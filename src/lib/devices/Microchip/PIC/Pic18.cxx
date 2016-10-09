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
 * $Id: Pic18.cxx,v 1.11 2002/11/07 14:25:20 marka Exp $
 */
#include <stdexcept>

using namespace std;

#include "Microchip.h"
#include "Util.h"

#define PANEL_SHIFT 13
#define PANELSIZE (1 << PANEL_SHIFT) /* bytes */

#define REG_EEADR            0xa9
#define REG_EEADRH           0xaa
#define REG_EEDATA           0xa8
#define REG_EECON2           0xa7
#define REG_TABLAT           0xf5
#define REG_TBLPTRU          0xf8
#define REG_TBLPTRH          0xf7
#define REG_TBLPTRL          0xf6

#define ASM_NOP              0x0000          /* nop               */
#define ASM_BCF_EECON1_EEPGD 0x9ea6          /* bcf EECON1, EEPGD */
#define ASM_BCF_EECON1_CFGS  0x9ca6          /* bcf EECON1, CPGS  */
#define ASM_BCF_EECON1_WREN  0x94a6          /* bcf EECON1, WREN  */
#define ASM_BSF_EECON1_EEPGD 0x8ea6          /* bsf EECON1, EEPGD */
#define ASM_BSF_EECON1_CFGS  0x8ca6          /* bsf EECON1, CFGS  */
#define ASM_BSF_EECON1_WREN  0x84a6          /* bsf EECON1, WREN  */
#define ASM_BSF_EECON1_WR    0x82a6          /* bsf EECON1, WR    */
#define ASM_BSF_EECON1_RD    0x80a6          /* bsf EECON1, RD    */
#define ASM_MOVLW(addr)      0x0e00 | (addr) /* movlw <addr>      */
#define ASM_MOVWF(addr)      0x6e00 | (addr) /* movwf <addr>      */
#define ASM_MOVF_EEDATA_W_0  0x50a8          /* movf EEDATA, W, 0 */
#define ASM_MOVF_EECON1_W_0  0x50a6          /* movf EECON1, W, 0 */
#define ASM_INCF_TBLPTR      0x2af6          /* incf TBLPTR       */

/* ID locations: 0x200000 - 0x200007 (byte address)
 * Config words: 0x300000 - 0x30000D (byte address)
 * Device ID:    0x3ffffe - 0x3fffff (byte address)
 * Data EEPROM:  0xf00000 (byte address)
 */

Pic18::Pic18(char *name) : Pic(name)
{
char buf[16];
int i;

    /* Read in config bits */
    for (i=0; i<7; i++) {
        if (
            !config->getHex (
                Preferences::Name("cw_mask_%02d", i),
                (int &)config_masks[i],
                0
            )
        ) {
            config_masks[i] = 0xffff;
        }
    }
    /* Create the memory map for this device. Note that these are 16-bit word
     * offsets and lengths which are 1/2 of their byte equivalents */
    this->memmap.push_back(IntPair(0,this->codesize));
    this->memmap.push_back(IntPair(0x200000/2,4));  /* ID locations */
    this->memmap.push_back(IntPair(0x300000/2,7));  /* Config words */
    if (this->flags & PIC_FEATURE_EEPROM) {
        this->memmap.push_back(IntPair(0xf00000/2,this->eesize/2));
    }
}

Pic18::~Pic18()
{
}

void Pic18::erase(void)
{
    if (this->memtype != MEMTYPE_FLASH) {
        throw runtime_error("Operation not supported by device");
    }
    try {
        set_program_mode();

        set_tblptr(0x3c0004);
        write_command(COMMAND_TABLE_WRITE, 0x0080);      /* "Chip Erase" */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
        this->io->usleep(this->erase_time);

        pic_off();
    } catch (std::exception& e) {
        pic_off();
        throw;
    }
}

void Pic18::program(DataBuffer& buf)
{
    switch (this->memtype) {
        case MEMTYPE_EPROM:
        case MEMTYPE_FLASH:
            break;
        default:
            throw runtime_error("Unsupported memory type in device");
    }

    /* Progress_total is x2 because we write and verify every location */
    this->progress_total = 2 * (this->codesize + 4 + 7 + this->eesize) - 1;
    this->progress_count = 0;

    try {
        set_program_mode();

        write_program_memory(buf, true);
        write_id_memory(buf, 0x200000, true);
        if (flags & PIC_FEATURE_EEPROM) {
            write_data_memory(buf, 0xf00000, true);
        }
        write_config_memory(buf, 0x300000, true);

        pic_off();
    } catch (std::exception& e) {
        pic_off();
        throw;
    }
}

void Pic18::write_program_memory(DataBuffer& buf, bool verify)
{
unsigned int npanels, panel, offset;

    panel = 0;
    offset = 0;
    npanels = this->codesize/(PANELSIZE/2);
    try {
        /* Step 1: Enable multi-panel writes */
        set_tblptr(0x3c0006);
        write_command(COMMAND_TABLE_WRITE, 0x0040);

        /* Step 2: Direct access to code memory */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset < PANELSIZE; offset+=8) {
            /* Step 3,4,5,6: Load write buffer for Panel 1,2,3,4 */
            for (panel=0; panel<npanels-1; panel++) {
                /* Give byte addresses to progress() to match datasheet. */
                progress((panel << PANEL_SHIFT) + offset);
                load_write_buffer(buf, panel, offset, false);
            }
            load_write_buffer(buf, panel, offset, true);

            program_wait();

            if (verify) {
                /* Verify the memory just written */
                for (unsigned int panel=0; panel<npanels; panel++) {
                    progress((panel << PANEL_SHIFT) + offset);
                    /* Verify the 4 words per panel */
                    read_memory(buf, (panel << PANEL_SHIFT) + offset, 4, true);
                }
            }
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write program memory at address 0x%06lx",
                (unsigned long)(panel << PANEL_SHIFT) + offset
            )
        );
    }
}

void Pic18::write_id_memory (
    DataBuffer& buf,
    unsigned long addr,
    bool verify
) {
    progress(addr);
    try {
        /* Step 1: Direct access to config memory */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_CFGS);

        /* Step 2: Configure device for single panel writes */
        set_tblptr(0x3c0006);
        write_command(COMMAND_TABLE_WRITE, 0x0000);

        /* Step 3: Direct access to code memory */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        /* Step 4: Load write buffer. Panel will be automatically determined
         * by address. */
        load_write_buffer(buf, addr/PANELSIZE, addr%PANELSIZE, true);
        program_wait();

        if (verify) {
            /* Verify the memory just written */
            read_memory(buf, addr, 4, true);
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

void Pic18::write_data_memory (
    DataBuffer& buf,
    unsigned long addr,
    bool verify
) {
uint32_t ins;
unsigned int offset;

    offset = 0;
    try {
        /* Step 1: Direct access to data EEPROM */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset < this->eesize; offset++) {
            progress(addr+(2*offset));

            /* Step 2: Set the data EEPROM address pointer */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset & 0xff));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADR));
            ins = ASM_MOVLW((offset >> 8) & 0xff);
            write_command(COMMAND_CORE_INSTRUCTION, ins);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADRH));

            /* Step 3: Load the data to be written */
            ins = ASM_MOVLW(buf[(addr/2)+offset] & 0xff);
            write_command(COMMAND_CORE_INSTRUCTION, ins);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEDATA));

            /* Step 4: Enable memory writes */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WREN);

            /* Step 5: Perform required sequence */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(0x55));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EECON2));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(0xaa));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EECON2));

            /* Step 6: Initiate write */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WR);

            /* Step 7: Poll WR bit, repeat until the bit is clear */
            do {
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EECON1_W_0);
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));

                ins = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
            } while (ins & 0x02);

            /* Step 8: Disable writes */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_WREN);

            this->progress_count++;

            if (verify) {
                /* Initiate a memory read */
                /* BSF EECON1, RD */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_RD);

                /* Load data into the serial data holding register */
                /* MOVF EEDATA, W, 0 */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EEDATA_W_0);
                /* MOVWF TABLAT */
                write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));

                /* Shift out data and check */
                ins = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
                if (diff(buf[(addr/2)+offset],ins,0xff)) {
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

void Pic18::write_config_memory (
    DataBuffer& buf,
    unsigned long addr,
    bool verify
) {
int i;

    i = 0;
    try {
        /* Step 1: Direct access to config memory */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_CFGS);

        /* Step 2: Position the program counter */
        /* GOTO 100000h */
        write_command(COMMAND_CORE_INSTRUCTION, 0xef00);
        write_command(COMMAND_CORE_INSTRUCTION, 0xf800);

        for (i=0; i<7; i++) {
            /* Give byte addresses to the callback to match datasheet. */
            progress(addr);

            /* Step 3: Set Table Pointer for config byte to be written. Write
             * even/odd addresses */
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
            addr += 2;
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

void Pic18::read(DataBuffer& buf, bool verify)
{
    this->progress_total = this->codesize + 4 + 7 + this->eesize - 1;
    this->progress_count = 0;
    try {
        set_program_mode();

        read_memory(buf, 0, this->codesize, verify);    /* Program memory */
        read_memory(buf, 0x200000, 4, verify);          /* ID memory */
        read_config_memory(buf, 0x300000, 7, verify);   /* Config words */
        if (flags & PIC_FEATURE_EEPROM) {
            read_data_memory(buf, 0xf00000, verify);
        }
        pic_off();
    } catch (std::exception& e) {
        pic_off();
        throw;
    }
}

void Pic18::read_memory (
    DataBuffer& buf,
    unsigned long addr,
    unsigned long len,
    bool verify
) {
unsigned int data;

    try {
        addr >>= 1;         /* Shift to word addresses */
        set_tblptr(2*addr);
        while (len > 0) {
            /* Give byte addresses to progress() to match datasheet. */
            progress(addr*2);

            /* Read memory a byte at a time (little endian format) */
            data  = write_command_read_data(COMMAND_TABLE_READ_POSTINC);
            data |= (write_command_read_data(COMMAND_TABLE_READ_POSTINC) << 8);
            if (verify) {
                if (diff(buf[addr],data,0xffffffff)) { // TODO: verify the mask
                    throw runtime_error("");
                }
            } else {
                buf[addr] = data;
            }
            addr++;
            len--;
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at address 0x%06lx",
                verify ? "Verification failed"
                       : "Couldn't read memory",
                2*addr
            )
        );
    }
}

void Pic18::read_config_memory (
    DataBuffer& buf,
    unsigned long addr,
    unsigned long len,
    bool verify
) {
unsigned int data, cword_num;
unsigned long i;

    cword_num = 0;
    try {
        addr >>= 1;         /* Shift to word addresses */
        set_tblptr(2*addr);
        for (i=0; i<len; i++) {
            /* Give byte addresses to progress() to match datasheet. */
            progress(addr*2);

            /* Read memory a byte at a time (little endian format) */
            cword_num = addr - (0x300000/2);

            data  = write_command_read_data(COMMAND_TABLE_READ_POSTINC);
            data |= (write_command_read_data(COMMAND_TABLE_READ_POSTINC) << 8);

            if (verify) {
                if (diff(buf[addr],data,this->config_masks[cword_num])) {
                    throw runtime_error("");
                }
            } else {
                buf[addr] = data;
            }
            addr++;
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s of configuration word #%u failed",
                verify ? "Verification" : "Read",
                cword_num
            )
        );
    }
}

void Pic18::read_data_memory (
    DataBuffer& buf,
    unsigned long addr,
    bool verify
) {
uint32_t ins;
unsigned int offset, data = 0;

    offset = 0;
    try {
        /* Direct address to data EEPROM */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_EEPGD);
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);

        for (offset=0; offset<this->eesize; offset++) {
            /* Give byte addresses to progress() to match datasheet. */
            progress(addr+(2*offset));

            /* Set the data EEPROM address pointer */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(offset & 0xff));
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADR));

            /* Initiate a memory read */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_RD);

            /* Load data into the serial data holding register */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EEDATA_W_0);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));

            /* Shift out data */
            data = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
            if (verify) {
                if (diff(buf[(addr/2)+offset],data,0xff)) {
                    throw runtime_error("");
                }
            } else {
                buf[(addr/2)+offset] = data;
            }
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error ( 
            (const char *)Preferences::Name (
                "%s at location %d",
                verify ? "Data memory verification failed" :
                         "Couldn't read data memory",
                offset
            )
        );
    }
}

void Pic18::load_write_buffer (
    DataBuffer& buf,
    unsigned int panel,
    unsigned int offset,
    bool last
) {
unsigned long addr;
int i;

    addr = (panel << PANEL_SHIFT) + offset;
    set_tblptr(addr);
    for (i=0; i<3; i++) {        /* Write 3 words */
        write_command(COMMAND_TABLE_WRITE_POSTINC, buf[(addr / 2) + i]);
        this->progress_count++;
    }
    if (last) {
        /* Do the final word write which also starts the programming */
        write_command(COMMAND_TABLE_WRITE_START, buf[(addr / 2) + 3]);
    } else {
        /* Just another load */
        write_command(COMMAND_TABLE_WRITE_POSTINC, buf[(addr / 2) + 3]);
    }
    this->progress_count++;
}

void Pic18::program_wait(void)
{
    this->io->shift_bits_out(0x00, 3);
    this->io->data(false);
    this->io->clock(true);    /* Hold clk high */

    this->io->usleep(program_time);

    this->io->clock(false);
    this->io->usleep(5);    /* High-voltage discharge time */
    this->io->shift_bits_out(0x0000, 16);/* 16-bit payload (NOP) */
}

void Pic18::set_tblptr(unsigned long addr)
{
uint32_t ins;

    ins = ASM_MOVLW((addr >> 16) & 0x3f);   /* addr[21:16] */
    write_command(COMMAND_CORE_INSTRUCTION, ins);
    write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TBLPTRU));
    ins = ASM_MOVLW((addr >> 8) & 0xff);    /* addr[15:8] */
    write_command(COMMAND_CORE_INSTRUCTION, ins);
    write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TBLPTRH));
    ins = ASM_MOVLW(addr & 0xff);           /* addr[7:0] */
    write_command(COMMAND_CORE_INSTRUCTION, ins);
    write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TBLPTRL));
}

void Pic18::write_command(unsigned int command)
{
    this->io->shift_bits_out(command, 4);
    this->io->usleep(1);
}

void Pic18::write_command(unsigned int command, unsigned int data)
{
    write_command(command);
    this->io->shift_bits_out(data, 16);
    this->io->usleep(1);
}

unsigned int Pic18::write_command_read_data(unsigned int command)
{
    write_command(command);
    this->io->shift_bits_out(0x00, 8);      /* 8 dummy bits */
    return this->io->shift_bits_in(8, 1);
}
