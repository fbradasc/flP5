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
#define ASM_BCF_EECON1_CFGS  0x9ca6          /* bcf EECON1, CfGS  */
#define ASM_BCF_EECON1_FREE  0x98a6          /* bcf EECON1, FREE  */
#define ASM_BCF_EECON1_WRERR 0x96a6          /* bcf EECON1, WRERR */
#define ASM_BCF_EECON1_WREN  0x94a6          /* bcf EECON1, WREN  */
#define ASM_BSF_EECON1_EEPGD 0x8ea6          /* bsf EECON1, EEPGD */
#define ASM_BSF_EECON1_CFGS  0x8ca6          /* bsf EECON1, CFGS  */
#define ASM_BSF_EECON1_FREE  0x88a6          /* bsf EECON1, FREE  */
#define ASM_BSF_EECON1_WRERR 0x86a6          /* bsf EECON1, WRERR */
#define ASM_BSF_EECON1_WREN  0x84a6          /* bsf EECON1, WREN  */
#define ASM_BSF_EECON1_WR    0x82a6          /* bsf EECON1, WR    */
#define ASM_BSF_EECON1_RD    0x80a6          /* bsf EECON1, RD    */
#define ASM_MOVLW(addr)      0x0e00 | (addr) /* movlw <addr>      */
#define ASM_MOVWF(addr)      0x6e00 | (addr) /* movwf <addr>      */
#define ASM_MOVF_EEDATA_W_0  0x50a8          /* movf EEDATA, W, 0 */
#define ASM_MOVF_EECON1_W_0  0x50a6          /* movf EECON1, W, 0 */
#define ASM_INCF_TBLPTR      0x2af6          /* incf TBLPTR       */

const Instruction Pic18::opcodes[] = {
    /* PIC 16-bit "Special" instruction set */
    { "clrc"   , 0xffff, 0x90d8, INSN_CLASS_IMPLICIT },
    { "clrdc"  , 0xffff, 0x92d8, INSN_CLASS_IMPLICIT },
    { "clrn"   , 0xffff, 0x98d8, INSN_CLASS_IMPLICIT },
    { "clrov"  , 0xffff, 0x96d8, INSN_CLASS_IMPLICIT },
    { "clrw"   , 0xffff, 0x6ae8, INSN_CLASS_IMPLICIT },
    { "clrz"   , 0xffff, 0x94d8, INSN_CLASS_IMPLICIT },
    { "setc"   , 0xffff, 0x80d8, INSN_CLASS_IMPLICIT },
    { "setdc"  , 0xffff, 0x82d8, INSN_CLASS_IMPLICIT },
    { "setn"   , 0xffff, 0x88d8, INSN_CLASS_IMPLICIT },
    { "setov"  , 0xffff, 0x86d8, INSN_CLASS_IMPLICIT },
    { "setz"   , 0xffff, 0x84d8, INSN_CLASS_IMPLICIT },
    { "skpc"   , 0xffff, 0xa0d8, INSN_CLASS_IMPLICIT },
    { "skpdc"  , 0xffff, 0xa2d8, INSN_CLASS_IMPLICIT },
    { "skpn"   , 0xffff, 0xa8d8, INSN_CLASS_IMPLICIT },
    { "skpov"  , 0xffff, 0xa6d8, INSN_CLASS_IMPLICIT },
    { "skpz"   , 0xffff, 0xa4d8, INSN_CLASS_IMPLICIT },
    { "skpnc"  , 0xffff, 0xb0d8, INSN_CLASS_IMPLICIT },
    { "skpndc" , 0xffff, 0xb2d8, INSN_CLASS_IMPLICIT },
    { "skpnn"  , 0xffff, 0xb8d8, INSN_CLASS_IMPLICIT },
    { "skpnov" , 0xffff, 0xb6d8, INSN_CLASS_IMPLICIT },
    { "skpnz"  , 0xffff, 0xb4d8, INSN_CLASS_IMPLICIT },
    { "tgc"    , 0xffff, 0x70d8, INSN_CLASS_IMPLICIT },
    { "tgdc"   , 0xffff, 0x72d8, INSN_CLASS_IMPLICIT },
    { "tgn"    , 0xffff, 0x78d8, INSN_CLASS_IMPLICIT },
    { "tgov"   , 0xffff, 0x76d8, INSN_CLASS_IMPLICIT },
    { "tgz"    , 0xffff, 0x74d8, INSN_CLASS_IMPLICIT },

    /* PIC 16-bit Normal instruction set */
    { "addlw"  , 0xff00, 0x0f00, INSN_CLASS_LIT8     },
    { "addwf"  , 0xfc00, 0x2400, INSN_CLASS_OPWFA8   },
    { "addwfc" , 0xfc00, 0x2000, INSN_CLASS_OPWFA8   },
    { "andlw"  , 0xff00, 0x0b00, INSN_CLASS_LIT8     },
    { "andwf"  , 0xfc00, 0x1400, INSN_CLASS_OPWFA8   },
    { "bc"     , 0xff00, 0xe200, INSN_CLASS_RBRA8    },
    { "bcf"    , 0xf000, 0x9000, INSN_CLASS_BA8      },
    { "bn"     , 0xff00, 0xe600, INSN_CLASS_RBRA8    },
    { "bnc"    , 0xff00, 0xe300, INSN_CLASS_RBRA8    },
    { "bnn"    , 0xff00, 0xe700, INSN_CLASS_RBRA8    },
    { "bnov"   , 0xff00, 0xe500, INSN_CLASS_RBRA8    },
    { "bnz"    , 0xff00, 0xe100, INSN_CLASS_RBRA8    },
    { "bov"    , 0xff00, 0xe400, INSN_CLASS_RBRA8    },
    { "bra"    , 0xf800, 0xd000, INSN_CLASS_RBRA11   },
    { "bsf"    , 0xf000, 0x8000, INSN_CLASS_BA8      },
    { "btfsc"  , 0xf000, 0xb000, INSN_CLASS_BA8      },
    { "btfss"  , 0xf000, 0xa000, INSN_CLASS_BA8      },
    { "btg"    , 0xf000, 0x7000, INSN_CLASS_BA8      },
    { "bz"     , 0xff00, 0xe000, INSN_CLASS_RBRA8    },
    { "call"   , 0xfe00, 0xec00, INSN_CLASS_CALL20   },
    { "clrf"   , 0xfe00, 0x6a00, INSN_CLASS_OPFA8    },
    { "clrwdt" , 0xffff, 0x0004, INSN_CLASS_IMPLICIT },
    { "comf"   , 0xfc00, 0x1c00, INSN_CLASS_OPWFA8   },
    { "cpfseq" , 0xfe00, 0x6200, INSN_CLASS_OPFA8    },
    { "cpfsgt" , 0xfe00, 0x6400, INSN_CLASS_OPFA8    },
    { "cpfslt" , 0xfe00, 0x6000, INSN_CLASS_OPFA8    },
    { "daw"    , 0xffff, 0x0007, INSN_CLASS_IMPLICIT },
    { "decf"   , 0xfc00, 0x0400, INSN_CLASS_OPWFA8   },
    { "decfsz" , 0xfc00, 0x2c00, INSN_CLASS_OPWFA8   },
    { "dcfsnz" , 0xfc00, 0x4c00, INSN_CLASS_OPWFA8   },
    { "goto"   , 0xff00, 0xef00, INSN_CLASS_LIT20    },
    { "incf"   , 0xfc00, 0x2800, INSN_CLASS_OPWFA8   },
    { "incfsz" , 0xfc00, 0x3c00, INSN_CLASS_OPWFA8   },
    { "infsnz" , 0xfc00, 0x4800, INSN_CLASS_OPWFA8   },
    { "iorlw"  , 0xff00, 0x0900, INSN_CLASS_LIT8     },
    { "iorwf"  , 0xfc00, 0x1000, INSN_CLASS_OPWFA8   },
    { "lfsr"   , 0xffc0, 0xee00, INSN_CLASS_FLIT12   },
    { "movf"   , 0xfc00, 0x5000, INSN_CLASS_OPWFA8   },
    { "movff"  , 0xf000, 0xc000, INSN_CLASS_FF       },
    { "movlb"  , 0xff00, 0x0100, INSN_CLASS_LIT8     },
    { "movlw"  , 0xff00, 0x0e00, INSN_CLASS_LIT8     },
    { "movwf"  , 0xfe00, 0x6e00, INSN_CLASS_OPFA8    },
    { "mullw"  , 0xff00, 0x0d00, INSN_CLASS_LIT8     },
    { "mulwf"  , 0xfe00, 0x0200, INSN_CLASS_OPFA8    },
    { "negf"   , 0xfe00, 0x6c00, INSN_CLASS_OPFA8    },
    { "nop"    , 0xffff, 0x0000, INSN_CLASS_IMPLICIT },
    { "pop"    , 0xffff, 0x0006, INSN_CLASS_IMPLICIT },
    { "push"   , 0xffff, 0x0005, INSN_CLASS_IMPLICIT },
    { "rcall"  , 0xf800, 0xd800, INSN_CLASS_RBRA11   },
    { "reset"  , 0xffff, 0x00ff, INSN_CLASS_IMPLICIT }, 
    { "retfie" , 0xfffe, 0x0010, INSN_CLASS_LIT1     },
    { "retlw"  , 0xff00, 0x0c00, INSN_CLASS_LIT8     },
    { "return" , 0xfffe, 0x0012, INSN_CLASS_LIT1     },
    { "rlcf"   , 0xfc00, 0x3400, INSN_CLASS_OPWFA8   },
    { "rlncf"  , 0xfc00, 0x4400, INSN_CLASS_OPWFA8   },
    { "rrcf"   , 0xfc00, 0x3000, INSN_CLASS_OPWFA8   },
    { "rrncf"  , 0xfc00, 0x4000, INSN_CLASS_OPWFA8   },
    { "setf"   , 0xfe00, 0x6800, INSN_CLASS_OPFA8    },
    { "sleep"  , 0xffff, 0x0003, INSN_CLASS_IMPLICIT },
    { "subfwb" , 0xfc00, 0x5400, INSN_CLASS_OPWFA8   },
    { "sublw"  , 0xff00, 0x0800, INSN_CLASS_LIT8     },
    { "subwf"  , 0xfc00, 0x5c00, INSN_CLASS_OPWFA8   },
    { "subwfb" , 0xfc00, 0x5800, INSN_CLASS_OPWFA8   },
    { "swapf"  , 0xfc00, 0x3800, INSN_CLASS_OPWFA8   },
    { "tblrd"  , 0xfffc, 0x0008, INSN_CLASS_TBL      },
    { "tblwt"  , 0xfffc, 0x000c, INSN_CLASS_TBL      },
    { "tstfsz" , 0xfe00, 0x6600, INSN_CLASS_OPFA8    },
    { "xorlw"  , 0xff00, 0x0a00, INSN_CLASS_LIT8     },
    { "xorwf"  , 0xfc00, 0x1800, INSN_CLASS_OPWFA8   },

    /* PIC 16-bit Extended instruction set */
    { "addfsr" , 0xff00, 0xe800, INSN_CLASS_LITFSR   },
    { "addulnk", 0xffc0, 0xe8c0, INSN_CLASS_LIT6     },
    { "callw"  , 0xffff, 0x0014, INSN_CLASS_IMPLICIT },
    { "movsf"  , 0xff80, 0xeb00, INSN_CLASS_SF       },
    { "movss"  , 0xff80, 0xeb80, INSN_CLASS_SS       },
    { "pushl"  , 0xff00, 0xea00, INSN_CLASS_LIT8     },
    { "subfsr" , 0xff00, 0xe900, INSN_CLASS_LITFSR   },
    { "subulnk", 0xffc0, 0xe9c0, INSN_CLASS_LIT6     },

    { 0        , 0x0000, 0x0000, INSN_CLASS_NULL     }
};

/* ID locations: 0x200000 - 0x200007 (byte address)
 * Config words: 0x300000 - 0x30000D (byte address)
 * Device ID:    0x3ffffe - 0x3fffff (byte address)
 * Data EEPROM:  0xf00000 (byte address)
 */
#define PROG_MEM_ADDR  (0)
#define PROG_MEM_WRDS  (this->codesize)
#define ID_LOC_ADDR    (0x200000/2)
#define ID_LOC_WRDS    (8/2)
#define CFG_WORDS_ADDR (0x300000/2)
#define CFG_WORDS_WRDS (14/2)
#define DEV_ID_ADDR    (0x3ffffe/2)
#define DEV_ID_WRDS    (1)
#define EEPROM_ADDR    (0xf00000/2)
#define EEPROM_WRDS    (this->eesize/2)

Pic18::Pic18(char *name) : Pic(name)
{
char buf[16];
int i;

    this->popcodes = this->opcodes;

    /* Read in config bits */
    for (i=0; i<CFG_WORDS_WRDS; i++) {
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
    this->memmap.push_back(IntPair(PROG_MEM_ADDR,PROG_MEM_WRDS));
    this->memmap.push_back(IntPair(ID_LOC_ADDR,ID_LOC_WRDS));
    this->memmap.push_back(IntPair(CFG_WORDS_ADDR,CFG_WORDS_WRDS));
    if (this->flags & PIC_FEATURE_EEPROM) {
        this->memmap.push_back(IntPair(EEPROM_ADDR,EEPROM_WRDS));
    }
}

Pic18::~Pic18()
{
}

unsigned int Pic18::get_clearvalue(size_t addr)
{
    if ((addr >= CFG_WORDS_ADDR) && (addr < (CFG_WORDS_ADDR+CFG_WORDS_WRDS))) {
        return this->wordmask & ~this->config_masks[addr-CFG_WORDS_ADDR];
    } else if ((addr >= ID_LOC_ADDR) && (addr < (ID_LOC_ADDR+ID_LOC_WRDS))) { // ID locations
        return this->wordmask;
    } else if ((addr >= PROG_MEM_ADDR) && (addr < (PROG_MEM_ADDR+PROG_MEM_WRDS))) {  // program memory
        return this->wordmask;
    }
    return 0x00ff; // data memory clear value
}

void Pic18::erase(void)
{
    if (this->memtype != MEMTYPE_FLASH) {
        throw runtime_error("Operation not supported by device");
    }
    try {
        set_program_mode();
        chip_erase();
        pic_off();
    } catch (std::exception& e) {
        pic_off();
        throw;
    }
}

void Pic18::range_erase(int memstart, int memlen)
{
int blockstart, blocklen;

    blockstart = memstart / 64 * 64;
    blocklen = ((memstart+memlen) / 64 * 64) - blockstart + 64;
    block_erase(blocklen, blockstart);
}

void Pic18::chip_erase(void)
{
    set_tblptr(0x3c0004);
    write_command(COMMAND_TABLE_WRITE, 0x0080);      /* "Chip Erase" */
    write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
    program_delay(false);
    this->io->usleep(this->erase_time);
}

void Pic18::panel_erase(int panel)
{
    if ((panel > 4) || (panel < 0)) {
        throw runtime_error("Panel selected out of range");
    }
    set_tblptr(0x3c0004);
    write_command(COMMAND_TABLE_WRITE, (panel==0) ? 0x0083 : 0x0088 | (panel-1));      /* "Panel Erase" */
    write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
    program_delay(false);
    this->io->usleep(this->erase_time);
}

void Pic18::block_erase(int len, int address)
{
    if (address % 64) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Misaligned block address: [%x] %x",
                address, len
            )
        );
    }
    if (len % 64) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Misaligned block length: [%x] %x",
                address, len
            )
        );
    }
    set_tblptr(0x3c0006);
    write_command(COMMAND_TABLE_WRITE, 0);
    write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_EEPGD);
    write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_CFGS);
    write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WREN);

    while (len > 0) {
        set_tblptr(address);

        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_FREE);

        /* Perform required sequence */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(0x55));
        write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EECON2));
        write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVLW(0xaa));
        write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EECON2));

        /* Initiate write */
        write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_WR);
        program_delay(false);
        
        address += 64;
        len -= 64;
    }
    write_command(COMMAND_CORE_INSTRUCTION, ASM_BCF_EECON1_WREN);
    
    /* If we don't pause here, sometimes the next command sent to the
     * PIC fails.  This may be an undocumented bug of the
     * microcontroller, or it may be something I missed in the
     * programmer documentation.  The following function is used because
     * it provides a 1 ms delay, not because the line needs to be held
     * high.
     */
    program_delay();
    this->io->usleep(this->erase_time);
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

            program_delay();

            if (verify) {
                /* Verify the memory just written */
                for (panel=0; panel<npanels; panel++) {
                    progress((panel << PANEL_SHIFT) + offset);
                    /* Verify the 4 words per panel */
                    read_memory(buf, (panel << PANEL_SHIFT) + offset, 4, true);
                }
            }
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write program memory at address 0x%06lx: %s",
                (unsigned long)(panel << PANEL_SHIFT) + offset,
                e.what()
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
        program_delay();

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
uint8_t data;
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
            if ((offset & 1) == 0) {
                data = buf[(addr+offset)/2] & 0xff;
            } else {
                data = (buf[(addr+offset)/2] >> 8) & 0xff;
            }
            ins = ASM_MOVLW(data);
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

            write_command(COMMAND_CORE_INSTRUCTION, ASM_NOP);
            program_delay(false);

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
            program_delay();

            write_command(COMMAND_CORE_INSTRUCTION, ASM_INCF_TBLPTR);
            write_command(COMMAND_TABLE_WRITE_START, buf[(addr/2)] & 0xff00);
            program_delay();

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
            progress(addr+offset);

            /* Set the data EEPROM address pointer */
            ins = ASM_MOVLW(offset & 0xff);
            write_command(COMMAND_CORE_INSTRUCTION, ins);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADR));
            ins = ASM_MOVLW((offset >> 8) & 0xff);
            write_command(COMMAND_CORE_INSTRUCTION, ins);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_EEADRH));

            /* Initiate a memory read */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_BSF_EECON1_RD);

            /* Load data into the serial data holding register */
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVF_EEDATA_W_0);
            write_command(COMMAND_CORE_INSTRUCTION, ASM_MOVWF(REG_TABLAT));

            /* Shift out data */
            data = write_command_read_data(COMMAND_SHIFT_OUT_TABLAT);
            if (verify) {
                /* The data is packed 2 bytes per word, little endian */
                if ((offset & 1) == 0) {
                    if (diff(data,buf[(addr + offset)/2],0xff)) {
                        throw runtime_error("");
                    }
                } else {
                    if (diff(data,(buf[(addr + offset)/2] >> 8),0xff)) {
                        throw runtime_error("");
                    }
                }
            } else {
                /* The data is packed 2 bytes per word, little endian */
                if((offset & 1) == 0) {
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

void Pic18::program_delay(bool hold_clock_high)
{
    this->io->shift_bits_out(0x00, BITS_AND_CLK(4,hold_clock_high));
    this->io->usleep(program_time);
    if (hold_clock_high) {
        this->io->clock(false);
    }
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
    this->io->usleep(1);
    return (this->io->shift_bits_in(8) & 0xff);
}

uint32_t Pic18::read_deviceid(void)
{
unsigned int d1, d2;

    /* Read and check the device ID */
    set_tblptr(0x3ffffe);
    d1 = write_command_read_data(COMMAND_TABLE_READ_POSTINC);
    d2 = write_command_read_data(COMMAND_TABLE_READ_POSTINC);

    return d1 | d2<<8;
}
