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
#ifndef proto_flP5_h
#define proto_flP5_h

#include "Preferences.h"
#include "HexFile.h"
#include "DataBuffer.h"
#include "Device.h"
#include "IO.h"

typedef enum DEV_PARAM {
    PAR_WORD_SIZE=0,
    PAR_CODE_SIZE,
    PAR_EEPROM_SIZE,
    PAR_PROG_COUNT,
    PAR_PROG_MULT,
    PAR_PROG_TIME,
    PAR_ERASE_TIME,
    PAR_VPP_MIN,
    PAR_VPP_MAX,
    PAR_VDD_MIN,
    PAR_VDD_MAX,
    PAR_VDDP_MIN,
    PAR_VDDP_MAX,
    LAST_PARAM
} DevParam;

#define CFG_OPER_FIRST 0
typedef enum CFG_OPER {
    CFG_NEW=CFG_OPER_FIRST,
    CFG_EDIT,
    CFG_COPY,
    CFG_SAVE,
    CFG_DELETE,
    CFG_LOAD,
    CFG_IMPORT,
    CFG_NONE
} CfgOper;

#define PB_OPER_FIRST 0
typedef enum PB_OPER {
    PB_READ=PB_OPER_FIRST,
    PB_ERASE,
    PB_BLANCK_CHECK,
    PB_WRITE,
    PB_VERIFY,
    PB_TEST,
    PB_RESET,
    PB_CALIBRATE,
    PB_NONE
} PbOper;

#define CHIP_OPER_FIRST 0
typedef enum CHIP_OPER {
    CHIP_READ=CHIP_OPER_FIRST,
    CHIP_ERASE,
    CHIP_BLANCK_CHECK,
    CHIP_WRITE,
    CHIP_VERIFY,
    CHIP_VERIFY_MIN,
    CHIP_VERIFY_MAX,
    CHIP_TEST_ON,
    CHIP_TEST_OFF,
    CHIP_TEST_RESET,
    CHIP_TEST_RESET_ON,
    CHIP_TEST_RESET_OFF,
    CHIP_CALIBRATE,
    CHIP_NONE
} ChipOper;

#define PP_PINS_FIRST 0
typedef enum PP_PINS {
    ICSP_CLOCK=PP_PINS_FIRST,
    ICSP_DATA_IN,
    ICSP_DATA_OUT,
    ICSP_VDD_ON,
    ICSP_VPP_ON,
    SEL_MIN_VDD,
    SEL_PRG_VDD,
    SEL_MAX_VDD,
    SEL_VIHH_VPP,
    LAST_PIN
} PpPins;

extern bool verifyDeviceConfig(bool verbose);
extern bool loadDevicesSettings(const char *fname);
extern bool deviceConfigCB(CfgOper oper);
extern bool verifyProgrammerConfig(bool verbose);
extern bool loadProgrammersSettings(const char *fname);
extern bool programmerConfigCB(CfgOper oper);
extern void loadPreferences(void);
extern bool cfgWordsCB(CfgOper oper);
extern bool showMemoryDumpCB(void *data,const char *line,int progress);
extern void dumpHexFile(bool set_wordsize);
extern bool processOperation(ChipOper oper);
extern void loadHexFile(void);
extern void saveHexFile(void);
extern void showManual(void);
extern bool checkRegistration(unsigned int prodCode, unsigned int regKey);

extern int currentDevice;
extern int currentProgrammer;

extern DataBuffer buf;
extern HexFile *hexFile;
extern Device *chip;
extern IO *io;

extern Preferences app;
extern Preferences programmers;
extern Preferences devices;

extern char *copyrightText;

#endif
