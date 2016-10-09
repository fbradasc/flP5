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
#ifndef __Microchip_h
#define __Microchip_h

#ifdef WIN32
typedef unsigned long uint32_t;
#else
#  include <stdint.h>
#endif
#include "DataBuffer.h"
#include "Device.h"
#include "IO.h"

/** \file */

typedef pair<int, int> IntPair;         /**< A pair of integers */

#define PIC_FEATURE_EEPROM 0x00000001   /**< PIC has a data EEPROM */
#define PIC_REQUIRE_EPROG  0x00000002   /**< PIC requires END_PROG command */
#define PIC_FEATURE_BKBUG  0x00000004   /**< PIC has on-chip debugger */
#define PIC_HAS_OSCAL      0x00000008   /**< PIC has an OSCAL value that must
                                             be saved and restored on a chip
                                             erase */
#define PIC_HAS_DEVICEID   0x00000010   /**< PIC has a device ID that can be
                                             read by the programming software */


/** A Device implementation which implements a base class for Microchip's
 * devices. */
class Microchip: public Device
{
public:
    /** Creates an instance of a Microchip device from its name. This function
     * is called from Device::load and selects the appropriate MicrochipDevice
     * subclass to instantiate. This allows for efficient implementation of
     * different Microchip devices programming algorithms.
     *
     * \param name The name of the device (case sensitive).
     * \retval NULL if the device is unknown.
     * \retval Device An instance of a subclass of Device representing the
     *         device given by the name parameter.
     */
    static Device *load(char *name);

    /** Constructor */
    Microchip(char *name);

    /** Destructor */
    ~Microchip();
};

enum Instruction_Class {
      INSN_CLASS_NULL = 0 ,
      INSN_CLASS_LIT1     , // bit 0 contains a 1 bit literal
      INSN_CLASS_LIT4S    , // Bits 7:4 contain a 4 bit literal, bits 3:0 are unused
      INSN_CLASS_LIT6     , // bits 5:0 contain an 6 bit literal
      INSN_CLASS_LIT8     , // bits 7:0 contain an 8 bit literal
      INSN_CLASS_LIT8C12  , // bits 7:0 contain an 8 bit literal, 12 bit CALL
      INSN_CLASS_LIT8C16  , // bits 7:0 contain an 8 bit literal, 16 bit CALL
      INSN_CLASS_LIT9     , // bits 8:0 contain a 9 bit literal
      INSN_CLASS_LIT11    , // bits 10:0 contain an 11 bit literal
      INSN_CLASS_LIT13    , // bits 12:0 contain an 11 bit literal
      INSN_CLASS_LITFSR   , // bits 5:0 contain an 6 bit literal for fsr 7:6
      INSN_CLASS_IMPLICIT , // instruction has no variable bits at all
      INSN_CLASS_OPF5     , // bits 4:0 contain a register address
      INSN_CLASS_OPWF5    , // as above, but bit 5 has a destination flag
      INSN_CLASS_B5       , // as for OPF5, but bits 7:5 have bit number
      INSN_CLASS_OPF7     , // bits 6:0 contain a register address
      INSN_CLASS_OPWF7    , // as above, but bit 7 has destination flag
      INSN_CLASS_B7       , // as for OPF7, but bits 9:7 have bit number
      INSN_CLASS_OPF8     , // bits 7:0 contain a register address
      INSN_CLASS_OPFA8    , // bits 7:0 contain a register address & bit has access flag
      INSN_CLASS_OPWF8    , // as above, but bit 8 has dest flag
      INSN_CLASS_OPWFA8   , // as above, but bit 9 has dest flag & bit 8 has access flag
      INSN_CLASS_B8       , // like OPF7, but bits 9:11 have bit number
      INSN_CLASS_BA8      , // like OPF7, but bits 9:11 have bit number & bit 8 has access flag
      INSN_CLASS_LIT20    , // 20bit lit, bits 7:0 in first word bits 19:8 in second
      INSN_CLASS_CALL20   , // Like LIT20, but bit 8 has fast push flag
      INSN_CLASS_RBRA8    , // Bits 7:0 contain a relative branch address
      INSN_CLASS_RBRA11   , // Bits 10:0 contain a relative branch address
      INSN_CLASS_FLIT12   , // LFSR, 12bit lit loaded into 1 of 4 FSRs
      INSN_CLASS_FF       , // two 12bit file addresses
      INSN_CLASS_FP       , // Bits 7:0 contain a register address, bits 12:8 contains a peripheral address
      INSN_CLASS_PF       , // Bits 7:0 contain a register address, bits 12:8 contains a peripheral address
      INSN_CLASS_SF       , // 7 bit offset added to FSR2, fetched memory placed at 12 bit address
      INSN_CLASS_SS       , // two 7 bit offsets, memory moved using FSR2
      INSN_CLASS_TBL      , // a table read or write instruction
      INSN_CLASS_TBL2     , // a table read or write instruction. 
                            // Bits 7:0 contains a register address; Bit 8 is unused;
                            // Bit 9, table byte select. (0:lower ; 1:upper)
      INSN_CLASS_TBL3     , // a table read or write instruction. 
                            // Bits 7:0 contains a register address; 
                            // Bit 8, 1 if increment pointer, 0 otherwise;
                            // Bit 9, table byte select. (0:lower ; 1:upper)
      INSN_CLASS_FUNC     , // instruction is an assembler function
      INSN_CLASS_LIT3_BANK, // SX: bits 3:0 contain a 3 bit literal, shifted 5 bits
      INSN_CLASS_LIT3_PAGE, // SX: bits 3:0 contain a 3 bit literal, shifted 9 bits
      INSN_CLASS_LIT4       // SX: bits 3:0 contain a 4 bit literal
};

class Instruction
{
public:
    const char *name;
    const long int mask;
    const long int opcode;
    const enum Instruction_Class type;
};

/** A Device implementation which implements a base class for Microchip's PIC
 * microcontrollers. These microcontrollers are programmed serially and have
 * a word size of 12, 14, or 16 bits. They come in memory configurations of
 * flash or EPROM with some devices having an optional EEPROM dedicated to
 * data storage. */
class Pic : public Microchip
{
public:
    /* PIC commands */
    // const static int
    enum Commands_List {
        COMMAND_LOAD_CONFIG    = 0x00, /**< Load Configuration            */
        COMMAND_LOAD_PROG_DATA = 0x02, /**< Load Data for Program Memory  */
        COMMAND_READ_PROG_DATA = 0x04, /**< Read Data from Program Memory */
        COMMAND_INC_ADDRESS    = 0x06, /**< Increment Address             */
        COMMAND_BEGIN_PROG     = 0x08, /**< Begin Erase/Programming       */
        COMMAND_END_PROG       = 0x0e, /**< End Programming               */
        COMMAND_LOAD_DATA_DATA = 0x03, /**< Load Data for Data Memory     */
        COMMAND_READ_DATA_DATA = 0x05, /**< Read Data from Data Memory    */
        COMMAND_ERASE_PROG_MEM = 0x09, /**< Bulk Erase Program Memory     */
        COMMAND_ERASE_DATA_MEM = 0x0b  /**< Bulk Erase Data Memory        */
    };

    /** Values for the different types of PIC memory and their algorithms */
    typedef enum {
        MEMTYPE_EPROM=0,
        MEMTYPE_FLASH,
    } pic_memtype_t;

    /** Creates an instance of a PIC device from its name. This function is
     * called from Device::load and selects the appropriate Pic16 subclass
     * to instantiate. This allows for efficient implementation of different
     * PIC programming algorithms.
     *
     * \param name The name of the device (case sensitive).
     * \retval NULL if the device is unknown.
     * \retval Device An instance of a subclass of Device representing the
     *         device given by the name parameter.
     */
    static Device *load(char *name);

    /** Dumps/disassemblates the contents of the DataBuffer.
     * \param buf The DataBuffer containing the data to dump/disassemblate.
     * \throws runtime_error Contains a textual description of the error.
     */
    virtual void dump(DataBuffer& buf);

    /** Constructor */
    Pic(char *name);

    /** Destructor */
    ~Pic();

    virtual bool check(void);

protected:
    int mem2asm(int org, DataBuffer& buf, char *buffer, size_t sizeof_buffer);

    /** Perform a single program cycle for program memory. The following steps
     * are performed:
     *   - The data is written to the PIC with write_prog_data().
     *   - The BEGIN_PROG command is sent.
     *   - A delay of program_time is initiated.
     *   - If the PIC requires it, the END_PROG command is sent.
     *   - A readback is performed and is compared with the original data.
     * \param data The data word to program into program memory at the PIC's
     *        program counter.
     * \param mask A mask which is used when verifying the data. The read back
     *             data and the data parameter are both masked with this mask
     *             before being compared. This is required for properly
     *             verifying the configuration word.
     * \returns A boolean value indicating if the data read back matches the
     *          data parameter.
     */
    virtual bool program_cycle(uint32_t data, uint32_t mask=0xffffffff);

    /** Put the PIC device in program/verify mode. On entry to
     * program/verify mode, the program counter is set to 0 or -1 depending
     * on the device.
     */
    virtual void set_program_mode(void);

    /** Turn off the PIC device. This will set the clock and data lines to
     * low and shut off both Vpp and Vcc.
     */
    virtual void pic_off(void);

    /** Write a 6 bit command to the PIC. After the write, a 1us delay is
     * performed as required by the device.
     * \param command The 6 bit command to write.
     */
    virtual void write_command(uint32_t command);

    /** Send the LOAD_PROG_DATA command and write the data to the PIC at
     * the address specified by the program counter. This method doesn't
     * actually program the data permanently into the PIC. It's part of the
     * procedure though.
     * \param data The program data to send to the PIC.
     */
    virtual void write_prog_data(uint32_t data);

    /** Send the READ_PROG_DATA command and read in the program data at the
     * address specified by program counter.
     * \returns The program data at the current PIC address.
     */
    virtual uint32_t read_prog_data(void);

    /** Read the device ID from the device. 
     * \pre The device has just been put into program mode.
     * \post The device is in an undetermined state. Programming may not
     *       be possible unless program mode is exited and re-entered.
     * \returns The value of the device ID word.
     */
    virtual uint32_t read_deviceid(void);

    /** Prepare the default value of the configuration registers.
     */
    virtual void set_config_default(DataBuffer& buf);

/* Protected data: */
    /** The calculated bitmask for clearing upper bits of a word. */
    uint32_t wordmask;

    /** Flags for this PIC device. */
    uint32_t flags;

    /** The memory type of this PIC. */
    pic_memtype_t memtype;

    /** Number of words of code this PIC has. */
    unsigned int codesize;
    
    /** Number of configuration words for this PIC. */
    unsigned int config_words;

    /** Number of bytes of data EEPROM this PIC has. Only valid if
     * \c PIC_FEATURE_EEPROM is set. */
    unsigned int eesize;

    /** Maximum number of times to attempt to program a memory location before
     * reporting an error. */
    unsigned int program_count;

    /** Overprogramming multiplier. After a memory location has been programmed
     * and successfully verified, program it (count * program_multiplier) more
     * times. */
    unsigned int program_multiplier;

    /** The number of microseconds it takes to program one memory location.
     * This value is defined in the PIC datasheet. */
    unsigned int program_time;

    /** The amount of time (in microseconds) it takes to erase an electrically
     * erasable device. */
    unsigned int erase_time;

    /** Expected device ID value and mask for which bits to care about.
     * These are only valid if the PIC_HAS_DEVICEID flag is set. */
    unsigned int deviceid;
    unsigned int deviceidmask;

    unsigned int write_buffer_size;
    unsigned int erase_buffer_size;

    const Instruction *popcodes;
};


/** An implementation of the Device class which supports most 14-bit devices
 * with the part number prefix 16. */
class Pic16 : public Pic
{
public:
    /** Create a new instance and read in in the configuration for the PIC
     * device. This constructor is called from Device::load() when the device
     * name begins with the string "PIC". This function will open the PIC
     * device configuration file "pic.conf" and read the configuration for
     * the device specified.
     * \param name The name of the PIC device.
     * \throws runtime_error Contains a description of the error.
     */
    Pic16(char *name);

    /** Destructor */
    ~Pic16();

    virtual void erase(void);
    virtual void program(DataBuffer& buf);
    virtual void read(DataBuffer& buf, bool verify=false);

    /** Gets the native clearvalue depending on the memory address
     * \returns The clearvalue.
     */
    virtual unsigned int get_clearvalue(size_t addr);


protected:
    /** Bulk erase the program and data memory of a flash device.
     * \pre The device has flash memory.
     * \pre Code protection on the device is verified to be completely off.
     * \pre The device is turned completely off.
     * \post The device is turned completely off.
     * \throws runtime_error Contains a description of the error.
     */
    virtual void bulk_erase(void);

    /** Disables code protect on a flash PIC and clears the program and data
     * memory.
     * \pre The device has flash memory.
     * \pre The device is turned completely off.
     * \post The device is turned completely off.
     * \throws runtime_error Contains a description of the error.
     */
    virtual void disable_codeprotect(void);

    /** Program the entire contents of program memory to the PIC device.
     * \param buf A DataBuffer containing the data to program.
     * \param base The offset within the data buffer to start retrieving data.
     *        If this parameter isn't specified, it defaults to 0.
     * \pre The PIC should have it's program counter set to the beginning of
     *      program memory.
     * \post The program counter is pointing to the address immediately after
     *       the last program memory address.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_program_memory(DataBuffer& buf, long base=0);

    /** Read the entire contents of program memory from the PIC device.
     * \param buf A DataBuffer in which to store the read data.
     * \param base The offset within the data buffer to start storing data.
     *        If this parameter isn't specified, it defaults to 0.
     * \param verify If this flag is true, don't store the data in the
     *        DataBuffer but verify the contents of it.
     * \pre The PIC should have it's program counter set to the beginning of
     *      program memory.
     * \post The program counter is pointing to the address immediatly after
     *       the last program memory address.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void read_program_memory(DataBuffer& buf, long base=0,
      bool verify=false);

    /** Program the data EEPROM contents to the PIC device.
     * \param buf A DataBuffer containing the data to program.
     * \param base The offset within the data buffer to start retrieving data.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_data_memory(DataBuffer& buf, long base);

    /** Read the data EEPROM contents from the PIC device.
     * \param buf A DataBuffer in which to store the read data.
     * \param base The offset within the data buffer to start storing data.
     * \param verify If this flag is true, don't store the data in the
     *        DataBuffer but verify the contents of it.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void read_data_memory(DataBuffer& buf, long base, bool verify);

    /** Program the ID words to the PIC device.
     * \param buf A DataBuffer containing the data to program.
     * \param base The offset within the data buffer to start retrieving data.
     * \pre The PIC should have it's program counter set to the location of
     *      the ID words.
     * \post The program counter is pointing to the address immediatly after
     *       the last ID word.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_id_memory(DataBuffer& buf, long base);

    /** Read the ID words from the PIC device.
     * \param buf A DataBuffer in which to store the read data.
     * \param base The offset within the data buffer to start storing data.
     * \param verify If this flag is true, don't store the data in the
     *        DataBuffer but verify the contents of it.
     * \pre The PIC should have it's program counter set to the location of
     *      the ID words.
     * \post The program counter is pointing to the address immediatly after
     *       the last ID word.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void read_id_memory(DataBuffer& buf, long base, bool verify);

    /** Program the config word to the PIC device.
     * \param data The value of the configuration word to write.
     * \pre The PIC program counter should be pointing to the location of
     *      the configuration word.
     * \post The PIC program counter still points to the configuration word.
     * \throws runtime_error Contains a description of the error.
     */
    virtual void write_config_word(uint32_t data);

    /** Read the config word from the PIC device.
     * \param verify If this flag is true, compare \c verify_data against the
     *        configuration word.
     * \param verify_data If \c verify is true, read the configuration word
     *        and compare it against this value. The comparison will only
     *        include the bits in \c config_mask.
     * \pre The PIC program counter should be pointing to the location of
     *      the configuration word.
     * \post The PIC program counter still points to the configuration word.
     * \throws runtime_error Contains a description of the error.
     * \returns The raw value of the configuration word as read from the PIC
     *          device.
     */
    virtual uint32_t read_config_word (
        bool verify=false,
        uint32_t verify_data=0
    );

    /** Applies up to \c program_count programming cycles to the PIC device
     * and then applies the appropriate number of overprogramming cycles.
     * This algorithm is probably used for every type of PIC in existance.
     * \param data The data value to program into memory.
     * \pre The PIC program counter should be pointing to the location in
     *      memory that will be programmed.
     * \returns A boolean value indicating if the programming was successful.
     */
    virtual bool program_one_location(uint32_t data);

    /** Write a byte to the data EEPROM. The data is read from the location
     * specified by the program counter. The first call to this or
     * read_ee_data() sets the program counter to the beginning of the EEPROM
     * and writes to there.
     * \param data The daya byte to write to the EEPROM.
     */
    virtual void write_ee_data(uint32_t data);

    /** Read a byte from the data EEPROM. The data is read from the location
     * specified by the program counter. The first call to this or
     * write_ee_data() sets the program counter to the beginning of the EEPROM
     * and reads from there.
     * \returns The data byte that was read from the EEPROM.
     */
    uint32_t read_ee_data(void);

    virtual uint32_t read_deviceid(void);

/* Protected data: */
    /** The default value of the configuration bits after an erase */
    unsigned int default_config_word[2];

    /** Bitmask for valid bits in the configuration word. */
    unsigned int config_mask[2];

    /** A bitmask for configuration bits that must be preserved. */
    unsigned int persistent_config_mask[2];

    /** Code protection config bits. */
    unsigned int cp_mask, cp_all, cp_none;

    /** Data protection bits. */
    unsigned int cpd_mask, cpd_on, cpd_off;

    static const Instruction opcodes[];
};

/** Implement functions specific to Pic16f88[23467] chips, programming spec.
 * DS41287D.
 */
class Pic16f88x : public Pic16
{
public:
    /* PIC commands */
    // const static int
    enum Commands_List {
        COMMAND_LOAD_CONFIG    = 0x00, /**< Load Configuration            */
        COMMAND_LOAD_PROG_DATA = 0x02, /**< Load Data for Program Memory  */
        COMMAND_LOAD_DATA_DATA = 0x03, /**< Load Data for Data Memory     */
        COMMAND_READ_PROG_DATA = 0x04, /**< Read Data from Program Memory */
        COMMAND_READ_DATA_DATA = 0x05, /**< Read Data from Data Memory    */
        COMMAND_INC_ADDRESS    = 0x06, /**< Increment Address             */
        COMMAND_BEGIN_PROG     = 0x08, /**< Begin Erase/Programming       */
        COMMAND_ERASE_PROG_MEM = 0x09, /**< Bulk Erase Program Memory     */
        COMMAND_END_PROG       = 0x0a, /**< End Programming               */
        COMMAND_ERASE_DATA_MEM = 0x0b, /**< Bulk Erase Data Memory        */
        COMMAND_ERASE_PROG_ROW = 0x11, /**< Erase Program memory row      */
        COMMAND_BEGIN_PROG_EXT = 0x18  /**< Begin Programming, ext timing */
    };

    Pic16f88x(char *name);    /**< Constructor */
    ~Pic16f88x();             /**< Destructor */

protected:
	virtual void	erase(void);

    /** Program the entire contents of program memory to the PIC device.
     * \param buf A DataBuffer containing the data to program.
     * \param base The offset within the data buffer to start retrieving data.
     *        If this parameter isn't specified, it defaults to 0.
     * \pre The PIC should have it's program counter set to the beginning of
     *      program memory.
     * \post The program counter is pointing to the address immediately after
     *       the last program memory address.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_program_memory(DataBuffer& buf, long base=0);

    /* Implement 4- or 8- word algorithm for program memory.  Program 4 or 8
     * words at the current PC location.  PC is assumed to be 0 modulo 4 or 8,
     * depending on the value of write_buffer_size.  Afterward, the PC has
     * been advanced to the next 4 or 8 word boundary.	*/
	virtual void program_mult_prog_loc(DataBuffer& buf, long base);
	
    /** Program the data EEPROM contents to the PIC device.
     * \param buf A DataBuffer containing the data to program.
     * \param base The offset within the data buffer to start retrieving data.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
	virtual void write_data_memory(DataBuffer& buf, long base=0x2100);
	
    /** Perform a single program cycle for program memory. The following steps
     * are performed:
     *   - The data is written to the PIC with write_prog_data().
     *   - The BEGIN_PROG command is sent.
     *   - A delay of program_time is initiated.
     *   - If the PIC requires it, the END_PROG command is sent.
     *   - A delay of 100us discharge time is performed.
     *   - A readback is performed and is compared with the original data.
     * \param data The data word to program into program memory at the PIC's
     *        program counter.
     * \param mask A mask which is used when verifying the data. The read back
     *             data and the data parameter are both masked with this mask
     *             before being compared. This is required for properly
     *             verifying the configuration word.
     * \returns A boolean value indicating if the data read back matches the
     *          data parameter.
     */
	virtual bool program_cycle(uint32_t data, uint32_t mask);
};

/** A class which implements device-specific functions for PIC16F8xx devices.
 */
class Pic16f8xx : public Pic16
{
public:
    Pic16f8xx(char *name);    /**< Constructor */
    ~Pic16f8xx();             /**< Destructor */

protected:
    void bulk_erase(void);
    void disable_codeprotect(void);
};


/** A class which implements device-specific functions for PIC16F8xx devices.
 */
class Pic16f87xA : public Pic16 {
public:
    // const static int
    enum Pic16f87xA_Commands_List {
        COMMAND_BEGIN_PROG_ONLY = 0x18, /**< Begin Program. Only (no erase) */
        COMMAND_END_PROG        = 0x17, /**< End Programming                */
        COMMAND_CHIP_ERASE      = 0x1f  /**< Chip Erase                     */
    };

    Pic16f87xA(char *name);   /**< Constructor */
    ~Pic16f87xA();            /**< Destructor */

protected:
    void disable_codeprotect(void);
    void bulk_erase(void);
    void write_program_memory(DataBuffer& buf, long base=0);
    void write_config_word(uint32_t data);
};

/** A class for device-specific functions of the PIC16F6XX devices.
 */
class Pic16f6xx : public Pic16
{
public:
    Pic16f6xx(char *name);    /**< Constructor */
    ~Pic16f6xx();             /**< Destructor */

protected:
    void bulk_erase(void);
    void disable_codeprotect(void);
};


/** A class for device-specific functions of the PIC12F6XX devices. Although
 * the part numbers have a "12" prefix, they have 14-bit instruction words and
 * look very much like PIC16 devices from a programmers point of view.
 */
class Pic12f6xx : public Pic16
{
public:
    Pic12f6xx(char *name);    /**< Constructor */
    ~Pic12f6xx();             /**< Destructor */

protected:
    void disable_codeprotect(void);
};


/** A class which implements device-specific functions for PIC16F7x devices.
 */
class Pic16f7x : public Pic16
{
public:
    Pic16f7x(char *name); /**< Constructor */
    ~Pic16f7x();          /**< Destructor */

protected:
    void disable_codeprotect(void);
};

#define REG_EEADR            0xa9
#define REG_EEADRH           0xaa
#define REG_EEDATA           0xa8
#define REG_EECON2           0xa7
#define REG_TABLAT           0xf5
#define REG_TBLPTRU          0xf8
#define REG_TBLPTRH          0xf7
#define REG_TBLPTRL          0xf6

#define ASM_NOP              0x0000                 /* nop               */
#define ASM_BCF_EECON1_EEPGD 0x9ea6                 /* bcf EECON1, EEPGD */
#define ASM_BCF_EECON1_CFGS  0x9ca6                 /* bcf EECON1, CfGS  */
#define ASM_BCF_EECON1_FREE  0x98a6                 /* bcf EECON1, FREE  */
#define ASM_BCF_EECON1_WRERR 0x96a6                 /* bcf EECON1, WRERR */
#define ASM_BCF_EECON1_WREN  0x94a6                 /* bcf EECON1, WREN  */
#define ASM_BSF_EECON1_EEPGD 0x8ea6                 /* bsf EECON1, EEPGD */
#define ASM_BSF_EECON1_CFGS  0x8ca6                 /* bsf EECON1, CFGS  */
#define ASM_BSF_EECON1_FREE  0x88a6                 /* bsf EECON1, FREE  */
#define ASM_BSF_EECON1_WRERR 0x86a6                 /* bsf EECON1, WRERR */
#define ASM_BSF_EECON1_WREN  0x84a6                 /* bsf EECON1, WREN  */
#define ASM_BSF_EECON1_WR    0x82a6                 /* bsf EECON1, WR    */
#define ASM_BSF_EECON1_RD    0x80a6                 /* bsf EECON1, RD    */
#define ASM_MOVLW(addr)      0x0e00 | ((addr)&0xff) /* movlw <addr>      */
#define ASM_MOVWF(addr)      0x6e00 | ((addr)&0xff) /* movwf <addr>      */
#define ASM_MOVF_EEDATA_W_0  0x50a8                 /* movf EEDATA, W, 0 */
#define ASM_MOVF_EECON1_W_0  0x50a6                 /* movf EECON1, W, 0 */
#define ASM_INCF_TBLPTR      0x2af6                 /* incf TBLPTR       */
#define ASM_MOVWF_EECON2     0x6ea7                 /* movwf EECON2      */

/** A class which implements the programming algorithm PIC18* devices. The
 * PIC18* devices are different in many ways:
 *   * Programming/reading is done in bytes.
 *   * Much larger internal address space
 *   * Programming is accomplished by executing single instructions on the
 *     PIC core.
 *   * Programming can be done 32 bytes at a time.
 *   * Data EEPROM, configuration words, and ID locations  are at different
 *     addresses than the PIC16* devices.
 *   * There are multiple configuration words.
 * This class will provide a base class for devices of this type.
 */
class Pic18 : public Pic
{
public:
    /* PIC18* commands */
    // const static int
    enum Commands_List {
        COMMAND_CORE_INSTRUCTION    = 0x00, /**< Execute an instruction      */
        COMMAND_SHIFT_OUT_TABLAT    = 0x02, /**< Shift out TABLAT register   */
        COMMAND_TABLE_READ          = 0x08, /**< Table Read                  */
        COMMAND_TABLE_READ_POSTINC  = 0x09, /**< Table Read, post-increment  */
        COMMAND_TABLE_READ_POSTDEC  = 0x0a, /**< Table Read, post-decrement  */
        COMMAND_TABLE_READ_PREINC   = 0x0b, /**< Table Read, pre-increment   */
        COMMAND_TABLE_WRITE         = 0x0c, /**< Table Write                 */
        COMMAND_TABLE_WRITE_POSTINC = 0x0d, /**< Table Write, post-inc by 2  */
        COMMAND_TABLE_WRITE_POSTDEC = 0x0e, /**< Table Write, post-dec by 2  */
        COMMAND_TABLE_WRITE_START   = 0x0f  /**< Table Write, start program. */
    };

    Pic18(char *name);    /**< Constructor */
    virtual ~Pic18();     /**< Destructor */

    virtual void erase(void);
    virtual void program(DataBuffer& buf);
    virtual void read(DataBuffer& buf, bool verify=false);

    /** Gets the native clearvalue depending on the memory address
     * \returns The clearvalue.
     */
    virtual unsigned int get_clearvalue(size_t addr);

protected:
    void range_erase(int memstart, int memlen);
    virtual void chip_erase(void);
    virtual void panel_erase(int panel);
    virtual void block_erase(int len, int address);

    /** Writes data to the program memory. The entire code space is written
     * to take advantage of multi-panel writes.
     * \param buf The DataBuffer from which to retrieve the data to write.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of words
     *       written to program memory. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of words written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_program_memory(DataBuffer& buf, bool verify);

    /** Writes the ID memory locations.
     * \param buf The DataBuffer from which data is read.
     * \param addr The byte address of the ID words in the PIC address space.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of words
     *       written to the ID locations. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of words written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_id_memory (
        DataBuffer& buf,
        unsigned long addr,
        bool verify
    );

    /** Writes data to the data eeprom.
     * \param buf The DataBuffer from which to retrieve the data to write.
     * \param addr The byte offset into the DataBuffer from which to start
     *             retrieving data.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of bytes
     *       written to the data memory. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of bytes written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the data memory location at which the error occured.
     */
    virtual void write_data_memory (
        DataBuffer& buf,
        unsigned long addr,
        bool verify
    );

    /** Writes the configuration words
     * \param buf The DataBuffer from which data is read.
     * \param addr The byte address of the configuration words in the PIC
     *        address space.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of
     *       configuration words written. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of words written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the configuration word number where the error occurred.
     */
    virtual void write_config_memory (
        DataBuffer& buf,
        unsigned long addr,
        bool verify
    );

    /** Reads a portion of the PIC memory.
     * \param buf The DataBuffer to store the read data. On a verify, this
     *        data is compared with the data on the PIC.
     * \param addr The byte address in the PIC's memory to begin the read.
     *        Since the PIC's memory is byte-oriented and the DataBuffer is
     *        16-bit word oriented, the offset into the DataBuffer will be
     *        1/2 the PIC memory address.
     * \param len The number of 16-bit words to read into the DataBuffer.
     * \param verify Boolean flag which indicates a verify operation against
     *        \c buf.
     * \post The \c progress_count is incremented by the number of
     *       program memory words read/verified.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void read_memory (
        DataBuffer& buf,
        unsigned long addr,
        unsigned long len,
        bool verify
    );

    /** Reads a portion of the PIC configuration memory. This is basically the
     * same as \c read_memory except there is some masking of the data.
     * \param buf The DataBuffer to store the read data. On a verify, this
     *        data is compared with the data on the PIC.
     * \param addr The byte address in the PIC's memory to begin the read.
     *        Since the PIC's memory is byte-oriented and the DataBuffer is
     *        16-bit word oriented, the offset into the DataBuffer will be
     *        1/2 the PIC memory address.
     * \param len The number of 16-bit words to read into the DataBuffer.
     * \param verify Boolean flag which indicates a verify operation against
     *        \c buf.
     */
    virtual void read_config_memory (
        DataBuffer& buf,
        unsigned long addr,
        unsigned long len,
        bool verify
    );

    /** Reads the entire PIC data EEPROM. The bytes are packed into the
     * DataBuffer as 1 byte per 16-bit word.
     * \param buf The DataBuffer to store the read data. On a verify, this
     *        data is compared with the data on the PIC.
     * \param addr The byte address in the DataBuffer to store the EEPROM
     *        data. Since the DataBuffer consists of 16-bit words, the
     *        DataBuffer offset will be 1/2 this.
     * \param verify Boolean flag which indicates a verify operation against
     *        \c buf.
     * \post The \c progress_count is incremented by the number of
     *       bytes read/verified from the data memory.
     * \throws runtime_error Contains a description of the error along with
     *         the data memory location at which the error occured.
     */
    virtual void read_data_memory (
        DataBuffer& buf,
        unsigned long addr,
        bool verify
    );

    /** Loads the write buffer for a panel and optionally begins the write
     * sequence.
     * \param buf The DataBuffer from which to retrieve data.
     * \param panel The panel number whose buffer to fill.
     * \param offset The offset into the panel to write the data.
     * \param last A boolean value indicating if this is the last panel buffer
     *        to be loaded. If so, the write is initiated after this panel
     *        buffer is written.
     * \post The \c progress_count is incremented by the number of words written
     *       to the write buffer. On success this is 4.
     * \post If \c last is true, a call to program_delay() or the equivalent
     *       must be made so the write is properly timed.
     */
    virtual void load_write_buffer (
        DataBuffer& buf,
        unsigned int panel,
        unsigned int offset,
        bool last
    );

    /** Does a custom NOP/program wait. This will output
     * COMMAND_CORE_INSTRUCTION, holding clk high for the programming time,
     * if hold_clock_high is true, and then finish up by clocking out a nop
     * instruction (16 0's).
     * \param hold_clock_high Wether the clock must be held high diring the
     *                        programming time.
     */
    virtual void program_delay(bool hold_clock_high=true);

    /** Sets the value of the PIC's internal TBLPTR register. This register
     * contains the address of the current read/write operation.
     * \param addr The byte address within the PIC's address space.
     */
    virtual void set_tblptr(unsigned long addr);

    /** Send a 4-bit command to the PIC.
     * \param command The 4-bit command to write.
     */
    virtual void write_command(unsigned int command);

    /** Sends a 4-bit command with a 16-bit operand to the PIC.
     * \param command The 4-bit command to write.
     * \param data The 16-bit operand of the command.
     */
    virtual void write_command(unsigned int command, unsigned int data);

    /** Writes a 4-bit command and read an 8-bit response. The actual
     * procedure is to write the 4 bit command, strobe 8 times, and then
     * clock in the 8 data bits.
     * \param command The 4-bit command to send.
     * \returns The 8 bits of data that were read back.
     */
    virtual unsigned int write_command_read_data(unsigned int command);

    virtual uint32_t read_deviceid(void);

    /** Prepare the default value of the configuration registers.
     */
    virtual void set_config_default(DataBuffer& buf);

/* Protected Data: */
    /** The bitmasks for each configuration word */
    unsigned int config_masks[7];
    unsigned int config_deflt[7];

    static const Instruction opcodes[];
};

class Pic18fxx20 : public Pic18
{
public:
    Pic18fxx20(char *name);
    virtual ~Pic18fxx20();

    virtual void erase(void);

protected:    
    /** Writes data to the program memory. The entire code space is written
     * to take advantage of multi-panel writes.
     * \param buf The DataBuffer from which to retrieve the data to write.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of words
     *       written to program memory. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of words written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_program_memory(DataBuffer& buf, bool verify);

    /** Writes the ID memory locations.
     * \param buf The DataBuffer from which data is read.
     * \param addr The byte address of the ID words in the PIC address space.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of words
     *       written to the ID locations. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of words written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the address at which the error occurred.
     */
    virtual void write_id_memory (
        DataBuffer& buf, 
        unsigned long addr,
        bool verify
    );

    /** Writes data to the data eeprom.
     * \param buf The DataBuffer from which to retrieve the data to write.
     * \param addr The byte offset into the DataBuffer from which to start
     *             retrieving data.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of bytes
     *       written to the data memory. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of bytes written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the data memory location at which the error occured.
     */
    virtual void write_data_memory (
        DataBuffer& buf, 
        unsigned long addr,
        bool verify
    );

    /** Writes the configuration words
     * \param buf The DataBuffer from which data is read.
     * \param addr The byte address of the configuration words in the PIC
     *        address space.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of
     *       configuration words written. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of words written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the configuration word number where the error occurred.
     */
    virtual void write_config_memory (
        DataBuffer& buf, 
        unsigned long addr,
        bool verify
    );

    /** Reads the entire PIC data EEPROM. The bytes are packed into the
     * DataBuffer as 1 byte per 16-bit word.
     * \param buf The DataBuffer to store the read data. On a verify, this
     *        data is compared with the data on the PIC.
     * \param addr The byte address in the DataBuffer to store the EEPROM
     *        data. Since the DataBuffer consists of 16-bit words, the
     *        DataBuffer offset will be 1/2 this.
     * \param verify Boolean flag which indicates a verify operation against
     *        \c buf.
     * \post The \c progress_count is incremented by the number of
     *       bytes read/verified from the data memory.
     * \throws runtime_error Contains a description of the error along with
     *         the data memory location at which the error occured.
     */
    virtual void read_data_memory (
        DataBuffer& buf, 
        unsigned long addr,
        bool verify
    );

    /** Loads the write buffer for the current write sequence.
     * \param buf The DataBuffer from which to retrieve data.
     * \param addr The byte address in the PIC's memory to begin the write.
     * \param offset The offset into the panel to write the data.
     * \returns TRUE if any word in the write buffer was non-blank 
     *                 (other than 0xffff), otherwise FALSE.
     * \post The \c progress_count is incremented by the number of words written
     *       to the write buffer. On success this is the write buffer size.
     */
    virtual bool load_write_buffer (
        DataBuffer& buf, 
        unsigned long addr, 
        unsigned long count
    );

    /** Does a custom NOP/program wait. This will output
     * COMMAND_CORE_INSTRUCTION, hold clk high for the programming time,
     * and then finish up by clocking out a nop instruction (16 0's).
     * Reimplemented here to add 100us high voltage discharge time after
     * program delay.
     */
    virtual void program_wait(void);

};

class Pic18f2xx0 : public Pic18fxx20
{
public:
    /**< Table Write, start programming, post-inc by 2 */
    const static int COMMAND_TABLE_WRITE_START_POSTINC=0x0e; 

    Pic18f2xx0(char *name);
    virtual ~Pic18f2xx0();

    virtual void erase(void);

protected:
    /** Writes data to the data eeprom.
     * \param buf The DataBuffer from which to retrieve the data to write.
     * \param addr The byte offset into the DataBuffer from which to start
     *             retrieving data.
     * \param verify A boolean value indicating if the written data should be
     *        read back and verified.
     * \post The \c progress_count is incremented by the number of bytes
     *       written to the data memory. If \c verify is true then
     *       \c progress_count will have been incremented by two times the
     *       number of bytes written; once for the write and once for the
     *       verify.
     * \throws runtime_error Contains a description of the error along with
     *         the data memory location at which the error occured.
     */
    virtual void write_data_memory (
        DataBuffer& buf, 
        unsigned long addr,
        bool verify
    );

    /** Reads the entire PIC data EEPROM. The bytes are packed into the
     * DataBuffer as 1 byte per 16-bit word.
     * \param buf The DataBuffer to store the read data. On a verify, this
     *        data is compared with the data on the PIC.
     * \param addr The byte address in the DataBuffer to store the EEPROM
     *        data. Since the DataBuffer consists of 16-bit words, the
     *        DataBuffer offset will be 1/2 this.
     * \param verify Boolean flag which indicates a verify operation against
     *        \c buf.
     * \post The \c progress_count is incremented by the number of
     *       bytes read/verified from the data memory.
     * \throws runtime_error Contains a description of the error along with
     *         the data memory location at which the error occured.
     */
    virtual void read_data_memory (
        DataBuffer& buf, 
        unsigned long addr, bool verify
    );
};

#endif
