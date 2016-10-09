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
 * $Id: Avr.cxx,v 1.20 2002/10/19 19:09:00 marka Exp $
 */
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "Atmel.h"
#include "IO.h"
#include "Util.h"

extern "C" {
#include "avr_disasm.h"
}

#define PAR_LENGTH_MAX 256

const char *Avr::validInstructionChars = "01abHoix";
const char *Avr::validHexDigit         = "0123456789ABCDEF";
const char *Avr::validBitNameChars     = "-,ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

const t_NamedSettings Avr::sparams[Avr::LAST_PARAM] = {
    "vccMin"                          , (void*)0,
    "vccMax"                          , (void*)0,
    "waitDelays.reset"                , (void*)0,
    "waitDelays.erase"                , (void*)0,
    "waitDelays.flash"                , (void*)0,
    "waitDelays.eeprom"               , (void*)0,
    "waitDelays.fuse"                 , (void*)0,
    "flash.size"                      , (void*)0,
    "flash.pageSize"                  , (void*)0,
    "flash.pages"                     , (void*)0,
    "eeprom.size"                     , (void*)0,
    "eeprom.pageSize"                 , (void*)0,
    "eeprom.pages"                    , (void*)0,
    "signature.b0"                    , (void*)"",
    "signature.b1"                    , (void*)"",
    "signature.b2"                    , (void*)"",
    "flash.readBack.p1"               , (void*)"",
    "flash.readBack.p2"               , (void*)"",
    "eeprom.readBack.p1"              , (void*)"",
    "eeprom.readBack.p2"              , (void*)"",
    "calibration.bytes"               , (void*)0,
    "instructions.signatureRead"      , (void*)"",
    "instructions.programmingEnable"  , (void*)"",
    "instructions.chipErase"          , (void*)"",
    "instructions.readFlash"          , (void*)"",
    "instructions.writeFlash"         , (void*)"",
    "instructions.readEeprom"         , (void*)"",
    "instructions.writeEeprom"        , (void*)"",
    "instructions.readFuse"           , (void*)"",
    "instructions.writeFuse"          , (void*)"",
    "instructions.readLock"           , (void*)"",
    "instructions.writeLock"          , (void*)"",
    "instructions.poll"               , (void*)"",
    "instructions.loadExtAddr"        , (void*)"",
    "instructions.loadFlashPage"      , (void*)"",
    "instructions.writeFlashPage"     , (void*)"",
    "instructions.loadEepromPage"     , (void*)"",
    "instructions.writeEepromPage"    , (void*)"",
    "instructions.readFuseHigh"       , (void*)"",
    "instructions.writeFuseHigh"      , (void*)"",
    "instructions.readExtFuse"        , (void*)"",
    "instructions.writeExtFuse"       , (void*)"",
    "instructions.readCalibrationByte", (void*)"",
};
const t_NamedSettings Avr::sbitNames[Avr::LAST_BIT_NAME*Avr::BITS_IN_FUSES] = {
    "bitNames.fuseHigh.bit0"               , (void*)"-",
    "bitNames.fuseHigh.bit1"               , (void*)"-",
    "bitNames.fuseHigh.bit2"               , (void*)"-",
    "bitNames.fuseHigh.bit3"               , (void*)"-",
    "bitNames.fuseHigh.bit4"               , (void*)"-",
    "bitNames.fuseHigh.bit5"               , (void*)"-",
    "bitNames.fuseHigh.bit6"               , (void*)"-",
    "bitNames.fuseHigh.bit7"               , (void*)"-",

    "bitNames.fuseLow.bit0"                , (void*)"-",
    "bitNames.fuseLow.bit1"                , (void*)"-",
    "bitNames.fuseLow.bit2"                , (void*)"-",
    "bitNames.fuseLow.bit3"                , (void*)"-",
    "bitNames.fuseLow.bit4"                , (void*)"-",
    "bitNames.fuseLow.bit5"                , (void*)"-",
    "bitNames.fuseLow.bit6"                , (void*)"-",
    "bitNames.fuseLow.bit7"                , (void*)"-",

    "bitNames.extFuse.bit0"                , (void*)"-",
    "bitNames.extFuse.bit1"                , (void*)"-",
    "bitNames.extFuse.bit2"                , (void*)"-",
    "bitNames.extFuse.bit3"                , (void*)"-",
    "bitNames.extFuse.bit4"                , (void*)"-",
    "bitNames.extFuse.bit5"                , (void*)"-",
    "bitNames.extFuse.bit6"                , (void*)"-",
    "bitNames.extFuse.bit7"                , (void*)"-",

    "bitNames.lock.bit0"                   , (void*)"-",
    "bitNames.lock.bit1"                   , (void*)"-",
    "bitNames.lock.bit2"                   , (void*)"-",
    "bitNames.lock.bit3"                   , (void*)"-",
    "bitNames.lock.bit4"                   , (void*)"-",
    "bitNames.lock.bit5"                   , (void*)"-",
    "bitNames.lock.bit6"                   , (void*)"-",
    "bitNames.lock.bit7"                   , (void*)"-",
};

Device *Avr::load(char *vendor, char *spec, char *device)
{
    if(
        Util::regexMatch("^AT[0-9].*", device)     ||
        Util::regexMatch("^ATtiny[0-9].*", device) ||
        Util::regexMatch("^ATmega[0-9].*", device)
    ) {
        return new Avr(vendor, spec, device);
    } else {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Unknown AVR device %s",
                device
            )
        );
    }
    return NULL;
}

Avr::Avr(char *vendor, char *spec, char *device) : Atmel(vendor, spec, device)
{
int i,j;
char buf[33];

    verifyConfig(*config, true);
 
    config->get("powerOffAfterWriteFuse", powerOffAftwerWriteFuse, 0);

    config->get(sparams[PAR_VCC_MIN].name,vcc[VCC_NDX(PAR_VCC_MIN)],(double)((int)sparams[PAR_VCC_MIN].defv));
    config->get(sparams[PAR_VCC_MAX].name,vcc[VCC_NDX(PAR_VCC_MAX)],(double)((int)sparams[PAR_VCC_MAX].defv));

    for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
        config->get(sparams[i].name,wait_delays[WTD_NDX(i)],(int)sparams[i].defv);
    }
    for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
        config->get(sparams[i].name,boundary_sizes[BND_NDX(i)],(int)sparams[i].defv);
    }
    for (i=PAR_SIGN_B0; i<=PAR_SIGN_B2; i++) {
        config->getHex(sparams[i].name,signature[SIG_NDX(i)],(int)sparams[i].defv);
    }
    for (i=PAR_FLASH_READ_BACK_P1; i<=PAR_FLASH_READ_BACK_P2; i++) {
        config->getHex(sparams[i].name,flash_readBack[FRB_NDX(i)],(int)sparams[i].defv);
    }
    for (i=PAR_EEPROM_READ_BACK_P1; i<=PAR_EEPROM_READ_BACK_P2; i++) {
        config->getHex(sparams[i].name,eeprom_readBack[FRB_NDX(i)],(int)sparams[i].defv);
    }
    config->get (
        sparams[PAR_CALIBRATION_BYTES].name,
        calibration_bytes[CAL_NDX(PAR_CALIBRATION_BYTES)],
        (int)sparams[PAR_CALIBRATION_BYTES].defv
    );
    for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        config->get(sparams[i].name,buf,(const char *)sparams[i].defv,32);
        buf[32]='\0';
        instructions[INS_NDX(i)].parse(buf);
    }
    for (i=0; i<LAST_BIT_NAME;i++) {
        for (j=0;j<BITS_IN_FUSES;j++) {
            config->get(sbitNames[j*i].name,buf,(const char *)sbitNames[j*i].defv,BIT_NAME_MAX_SIZE);
            buf[BIT_NAME_MAX_SIZE]='\0';
            strncpy(bitNames[i][j],buf,BIT_NAME_MAX_SIZE);
            bitNames[i][j][BIT_NAME_MAX_SIZE] = '\0';
        }
    }

    // Create the memory map for this device

    IntPair p(0,0);

    // signature bytes
    p.first  += p.second;
    p.second  = 3;
    this->memmap.push_back(p);
    this->signature_extent = p;

    // fuse low byte
    p.first  += p.second;
    p.second  = instructions[INS_NDX(PAR_READ_FUSE_INST)].isValid();
    this->memmap.push_back(p);
    this->config_extent[FUSE_LOW_BITS] = p;

    // fuse high byte
    p.first  += p.second;
    p.second  = instructions[INS_NDX(PAR_READ_FUSE_HIGH_INST)].isValid();
    this->memmap.push_back(p);
    this->config_extent[FUSE_HIGH_BITS] = p;

    // ext fuse bytes
    p.first  += p.second;
    p.second  = instructions[INS_NDX(PAR_READ_EXT_FUSE_INST)].isValid() ;
    this->memmap.push_back(p);
    this->config_extent[EXT_FUSE_BITS] = p;

    // lock byte
    p.first  += p.second;
    p.second  = instructions[INS_NDX(PAR_READ_LOCK_INST)].isValid();
    this->memmap.push_back(p);
    this->config_extent[LOCK_BITS] = p;

    // calibration bytes
    p.first  += p.second;
    p.second  = calibration_bytes[CAL_NDX(PAR_CALIBRATION_BYTES)];
    this->memmap.push_back(p);
    this->calib_extent = p;

    // code memory
    p.first  += p.second;
    p.second  = this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)];
    this->memmap.push_back(p);
    this->code_extent = p;

    // data memory
    p.first  += p.second;
    p.second  = this->boundary_sizes[BND_NDX(PAR_EEPROM_SIZE)];
    this->memmap.push_back(p);
    this->data_extent = p;
}

Avr::~Avr()
{
}

void Avr::check_instruction(int inst)
{
    if ( ! instructions[INS_NDX(inst)].isValid() ) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s: Missing Instruction",
                this->sparams[inst].name
            )
        );
    }
}

void Avr::set_program_mode(void)
{
    check_instruction(PAR_PROGRAMMING_ENABLE_INST);

    // 1. Power-up sequence:
    //    Apply power between VCC and GND while RESET and SCK are set to "0". In some systems,
    //    the programmer can not guarantee that SCK is held low during power-up. In this
    //    case, RESET must be given a positive pulse of at least two CPU clock cycles duration
    //    after SCK has been set to "0".

    this->io->data  (false);
    this->io->reset (false);
    this->io->clock (false);
    this->io->reset (true);
    this->io->usleep(1000);
    this->io->reset (false);
    this->io->vdd   (IO::VDD_TO_ON);

    // 2. Wait for at least 20 ms and enable serial programming by sending the Programming
    //    Enable serial instruction to pin MOSI.

    this->io->usleep(wait_delays[WTD_NDX(PAR_WD_RESET)]);

    // 3. The serial programming instructions will not work if the communication is out of synchronization.
    //    When in sync. the second byte (0x53), will echo back when issuing the third
    //    byte of the Programming Enable instruction. Whether the echo is correct or not, all four
    //    bytes of the instruction must be transmitted. If the 0x53 did not echo back, give RESET a
    //    positive pulse and issue a new Programming Enable command.

    for (int tries=0; tries<15; tries++) {
        InstResponse risp;
        risp.dword = instructions[INS_NDX(PAR_PROGRAMMING_ENABLE_INST)].execute();
        if (risp.bytes[3] == 0x53) {
            break;
        }
        this->io->reset (true);
        this->io->usleep(1000);
        this->io->reset (false);
    }
}

void Avr::off(void)
{
    // Shut everything down
    this->io->clock (false);
    this->io->data  (false);
    this->io->reset (true);
    this->io->vdd   (IO::VDD_TO_OFF);
}

void Avr::read_signature(char signature[3])
{
uint32_t output;

    check_instruction(PAR_READ_SIGNATURE_INST);

    instructions[INS_NDX(PAR_READ_SIGNATURE_INST)].execute(0,output,0);
    signature[0] = output & 0x000000ff;

    instructions[INS_NDX(PAR_READ_SIGNATURE_INST)].execute(0,output,1);
    signature[1] = output & 0x000000ff;

    instructions[INS_NDX(PAR_READ_SIGNATURE_INST)].execute(0,output,2);
    signature[2] = output & 0x000000ff;
}

void Avr::check_signature(void)
{
char chip_sig[3];

    read_signature(chip_sig);

    if ((chip_sig[0] != this->signature[0]) ||
        (chip_sig[1] != this->signature[1]) ||
        (chip_sig[2] != this->signature[2]) ) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Chip signature[%02x:%02x:%02x] does not match Device signature[%02x:%02x:%02x]",
                chip_sig[0], chip_sig[1], chip_sig[2],
                this->signature[0], this->signature[1], this->signature[2]
            )
        );
    }
}

void Avr::erase(void)
{
    check_signature();

    check_instruction(PAR_CHIP_ERASE_INST);

    try {
        this->set_program_mode();
        instructions[INS_NDX(PAR_CHIP_ERASE_INST)].execute();
        this->io->usleep(wait_delays[WTD_NDX(PAR_WD_ERASE)]);
        this->off();
    } catch (std::exception& e) {
        this->off();
        throw;
    }
}

void Avr::polling_flash_memory(uint32_t value, uint32_t address)
{
uint32_t    output = 0;
microtime_t start_time;
microtime_t prog_time;

    if ( ( value != flash_readBack[0] ) &&
         ( value != flash_readBack[1] ) ) {
        start_time = this->io->now();
        do {
            instructions[INS_NDX(PAR_READ_FLASH_INST)].execute(0,output,address);
            prog_time = this->io->now();
        } while ( ( output != value ) &&
                  ( ( prog_time - start_time ) < wait_delays[WTD_NDX(PAR_WD_FLASH)] ) );
    } else {
        this->io->usleep(wait_delays[WTD_NDX(PAR_WD_FLASH)]);
    }
}

void Avr::write_flash_memory(DataBuffer& buf)
{
uint32_t output          = 0;
uint32_t offset          = 0;
uint32_t polling_address = 0;
uint32_t polling_value   = 0;
uint32_t base            = code_extent.first;

    try {
        bool     paged      = instructions[INS_NDX(PAR_LOAD_FLASH_PAGE_INST)] .isValid() &&
                              instructions[INS_NDX(PAR_WRITE_FLASH_PAGE_INST)].isValid() ;
        uint32_t part_pages = this->boundary_sizes[BND_NDX(PAR_FLASH_PAGES)];
        uint32_t ext_addr   = 0xffffffff;

        // Let's assume the AVR flash memory is structured as follow:
        // there are w Words per page and p Pages per partition
        // And the partition is addressed by the Extended Address value.
        //
        // As for the data sheets:
        // << The extended address byte is stored until the command is re-issued,
        //    i.e., the command needs only be issued for the first page,
        //    and when crossing the 64KWord boundary. >>
        //
        // Thus we know that a partition is composed by a total of 64K Words (1<<16)
        // or 128Kb (1<<17). We have to base our calculations in bytes as the total
        // memory size is expressed in bytes as well, for this reason we will multiply
        // or divide by a factor of 2^17  
        // 
        if ( this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)] >= (1<<17) ) {
            part_pages /= ( this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)] >> 17 );
        }
        for (
            uint32_t page=0;
                     page < this->boundary_sizes[BND_NDX(PAR_FLASH_PAGES)];
                     page++
        ) {
            for (
                uint32_t byte_address=0;
                         byte_address < this->boundary_sizes[BND_NDX(PAR_FLASH_PAGE_SIZE)];
                         byte_address++
            ) { 
                progress(base+offset);
                if ( paged ) {
                    instructions[INS_NDX(PAR_LOAD_FLASH_PAGE_INST)].execute((uint32_t)buf[base+offset],output,byte_address);
                    if (!buf.isblank(base+offset)) {
                        polling_address = byte_address;
                        polling_value   = (uint32_t)buf[base+offset];
                    }
                } else {
                    instructions[INS_NDX(PAR_READ_FLASH_INST)].execute(0,output,offset);
                    if (output != (uint32_t)buf[base+offset]) {
                        instructions[INS_NDX(PAR_WRITE_FLASH_INST)].execute((uint32_t)buf[base+offset],output,offset);
                        polling_flash_memory(buf[base+offset],offset);
                    }
                }
                offset++;
                this->progress_count++;
            }
            if ( paged ) {
                if (ext_addr != (offset >> 17)) {
                    ext_addr = offset >> 17;
                    instructions[INS_NDX(PAR_LOAD_EXT_ADDR_INST)].execute(ext_addr);
                }
                instructions[INS_NDX(PAR_WRITE_FLASH_PAGE_INST)].execute(page % part_pages);
                polling_flash_memory(polling_value,polling_address);
            }
        }
        if ( offset < this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)] ) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write flash memory at address 0x%04lx",
                base+offset
            )
        );
    }
}

void Avr::polling_eeprom_memory(uint32_t value, uint32_t address)
{
uint32_t    output = 0;
microtime_t start_time;
microtime_t prog_time;

    if ( ( value != eeprom_readBack[0] ) &&
         ( value != eeprom_readBack[1] ) ) {
        start_time = this->io->now();
        do {
            instructions[INS_NDX(PAR_READ_EEPROM_INST)].execute(0,output,address);
            prog_time = this->io->now();
        } while ( ( output != value ) &&
                  ( ( prog_time - start_time ) < wait_delays[WTD_NDX(PAR_WD_EEPROM)] ) );
    } else {
        this->io->usleep(wait_delays[WTD_NDX(PAR_WD_EEPROM)]);
    }
}

void Avr::write_eeprom_memory(DataBuffer& buf)
{
uint32_t    output          = 0;
uint32_t    offset          = 0;
uint32_t    polling_address = 0;
uint32_t    polling_value   = 0;
uint32_t    base            = data_extent.first;
microtime_t start_time;
microtime_t prog_time;

    try {
        bool paged = instructions[INS_NDX(PAR_LOAD_EEPROM_PAGE_INST)] .isValid() &&
                     instructions[INS_NDX(PAR_WRITE_EEPROM_PAGE_INST)].isValid() ;

        for (
            uint32_t page=0;
                     page < this->boundary_sizes[BND_NDX(PAR_EEPROM_PAGES)];
                     page++
        ) {
            for (
                uint32_t byte_address=0;
                         byte_address < this->boundary_sizes[BND_NDX(PAR_EEPROM_PAGE_SIZE)];
                         byte_address++
            ) { 
                progress(base+offset);
                if ( paged ) {
                    instructions[INS_NDX(PAR_LOAD_EEPROM_PAGE_INST)].execute((uint32_t)buf[base+offset],output,byte_address);
                    if (!buf.isblank(base+offset)) {
                        polling_address = byte_address;
                        polling_value   = (uint32_t)buf[base+offset];
                    }
                } else {
                    instructions[INS_NDX(PAR_READ_EEPROM_INST)].execute(0,output,offset);
                    if (output != (uint32_t)buf[base+offset]) {
                        instructions[INS_NDX(PAR_WRITE_EEPROM_INST)].execute((uint32_t)buf[base+offset],output,offset);
                        polling_eeprom_memory(buf[base+offset],offset);
                    }
                }
                offset++;
                this->progress_count++;
            }
            if ( paged ) {
                instructions[INS_NDX(PAR_WRITE_EEPROM_PAGE_INST)].execute(page);
                polling_eeprom_memory(polling_value,polling_address);
            }
        }
        if ( offset < this->boundary_sizes[BND_NDX(PAR_EEPROM_SIZE)] ) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Couldn't write EEPROM memory at address 0x%04lx",
                base+offset
            )
        );
    }
}

void Avr::write_config_bits(DataBuffer& buf)
{
uint32_t value;
uint32_t byte;
uint8_t  tries;

    if (instructions[INS_NDX(PAR_WRITE_FUSE_INST)].isValid()) {
        for (
            byte = config_extent[FUSE_LOW_BITS].first;
            byte < config_extent[FUSE_LOW_BITS].first + config_extent[FUSE_LOW_BITS].second;
            byte++
        ) {
            tries = 0;
            progress(byte);
        
            do {
                value = (uint32_t)buf[byte];
                instructions[INS_NDX(PAR_WRITE_FUSE_INST)].execute(0,value,0);
                this->io->usleep(wait_delays[WTD_NDX(PAR_WD_FUSE)]);
                if (powerOffAftwerWriteFuse) {
                    this->off();
                    this->set_program_mode();
                }
                if (instructions[INS_NDX(PAR_READ_FUSE_INST)].isValid()) {
                    instructions[INS_NDX(PAR_READ_FUSE_INST)].execute(0,value,0);
                }
            } while ( ( value != (uint32_t)buf[byte] ) && ( tries < 3 ) );
        
            this->progress_count++;
        }
    }
    if (instructions[INS_NDX(PAR_WRITE_FUSE_HIGH_INST)].isValid()) {
        for (
            byte = config_extent[FUSE_HIGH_BITS].first;
            byte < config_extent[FUSE_HIGH_BITS].first + config_extent[FUSE_HIGH_BITS].second;
            byte++
        ) {
            tries = 0;
            progress(byte);
        
            do {
                value = (uint32_t)buf[byte];
                instructions[INS_NDX(PAR_WRITE_FUSE_HIGH_INST)].execute(0,value,0);
                this->io->usleep(wait_delays[WTD_NDX(PAR_WD_FUSE)]);
                if (powerOffAftwerWriteFuse) {
                    this->off();
                    this->set_program_mode();
                }
                if (instructions[INS_NDX(PAR_READ_FUSE_HIGH_INST)].isValid()) {
                    instructions[INS_NDX(PAR_READ_FUSE_HIGH_INST)].execute(0,value,0);
                }
            } while ( ( value != (uint32_t)buf[byte] ) && ( tries < 3 ) );
        
            this->progress_count++;
        }
    }
    if (instructions[INS_NDX(PAR_WRITE_EXT_FUSE_INST)].isValid()) {
        for (
            byte = config_extent[EXT_FUSE_BITS].first;
            byte < config_extent[EXT_FUSE_BITS].first + config_extent[EXT_FUSE_BITS].second;
            byte++
        ) {
            tries = 0;
            progress(byte);
        
            do {
                value = (uint32_t)buf[byte];
                instructions[INS_NDX(PAR_WRITE_EXT_FUSE_INST)].execute(0,value,0);
                this->io->usleep(wait_delays[WTD_NDX(PAR_WD_FUSE)]);
                if (powerOffAftwerWriteFuse) {
                    this->off();
                    this->set_program_mode();
                }
                if (instructions[INS_NDX(PAR_READ_EXT_FUSE_INST)].isValid()) {
                    instructions[INS_NDX(PAR_READ_EXT_FUSE_INST)].execute(0,value,0);
                }
            } while ( ( value != (uint32_t)buf[byte] ) && ( tries < 3 ) );
        
            this->progress_count++;
        }
    }
    if (instructions[INS_NDX(PAR_WRITE_LOCK_INST)].isValid()) {
        for (
            byte = config_extent[LOCK_BITS].first;
            byte < config_extent[LOCK_BITS].first + config_extent[LOCK_BITS].second;
            byte++
        ) {
            tries = 0;
            progress(byte);
        
            do {
                value = (uint32_t)buf[byte];
                instructions[INS_NDX(PAR_WRITE_LOCK_INST)].execute(0,value,0);
                this->io->usleep(wait_delays[WTD_NDX(PAR_WD_FUSE)]);
                if (instructions[INS_NDX(PAR_READ_LOCK_INST)].isValid()) {
                    instructions[INS_NDX(PAR_READ_LOCK_INST)].execute(0,value,0);
                }
            } while ( ( value != (uint32_t)buf[byte] ) && ( tries < 3 ) );
        
            this->progress_count++;
        }
    }
}

void Avr::program(DataBuffer& buf)
{
    check_signature();

    check_instruction(PAR_WRITE_FLASH_INST );
    check_instruction(PAR_WRITE_EEPROM_INST);
    check_instruction(PAR_WRITE_FUSE_INST  );
    check_instruction(PAR_WRITE_LOCK_INST  );

    if ( this->boundary_sizes[BND_NDX(PAR_FLASH_PAGES)] > 1 ) {
        check_instruction(PAR_LOAD_FLASH_PAGE_INST );
        check_instruction(PAR_WRITE_FLASH_PAGE_INST);
    }
    if ( this->boundary_sizes[BND_NDX(PAR_EEPROM_PAGES)] > 1 ) {
        check_instruction(PAR_LOAD_EEPROM_PAGE_INST );
        check_instruction(PAR_WRITE_EEPROM_PAGE_INST);
    }

    this->progress_count = 0;
    this->progress_total = this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE )]             +
                           this->boundary_sizes[BND_NDX(PAR_EEPROM_SIZE)]             +
                           instructions[INS_NDX(PAR_WRITE_LOCK_INST     )].isValid() +
                           instructions[INS_NDX(PAR_WRITE_FUSE_INST     )].isValid() +
                           instructions[INS_NDX(PAR_WRITE_FUSE_HIGH_INST)].isValid() +
                           instructions[INS_NDX(PAR_WRITE_EXT_FUSE_INST )].isValid() ;

    try {
        this->set_program_mode();
        this->write_flash_memory (buf);
        this->write_eeprom_memory(buf);
        this->write_config_bits  (buf);
        this->off();
    } catch (std::exception& e) {
        this->off();
        throw;
    }
}

void Avr::read_flash_memory(DataBuffer& buf, bool verify)
{
uint32_t output = 0;
uint32_t offset = 0;
uint32_t base   = code_extent.first;

    try {
        bool     paged      = instructions[INS_NDX(PAR_LOAD_FLASH_PAGE_INST)] .isValid() &&
                              instructions[INS_NDX(PAR_WRITE_FLASH_PAGE_INST)].isValid() ;
        uint32_t part_pages = this->boundary_sizes[BND_NDX(PAR_FLASH_PAGES)];
        uint32_t ext_addr   = 0xffffffff;

        // Let's assume the AVR flash memory is structured as follow:
        // there are w Words per page and p Pages per partition
        // And the partition is addressed by the Extended Address value.
        //
        // As for the data sheets:
        // << The extended address byte is stored until the command is re-issued,
        //    i.e., the command needs only be issued for the first page,
        //    and when crossing the 64KWord boundary. >>
        //
        // Thus we know that a partition is composed by a total of 64K Words (1<<16)
        // or 128Kb (1<<17). We have to base our calculations in bytes as the total
        // memory size is expressed in bytes as well, for this reason we will multiply
        // or divide by a factor of 2^17  
        // 
        if ( this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)] >= (1<<17) ) {
            part_pages /= ( this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)] >> 17 );
        }
        for (
            uint32_t page=0;
                     page < this->boundary_sizes[BND_NDX(PAR_FLASH_PAGES)];
                     page++
        ) {
            for (
                uint32_t byte_address=0;
                         byte_address < this->boundary_sizes[BND_NDX(PAR_FLASH_PAGE_SIZE)];
                         byte_address++
            ) { 
                progress(base+offset);
                instructions[INS_NDX(PAR_READ_FLASH_INST)].execute(0,output,offset);
                if (verify) {
                    if (output != (uint32_t)buf[base+offset]) {
                        throw runtime_error("");
                    }
                } else {
                    buf[base+offset] = (uint8_t)output;
                }
                offset++;
                this->progress_count++;
            }
            if ( paged && (ext_addr != (offset >> 17))) {
                ext_addr = offset >> 17;
                instructions[INS_NDX(PAR_LOAD_EXT_ADDR_INST)].execute(ext_addr);
            }
        }
        if ( offset < this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE)] ) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at address 0x%04lx",
                verify ? "Flash Verification failed"
                       : "Couldn't read Flash memory",
                base+offset
            )
        );
    }
}

void Avr::read_eeprom_memory(DataBuffer& buf, bool verify)
{
uint32_t output = 0;
uint32_t offset = 0;
uint32_t base   = data_extent.first;

    try {
        for (
            uint32_t page=0;
                     page < this->boundary_sizes[BND_NDX(PAR_EEPROM_PAGES)];
                     page++
        ) {
            for (
                uint32_t byte_address=0;
                         byte_address < this->boundary_sizes[BND_NDX(PAR_EEPROM_PAGE_SIZE)];
                         byte_address++
            ) { 
                progress(base+offset);
                instructions[INS_NDX(PAR_READ_EEPROM_INST)].execute(0,output,offset);
                if (verify) {
                    if (output != (uint32_t)buf[base+offset]) {
                        throw runtime_error("");
                    }
                } else {
                    buf[base+offset] = (uint8_t)output;
                }
                offset++;
                this->progress_count++;
            }
        }
        if ( offset < this->boundary_sizes[BND_NDX(PAR_EEPROM_SIZE)] ) {
            throw runtime_error("");
        }
    } catch (std::exception& e) {
        throw runtime_error (
            (const char *)Preferences::Name (
                "%s at address 0x%04lx",
                verify ? "EEPROM Verification failed"
                       : "Couldn't read EEPROM memory",
                base+offset
            )
        );
    }
}

void Avr::read_config_bits(DataBuffer& buf, bool verify)
{
uint32_t value;
uint32_t byte=0;

    if (instructions[INS_NDX(PAR_READ_FUSE_INST)].isValid()) {
        for (
            byte = config_extent[FUSE_LOW_BITS].first;
            byte < config_extent[FUSE_LOW_BITS].first + config_extent[FUSE_LOW_BITS].second;
            byte++
        ) {
            try {
                progress(byte);
        
                instructions[INS_NDX(PAR_READ_FUSE_INST)].execute(0,value,0);
        
                if (verify) {
                    if (value != (uint32_t)buf[byte]) {
                        throw runtime_error("");
                    } 
                } else {
                    buf[byte] = (uint8_t)value;
                }
                this->progress_count++;
            } catch (std::exception& e) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "%s at address 0x%04lx",
                        verify ? "Fuse Byte Verification failed"
                               : "Couldn't read Fuse Byte",
                        byte
                    )
                );
            }
        }
    }
    if (instructions[INS_NDX(PAR_READ_FUSE_HIGH_INST)].isValid()) {
        for (
            byte = config_extent[FUSE_HIGH_BITS].first;
            byte < config_extent[FUSE_HIGH_BITS].first + config_extent[FUSE_HIGH_BITS].second;
            byte++
        ) {
            try {
                progress(byte);
        
                instructions[INS_NDX(PAR_READ_FUSE_HIGH_INST)].execute(0,value,0);
        
                if (verify) {
                    if (value != (uint32_t)buf[byte]) {
                        throw runtime_error("");
                    } 
                } else {
                    buf[byte] = (uint8_t)value;
                }
                this->progress_count++;
            } catch (std::exception& e) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "%s at address 0x%04lx",
                        verify ? "Fuse High Byte Verification failed"
                               : "Couldn't read Fuse High Byte",
                        byte
                    )
                );
            }
        }
    }
    if (instructions[INS_NDX(PAR_READ_EXT_FUSE_INST)].isValid()) {
        for (
            byte = config_extent[EXT_FUSE_BITS].first;
            byte < config_extent[EXT_FUSE_BITS].first + config_extent[EXT_FUSE_BITS].second;
            byte++
        ) {
            try {
                progress(byte);
        
                instructions[INS_NDX(PAR_READ_EXT_FUSE_INST)].execute(0,value,0);
        
                if (verify) {
                    if (value != (uint32_t)buf[byte]) {
                        throw runtime_error("");
                    } 
                } else {
                    buf[byte] = (uint8_t)value;
                }
                this->progress_count++;
            } catch (std::exception& e) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "%s at address 0x%04lx",
                        verify ? "Ext Fuse Byte Verification failed"
                               : "Couldn't read Ext Fuse Byte",
                        byte
                    )
                );
            }
        }
    }
    if (instructions[INS_NDX(PAR_READ_LOCK_INST)].isValid()) {
        for (
            byte = config_extent[LOCK_BITS].first;
            byte < config_extent[LOCK_BITS].first + config_extent[LOCK_BITS].second;
            byte++
        ) {
            try {
                progress(byte);
        
                instructions[INS_NDX(PAR_READ_LOCK_INST)].execute(0,value,0);
        
                if (verify) {
                    if (value != (uint32_t)buf[byte]) {
                        throw runtime_error("");
                    } 
                } else {
                    buf[byte] = (uint8_t)value;
                }
                this->progress_count++;
            } catch (std::exception& e) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "%s at address 0x%04lx",
                        verify ? "Lock Byte Verification failed"
                               : "Couldn't read Lock Byte",
                        byte
                    )
                );
            }
        }
    }
}

void Avr::read_calibration(DataBuffer& buf, bool verify)
{
uint32_t value;
uint32_t base = calib_extent.first; 

    if (instructions[INS_NDX(PAR_READ_CALIBRATION_INST)].isValid()) {
        for (int byte=0; byte<calib_extent.second; byte++) {
            try {
                progress(base+byte);
        
                instructions[INS_NDX(PAR_READ_CALIBRATION_INST)].execute(0,value,byte);
        
                if (verify) {
                    if (value != (uint32_t)buf[base+byte]) {
                        throw runtime_error("");
                    } 
                } else {
                    buf[base+byte] = (uint8_t)value;
                }
                this->progress_count++;
            } catch (std::exception& e) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "%s at address 0x%04lx",
                        verify ? "Calibration Byte Verification failed"
                               : "Couldn't read Calibration Byte",
                        base+byte
                    )
                );
            }
        }
    }
}

void Avr::read(DataBuffer& buf, bool verify)
{
    check_signature();

    check_instruction(PAR_READ_FLASH_INST );
    check_instruction(PAR_READ_EEPROM_INST);
    check_instruction(PAR_READ_FUSE_INST  );
    check_instruction(PAR_READ_LOCK_INST  );

    if ( this->boundary_sizes[BND_NDX(PAR_FLASH_PAGES)] > 1 ) {
        check_instruction(PAR_LOAD_FLASH_PAGE_INST );
    }
    if ( this->boundary_sizes[BND_NDX(PAR_EEPROM_PAGES)] > 1 ) {
        check_instruction(PAR_LOAD_EEPROM_PAGE_INST );
    }

    this->progress_count = 0;
    this->progress_total = this->boundary_sizes[BND_NDX(PAR_FLASH_SIZE )]           +
                           this->boundary_sizes[BND_NDX(PAR_EEPROM_SIZE)]           +
                           calibration_bytes[CAL_NDX(PAR_CALIBRATION_BYTES)]        +
                           instructions[INS_NDX(PAR_READ_LOCK_INST     )].isValid() +
                           instructions[INS_NDX(PAR_READ_FUSE_INST     )].isValid() +
                           instructions[INS_NDX(PAR_READ_FUSE_HIGH_INST)].isValid() +
                           instructions[INS_NDX(PAR_READ_EXT_FUSE_INST )].isValid() ;

    try {
        this->set_program_mode();
        if ( ! verify ) {
            this->read_config_bits(buf, verify);
            this->read_calibration(buf, verify);
        }
        this->read_flash_memory (buf, verify);
        this->read_eeprom_memory(buf, verify);
        this->off();
    } catch (std::exception& e) {
        this->off();
        throw;
    }
}

void Avr::dump(DataBuffer& buf)
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
            if (start==code_extent.first && len==code_extent.second) {
                // code area, need to disassemble
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Program memory"
                    );
                }
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
                            (addr-start) << byte_address,
                            buf[addr] & 0xffff,
                            disasm
                        );
                        num_words--;
                    }
                }
                addr++;
            } else if (start==config_extent[FUSE_LOW_BITS].first && len==config_extent[FUSE_LOW_BITS].second) {
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Fuses (Low) bits"
                    );
                }
                line[0]='\0';
                for (i=0; i<BITS_IN_FUSES; i++) {
                    if (!(buf[addr] & (1<<i))) { // bit set when 0
                        strcat(line,bitNames[FUSE_LOW_BITS][i]);
                        strcat(line,",");
                    }
                }
                if (line[strlen(line)-1]==',') {
                    line[strlen(line)-1]='\0';
                } else {
                    strcat(line,"none");
                }
            } else if (start==config_extent[FUSE_HIGH_BITS].first && len==config_extent[FUSE_HIGH_BITS].second) {
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Fuses High bits"
                    );
                }
                line[0]='\0';
                for (i=0; i<BITS_IN_FUSES; i++) {
                    if (!(buf[addr] & (1<<i))) { // bit set when 0
                        strcat(line,bitNames[FUSE_HIGH_BITS][i]);
                        strcat(line,",");
                    }
                }
                if (line[strlen(line)-1]==',') {
                    line[strlen(line)-1]='\0';
                } else {
                    strcat(line,"none");
                }
            } else if (start==config_extent[EXT_FUSE_BITS].first && len==config_extent[EXT_FUSE_BITS].second) {
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Ext Fuses bits"
                    );
                }
                line[0]='\0';
                for (i=0; i<BITS_IN_FUSES; i++) {
                    if (!(buf[addr] & (1<<i))) { // bit set when 0
                        strcat(line,bitNames[EXT_FUSE_BITS][i]);
                        strcat(line,",");
                    }
                }
                if (line[strlen(line)-1]==',') {
                    line[strlen(line)-1]='\0';
                } else {
                    strcat(line,"none");
                }
            } else if (start==config_extent[LOCK_BITS].first && len==config_extent[LOCK_BITS].second) {
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Locks bits"
                    );
                }
                line[0]='\0';
                for (i=0; i<BITS_IN_FUSES; i++) {
                    if (!(buf[addr] & (1<<i))) { // bit set when 0
                        strcat(line,bitNames[LOCK_BITS][i]);
                        strcat(line,",");
                    }
                }
                if (line[strlen(line)-1]==',') {
                    line[strlen(line)-1]='\0';
                } else {
                    strcat(line,"none");
                }
            } else if (start==calib_extent.first && len==calib_extent.second) {
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Calibration bytes"
                    );
                }
                sprintf(line,"%07x) %02x", addr-start, buf[addr]);
            } else if (start==signature_extent.first && len==signature_extent.second) {
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0Signature"
                    );
                }
                sprintf(line,"%07x) %02x", addr-start, buf[addr]);
            } else {
                // data area, do not disassemble
                //
                if (put_separator) {
                    snprintf (
                        title, sizeof(title),
                        "@C255@_@b@C0EEPROM"
                    );
                }
                sprintf(line,"%07x)", (addr-start) << byte_address);
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

// Set default address field width for this version.
//
#define ADDRESS_FIELD_WIDTH                     3

// AVR Operand prefixes
// In order to keep the disassembler as straightforward as possible,
// but still have some fundamental formatting options, I decided to hardcode
// the operand prefixes. Feel free to change them here.
//
#define OPERAND_PREFIX_REGISTER                 "R"     // i.e. mov R0, R2
#define OPERAND_PREFIX_DATA_HEX                 "0x"    // i.e. ldi R16, 0x3D
#define OPERAND_PREFIX_DATA_BIN                 "0b"    // i.e. ldi R16, 0b00111101
#define OPERAND_PREFIX_DATA_DEC                 ""      // i.e. ldi R16, 61
#define OPERAND_PREFIX_BIT                      ""      // i.e. bset 7
#define OPERAND_PREFIX_IO_REGISTER              "$"     // i.e. out $39, R16
#define OPERAND_PREFIX_ABSOLUTE_ADDRESS         "0x"    // i.e. call 0x23
#define OPERAND_PREFIX_BRANCH_ADDRESS           "."     // i.e. rcall .+4
#define OPERAND_PREFIX_WORD_DATA                "0x"    // i.e. .DW 0x1234

int Avr::mem2asm(int addr, DataBuffer& buf, char *buffer, size_t sizeof_buffer)
{
int num_words = 1;
const assembledInstruction aInstruction = { addr, buf[addr] };
disassembledInstruction dInstruction;
size_t pos = 0;
int i;

    if ( disassembleInstruction(&dInstruction, aInstruction) != 0 ) {
        return 0;
    }

    //-------------------------------------------------------------------------------------//
    //-- The following code has been copied from the file vavrdisasm-1.4/format_disasm.c --//
    //-------------------------------------------------------------------------------------//

    // If we just found a long instruction, there is nothing to be printed yet, since we don't
    // have the entire long address ready yet.
    // 
    if (AVR_Long_Instruction == AVR_LONG_INSTRUCTION_FOUND) {
        return 0;
    }
    
    // Just print the address that the instruction is located at, without address labels.
    // 
    Util::snprintfcat (
        pos, buffer, sizeof_buffer,
        "%4X:\t%s ",
        dInstruction.address,
        dInstruction.instruction->mnemonic
    );

    for (i = 0; i < dInstruction.instruction->numOperands; i++) {
        // If we're not on the first operand, but not on the last one either, print a comma separating
        // the operands.
        // 
        if (i > 0 && i != dInstruction.instruction->numOperands) {
            Util::snprintfcat(pos, buffer, sizeof_buffer, ", ");
        }

        // Formats a disassembled operand with its prefix (such as 'R' to indicate a register).
        // I decided to format the disassembled operands individually into strings for maximum flexibility,
        // and so that the printing of the formatted operand is not hard coded into the format operand code.
        // If an addressLabelPrefix is specified in formattingOptions (option is set and string is not NULL), 
        // it will print the relative branch/jump/call with this prefix and the destination address as the label.
        // 
        switch (dInstruction.instruction->operandTypes[i]) {
            case OPERAND_NONE:
            case OPERAND_REGISTER_GHOST:
                break;
            case OPERAND_REGISTER:
            case OPERAND_REGISTER_STARTR16:
            case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
            case OPERAND_REGISTER_EVEN_PAIR:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "%s%d",
                    OPERAND_PREFIX_REGISTER,
                    dInstruction.operands[i]
                );
                break;
            case OPERAND_DATA:
            case OPERAND_COMPLEMENTED_DATA:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "%s%02X",
                    OPERAND_PREFIX_DATA_HEX,
                    dInstruction.operands[i]
                );
                break;
            case OPERAND_BIT:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "%s%d",
                    OPERAND_PREFIX_BIT,
                    dInstruction.operands[i]
                );
                break;
            case OPERAND_BRANCH_ADDRESS:
            case OPERAND_RELATIVE_ADDRESS:
                // If we have an address label, print it, otherwise just print the
                // relative distance to the destination address.
                // Check if the operand is greater than 0 so we can print the + sign.
                if (dInstruction.operands[i] > 0) {
                    Util::snprintfcat (
                        pos, buffer, sizeof_buffer,
                        "%s+%d",
                        OPERAND_PREFIX_BRANCH_ADDRESS,
                        dInstruction.operands[i]
                    );
                } else {
                    // Since the operand variable is signed, the negativeness of the distance
                    // to the destination address has been taken care of in disassembleOperands()
                    Util::snprintfcat (
                        pos, buffer, sizeof_buffer,
                        "%s%d",
                        OPERAND_PREFIX_BRANCH_ADDRESS,
                        dInstruction.operands[i]
                    );
                }
                break;
            case OPERAND_LONG_ABSOLUTE_ADDRESS:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "%s%0*X",
                    OPERAND_PREFIX_ABSOLUTE_ADDRESS,
                    ADDRESS_FIELD_WIDTH,
                    AVR_Long_Address
                );
                break;
            case OPERAND_IO_REGISTER:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "%s%02X",
                    OPERAND_PREFIX_IO_REGISTER,
                    dInstruction.operands[i]
                );
                break;  
            case OPERAND_WORD_DATA:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "%s%0*X",
                    OPERAND_PREFIX_WORD_DATA,
                    ADDRESS_FIELD_WIDTH,
                    dInstruction.operands[i]
                );
                break;
            case OPERAND_YPQ:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "Y+%d",
                    dInstruction.operands[i]
                );
                break;
            case OPERAND_ZPQ:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "Z+%d",
                    dInstruction.operands[i]
                );
                break;
            case OPERAND_X:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "X"
                );
                break;
            case OPERAND_XP:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "X+"
                );
                break;
            case OPERAND_MX:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "-X"
                );
                break;
            case OPERAND_Y:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "Y"
                );
                break;
            case OPERAND_YP:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "Y+"
                );
                break;
            case OPERAND_MY:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "-Y"
                );
                break;
            case OPERAND_Z:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "Z"
                );
                break;
            case OPERAND_ZP:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "Z+"
                );
                break;
            case OPERAND_MZ:
                Util::snprintfcat (
                    pos, buffer, sizeof_buffer,
                    "-Z"
                );
                break;

            default:
                // This is impossible by normal operation.
                break;
        }
    }

    // Append a comment specifying what address the rjmp or rcall would take us,
    // i.e. rcall .+4 ; 0x56
    // 
    for (i = 0; i < dInstruction.instruction->numOperands; i++) {
        // This is only done for operands with relative addresses,
        // where the destination address is relative from the address 
        // the PC is on.
        //
        if (dInstruction.instruction->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
            dInstruction.instruction->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
            Util::snprintfcat (
                pos, buffer, sizeof_buffer,
                "%1s\t; %s%X",
                "",
                OPERAND_PREFIX_ABSOLUTE_ADDRESS,
                dInstruction.address+dInstruction.operands[i]+2
            );
            // Stop after one relative address operand
            //
            break;
        }
    }
    Util::snprintfcat(pos, buffer, sizeof_buffer, "\n");

    return num_words;
}

bool Avr::check(void)
{
    check_signature();

    return true;
}

bool Avr::verifyConfig(Preferences &device, bool raise_exceptions)
{
bool ok           = true;
int  eeprom_pages = 0;
int  flash_pages  = 0;
int  calib_bytes  = 0;
int  r = 0;

    // Checking Vcc Min and Vcc Max
    {
    double dv;

        r = device.get(sparams[PAR_VCC_MIN].name,dv,(double)((int)sparams[PAR_VCC_MIN].defv)) ? 1 : 0;
        if (!r || dv<=0.0) {
            if (raise_exceptions) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "Configuration Error: %s missing or wrong (%lf)",
                        sparams[PAR_VCC_MIN].name,
                        dv
                    )
                );
            }
            ok = false;
        }

        r = device.get(sparams[PAR_VCC_MAX].name,dv,(double)((int)sparams[PAR_VCC_MAX].defv)) ? 1 : 0;
        if (!r || dv<=0.0) {
            if (raise_exceptions) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "Configuration Error: %s missing or wrong (%lf)",
                        sparams[PAR_VCC_MAX].name,
                        dv
                    )
                );
            }
            ok = false;
        }
    }

    // Checking Wait Delays
    {
    int i;
    int iv[PAR_WAIT_DELAYS_END-PAR_WAIT_DELAYS_START];

        for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
            r = device.get(sparams[i].name,iv[i-PAR_WAIT_DELAYS_START],(int)sparams[i].defv) ? 1 : 0;
            if (!r || iv[i-PAR_WAIT_DELAYS_START]<=0) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or wrong (%d)",
                            sparams[i].name,
                            iv[i-PAR_WAIT_DELAYS_START]
                        )
                    );
                }
                ok = false;
            }
        }
    }
    // Checking Size Boundaries
    {
    int i;
    int iv[PAR_SIZE_BOUNDARIES_END-PAR_SIZE_BOUNDARIES_START];

        for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
            r = device.get(sparams[i].name,iv[i-PAR_SIZE_BOUNDARIES_START],(int)sparams[i].defv) ? 1 : 0;
            if (!r || iv[i-PAR_SIZE_BOUNDARIES_START]<=0) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or wrong (%d)",
                            sparams[i].name,
                            iv[i-PAR_SIZE_BOUNDARIES_START]
                        )
                    );
                }
                ok = false;
            }
        }
        if ( ( iv[PAR_FLASH_PAGE_SIZE - PAR_SIZE_BOUNDARIES_START] *
               iv[PAR_FLASH_PAGES     - PAR_SIZE_BOUNDARIES_START] ) !=
               iv[PAR_FLASH_SIZE      - PAR_SIZE_BOUNDARIES_START]   ) {
            if (raise_exceptions) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "Configuration Error: Wrong Flash memory configuration"
                    )
                );
            }
            ok = false;
        } else {
            flash_pages = iv[PAR_FLASH_PAGES - PAR_SIZE_BOUNDARIES_START];
        }
        if ( ( iv[PAR_EEPROM_PAGE_SIZE - PAR_SIZE_BOUNDARIES_START] *
               iv[PAR_EEPROM_PAGES     - PAR_SIZE_BOUNDARIES_START] ) !=
               iv[PAR_EEPROM_SIZE      - PAR_SIZE_BOUNDARIES_START]   ) {
            if (raise_exceptions) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "Configuration Error: Wrong EEPROM configuration"
                    )
                );
            }
            ok = false;
        } else {
            eeprom_pages = iv[PAR_EEPROM_PAGES - PAR_SIZE_BOUNDARIES_START];
        }
    }
    // Checking Signature bytes
    {
    int i;

        for (i=PAR_SIGN_B0; i<=PAR_SIGN_B2; i++) {
            if ( ! validateString(device,i,validHexDigit,2) ) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[i].name
                        )
                    );
                }
                ok = false;
            }
        }
    }
    // Checking Calibration Bytes
    {
    int i;

        r = device.get(sparams[PAR_CALIBRATION_BYTES].name,
                       i,
                       (int)sparams[PAR_CALIBRATION_BYTES].defv) ? 1 : 0;
        if (!r || i<0) {
            if (raise_exceptions) {
                throw runtime_error (
                    (const char *)Preferences::Name (
                        "Configuration Error: %s missing or invalid (%d)",
                        sparams[PAR_CALIBRATION_BYTES].name,
                        i
                    )
                );
            }
            ok = false;
        } else {
            calib_bytes = i;
        }
    }
    // Checking Mandatory Instructions
    {
    int i;

        for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
            if ( ! validateString(device,i,validInstructionChars,32) ) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[i].name
                        )
                    );
                }
                ok = false;
            }
        }
    }
    // Checking Optional Instructions
    {
    int i;
    bool l1,l2,l3;

        for (i=PAR_OPTIONAL_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
            if ( ! validateString(device,i,validInstructionChars,32,true) ) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[i].name
                        )
                    );
                }
                ok = false;
            }
        }
        if (flash_pages>1) {
            l1 = validateString(device,PAR_LOAD_EXT_ADDR_INST   ,validInstructionChars,32);
            l2 = validateString(device,PAR_LOAD_FLASH_PAGE_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_FLASH_PAGE_INST,validInstructionChars,32);

            if ((flash_pages > 0xffff) && !l1) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[PAR_LOAD_EXT_ADDR_INST].name
                        )
                    );
                }
                ok = false;
            }
            if (!l2) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[PAR_LOAD_FLASH_PAGE_INST].name
                        )
                    );
                }
                ok = false;
            }
            if (!l3) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[PAR_WRITE_FLASH_PAGE_INST].name
                        )
                    );
                }
                ok = false;
            }
        }
        if (eeprom_pages>1) {
            l2 = validateString(device,PAR_LOAD_EEPROM_PAGE_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_EEPROM_PAGE_INST,validInstructionChars,32);

            if (!l2) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[PAR_LOAD_EEPROM_PAGE_INST].name
                        )
                    );
                }
                ok = false;
            }
            if (!l3) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[PAR_WRITE_EEPROM_PAGE_INST].name
                        )
                    );
                }
                ok = false;
            }
        }
        if (calib_bytes>0) {
            l1 = validateString(device,PAR_READ_CALIBRATION_INST,NULL,32);

            if (!l1) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: %s missing or invalid",
                            sparams[PAR_READ_CALIBRATION_INST].name
                        )
                    );
                }
                ok = false;
            }
        }
        // Cross checking Fuse High Instructions
        {
            l2 = validateString(device,PAR_READ_FUSE_HIGH_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_FUSE_HIGH_INST,validInstructionChars,32);

            if (l2 != l3) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: Mismatch between %s and %s",
                            sparams[PAR_READ_FUSE_HIGH_INST ].name,
                            sparams[PAR_WRITE_FUSE_HIGH_INST].name
                        )
                    );
                }
                ok = false;
            }
        }
        // Cross checking Extended Fuse Instructions
        {
            l2 = validateString(device,PAR_READ_EXT_FUSE_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_EXT_FUSE_INST,validInstructionChars,32);

            if (l2 != l3) {
                if (raise_exceptions) {
                    throw runtime_error (
                        (const char *)Preferences::Name (
                            "Configuration Error: Mismatch between %s and %s",
                            sparams[PAR_READ_EXT_FUSE_INST ].name,
                            sparams[PAR_WRITE_EXT_FUSE_INST].name
                        )
                    );
                }
                ok = false;
            }
        }
    }
    return ok;
}

bool Avr::validateString(Preferences &device, int param, const char *range, unsigned int size, bool can_be_empty)
{
char buf[PAR_LENGTH_MAX];
int  text_size;

    if (!size) {
        return false;
    }

    if (!device.get(sparams[param].name,buf,(char*)sparams[param].defv,sizeof(buf)-1)) {
        return false;
    }

    if (((text_size = strlen(buf)) != size) || (can_be_empty && (text_size!=0))) {
        return false;
    }

    if (text_size == 0) {
        return true;
    }

    if (range && range[0]!='\0') {
        for (int j=0; j<text_size; j++) {
            bool match = false;
            for (int i=0; range[i] != '\0'; i++) {
                if ( range[i] == buf[j] ) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                return false;
            }
        }
    }
    return true;
}

void Avr::set_config_default(DataBuffer& buf)
{
}

void Avr::set_config_word(int i, uint32_t word, DataBuffer& buf)
{
    if (i<0 || i>LAST_BIT_NAME) {
        return;
    }
    for (int i=0; i<config_extent[i].second && i<sizeof(word); i++) {
        buf[config_extent[i].first+i] = word & 0x000000ff;
        word >>= 8;
    }
}

Avr::Instruction::Instruction()
{
    bits_mask = 0x00000000; // bits set
    msba_mask = 0x00000000; // bits for the address' MSB
    lsba_mask = 0x00000000; // bits for the address' LSB
    hldb_mask = 0x00000000; // bit(s) to select the High/Low byte in the memory
    dout_mask = 0x00000000; // data output bits
    dinp_mask = 0x00000000; // data input bits

    valid     = false;
}

bool Avr::Instruction::parse(const char *string)
{
    if (strlen(string) < 32) {
        return false;
    }
    uint32_t l_bits_mask = 0x00000000;
    uint32_t l_msba_mask = 0x00000000;
    uint32_t l_lsba_mask = 0x00000000;
    uint32_t l_hldb_mask = 0x00000000;
    uint32_t l_dout_mask = 0x00000000;
    uint32_t l_dinp_mask = 0x00000000;

    for (int i=0; i<32; i++) {
        switch (string[i]) {
            case '0':
                    // do nothing, the bits are already initialized to 0
                break;
            case 'x':
            case '1':
                    l_bits_mask |= 1 << i; 
                break;
            case 'a':
                    l_msba_mask |= 1 << i; 
                break;
            case 'b':
                    l_lsba_mask |= 1 << i; 
                break;
            case 'H':
                    l_hldb_mask |= 1 << i; 
                break;
            case 'o':
                    l_dout_mask |= 1 << i; 
                break;
            case 'i':
                    l_dinp_mask |= 1 << i; 
                break;
            default :
                    return false;
                break;
        }
    }
    bits_mask = l_bits_mask;
    msba_mask = l_msba_mask;
    lsba_mask = l_lsba_mask;
    hldb_mask = l_hldb_mask;
    dout_mask = l_dout_mask;
    dinp_mask = l_dinp_mask;
    valid     = true;

    return true;
}

uint32_t Avr::Instruction::execute()
{
    return execute(0);
}

uint32_t Avr::Instruction::execute(uint32_t address)
{
uint32_t dummy;

    return execute(0, dummy, address);
}

uint32_t Avr::Instruction::execute(uint32_t input, uint32_t &output, uint32_t address)
{
uint32_t  risp = 0, inst = bits_mask;
uint8_t   offset = 0;

    if ( ! valid ) {
        return risp;
    }

    // Set the Low/High byte selection
    if (hldb_mask) {
        if (address & 0x00000001) {
            inst |=  hldb_mask; // High byte selection
        } else {
            inst &= ~hldb_mask; // Low byte selection
        }
        address >>= 1;
    }

    // Set the byte address
    if (msba_mask && lsba_mask) {
        offset = setBits(inst, lsba_mask, address);
        setBits(inst, msba_mask, address, offset);
    } else if (msba_mask) {
        setBits(inst, msba_mask, address);
    } else if (lsba_mask) {
        setBits(inst, lsba_mask, address);
    }

    // Set the input value
    setBits(inst, dinp_mask, input);

    // Send the command to the chip
    risp = ((Avr*)this)->io->shift_bits_out_in(inst,32);

    // Extract the output value from the chip response
    getBits(output, dout_mask, risp);

    return risp;
}

uint8_t Avr::Instruction::setBits(uint32_t &inst, uint32_t mask, uint32_t data, uint8_t offset)
{
uint8_t count = 0;

    data >> offset;              // skip already processed bits
    for (int i=0; (mask!=0) && (i<32); i++) {
        uint32_t bit = ( 1 << i );
        if ( mask & bit ) {
            inst &= ~bit;        // clear the bit
            if ( data & 0x01 ) {
                inst |= bit;     // set the bit
            }
            data >> 1;           // select next bit
            count++;             // count how many bits are in the mask
        }
    }
    return count;
}

uint8_t Avr::Instruction::getBits(uint32_t &data, uint32_t mask, uint32_t risp)
{
uint8_t count = 0;

    for (int i=0; (mask!=0) && (i<32); i++) {
        uint32_t bit = ( 1 << i );
        if ( mask & bit ) {
            data <<= 1;
            if (risp & bit) {
                data |= 1;       // set the bit
            } else {
                data &= ~1;      // clear the bit
            }
            count++;             // count how many bits are in the mask
        }
    }
    return count;
}
