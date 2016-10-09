/* Copyright (C) 2003  Francesco Bradascio <fbradasc@yahoo.it>
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
 */
#ifndef __Atmel_h
#define __Atmel_h

#if defined(WIN32) && !defined(__MINGW32__)
typedef unsigned long  uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;
#else
#  include <stdint.h>
#endif
#include "DataBuffer.h"
#include "Device.h"
#include "IO.h"
#include "proto_flP5.h"
// #include "flP5.h"

/** \file */

/* See avr_disasm.c for more information on these variables. */
extern "C" int AVR_Long_Instruction;
extern "C" uint32_t AVR_Long_Address;

/** A Device implementation which implements a base class for Atmel's
 * devices. */
class Atmel: public Device
{
public:
    /** Creates an instance of a Atmel device from its name. This function
     * is called from Device::load and selects the appropriate AtmelDevice
     * subclass to instantiate. This allows for efficient implementation of
     * different Atmel devices programming algorithms.
     *
     * \param name The name of the device (case sensitive).
     * \retval NULL if the device is unknown.
     * \retval Device An instance of a subclass of Device representing the
     *         device given by the name parameter.
     */
    static Device *load(char *vendor, char *name);

    /** Constructor */
    Atmel(char *vendor, char *spec, char *device);

    /** Destructor */
    ~Atmel();
};

#define VCC_SIZE    (2)
#define VCC_NDX(i)  ((i)-PAR_VCC_MIN)

#define WTD_SIZE    (PAR_WAIT_DELAYS_END-PAR_WAIT_DELAYS_START)
#define WTD_NDX(i)  ((i)-PAR_WAIT_DELAYS_START)

#define BND_SIZE    (PAR_SIZE_BOUNDARIES_END-PAR_SIZE_BOUNDARIES_START)
#define BND_NDX(i)  ((i)-PAR_SIZE_BOUNDARIES_START)

#define SIG_SIZE    (PAR_SIGN_B2-PAR_SIGN_B0)
#define SIG_NDX(i)  ((i)-PAR_SIGN_B0)

#define CAL_SIZE    (1)
#define CAL_NDX(i)  ((i)-PAR_CALIBRATION_BYTES)

#define FRB_SIZE    (PAR_FLASH_READ_BACK_P2-PAR_FLASH_READ_BACK_P1)
#define FRB_NDX(i)  ((i)-PAR_FLASH_READ_BACK_P1)

#define ERB_SIZE    (PAR_EEPROM_READ_BACK_P2-PAR_EEPROM_READ_BACK_P1)
#define ERB_NDX(i)  ((i)-PAR_EEPROM_READ_BACK_P1)

#define INS_SIZE    (PAR_OPTIONAL_INSTRUCTIONS_END-PAR_MANDATORY_INSTRUCTIONS_START)
#define INS_NDX(i)  ((i)-PAR_MANDATORY_INSTRUCTIONS_START)

typedef union {
    uint32_t dword;
    uint8_t  bytes[4];
} InstResponse;

class AvrUI;

/** A Device implementation which implements a base class for Atmel's AVR
 * microcontrollers. These microcontrollers are programmed serially and have
 * a word size of 12, 14, or 16 bits. They come in memory configurations of
 * flash or EPROM with some devices having an optional EEPROM dedicated to
 * data storage. */
class Avr : public Atmel
{
public:
    class Instruction
    {
    public:
        Instruction();

        bool parse(const char *string);
        uint32_t execute();
        uint32_t execute(                                  uint32_t address);
        uint32_t execute(uint32_t input, uint32_t &output, uint32_t address=0);
        bool isValid() { return valid; }

    private:
        uint8_t setBits(uint32_t &inst, uint32_t mask, uint32_t data, uint8_t offset=0);
        uint8_t getBits(uint32_t &data, uint32_t mask, uint32_t risp);

        uint32_t bits_mask;
        uint32_t msba_mask;
        uint32_t lsba_mask;
        uint32_t hldb_mask;
        uint32_t dout_mask;
        uint32_t dinp_mask;

        bool     valid;
    };

    enum CONSTANTS {
        BITS_IN_FUSES     = 8,
        BIT_NAME_MAX_SIZE = 10
    };

    enum BIT_NAMES {
        FUSE_LOW_BITS  = 0,
        FUSE_HIGH_BITS    ,
        EXT_FUSE_BITS     ,
        LOCK_BITS         ,
        LAST_BIT_NAME
    };

    enum DEV_PARAM {
        PAR_VOLTAGES_START = 0,

            PAR_VCC_MIN = PAR_VOLTAGES_START,
            PAR_VCC_MAX,

        PAR_VOLTAGES_END,
        PAR_WAIT_DELAYS_START = PAR_VOLTAGES_END,

            PAR_WD_RESET = PAR_WAIT_DELAYS_START,
            PAR_WD_ERASE,
            PAR_WD_FLASH,
            PAR_WD_EEPROM,
            PAR_WD_FUSE,

        PAR_WAIT_DELAYS_END,
        PAR_SIZE_BOUNDARIES_START = PAR_WAIT_DELAYS_END,

            PAR_FLASH_SIZE = PAR_SIZE_BOUNDARIES_START,
            PAR_FLASH_PAGE_SIZE,
            PAR_FLASH_PAGES,

            PAR_EEPROM_SIZE,
            PAR_EEPROM_PAGE_SIZE,
            PAR_EEPROM_PAGES,

        PAR_SIZE_BOUNDARIES_END,
        PAR_MISC_START = PAR_SIZE_BOUNDARIES_END,

            PAR_SIGN_B0 = PAR_MISC_START,
            PAR_SIGN_B1,
            PAR_SIGN_B2,

            PAR_FLASH_READ_BACK_P1,
            PAR_FLASH_READ_BACK_P2,

            PAR_EEPROM_READ_BACK_P1,
            PAR_EEPROM_READ_BACK_P2,

            PAR_CALIBRATION_BYTES,

        PAR_MISC_END,
        PAR_MANDATORY_INSTRUCTIONS_START = PAR_MISC_END,

            PAR_READ_SIGNATURE_INST = PAR_MANDATORY_INSTRUCTIONS_START,

            PAR_PROGRAMMING_ENABLE_INST,
            PAR_CHIP_ERASE_INST,

            PAR_READ_FLASH_INST,
            PAR_WRITE_FLASH_INST,

            PAR_READ_EEPROM_INST,
            PAR_WRITE_EEPROM_INST,

            PAR_READ_FUSE_INST,
            PAR_WRITE_FUSE_INST,

            PAR_READ_LOCK_INST,
            PAR_WRITE_LOCK_INST,

        PAR_MANDATORY_INSTRUCTIONS_END,
        PAR_OPTIONAL_INSTRUCTIONS_START = PAR_MANDATORY_INSTRUCTIONS_END,

            PAR_POLL_INST = PAR_OPTIONAL_INSTRUCTIONS_START,

            PAR_LOAD_EXT_ADDR_INST,

            PAR_LOAD_FLASH_PAGE_INST,
            PAR_WRITE_FLASH_PAGE_INST,

            PAR_LOAD_EEPROM_PAGE_INST,
            PAR_WRITE_EEPROM_PAGE_INST,

            PAR_READ_FUSE_HIGH_INST,
            PAR_WRITE_FUSE_HIGH_INST,

            PAR_READ_EXT_FUSE_INST,
            PAR_WRITE_EXT_FUSE_INST,

            PAR_READ_CALIBRATION_INST,

        PAR_OPTIONAL_INSTRUCTIONS_END,
        LAST_PARAM = PAR_OPTIONAL_INSTRUCTIONS_END
    };

    /** Creates an instance of a AVR device from its name. This function is
     * called from Device::load and selects the appropriate AVR subclass
     * to instantiate. This allows for efficient implementation of different
     * AVR programming algorithms.
     *
     * \param name The name of the device (case sensitive).
     * \retval NULL if the device is unknown.
     * \retval Device An instance of a subclass of Device representing the
     *         device given by the name parameter.
     */
    static Device *load(char *vendor, char *spec, char *device);

    virtual bool check(void);
    virtual void erase(void);
    virtual void program(DataBuffer& buf);
    virtual void dump(DataBuffer& buf);
    virtual void read(DataBuffer& buf, bool verify=false);

    /** Constructor */
    Avr(char *vendor, char *spec, char *device);

    /** Destructor */
    ~Avr();

    static bool verifyConfig  (Preferences &device, bool raise_exceptions=false);
    static bool validateString(Preferences &device, int param, const char *range, unsigned int size, bool can_be_empty=false);

    static const char            *validInstructionChars;
    static const char            *validHexDigit;
    static const char            *validBitNameChars;
    static const t_NamedSettings  sparams[Avr::LAST_PARAM];
    static const t_NamedSettings  sbitNames[Avr::LAST_BIT_NAME*Avr::BITS_IN_FUSES];

protected:
    friend class AvrUI;

    int mem2asm(int org, DataBuffer& buf, char *buffer, size_t sizeof_buffer);

    virtual void write_flash_memory (DataBuffer& buf);
    virtual void write_eeprom_memory(DataBuffer& buf);
    virtual void write_config_bits  (DataBuffer& buf);

    virtual void polling_flash_memory (uint32_t value, uint32_t address);
    virtual void polling_eeprom_memory(uint32_t value, uint32_t address);

    virtual void read_flash_memory  (DataBuffer& buf, bool verify);
    virtual void read_eeprom_memory (DataBuffer& buf, bool verify);
    virtual void read_config_bits   (DataBuffer& buf, bool verify);
    virtual void read_calibration   (DataBuffer& buf, bool verify);

    /** Put the AVR device in program/verify mode. On entry to
     * program/verify mode, the program counter is set to 0 or -1 depending
     * on the device.
     */
    virtual void set_program_mode(void);

    /** Turn off the AVR device. This will set the clock and data lines to
     * low and shut off both Vpp and Vcc.
     */
    virtual void off(void);

    /** Read the signature from the chip
     * \pre The device has just been put into program mode.
     * \post The device is in an undetermined state. Programming may not
     *       be possible unless program mode is exited and re-entered.
     * \returns The chip signature
     */
    virtual void read_signature(char signature[3]);

    /** Check the signature
     * \pre The device has just been put into program mode.
     * \post The device is in an undetermined state. Programming may not
     *       be possible unless program mode is exited and re-entered.
     * \returns None
     * \returns 
     * \throws  runtime_error if the chip signature does not matches
     *                        this device signature
     */
    void check_signature(void); // throw runtime_error;

    void check_instruction(int inst); // throw runtime_error;

    /** Prepare the default value of the configuration registers.
     */
    virtual void set_config_default(DataBuffer& buf);

    virtual void set_config_word(int i, uint32_t word, DataBuffer& buf);

/* Protected data: */
    int              powerOffAftwerWriteFuse;
    float            vcc               [VCC_SIZE];
    int              wait_delays       [WTD_SIZE];
    int              boundary_sizes    [BND_SIZE];
    int              signature         [SIG_SIZE];
    int              flash_readBack    [FRB_SIZE];
    int              eeprom_readBack   [ERB_SIZE];
    int              calibration_bytes [CAL_SIZE];
    Avr::Instruction instructions      [INS_SIZE];
    char             bitNames          [Avr::LAST_BIT_NAME][Avr::BITS_IN_FUSES][Avr::BIT_NAME_MAX_SIZE];

    IntPair          signature_extent;
    IntPair          config_extent[Avr::LAST_BIT_NAME];
    IntPair          calib_extent;
};

#endif
