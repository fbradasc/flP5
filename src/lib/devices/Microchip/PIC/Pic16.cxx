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
 * $Id: Pic16.cxx,v 1.2 2002/11/06 22:42:37 marka Exp $
 */
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "Microchip.h"
#include "IO.h"
#include "Util.h"

const Instruction Pic16::opcodes[] = {
  { "addlw" , 0x3e00, 0x3e00, INSN_CLASS_LIT8     },
  { "addwf" , 0x3f00, 0x0700, INSN_CLASS_OPWF7    },
  { "andlw" , 0x3f00, 0x3900, INSN_CLASS_LIT8     },
  { "andwf" , 0x3f00, 0x0500, INSN_CLASS_OPWF7    },
  { "bcf"   , 0x3c00, 0x1000, INSN_CLASS_B7       },
  { "bsf"   , 0x3c00, 0x1400, INSN_CLASS_B7       },
  { "btfsc" , 0x3c00, 0x1800, INSN_CLASS_B7       },
  { "btfss" , 0x3c00, 0x1c00, INSN_CLASS_B7       },
  { "call"  , 0x3800, 0x2000, INSN_CLASS_LIT11    },
  { "clrf"  , 0x3f80, 0x0180, INSN_CLASS_OPF7     },
  { "clrw"  , 0x3fff, 0x0103, INSN_CLASS_IMPLICIT },
  { "clrwdt", 0x3fff, 0x0064, INSN_CLASS_IMPLICIT },
  { "comf"  , 0x3f00, 0x0900, INSN_CLASS_OPWF7    },
  { "decf"  , 0x3f00, 0x0300, INSN_CLASS_OPWF7    },
  { "decfsz", 0x3f00, 0x0b00, INSN_CLASS_OPWF7    },
  { "goto"  , 0x3800, 0x2800, INSN_CLASS_LIT11    },
  { "incf"  , 0x3f00, 0x0a00, INSN_CLASS_OPWF7    },
  { "incfsz", 0x3f00, 0x0f00, INSN_CLASS_OPWF7    },
  { "iorlw" , 0x3f00, 0x3800, INSN_CLASS_LIT8     },
  { "iorwf" , 0x3f00, 0x0400, INSN_CLASS_OPWF7    },
  { "movf"  , 0x3f00, 0x0800, INSN_CLASS_OPWF7    },
  { "movlw" , 0x3c00, 0x3000, INSN_CLASS_LIT8     },
  { "movwf" , 0x3f80, 0x0080, INSN_CLASS_OPF7     },
  { "nop"   , 0x3f9f, 0x0000, INSN_CLASS_IMPLICIT },
  { "option", 0x3fff, 0x0062, INSN_CLASS_IMPLICIT },
  { "retfie", 0x3fff, 0x0009, INSN_CLASS_IMPLICIT },
  { "retlw" , 0x3c00, 0x3400, INSN_CLASS_LIT8     },
  { "return", 0x3fff, 0x0008, INSN_CLASS_IMPLICIT },
  { "rlf"   , 0x3f00, 0x0d00, INSN_CLASS_OPWF7    },
  { "rrf"   , 0x3f00, 0x0c00, INSN_CLASS_OPWF7    },
  { "sleep" , 0x3fff, 0x0063, INSN_CLASS_IMPLICIT },
  { "sublw" , 0x3e00, 0x3c00, INSN_CLASS_LIT8     },
  { "subwf" , 0x3f00, 0x0200, INSN_CLASS_OPWF7    },
  { "swapf" , 0x3f00, 0x0e00, INSN_CLASS_OPWF7    },
  { "tris"  , 0x3ff8, 0x0060, INSN_CLASS_OPF7     },
  { "xorlw" , 0x3f00, 0x3a00, INSN_CLASS_LIT8     },
  { "xorwf" , 0x3f00, 0x0600, INSN_CLASS_OPWF7    },
  { 0       , 0x0000, 0x0000, INSN_CLASS_NULL     }
};

Pic16::Pic16(char *name) : Pic(name)
{
int tmp;

    this->popcodes = this->opcodes;

    /* Read configuration word bits */
    config->getHex("cw_mask_00",(int &)config_mask[0],this->wordmask);
    config->getHex("cw_save_00",(int &)persistent_config_mask[0],0);
    config->getHex("cw_defs_00",(int &)default_config_word[0],0xffff);
    default_config_word[0] &= config_mask[0];
    if (this->config_words > 1) {
    	config->getHex("cw_mask_00",(int &)config_mask[1],this->wordmask);
    	config->getHex("cw_save_00",(int &)persistent_config_mask[1],0);
    	config->getHex("cw_defs_00",(int &)default_config_word[1],0xffff);
    	default_config_word[1] &= config_mask[1];
    }

    config->getHex("cp_mask_00",(int &)cp_mask, 0);
    config->getHex("cp_all__00",(int &)cp_all,  0);
    config->getHex("cp_none_00",(int &)cp_none, 0);
    config->getHex("dp_mask_00",(int &)cpd_mask,0);
    config->getHex("dp_on___00",(int &)cpd_on,  0);
    config->getHex("dp_off__00",(int &)cpd_off, 0);
    if (config->getHex("bd_mask_00", tmp, 0)) {
        this->flags |= PIC_FEATURE_BKBUG;
    }
    /* Create the memory map for this device */
    this->memmap.push_back(IntPair (0, this->codesize));
    this->memmap.push_back (
        IntPair (0x2000,(this->flags & PIC_FEATURE_BKBUG) ? 5 : 4)
    );
    this->memmap.push_back(IntPair (0x2007, this->config_words));
    if (this->flags & PIC_FEATURE_EEPROM) {
        this->memmap.push_back(IntPair (0x2100, this->eesize));
    }
}

Pic16::~Pic16()
{
}

void Pic16::erase(void)
{
    if (this->memtype != MEMTYPE_FLASH) {
        throw runtime_error("Operation not supported by device");
    }
    /* Read the config word and OSCAL if this PIC has it */
    unsigned long cword, oscal = 0;
    try {
        this->set_program_mode();

        /* If we need to save the oscillator calibration value, increment
         * to the OSCAL address and read it. */
        if (this->flags & PIC_HAS_OSCAL) {
            for (unsigned int i=0; i<this->codesize-1; i++) {
                this->write_command(COMMAND_INC_ADDRESS);
            }
            oscal = this->read_prog_data();
        }
        /* Read the current configuration word to determine if code
         * protection needs to be disabled. */
        this->write_command(COMMAND_LOAD_CONFIG);
        this->io->shift_bits_out(0x7ffe, 16, 1);   /* Dummy write of all 1's */
        this->io->usleep(1);

        /* Skip to the configuration word */
        for (int i=0; i < 7; i++) {
            this->write_command(COMMAND_INC_ADDRESS);
        }
        cword = read_config_word();
        this->pic_off();
    } catch (std::exception& e) {
        this->pic_off();
        throw;
    }
    /* Wait a bit after exiting program mode */
    this->io->usleep(10000);

    /* If code protection or "data code protection" is enabled, we need to do
     * a disable_codeprotect() */
    if (
        ((cword & this->cp_mask) != this->cp_none) ||
        ((cword & this->cpd_mask) != this->cpd_off)
    ) {
        this->disable_codeprotect();
    } else {
        this->bulk_erase();
    }
    
    /* Check if we need to restore some state. */
    if ((this->persistent_config_mask != 0) || (this->flags & PIC_HAS_OSCAL)) {
        /* Wait a bit after exiting program mode */
        this->io->usleep(10000);

        try {
            this->set_program_mode();

            /* Restore the OSCAL value */
            if (this->flags & PIC_HAS_OSCAL) {
                /* Increment to the OSCAL address */
                for (unsigned int i=0; i<this->codesize-1; i++) {
                    this->write_command(COMMAND_INC_ADDRESS);
                }
                if (!this->program_one_location(oscal)) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Couldn't restore OSCAL value of 0x%04lx",
                            oscal
                        )
                    );
                }
            }
            if (this->persistent_config_mask != 0) {
                try {
                    /* Restore the persistent configuration word bits */
                    this->write_command(COMMAND_LOAD_CONFIG);

                    /* Dummy write of all 1's */
                    this->io->shift_bits_out(0x7ffe, 16, 1);
                    this->io->usleep(1);

                    /* Skip to the configuration word */
                    for (int i=0; i < 7; i++) {
                        this->write_command(COMMAND_INC_ADDRESS);
                    }
                    write_config_word (
                        (cword & this->persistent_config_mask[0]) |
                        (this->wordmask & ~this->persistent_config_mask[0])
                    );
                } catch(std::exception& e) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Couldn't restore configuration bits."
                            " Values 0x%04lx",
                            cword & this->persistent_config_mask[0]
                        )
                    );
                }
            }
            this->pic_off();
        } catch(std::exception& e) {
            this->pic_off();
            throw;
        }
    }
}

void Pic16::program(DataBuffer& buf)
{
uint32_t data;

    switch(this->memtype) {
        case MEMTYPE_EPROM:
        case MEMTYPE_FLASH:
        break;
        default:
            throw runtime_error("Unsupported memory type in device");
    }
    this->progress_total = this->codesize + this->eesize + 
    						this->config_words + 4;
    this->progress_count = 0;

    try {
        this->set_program_mode();

        /* Write the program memory */
        this->write_program_memory(buf);

        /* Write the data EEPROM if this PIC has one */
        if (this->flags & PIC_FEATURE_EEPROM) {
            this->write_data_memory(buf, 0x2100);
        }
        /* Write the ID locations */
        this->write_command(COMMAND_LOAD_CONFIG);
        this->io->shift_bits_out(0x7ffe, 16, 1);
        this->io->usleep(1);
        this->write_id_memory(buf, 0x2000);

        /* Write the debugger interrupt location if this PIC has one */
        if (this->flags & PIC_FEATURE_BKBUG) {
            if (!program_one_location((uint32_t)buf[0x2004])) {
                throw runtime_error (
                    "Couldn't write the debugger " \
                    "interrupt vector at address 0x2004."
                );
            }
        }
        /* Skip past the next 3 addresses */
        for (int i=4; i < 7; i++) {
            this->write_command(COMMAND_INC_ADDRESS);
        }
        /* Program the config word, keeping the persistent bits. */
        for (int i=0; i < this->config_words; i++) {
        	data = buf[0x2007 + i] & ~this->persistent_config_mask[i];
        	data |= (read_config_word() & this->persistent_config_mask[i]);
        	this->write_config_word(data);
            this->write_command(COMMAND_INC_ADDRESS);
        	this->progress_count++;
	        progress(0x2007 + i);
        }

        this->pic_off();
    } catch (std::exception& e) {
        this->pic_off();
        throw;
    }
}

void Pic16::read(DataBuffer& buf, bool verify)
{
    this->progress_total = this->codesize + this->eesize + 4;
    this->progress_count = 0;

    try {
        this->set_program_mode();

        /* Read the program memory */
        this->read_program_memory(buf, 0, verify);

        /* Read the data EEPROM if this PIC has one */
        if (this->flags & PIC_FEATURE_EEPROM) {
            this->read_data_memory(buf, 0x2100, verify);
        }
        /* Read the ID locations */
        this->write_command(COMMAND_LOAD_CONFIG);
        this->io->shift_bits_out(0x7ffe, 16, 1); /* Dummy write of all 1's */
        this->io->usleep(1);
        this->read_id_memory(buf, 0x2000, verify);

        /* Read the debugger interrupt location if this PIC has one */
        if (this->flags & PIC_FEATURE_BKBUG) {
            if (verify) {
                if (diff(buf[0x2004],this->read_prog_data(),this->wordmask)) {
                    throw runtime_error (
                        "Verification failed at address 0x2004"
                    );
                }
            } else {
                buf[0x2004] = this->read_prog_data();
            }
        }

        /* Skip past 3 addresses */
       	this->write_command(COMMAND_INC_ADDRESS);
        this->write_command(COMMAND_INC_ADDRESS);
        this->write_command(COMMAND_INC_ADDRESS);
        
        progress(0x2007);
        for (int i = 0; i < this->config_words; i++) {
        	if (verify) {
        	    this->read_config_word(true, buf[0x2007 + i]);
        	} else {
        	    buf[0x2007 + i] = this->read_config_word();
        	}
        	this->progress_count++;
        	this->write_command(COMMAND_INC_ADDRESS);
        }

        this->pic_off();
    } catch (std::exception& e) {
        this->pic_off();
        throw;
    }
}

void Pic16::bulk_erase(void)
{
    try {
        /* Bulk Erase Program Memory */
        /*   Set PC to config memory (to erase ID locations) */
        /*   Do a "Load Data All 1's" command. */
        /*   Do a "Bulk Erase Program Memory" command. */
        /*   Do a "Begin Programming" command. */
        /*   Wait erase_time to complete bulk erase. */
        this->set_program_mode();
        this->write_command(COMMAND_LOAD_CONFIG);
        this->io->shift_bits_out(0x7ffe, 16, 1);
        this->io->usleep(1);
        this->write_command(COMMAND_ERASE_PROG_MEM);
        this->write_command(COMMAND_BEGIN_PROG);
        this->io->usleep(this->erase_time);

        if (this->flags & PIC_FEATURE_EEPROM) {
            /* Bulk Erase Data Memory */
            /*   Do a "Load Data All 1's" command. */
            /*   Do a "Bulk Erase Data Memory" command. */
            /*   Do a "Begin Programming" command. */
            /*   Wait erase_time to complete bulk erase. */
            this->write_ee_data(0x00ff);
            this->write_command(COMMAND_ERASE_DATA_MEM);
            this->write_command(COMMAND_BEGIN_PROG);
            this->io->usleep(this->erase_time);
        }
        this->pic_off();
    } catch (std::exception& e) {
        this->pic_off();
        throw;
    }
}

void Pic16::disable_codeprotect(void)
{
    throw runtime_error (
        "Disabling code protecion on this device isn't supported."
    );
}

void Pic16::write_program_memory(DataBuffer& buf, long base)
{
unsigned int offset;

    try {
        for (offset=0; offset < this->codesize; offset++) {
            progress(base+offset);

            /* Skip but verify blank locations to save time */
            if (buf.isblank(base+offset)) {
                /* Don't verify the OSCAL location. */
                if (
                    !((this->flags & PIC_HAS_OSCAL) &&
                     (offset == this->codesize-1))
                ) {
                    if (
                        diff (
                            buf[base+offset],
                            this->read_prog_data(),
                            this->wordmask
                        )
                    ) {
                        break;
                    }
                }
            } else {
                if (!this->program_one_location((uint32_t)buf[base+offset])) {
                    break;
                }
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
        }
        if (offset < this->codesize) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write program memory at address 0x%04lx",
                base+offset
            )
        );
    }
}

void Pic16::read_program_memory(DataBuffer& buf, long base, bool verify)
{
unsigned int offset;

    try {
        for (offset=0; offset < this->codesize; offset++) {
            progress(base+offset);

            if (verify) {
                /* Don't verify the OSCAL location. */
                if (
                    !((this->flags & PIC_HAS_OSCAL) &&
                     (offset == this->codesize-1))
                ) {
                    if (
                        diff (
                            buf[base+offset],
                            this->read_prog_data(),
                            this->wordmask
                        )
                    ) {
                        throw runtime_error("");
                    }
                }
            } else {
                buf[base+offset] = this->read_prog_data();
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at address 0x%04lx",
                verify ? "Verification failed"
                       : "Couldn't read program memory",
                base+offset
            )
        );
    }
}

void Pic16::write_data_memory(DataBuffer& buf, long base)
{
unsigned int offset;

    try {
        for (offset=0; offset < this->eesize; offset++) {
            progress(base+offset);

            /* Skip but verify blank locations to save time */
            if (!diff(buf[base+offset],0xff,0xff)) {
                if (diff(buf[base+offset],this->read_ee_data(),0xff)) {
                    break;
                }
            } else {
                this->write_ee_data(buf[base+offset]);
                this->write_command(COMMAND_BEGIN_PROG);
                this->io->usleep(this->program_time);
                if (diff(buf[base+offset],this->read_ee_data(),0xff)) {
                    break;
                }
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
        }
        if (offset < this->eesize) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write data memory at address 0x%04lx",
                base+offset
            )
        );
    }
}

void Pic16::read_data_memory(DataBuffer& buf, long base, bool verify)
{
unsigned int offset;

    try {
        for (offset=0; offset < this->eesize; offset++) {
            progress(base+offset);

            if (verify) {
                if (diff(buf[base+offset],this->read_ee_data(),0xff)) {
                    throw runtime_error("");
                }
            } else {
                buf[base+offset] = this->read_ee_data();
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at address 0x%04lx",
                verify ? "Verification failed"
                       : "Couldn't read data memory",
                base+offset
            )
        );
    }
}

void Pic16::write_id_memory(DataBuffer& buf, long base)
{
unsigned int offset;

    try {
        for (offset=0; offset < 4; offset++) {
            progress(base+offset);

            /* Skip but verify blank locations to save time */
            if (buf.isblank(base+offset)) {
                if (
                    diff (
                        buf[base+offset],
                        this->read_prog_data(),
                        this->wordmask
                    )
                ) {
                    break;
                }
            } else {
                if (!this->program_one_location((uint32_t)buf[base+offset])) {
                    break;
                }
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
        }
        if (offset < 4) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write ID memory at address 0x%04lx",
                base+offset
            )
        );
    }
}

void Pic16::read_id_memory(DataBuffer& buf, long base, bool verify)
{
unsigned int offset;

    try {
        for (offset=0; offset < 4; offset++) {
            progress(base+offset);

            if (verify) {
                if (
                    diff (
                        buf[base+offset],
                        this->read_prog_data(),
                        this->wordmask
                    )
                ) {
                    throw runtime_error("");
                }
            } else {
                buf[base+offset] = this->read_prog_data();
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at address 0x%04lx",
                verify ? "Verification failed"
                       : "Couldn't read ID memory",
                base+offset
            )
        );
    }
}

void Pic16::write_config_word(uint32_t data)
{
int count;

    switch(this->memtype) {
        case MEMTYPE_EPROM:
            count = 100;
        break;
        case MEMTYPE_FLASH:
            count = 1;
        break;
        default:
            throw logic_error("I'm confused");
    }
    /* Now do it! */
    bool ok = false;
    while (count > 0) {
        ok=this->program_cycle(data, this->config_mask[0]);
        count--;
    }
    if (!ok) {
        throw runtime_error("Couldn't write the configuration word");
    }
}

uint32_t Pic16::read_config_word(bool verify, uint32_t verify_data)
{
uint32_t data;

    try {
        data = read_prog_data();
        if (verify) {
            /* We don't include persistent config bits in a verify */
            uint32_t mask = 
            			this->config_mask[0] & ~this->persistent_config_mask[0];
            if ((verify_data & mask) != (data & mask)) {
                throw runtime_error("");
            }
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't %s config word.",
                verify ? "verify" : "read"
            )
        );
    }
    return data;
}

bool Pic16::program_one_location(uint32_t data)
{
unsigned int count;

    /* Program up to the initial count */
    for (count=1; count <= this->program_count; count++) {
        if (this->program_cycle(data, this->wordmask)) {
            break;
        }
    }
    if (count > this->program_count) {
        return false;
    }
    /* Program count*multiplier more times */
    count *= this->program_multiplier;
    while (count > 0) {
        if (!this->program_cycle(data, this->wordmask)) {
            return false;
        }
        count--;
    }
    return true;
}

void Pic16::write_ee_data(uint32_t data)
{
    data = (data & this->wordmask) << 1;
    this->write_command(COMMAND_LOAD_DATA_DATA);
    this->io->shift_bits_out(data, 16, 1);
    this->io->usleep(1);
}

uint32_t Pic16::read_ee_data(void)
{
uint32_t data;

    this->write_command(COMMAND_READ_DATA_DATA);
    data = this->io->shift_bits_in(16, 0);
    this->io->usleep(1);

    return (data >> 1) & 0xff;
}

unsigned int Pic16::get_clearvalue(size_t addr)
{
    if (addr == 0x2007) {        // config word
        // TODO: check if we need to use config_mask instead of wordmask
        return this->wordmask & ~this->persistent_config_mask[0];
    } else if (addr == 0x2000) { // ID memory
        return this->wordmask;
    } else if (addr < 0x2100) {  // program memory
        return this->wordmask;
    }
    return 0x00ff; // data memory clear value
}

uint32_t Pic16::read_deviceid(void)
{
uint32_t devid;

    /* Enter config memory space. The device ID is at address 0x2006 */
    this->write_command(COMMAND_LOAD_CONFIG);
    this->io->shift_bits_out(0x7ffe, 16, 1);    /* Dummy write of all 1's */
    this->io->usleep(1);

    /* Increment up to 0x2006 */
    for(int i=0; i < 6; i++) {
        this->write_command(COMMAND_INC_ADDRESS);
    }
    return read_prog_data();
}

