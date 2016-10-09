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
#include "AvrUI.h"
#include <string.h>
#include <FL/fl_ask.H>
#include <FL/filename.H>

#include "flP5.h"

#define DEVICE_VENDOR "Atmel"
#define DEVICE_SPEC   "AVR"
#define DEVICE_TYPE   "Atmel/AVR"

Fl_Input        *AvrUI::tx_Param[AvrUI::LAST_PARAM] = { 0 };
Fl_Group        *AvrUI::g_Settings                  = 0;
Fl_Group        *AvrUI::g_gencfg                    = 0;
Fl_Group        *AvrUI::g_flashcfg                  = 0;
Fl_Group        *AvrUI::g_eepromcfg                 = 0;
Fl_Group        *AvrUI::g_fusecfg                   = 0;
Fl_Group        *AvrUI::g_misccfg                   = 0;
Fl_Check_Button *AvrUI::tb_powerOffAfterWrite       = 0;
Fl_Check_Button *AvrUI::tb_Experimental             = 0;
const char      *AvrUI::validInstructionChars       = "01abHoix";
const char      *AvrUI::validHexDigit               = "0123456789ABCDEF";
const char      *AvrUI::instructionsFormatTips      = "\
32 bits Instruction word:\
\n - 0|1 = bit set to 0 or 1\
\n - a   = address' high bit\
\n - b   = address' low bit\
\n - H   = 0-low byte, 1-high byte\
\n - o   = data output bit\
\n - i   = data input bit\
\n - x   = don\'t care bit\
";

static t_NamedSettings sparams[] = {
    "vccMin"                          , (void*)0,
    "vccMax"                          , (void*)0,
    "waitDelays.reset"                , (void*)0,
    "waitDelays.erase"                , (void*)0,
    "waitDelays.flash"                , (void*)0,
    "waitDelays.eeprom"               , (void*)0,
    "waitDelays.fuse"                 , (void*)0,
    "flash.size"                      , (void*)0,
    "flash.pageSize"                  , (void*)0,
    "flash.pages"                     , (void*)1,
    "eeprom.size"                     , (void*)0,
    "eeprom.pageSize"                 , (void*)0,
    "eeprom.pages"                    , (void*)1,
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

bool AvrUI::match(const char *deviceSpec)
{
    return ( strncmp(deviceSpec, DEVICE_TYPE, sizeof(DEVICE_TYPE)-1) == 0 );
}

bool AvrUI::match(const char *vendor, const char *spec)
{
    return ( ( strncmp(vendor, DEVICE_VENDOR, sizeof(DEVICE_VENDOR)-1) == 0 ) &&
             ( strncmp(spec  , DEVICE_SPEC  , sizeof(DEVICE_SPEC  )-1) == 0 ) );
}

bool AvrUI::verifyConfig(const char *device, bool verbose)
{
bool ok           = true;
int  eeprom_pages = 0;
int  flash_pages  = 0;
int  calib_bytes  = 0;

    if (!match(device)) {
        return false;
    }
    // Checking Vcc Min and Vcc Max
    {
    double dv[2];

        tx_Param[PAR_VCC_MIN]->color(FL_WHITE);
        if (sscanf(tx_Param[PAR_VCC_MIN]->value(),"%lf",&dv[0])!=1 || dv[0]<=0.0) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s value must be a positive (>0) float.",
                    tx_Param[PAR_VCC_MIN]->label()
                );
            }
            tx_Param[PAR_VCC_MIN]->color(FL_YELLOW);
        }

        tx_Param[PAR_VCC_MAX]->color(FL_WHITE);
        if (sscanf(tx_Param[PAR_VCC_MAX]->value(),"%lf",&dv[1])!=1 || dv[1]<dv[0]) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s value must be greater or equal to the %s value.",
                    tx_Param[PAR_VCC_MAX]->label(),
                    tx_Param[PAR_VCC_MIN]->label()
                );
            }
            tx_Param[PAR_VCC_MAX]->color(FL_YELLOW);
            tx_Param[PAR_VCC_MIN]->color(FL_YELLOW);
        }
        tx_Param[PAR_VCC_MAX]->redraw();
        tx_Param[PAR_VCC_MIN]->redraw();
    }
    // Checking Wait Delays
    {
    int i;
    int iv[PAR_WAIT_DELAYS_END-PAR_WAIT_DELAYS_START];

        for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
            tx_Param[i]->color(FL_WHITE);
            if (sscanf(tx_Param[i]->value(),"%d",&iv[i-PAR_WAIT_DELAYS_START])!=1 ||
                iv[i-PAR_WAIT_DELAYS_START]<=0) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "The %s value must be a non negative (>=0) integer.",
                        tx_Param[i]->label()
                    );
                }
                tx_Param[i]->color(FL_YELLOW);
            }
            tx_Param[i]->redraw();
        }
    }
    // Checking Size Boundaries
    {
    int i;
    int iv[PAR_SIZE_BOUNDARIES_END-PAR_SIZE_BOUNDARIES_START];

        for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
            tx_Param[i]->color(FL_WHITE);
            if (sscanf(tx_Param[i]->value(),"%d",&iv[i-PAR_SIZE_BOUNDARIES_START])!=1 ||
                iv[i-PAR_SIZE_BOUNDARIES_START]<=0) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "The %s value must be a non negative (>=0) integer.",
                        tx_Param[i]->label()
                    );
                }
                tx_Param[i]->color(FL_YELLOW);
            }
        }
        if ( ( iv[PAR_FLASH_PAGE_SIZE - PAR_SIZE_BOUNDARIES_START] *
               iv[PAR_FLASH_PAGES     - PAR_SIZE_BOUNDARIES_START] ) !=
               iv[PAR_FLASH_SIZE      - PAR_SIZE_BOUNDARIES_START]   ) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s must be equal to:\n%s * %s.",
                    tx_Param[PAR_FLASH_SIZE     ]->label(),
                    tx_Param[PAR_FLASH_PAGE_SIZE]->label(),
                    tx_Param[PAR_FLASH_PAGES    ]->label()
                );
            }
            tx_Param[PAR_FLASH_PAGE_SIZE]->color(FL_YELLOW);
            tx_Param[PAR_FLASH_PAGES    ]->color(FL_YELLOW);
            tx_Param[PAR_FLASH_SIZE     ]->color(FL_YELLOW);
        } else {
            flash_pages = iv[PAR_FLASH_PAGES - PAR_SIZE_BOUNDARIES_START];
        }
        if ( ( iv[PAR_EEPROM_PAGE_SIZE - PAR_SIZE_BOUNDARIES_START] *
               iv[PAR_EEPROM_PAGES     - PAR_SIZE_BOUNDARIES_START] ) !=
               iv[PAR_EEPROM_SIZE      - PAR_SIZE_BOUNDARIES_START]   ) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s must be equal to:\n%s * %s.",
                    tx_Param[PAR_EEPROM_SIZE     ]->label(),
                    tx_Param[PAR_EEPROM_PAGE_SIZE]->label(),
                    tx_Param[PAR_EEPROM_PAGES    ]->label()
                );
            }
            tx_Param[PAR_EEPROM_PAGE_SIZE]->color(FL_YELLOW);
            tx_Param[PAR_EEPROM_PAGES    ]->color(FL_YELLOW);
            tx_Param[PAR_EEPROM_SIZE     ]->color(FL_YELLOW);
        } else {
            eeprom_pages = iv[PAR_EEPROM_PAGES - PAR_SIZE_BOUNDARIES_START];
        }
        for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
            tx_Param[i]->redraw();
        }
    }
    // Checking Signature bytes
    {
    int i;

        for (i=PAR_SIGN_B0; i<=PAR_SIGN_B2; i++) {
            tx_Param[i]->color(FL_WHITE);
            if ( strlen(tx_Param[i]->value()) < 1 ) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "%s can't be empty",
                        tx_Param[i]->label()
                    );
                }
                tx_Param[i]->color(FL_YELLOW);
            }
            tx_Param[i]->redraw();
        }
    }
    // Checking Calibration Bytes
    {
    int i;

        tx_Param[PAR_CALIBRATION_BYTES]->color(FL_WHITE);
        if (sscanf(tx_Param[PAR_CALIBRATION_BYTES]->value(),"%d",&i)!=1 || i<0) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s value must be a non negative (>=0) integer.",
                    tx_Param[PAR_CALIBRATION_BYTES]->label()
                );
            }
            tx_Param[PAR_CALIBRATION_BYTES]->color(FL_YELLOW);
        } else {
            calib_bytes = i;
        }
        tx_Param[PAR_CALIBRATION_BYTES]->redraw();
    }
    // Checking Mandatory Instructions
    {
    int i;

        for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
            tx_Param[i]->color(FL_WHITE);
            if ( strlen(tx_Param[i]->value()) < 32 ) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "%s can't be shorter than 32 digits",
                        tx_Param[i]->label()
                    );
                }
                tx_Param[i]->color(FL_YELLOW);
            }
            tx_Param[i]->redraw();
        }
    }
    // Checking Optional Instructions
    {
    int i;
    int l1,l2,l3;

        for (i=PAR_OPTIONAL_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
            tx_Param[i]->color(FL_WHITE);
            l1 = strlen(tx_Param[i]->value());
            if ( ( l1 > 0 ) && ( l1 < 32 ) ) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "If %s is not empty, it can't be shorter than 32 digits",
                        tx_Param[i]->label()
                    );
                }
                tx_Param[i]->color(FL_YELLOW);
            }
            tx_Param[i]->redraw();
        }
        if (flash_pages>1) {
            l1 = strlen(tx_Param[PAR_LOAD_EXT_ADDR_INST   ]->value());
            l2 = strlen(tx_Param[PAR_LOAD_FLASH_PAGE_INST ]->value());
            l3 = strlen(tx_Param[PAR_WRITE_FLASH_PAGE_INST]->value());

            if ((flash_pages > 0xffff) && (l1 < 32)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Being more than %d %s\n%s must be set",
                        0xffff,
                        tx_Param[PAR_FLASH_PAGES       ]->label(),
                        tx_Param[PAR_LOAD_EXT_ADDR_INST]->label()
                    );
                }
                tx_Param[PAR_FLASH_PAGES       ]->color(FL_YELLOW);
                tx_Param[PAR_LOAD_EXT_ADDR_INST]->color(FL_YELLOW);
            }
            tx_Param[PAR_FLASH_PAGES       ]->redraw();
            tx_Param[PAR_LOAD_EXT_ADDR_INST]->redraw();

            if ((l2 < 32) || (l3 < 32)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must be set",
                        tx_Param[PAR_LOAD_FLASH_PAGE_INST ]->label(),
                        tx_Param[PAR_WRITE_FLASH_PAGE_INST]->label()
                    );
                }
                tx_Param[PAR_LOAD_FLASH_PAGE_INST ]->color(FL_YELLOW);
                tx_Param[PAR_WRITE_FLASH_PAGE_INST]->color(FL_YELLOW);
            }
            tx_Param[PAR_LOAD_FLASH_PAGE_INST ]->redraw();
            tx_Param[PAR_WRITE_FLASH_PAGE_INST]->redraw();
        }
        if (eeprom_pages>1) {
            l2 = strlen(tx_Param[PAR_LOAD_EEPROM_PAGE_INST ]->value());
            l3 = strlen(tx_Param[PAR_WRITE_EEPROM_PAGE_INST]->value());

            if ((l2 < 32) || (l3 < 32)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must be set",
                        tx_Param[PAR_LOAD_EEPROM_PAGE_INST ]->label(),
                        tx_Param[PAR_WRITE_EEPROM_PAGE_INST]->label()
                    );
                }
                tx_Param[PAR_LOAD_EEPROM_PAGE_INST ]->color(FL_YELLOW);
                tx_Param[PAR_WRITE_EEPROM_PAGE_INST]->color(FL_YELLOW);
            }
            tx_Param[PAR_LOAD_EEPROM_PAGE_INST ]->redraw();
            tx_Param[PAR_WRITE_EEPROM_PAGE_INST]->redraw();
        }
        if (calib_bytes>0) {
            l1 = strlen(tx_Param[PAR_READ_CALIBRATION_INST]->value());

            if (l1 < 32) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "%s must be set when %s is > 0",
                        tx_Param[PAR_READ_CALIBRATION_INST]->label(),
                        tx_Param[PAR_CALIBRATION_BYTES    ]->label()
                    );
                }
                tx_Param[PAR_CALIBRATION_BYTES    ]->color(FL_YELLOW);
                tx_Param[PAR_READ_CALIBRATION_INST]->color(FL_YELLOW);
            }
            tx_Param[PAR_CALIBRATION_BYTES    ]->redraw();
            tx_Param[PAR_READ_CALIBRATION_INST]->redraw();
        }
        // Cross checking Fuse High Instructions
        {
            l2 = strlen(tx_Param[PAR_READ_FUSE_HIGH_INST ]->value());
            l3 = strlen(tx_Param[PAR_WRITE_FUSE_HIGH_INST]->value());

            if (l2 != l3) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must either be set or unset",
                        tx_Param[PAR_READ_FUSE_HIGH_INST ]->label(),
                        tx_Param[PAR_WRITE_FUSE_HIGH_INST]->label()
                    );
                }
                tx_Param[PAR_READ_FUSE_HIGH_INST ]->color(FL_YELLOW);
                tx_Param[PAR_WRITE_FUSE_HIGH_INST]->color(FL_YELLOW);
            }
            tx_Param[PAR_READ_FUSE_HIGH_INST ]->redraw();
            tx_Param[PAR_WRITE_FUSE_HIGH_INST]->redraw();
        }
        // Cross checking Extended Fuse Instructions
        {
            l2 = strlen(tx_Param[PAR_READ_EXT_FUSE_INST ]->value());
            l3 = strlen(tx_Param[PAR_WRITE_EXT_FUSE_INST]->value());

            if (l2 != l3) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must either be set or unset",
                        tx_Param[PAR_READ_EXT_FUSE_INST ]->label(),
                        tx_Param[PAR_WRITE_EXT_FUSE_INST]->label()
                    );
                }
                tx_Param[PAR_READ_EXT_FUSE_INST ]->color(FL_YELLOW);
                tx_Param[PAR_WRITE_EXT_FUSE_INST]->color(FL_YELLOW);
            }
            tx_Param[PAR_READ_EXT_FUSE_INST ]->redraw();
            tx_Param[PAR_WRITE_EXT_FUSE_INST]->redraw();
        }
    }
    return ok;
}

bool AvrUI::verifyConfig(Preferences &device)
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
            ok = false;
        }

        r = device.get(sparams[PAR_VCC_MAX].name,dv,(double)((int)sparams[PAR_VCC_MAX].defv)) ? 1 : 0;
        if (!r || dv<=0.0) {
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
                ok = false;
            }
        }
    }
    // Checking Size Boundaries
    {
    int i;
    int iv[PAR_SIZE_BOUNDARIES_END-PAR_SIZE_BOUNDARIES_START];

        for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
            r = device.get(sparams[i].name,iv[i-PAR_WAIT_DELAYS_START],(int)sparams[i].defv) ? 1 : 0;
            if (!r || iv[i-PAR_SIZE_BOUNDARIES_START]<=0) {
                ok = false;
            }
        }
        if ( ( iv[PAR_FLASH_PAGE_SIZE - PAR_SIZE_BOUNDARIES_START] *
               iv[PAR_FLASH_PAGES     - PAR_SIZE_BOUNDARIES_START] ) !=
               iv[PAR_FLASH_SIZE      - PAR_SIZE_BOUNDARIES_START]   ) {
            ok = false;
        } else {
            flash_pages = iv[PAR_FLASH_PAGES - PAR_SIZE_BOUNDARIES_START];
        }
        if ( ( iv[PAR_EEPROM_PAGE_SIZE - PAR_SIZE_BOUNDARIES_START] *
               iv[PAR_EEPROM_PAGES     - PAR_SIZE_BOUNDARIES_START] ) !=
               iv[PAR_EEPROM_SIZE      - PAR_SIZE_BOUNDARIES_START]   ) {
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
                ok = false;
            }
        }
        if (flash_pages>1) {
            l1 = validateString(device,PAR_LOAD_EXT_ADDR_INST   ,validInstructionChars,32);
            l2 = validateString(device,PAR_LOAD_FLASH_PAGE_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_FLASH_PAGE_INST,validInstructionChars,32);

            if ((flash_pages > 0xffff) && !l1) {
                ok = false;
            }
            if (!l2 || !l3) {
                ok = false;
            }
        }
        if (eeprom_pages>1) {
            l2 = validateString(device,PAR_LOAD_EEPROM_PAGE_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_EEPROM_PAGE_INST,validInstructionChars,32);

            if (!l2 || !l3) {
                ok = false;
            }
        }
        if (calib_bytes>0) {
            l1 = strlen(tx_Param[PAR_READ_CALIBRATION_INST]->value());

            if (l1 < 32) {
                ok = false;
            }
        }
        // Cross checking Fuse High Instructions
        {
            l2 = validateString(device,PAR_READ_FUSE_HIGH_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_FUSE_HIGH_INST,validInstructionChars,32);

            if (l2 != l3) {
                ok = false;
            }
        }
        // Cross checking Extended Fuse Instructions
        {
            l2 = validateString(device,PAR_READ_EXT_FUSE_INST ,validInstructionChars,32);
            l3 = validateString(device,PAR_WRITE_EXT_FUSE_INST,validInstructionChars,32);

            if (l2 != l3) {
                ok = false;
            }
        }
    }
    return ok;
}
bool AvrUI::initDevicesSettings (
    const char  *vendor,
    const char  *spec  ,
    Preferences &device
) {
int i,j,v;
char buf[FL_PATH_MAX];

    if (!match(vendor, spec)) {
        return false;
    }

    device.get("experimental",v,1);
    device.set("experimental",v);

    for (i=PAR_VOLTAGES_START; i<PAR_VOLTAGES_END; i++) {
        AvrUI::Float::init(i, device);
    }

    for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
        AvrUI::Int::init(i, device);
    }

    for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
        AvrUI::Int::init(i, device);
    }
    AvrUI::Int::init(PAR_CALIBRATION_BYTES, device);

    AvrUI::String::init(PAR_SIGN_B0, device);
    AvrUI::String::init(PAR_SIGN_B1, device);
    AvrUI::String::init(PAR_SIGN_B2, device);

    AvrUI::String::init(PAR_FLASH_READ_BACK_P1, device);
    AvrUI::String::init(PAR_FLASH_READ_BACK_P2, device);
    AvrUI::String::init(PAR_EEPROM_READ_BACK_P1, device);
    AvrUI::String::init(PAR_EEPROM_READ_BACK_P2, device);

    for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
        AvrUI::String::init(i, device);
    }

    for (i=PAR_OPTIONAL_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        AvrUI::String::init(i, device);
    }
    return true;
}

bool AvrUI::loadSettings (
    const char  *vendor,
    const char  *spec  ,
    char        *path  ,
    Preferences &from
) {
bool                loaded = false;
int                 new_val, cur_val, i, level;
const Fl_Menu_Item *mitem;
char               *mdata;
char                report[1024];

    if (!match(vendor, spec)) {
        return false;
    }

    if ( verifyConfig(from) ) {
        bool store   = false;
        bool compare = false;
        if (!devices.groupExists(path)) {
            int j = ch_devices->add (
                path,
                (const char *)0,
                (Fl_Callback *)0,
                (void *)strdup(path),
                0
            );
            ch_devices->value(j);
            ch_devices->set_changed();
            if ((mitem = ch_devices->mvalue())) {
                ((Fl_Menu_Item*)mitem)->labelcolor (
                    tb_Experimental->value() ? FL_BLUE
                                             : FL_BLACK
                );
            }
            ch_devices->redraw();
        } else {
            compare = true;
        }
        while (!devices.groupExists(path)) {
            Preferences to(devices,path);
        }
        Preferences to(devices,path);

        if (compare) {
            bool different = false;

            ls_report->clear();

            from.get("experimental",new_val,1);
              to.get("experimental",cur_val,1);

            if (new_val != cur_val) {
                ls_report->add("@bexperimental:");
                sprintf(report,"  cur=%d",cur_val);
                ls_report->add(report);
                sprintf(report,"@_  new=%d",new_val);
                ls_report->add(report);
                different = true;
            }

            for (i=PAR_VOLTAGES_START; i<PAR_VOLTAGES_END; i++) {
                AvrUI::Float::compare(i, from, to, different);
            }

            for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
                AvrUI::Int::compare(i, from, to, different);
            }

            for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
                AvrUI::Int::compare(i, from, to, different);
            }
            AvrUI::Int::compare(PAR_CALIBRATION_BYTES, from, to, different);

            AvrUI::String::compare(PAR_SIGN_B0, from, to, different);
            AvrUI::String::compare(PAR_SIGN_B1, from, to, different);
            AvrUI::String::compare(PAR_SIGN_B2, from, to, different);

            AvrUI::String::compare(PAR_FLASH_READ_BACK_P1, from, to, different);
            AvrUI::String::compare(PAR_FLASH_READ_BACK_P2, from, to, different);
            AvrUI::String::compare(PAR_EEPROM_READ_BACK_P1, from, to, different);
            AvrUI::String::compare(PAR_EEPROM_READ_BACK_P2, from, to, different);

            for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
                AvrUI::String::compare(i, from, to, different);
            }

            for (i=PAR_OPTIONAL_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
                AvrUI::String::compare(i, from, to, different);
            }

            sprintf(report,"Device: %s",path);
            if (different && show_report_window(report)) {
                store = true;
            }
        }
        if (store || !compare) {
            from.get("experimental",new_val,1);
              to.set("experimental",new_val);

            for (i=PAR_VOLTAGES_START; i<PAR_VOLTAGES_END; i++) {
                AvrUI::Float::clone(i, from, to);
            }

            for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
                AvrUI::Int::clone(i, from, to);
            }

            for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
                AvrUI::Int::clone(i, from, to);
            }
            AvrUI::Int::clone(PAR_CALIBRATION_BYTES, from, to);

            AvrUI::String::clone(PAR_SIGN_B0, from, to);
            AvrUI::String::clone(PAR_SIGN_B1, from, to);
            AvrUI::String::clone(PAR_SIGN_B2, from, to);

            AvrUI::String::clone(PAR_FLASH_READ_BACK_P1, from, to);
            AvrUI::String::clone(PAR_FLASH_READ_BACK_P2, from, to);
            AvrUI::String::clone(PAR_EEPROM_READ_BACK_P1, from, to);
            AvrUI::String::clone(PAR_EEPROM_READ_BACK_P2, from, to);

            for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
                AvrUI::String::clone(i, from, to);
            }

            for (
                level = 0, mitem = ch_devices->menu();
                // null label at level 0 means end of menu:
                level || mitem->label();
                mitem++
            ) {
                if (mitem->submenu()) {
                    // submenu: down one level
                    level++;
                } else if (!mitem->label()) {
                    // null label: up one level
                    level--;
                } else {
                    if ((mdata = (char *)mitem->user_data())) {
                        Preferences dev(devices,mdata);
                        dev.get("experimental",i,1);
                        ((Fl_Menu_Item*)mitem)->labelcolor (
                            i ? FL_BLUE : FL_BLACK
                        );
                    }
                }
            }
        }
        loaded = true;
    }
    return loaded;
}

void AvrUI::cleanConfigFields()
{
int i;

    for (i=0;i<LAST_PARAM;i++) {
        tx_Param[i]->value("");
        tx_Param[i]->color(FL_WHITE);
    }
    tx_Param[PAR_VCC_MIN]->value("0.0");
    tx_Param[PAR_VCC_MAX]->value("0.0");
    for (i=PAR_WAIT_DELAYS_START;i<PAR_WAIT_DELAYS_END;i++) {
        tx_Param[i]->value("0");
    }
    for (i=PAR_SIZE_BOUNDARIES_START;i<PAR_SIZE_BOUNDARIES_END;i++) {
        tx_Param[i]->value("0");
    }
    tx_Param[PAR_CALIBRATION_BYTES]->value("0");
    tb_Experimental->value(1);
}

bool AvrUI::resetConfigFields (
    Preferences &device,
    const char *deviceSpec,
    const char *path
) {
int i;

    if (!match(deviceSpec)) {
        return false;
    }

    device.get("experimental",i,1);
    tb_Experimental->value(i ? 1 : 0);

    for (i=PAR_VOLTAGES_START; i<PAR_VOLTAGES_END; i++) {
        AvrUI::Float::reset(i, device);
    }

    for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
        AvrUI::Int::reset(i, device);
    }

    for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
        AvrUI::Int::reset(i, device);
    }
    AvrUI::Int::reset(PAR_CALIBRATION_BYTES, device);

    AvrUI::String::reset(PAR_SIGN_B0, device);
    AvrUI::String::reset(PAR_SIGN_B1, device);
    AvrUI::String::reset(PAR_SIGN_B2, device);

    AvrUI::String::reset(PAR_FLASH_READ_BACK_P1, device);
    AvrUI::String::reset(PAR_FLASH_READ_BACK_P2, device);
    AvrUI::String::reset(PAR_EEPROM_READ_BACK_P1, device);
    AvrUI::String::reset(PAR_EEPROM_READ_BACK_P2, device);

    for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
        AvrUI::String::reset(i, device);
    }

    for (i=PAR_OPTIONAL_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        AvrUI::String::reset(i, device);
    }
    return true;
}

bool AvrUI::isExperimental(const char *deviceSpec)
{
    if (!match(deviceSpec)) {
        return true; // by default is experimental
    }
    return tb_Experimental->value();
}

bool AvrUI::saveConfig(const char *deviceSpec, const char *deviceName)
{
int i;

    if (!match(deviceSpec)) {
        return false;
    }
    while (!devices.groupExists(deviceName)) {
        Preferences device(devices,deviceName);
    }
    Preferences device(devices,deviceName);

    device.set("experimental",tb_Experimental->value() ? 1 : 0);

    for (i=PAR_VOLTAGES_START; i<PAR_VOLTAGES_END; i++) {
        AvrUI::Float::save(i, device);
    }

    for (i=PAR_WAIT_DELAYS_START; i<PAR_WAIT_DELAYS_END; i++) {
        AvrUI::Int::save(i, device);
    }

    for (i=PAR_SIZE_BOUNDARIES_START; i<PAR_SIZE_BOUNDARIES_END; i++) {
        AvrUI::Int::save(i, device);
    }
    AvrUI::Int::save(PAR_CALIBRATION_BYTES, device);

    AvrUI::String::save(PAR_SIGN_B0, device);
    AvrUI::String::save(PAR_SIGN_B1, device);
    AvrUI::String::save(PAR_SIGN_B2, device);

    AvrUI::String::save(PAR_FLASH_READ_BACK_P1, device);
    AvrUI::String::save(PAR_FLASH_READ_BACK_P2, device);
    AvrUI::String::save(PAR_EEPROM_READ_BACK_P1, device);
    AvrUI::String::save(PAR_EEPROM_READ_BACK_P2, device);

    for (i=PAR_MANDATORY_INSTRUCTIONS_START; i<PAR_MANDATORY_INSTRUCTIONS_END; i++) {
        AvrUI::String::save(i, device);
    }

    for (i=PAR_OPTIONAL_INSTRUCTIONS_START; i<PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        AvrUI::String::save(i, device);
    }
    return true;
}

void AvrUI::deactivate()
{
    g_gencfg->deactivate();
    g_flashcfg->deactivate();
    g_eepromcfg->deactivate();
    g_fusecfg->deactivate();
    g_misccfg->deactivate();
}

void AvrUI::activate()
{
    g_gencfg->activate();
    g_flashcfg->activate();
    g_eepromcfg->activate();
    g_fusecfg->activate();
    g_misccfg->activate();
}

bool AvrUI::getVoltageRanges (
    const char *deviceSpec,
    double &vpp_min , double &vpp_max ,
    double &vddp_min, double &vddp_max,
    double &vdd_min , double &vdd_max
) {
    if (!match(deviceSpec)) {
        return false;
    }
    vpp_min  = 0.0;
    vpp_max  = 0.0;
    vddp_min = 0.0;
    vddp_max = 0.0;
    vdd_min  = atof(tx_Param[PAR_VCC_MIN ]->value());
    vdd_max  = atof(tx_Param[PAR_VCC_MAX ]->value());

    return false; // for now - TODO
}

bool AvrUI::validateString(Preferences &device, int param, const char *range, unsigned int size, bool can_be_empty)
{
char buf[FL_PATH_MAX];
int  text_size;

    if (!range || !size) {
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
    return true;
}

void AvrUI::validateChar(Fl_Input *o, const char *range, unsigned int max_size)
{
    if (!o || !range) {
        return;
    }
    int  text_size = strlen(o->value());
    int  last_char = o->index(text_size-1);
    bool match = false;

    for (int i=0; range[i] != '\0'; i++) {
        if ( range[i] == last_char ) {
            match = true;
            break;
        }
    }
    if (text_size && ( !match || (text_size > max_size) ) ) {
        o->value(o->value(), text_size-1);
        if (text_size > max_size) {
            fl_beep();
        }
    }
}

//-----------------------------------------------------------------------------------
//                                FLOAT UTILITIES
//-----------------------------------------------------------------------------------
void AvrUI::Float::compare(int i, Preferences &from, Preferences &to, bool &different)
{
double cur_val, new_val;
char   report[1024];

    from.get(sparams[(i)].name,new_val,(double)((int)sparams[(i)].defv));
      to.get(sparams[(i)].name,cur_val,(double)((int)sparams[(i)].defv));

    if (cur_val != new_val) {
        sprintf(report,"@%s:",sparams[(i)].name);
        ls_report->add(report);
        sprintf(report,"  cur=%lf",cur_val);
        ls_report->add(report);
        sprintf(report,"@_  new=%lf",new_val);
        ls_report->add(report);
        different = true;
    }
}

void AvrUI::Float::clone(int i, Preferences &from, Preferences &to)
{
double new_val;

    from.get(sparams[(i)].name,new_val,(double)((int)sparams[(i)].defv));
      to.set(sparams[(i)].name,new_val);
}

void AvrUI::Float::reset(int i, Preferences &device)
{
    double d;
    char buf[FL_PATH_MAX];
    device.get(sparams[(i)].name,d,(double)((int)sparams[(i)].defv));
    sprintf(buf,"%.2lf",d);
    tx_Param[i]->value(buf);
}

void AvrUI::Float::init(int i, Preferences &device)
{
double d;

    device.get(sparams[(i)].name,d,(double)((int)sparams[(i)].defv));
    device.set(sparams[(i)].name,d);
}

void AvrUI::Float::save(int i, Preferences &device)
{
double d;

    if (sscanf(tx_Param[i]->value(),"%lf",&d)) {
        device.set(sparams[i].name,d);
    }
}

//-----------------------------------------------------------------------------------
//                                INT UTILITIES
//-----------------------------------------------------------------------------------
void AvrUI::Int::compare(int i, Preferences &from, Preferences &to, bool &different)
{
int  cur_val, new_val;
char report[1024];

    from.get(sparams[(i)].name,new_val,(int)sparams[(i)].defv);
      to.get(sparams[(i)].name,cur_val,(int)sparams[(i)].defv);

    if (cur_val != new_val) {
        sprintf(report,"@%s:",sparams[(i)].name);
        ls_report->add(report);
        sprintf(report,"  cur=%d",cur_val);
        ls_report->add(report);
        sprintf(report,"@_  new=%d",new_val);
        ls_report->add(report);
        different = true;
    }
}

void AvrUI::Int::clone(int i, Preferences &from, Preferences &to)
{
int new_val;

    from.get(sparams[(i)].name,new_val,(int)sparams[(i)].defv);
      to.set(sparams[(i)].name,new_val);
}

void AvrUI::Int::reset(int i, Preferences &device)
{
    int j;
    char buf[FL_PATH_MAX];
    device.get(sparams[(i)].name,j,(int)sparams[(i)].defv);
    sprintf(buf,"%d",j);
    tx_Param[i]->value(buf);
}

void AvrUI::Int::init(int i, Preferences &device)
{
int j;

    device.get(sparams[(i)].name,j,(int)sparams[(i)].defv);
    device.set(sparams[(i)].name,j);
}

void AvrUI::Int::save(int i, Preferences &device)
{
int j;

    if (sscanf(tx_Param[i]->value(),"%d",&j)) {
        device.set(sparams[i].name,j);
    }
}

//-----------------------------------------------------------------------------------
//                                STRING UTILITIES
//-----------------------------------------------------------------------------------
void AvrUI::String::compare(int i, Preferences &from, Preferences &to, bool &different)
{
char cur_val[FL_PATH_MAX];
char new_val[FL_PATH_MAX];
char report[1024];

    from.get(sparams[(i)].name,new_val,(char *)sparams[(i)].defv,sizeof(new_val)-1);
      to.get(sparams[(i)].name,cur_val,(char *)sparams[(i)].defv,sizeof(cur_val)-1);

    if (cur_val != new_val) {
        sprintf(report,"@%s:",sparams[(i)].name);
        ls_report->add(report);
        sprintf(report,"  cur=%s",cur_val);
        ls_report->add(report);
        sprintf(report,"@_  new=%s",new_val);
        ls_report->add(report);
        different = true;
    }
}

void AvrUI::String::clone(int i, Preferences &from, Preferences &to)
{
char new_val[FL_PATH_MAX];

    from.get(sparams[(i)].name,new_val,(char *)sparams[(i)].defv,sizeof(new_val)-1);
      to.set(sparams[(i)].name,new_val);
}

void AvrUI::String::reset(int i, Preferences &device)
{
    char buf[FL_PATH_MAX];
    device.get(sparams[(i)].name,buf,(char *)sparams[(i)].defv,sizeof(buf)-1); 
    tx_Param[i]->value(buf);
}

void AvrUI::String::init(int i, Preferences &device)
{
char buf[FL_PATH_MAX];

    device.get(sparams[(i)].name,buf,(char *)sparams[(i)].defv,sizeof(buf)-1);
    device.set(sparams[(i)].name,buf);
}

void AvrUI::String::save(int i, Preferences &device)
{
    device.set(sparams[(i)].name,tx_Param[i]->value());
}
