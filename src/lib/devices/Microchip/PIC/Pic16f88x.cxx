/* Copyright (C) 2002  Mark Andrew Aikens <marka@desert.cx>
 * Copyright (C) 2010  Edward Hildum      <ehildum@comcast.net>
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
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "Microchip.h"
#include "Util.h"

Pic16f88x::Pic16f88x(char *name) : Pic16(name)
{
    this->program_time += 100;
    this->flags |= PIC_HAS_OSCAL;
    
    // Protect against bad entry in devices.prefs.  More than two config words
    //	would clobber the OSC CAL word during writes.
    if (this->config_words > 2) this->config_words = 2;
    
    // Check and constrain value of write_buffer_size
    if ( (write_buffer_size != 4) && (write_buffer_size != 8) )
    	write_buffer_size = 4;
}

Pic16f88x::~Pic16f88x()
{
}

/** Override erase method:  On these Pics, the configuration memory can be
 * accessed by issuing a LOAD_CONFIGURATION command.
*/
void	Pic16f88x::erase(void)
{
    if (this->memtype != MEMTYPE_FLASH) {
        throw runtime_error("Operation not supported by device");
    }
    /* Read the config word and OSCAL */
    unsigned long cword[2]; 
    unsigned long oscal = 0;
    try {
    	unsigned int	c_addr = 0x2000;
    	
        this->set_program_mode();

        /* Read the current configuration word to determine if code
         * protection needs to be disabled. */
        this->write_command(COMMAND_LOAD_CONFIG);
        this->io->shift_bits_out(0x7ffe, 16, 1);   /* Dummy write of all 1's */
        this->io->usleep(1);
        /* Pic address is 0x2000

        /* Skip to the configuration word(s) */
        for (int i=0; i < 7; i++) {
            this->write_command(COMMAND_INC_ADDRESS);
            c_addr++;
        }
        /*  Read config word(s)	*/
        for (int i = 0; i < this->config_words; i++) {
        	cword[i] = read_config_word();
            this->write_command(COMMAND_INC_ADDRESS);
        	c_addr++;
        }

        /* If we need to save the oscillator calibration value, increment
         * to the OSCAL address and read it. */
        if (this->flags & PIC_HAS_OSCAL) {
            while ( c_addr < 0x2009 ) {
                this->write_command(COMMAND_INC_ADDRESS);
                c_addr++;
            }
            oscal = this->read_prog_data();
        }

        this->pic_off();
    } catch (std::exception& e) {
        this->pic_off();
        throw;
    }
    /* Wait a bit after exiting program mode */
    this->io->usleep(10000);

	/* Code protection prevents code memory from being read.  Code memory gets 
	 * erased by the Bulk Erase Program Memory command regardless of the CP bit
	 * setting in config word 0.  The code protect data (CPD) config word bit 
	 * prevents eeprom data from being read out.  If it is clear (data eeprom is
	 * protected), it gets erased as well by a Bulk Erase Program Memory 
	 * command.  If it is set (data eeprom not protected) a separate Bulk Erase
	 * Data Memory command is required to erase data eeprom memory AFTER the
	 * CPD bit is set by erasing the config word.	*/
    try {
        this->set_program_mode();

		// Erase program memory unconditionally, including user ID words
		this->write_command(COMMAND_LOAD_CONFIG);
		this->io->shift_bits_out(0x7ffe, 16, 1);
		this->io->usleep(1);
		this->write_command(COMMAND_ERASE_PROG_MEM);
		this->io->usleep(this->erase_time);

		// Check whether we need to separately erase data memeory
	    if ( (cword[0] & this->cpd_mask) == this->cpd_off ) 
	    {
	    	this->write_command(COMMAND_ERASE_DATA_MEM);
			this->io->usleep(this->erase_time);
	    }

        this->pic_off();
	} catch (std::exception& e) {
		this->pic_off();
		throw;
	}

	/* Check if we need to restore some state. */
	if ((this->persistent_config_mask != 0) || (this->flags & PIC_HAS_OSCAL)) {
		/* Wait a bit after exiting program mode */
		this->io->usleep(10000);

		try {
	    	unsigned int	c_addr = 0x2000;
	    	
			this->set_program_mode();

			/* Read the OSCAL value and compare it to the saved value	*/
	        this->write_command(COMMAND_LOAD_CONFIG);
    	    this->io->shift_bits_out(0x7ffe, 16, 1);   /* Dummy write of all 1's */
        	this->io->usleep(1);
	        /* Pic address is 0x2000

    	    /* Skip to the calibration word */
        	while (c_addr < 0x2009) {
            	this->write_command(COMMAND_INC_ADDRESS);
            	c_addr++;
        	}
			
			if ( oscal != this->read_prog_data() ) {
				/* Try to restore the OSCAL value	*/
			
// For now, if oscal was clobbered, throw a runtime error.  Later we can try
// erasing and reprogramming oscal.


                throw runtime_error (
                    (const char *)Preferences::Name (
                        "Couldn't restore calibration word."
                        " Value = 0x%04lx", oscal
                    )
                );
			
//				/* Restore the OSCAL value */
//				if (this->flags & PIC_HAS_OSCAL) {
//					/* Increment to the OSCAL address */
//					for (unsigned int i=0; i<this->codesize-1; i++) {
//						this->write_command(COMMAND_INC_ADDRESS);
//					}
//					if (!this->program_one_location(oscal)) {
//						throw runtime_error (
//							(const char *)Preferences::Name (
//								"Couldn't restore OSCAL value of 0x%04lx",
//								oscal
//							)
//						);
// 					}
//				}
			
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
                        (cword[0] & this->persistent_config_mask[0]) |
                        (this->wordmask & ~this->persistent_config_mask[0])
                    );
                } catch(std::exception& e) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Couldn't restore persistent configuration bits."
                            " Values 0x%04lx",
                            cword[0] & this->persistent_config_mask[0]
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

/* Use 4- or 8-word algorithm to write all of program memory.  Do a verify 
 * afterwards, taking advantage of the fact that the address will alias to 
 * PC = 0 after programming highest address or wrap to PC = 0 if 
 * codesize = 0x1FFF).  Verify of the write has to be done after finishing all
 * of program memory, since the PC can't be decremented.
 */
void Pic16f88x::write_program_memory(DataBuffer& buf, long base)
{
	unsigned int offset;
	
	// Do this because we can't verify as we write with multi-word programming.
	this->progress_total += this->codesize;

    try {
    	// Write all of program memory, 4 or 8 bytes at a time
		for (offset = 0; offset < this->codesize; 
										offset += this->write_buffer_size) {
			program_mult_prog_loc(buf, offset);
			this->progress_count += this->write_buffer_size;
			progress(base+offset);
		}
    	
    	// Now, go back and verify the write.  The PC will be 0 or aliased to 0.
        for (offset=0; offset < this->codesize; offset++) {
        	if (diff(buf[base+offset], this->read_prog_data(), this->wordmask ))
        		throw runtime_error("");
        	
            progress(base+offset);
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
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

/* Write 4 or 8 program memory locations using the appropriate algorithm.  If
 * all the locations in the buffer are blank, skip the programming command.	*/
void Pic16f88x::program_mult_prog_loc(DataBuffer& buf, long base)
{
	unsigned int	offset;
	bool			do_prog = false;
	
	// Write 3 or 7 bytes to the write buffers
	for (offset = 0; offset < this->write_buffer_size - 1; offset++) {
		this->write_prog_data(buf[base + offset]);	
		this->write_command(COMMAND_INC_ADDRESS);
		if ( !buf.isblank(base+offset) ) do_prog = true;
	}
	// Write 4th or 8th byte, then issue program command
	this->write_prog_data(buf[base + offset]);
	if ( !buf.isblank(base+offset) ) do_prog = true;
	if (do_prog) {
	    if (this->flags & PIC_REQUIRE_EPROG) {
			this->write_command(COMMAND_BEGIN_PROG_EXT);
	    } else {
			this->write_command(COMMAND_BEGIN_PROG);
	    }
		this->io->usleep(this->program_time);
	    if (this->flags & PIC_REQUIRE_EPROG) {
        	this->write_command(COMMAND_END_PROG);
        	this->io->usleep(100);					// discharge time
    	}
	}
    // Increment to next PC location before exiting
    this->write_command(COMMAND_INC_ADDRESS);
}

// Different from Pic16 method: needs end-programming command and discharge time 
void Pic16f88x::write_data_memory(DataBuffer& buf, long base)
{
unsigned int offset;

    try {
        for (offset=0; offset < this->eesize; offset++) {
        	unsigned int tmp;

           	tmp = buf[base+offset];

            /* Skip but verify blank locations to save time */
            if (!diff(buf[base+offset],0xff,0xff)) {
                if (diff(tmp,this->read_ee_data(),0xff)) {
                    break;
                }
            } else {
                this->write_ee_data(buf[base+offset]);
			    if (this->flags & PIC_REQUIRE_EPROG) {
					this->write_command(COMMAND_BEGIN_PROG_EXT);
	    		} else {
					this->write_command(COMMAND_BEGIN_PROG);
	    		}
                this->io->usleep(this->program_time);
			    if (this->flags & PIC_REQUIRE_EPROG) {
        			this->write_command(COMMAND_END_PROG);
                	this->io->usleep(100);	// Discharge time
			    }
                tmp = this->read_ee_data();
                if (diff(buf[base+offset],tmp,0xff))
                    throw runtime_error("");
            }
            this->write_command(COMMAND_INC_ADDRESS);
            this->progress_count++;
            progress(base+offset);
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

bool Pic16f88x::program_cycle(uint32_t data, uint32_t mask)
{
    this->write_prog_data(data);
    if (this->flags & PIC_REQUIRE_EPROG) {
		this->write_command(COMMAND_BEGIN_PROG_EXT);
	} else {
		this->write_command(COMMAND_BEGIN_PROG);
	}
    this->io->usleep(this->program_time);
    if (this->flags & PIC_REQUIRE_EPROG) {
        this->write_command(COMMAND_END_PROG);
        this->io->usleep(100);
    }
    if ((read_prog_data() & mask) == (data & mask)) {
        return true;
    }
    return false;
}
