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
#include "Atmel.h"
#include <string.h>
#include <FL/fl_ask.H>
#include <FL/filename.H>

#include "flP5.h"

#define DEVICE_VENDOR "Atmel"
#define DEVICE_SPEC   "AVR"
#define DEVICE_TYPE   "Atmel/AVR"

Fl_Input        *AvrUI::tx_Param[Avr::LAST_PARAM]                       = { 0 };
Fl_Input        *AvrUI::tx_bitNames[Avr::LAST_BIT_NAME]                 = { 0 };
Fl_Group        *AvrUI::g_Settings                                      = 0;
Fl_Group        *AvrUI::g_gencfg                                        = 0;
Fl_Group        *AvrUI::g_flashcfg                                      = 0;
Fl_Group        *AvrUI::g_eepromcfg                                     = 0;
Fl_Group        *AvrUI::g_fusecfg                                       = 0;
Fl_Group        *AvrUI::g_misccfg                                       = 0;
Fl_Check_Button *AvrUI::tb_powerOffAfterWrite                           = 0;
Fl_Check_Button *AvrUI::tb_Experimental                                 = 0;
Fl_Light_Button *AvrUI::lb_bits[Avr::LAST_BIT_NAME][Avr::BITS_IN_FUSES] = { 0 };
Fl_Group        *AvrUI::g_fuses[Avr::LAST_BIT_NAME]                     = { 0 };

const char      *AvrUI::instructionsFormatTips          = "\
32 bits Instruction word:\
\n - 0|1 = bit set to 0 or 1\
\n - a   = address' high bit\
\n - b   = address' low bit\
\n - H   = 0-low byte, 1-high byte\
\n - o   = data output bit\
\n - i   = data input bit\
\n - x   = don\'t care bit\
";

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

        tx_Param[Avr::PAR_VCC_MIN]->color(FL_WHITE);
        if (sscanf(tx_Param[Avr::PAR_VCC_MIN]->value(),"%lf",&dv[0])!=1 || dv[0]<=0.0) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s value must be a positive (>0) float.",
                    tx_Param[Avr::PAR_VCC_MIN]->label()
                );
            }
            tx_Param[Avr::PAR_VCC_MIN]->color(FL_YELLOW);
        }

        tx_Param[Avr::PAR_VCC_MAX]->color(FL_WHITE);
        if (sscanf(tx_Param[Avr::PAR_VCC_MAX]->value(),"%lf",&dv[1])!=1 || dv[1]<dv[0]) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s value must be greater or equal to the %s value.",
                    tx_Param[Avr::PAR_VCC_MAX]->label(),
                    tx_Param[Avr::PAR_VCC_MIN]->label()
                );
            }
            tx_Param[Avr::PAR_VCC_MAX]->color(FL_YELLOW);
            tx_Param[Avr::PAR_VCC_MIN]->color(FL_YELLOW);
        }
        tx_Param[Avr::PAR_VCC_MAX]->redraw();
        tx_Param[Avr::PAR_VCC_MIN]->redraw();
    }
    // Checking Wait Delays
    {
    int i;
    int iv[Avr::PAR_WAIT_DELAYS_END-Avr::PAR_WAIT_DELAYS_START];

        for (i=Avr::PAR_WAIT_DELAYS_START; i<Avr::PAR_WAIT_DELAYS_END; i++) {
            tx_Param[i]->color(FL_WHITE);
            if (sscanf(tx_Param[i]->value(),"%d",&iv[i-Avr::PAR_WAIT_DELAYS_START])!=1 ||
                iv[i-Avr::PAR_WAIT_DELAYS_START]<=0) {
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
    int iv[Avr::PAR_SIZE_BOUNDARIES_END-Avr::PAR_SIZE_BOUNDARIES_START];

        for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
            tx_Param[i]->color(FL_WHITE);
            if (sscanf(tx_Param[i]->value(),"%d",&iv[i-Avr::PAR_SIZE_BOUNDARIES_START])!=1 ||
                iv[i-Avr::PAR_SIZE_BOUNDARIES_START]<=0) {
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
        if ( ( iv[Avr::PAR_FLASH_PAGE_SIZE - Avr::PAR_SIZE_BOUNDARIES_START] *
               iv[Avr::PAR_FLASH_PAGES     - Avr::PAR_SIZE_BOUNDARIES_START] ) !=
               iv[Avr::PAR_FLASH_SIZE      - Avr::PAR_SIZE_BOUNDARIES_START]   ) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s must be equal to:\n%s * %s.",
                    tx_Param[Avr::PAR_FLASH_SIZE     ]->label(),
                    tx_Param[Avr::PAR_FLASH_PAGE_SIZE]->label(),
                    tx_Param[Avr::PAR_FLASH_PAGES    ]->label()
                );
            }
            tx_Param[Avr::PAR_FLASH_PAGE_SIZE]->color(FL_YELLOW);
            tx_Param[Avr::PAR_FLASH_PAGES    ]->color(FL_YELLOW);
            tx_Param[Avr::PAR_FLASH_SIZE     ]->color(FL_YELLOW);
        } else {
            flash_pages = iv[Avr::PAR_FLASH_PAGES - Avr::PAR_SIZE_BOUNDARIES_START];
        }
        if ( ( iv[Avr::PAR_EEPROM_PAGE_SIZE - Avr::PAR_SIZE_BOUNDARIES_START] *
               iv[Avr::PAR_EEPROM_PAGES     - Avr::PAR_SIZE_BOUNDARIES_START] ) !=
               iv[Avr::PAR_EEPROM_SIZE      - Avr::PAR_SIZE_BOUNDARIES_START]   ) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s must be equal to:\n%s * %s.",
                    tx_Param[Avr::PAR_EEPROM_SIZE     ]->label(),
                    tx_Param[Avr::PAR_EEPROM_PAGE_SIZE]->label(),
                    tx_Param[Avr::PAR_EEPROM_PAGES    ]->label()
                );
            }
            tx_Param[Avr::PAR_EEPROM_PAGE_SIZE]->color(FL_YELLOW);
            tx_Param[Avr::PAR_EEPROM_PAGES    ]->color(FL_YELLOW);
            tx_Param[Avr::PAR_EEPROM_SIZE     ]->color(FL_YELLOW);
        } else {
            eeprom_pages = iv[Avr::PAR_EEPROM_PAGES - Avr::PAR_SIZE_BOUNDARIES_START];
        }
        for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
            tx_Param[i]->redraw();
        }
    }
    // Checking Signature bytes
    {
    int i;

        for (i=Avr::PAR_SIGN_B0; i<=Avr::PAR_SIGN_B2; i++) {
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

        tx_Param[Avr::PAR_CALIBRATION_BYTES]->color(FL_WHITE);
        if (sscanf(tx_Param[Avr::PAR_CALIBRATION_BYTES]->value(),"%d",&i)!=1 || i<0) {
            ok = false;
            if (verbose) {
                fl_alert (
                    "The %s value must be a non negative (>=0) integer.",
                    tx_Param[Avr::PAR_CALIBRATION_BYTES]->label()
                );
            }
            tx_Param[Avr::PAR_CALIBRATION_BYTES]->color(FL_YELLOW);
        } else {
            calib_bytes = i;
        }
        tx_Param[Avr::PAR_CALIBRATION_BYTES]->redraw();
    }
    // Checking Mandatory Instructions
    {
    int i;

        for (i=Avr::PAR_MANDATORY_INSTRUCTIONS_START; i<Avr::PAR_MANDATORY_INSTRUCTIONS_END; i++) {
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

        for (i=Avr::PAR_OPTIONAL_INSTRUCTIONS_START; i<Avr::PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
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
            l1 = strlen(tx_Param[Avr::PAR_LOAD_EXT_ADDR_INST   ]->value());
            l2 = strlen(tx_Param[Avr::PAR_LOAD_FLASH_PAGE_INST ]->value());
            l3 = strlen(tx_Param[Avr::PAR_WRITE_FLASH_PAGE_INST]->value());

            if ((flash_pages > 0xffff) && (l1 < 32)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Being more than %d %s\n%s must be set",
                        0xffff,
                        tx_Param[Avr::PAR_FLASH_PAGES       ]->label(),
                        tx_Param[Avr::PAR_LOAD_EXT_ADDR_INST]->label()
                    );
                }
                tx_Param[Avr::PAR_FLASH_PAGES       ]->color(FL_YELLOW);
                tx_Param[Avr::PAR_LOAD_EXT_ADDR_INST]->color(FL_YELLOW);
            }
            tx_Param[Avr::PAR_FLASH_PAGES       ]->redraw();
            tx_Param[Avr::PAR_LOAD_EXT_ADDR_INST]->redraw();

            if ((l2 < 32) || (l3 < 32)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must be set",
                        tx_Param[Avr::PAR_LOAD_FLASH_PAGE_INST ]->label(),
                        tx_Param[Avr::PAR_WRITE_FLASH_PAGE_INST]->label()
                    );
                }
                tx_Param[Avr::PAR_LOAD_FLASH_PAGE_INST ]->color(FL_YELLOW);
                tx_Param[Avr::PAR_WRITE_FLASH_PAGE_INST]->color(FL_YELLOW);
            }
            tx_Param[Avr::PAR_LOAD_FLASH_PAGE_INST ]->redraw();
            tx_Param[Avr::PAR_WRITE_FLASH_PAGE_INST]->redraw();
        }
        if (eeprom_pages>1) {
            l2 = strlen(tx_Param[Avr::PAR_LOAD_EEPROM_PAGE_INST ]->value());
            l3 = strlen(tx_Param[Avr::PAR_WRITE_EEPROM_PAGE_INST]->value());

            if ((l2 < 32) || (l3 < 32)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must be set",
                        tx_Param[Avr::PAR_LOAD_EEPROM_PAGE_INST ]->label(),
                        tx_Param[Avr::PAR_WRITE_EEPROM_PAGE_INST]->label()
                    );
                }
                tx_Param[Avr::PAR_LOAD_EEPROM_PAGE_INST ]->color(FL_YELLOW);
                tx_Param[Avr::PAR_WRITE_EEPROM_PAGE_INST]->color(FL_YELLOW);
            }
            tx_Param[Avr::PAR_LOAD_EEPROM_PAGE_INST ]->redraw();
            tx_Param[Avr::PAR_WRITE_EEPROM_PAGE_INST]->redraw();
        }
        if (calib_bytes>0) {
            l1 = strlen(tx_Param[Avr::PAR_READ_CALIBRATION_INST]->value());

            if (l1 < 32) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "%s must be set when %s is > 0",
                        tx_Param[Avr::PAR_READ_CALIBRATION_INST]->label(),
                        tx_Param[Avr::PAR_CALIBRATION_BYTES    ]->label()
                    );
                }
                tx_Param[Avr::PAR_CALIBRATION_BYTES    ]->color(FL_YELLOW);
                tx_Param[Avr::PAR_READ_CALIBRATION_INST]->color(FL_YELLOW);
            }
            tx_Param[Avr::PAR_CALIBRATION_BYTES    ]->redraw();
            tx_Param[Avr::PAR_READ_CALIBRATION_INST]->redraw();
        }
        // Cross checking Fuse High Instructions
        {
            l2 = strlen(tx_Param[Avr::PAR_READ_FUSE_HIGH_INST ]->value());
            l3 = strlen(tx_Param[Avr::PAR_WRITE_FUSE_HIGH_INST]->value());

            if (l2 != l3) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must either be set or unset",
                        tx_Param[Avr::PAR_READ_FUSE_HIGH_INST ]->label(),
                        tx_Param[Avr::PAR_WRITE_FUSE_HIGH_INST]->label()
                    );
                }
                tx_Param[Avr::PAR_READ_FUSE_HIGH_INST ]->color(FL_YELLOW);
                tx_Param[Avr::PAR_WRITE_FUSE_HIGH_INST]->color(FL_YELLOW);
            }
            tx_Param[Avr::PAR_READ_FUSE_HIGH_INST ]->redraw();
            tx_Param[Avr::PAR_WRITE_FUSE_HIGH_INST]->redraw();
        }
        // Cross checking Extended Fuse Instructions
        {
            l2 = strlen(tx_Param[Avr::PAR_READ_EXT_FUSE_INST ]->value());
            l3 = strlen(tx_Param[Avr::PAR_WRITE_EXT_FUSE_INST]->value());

            if (l2 != l3) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "Both %s and %s must either be set or unset",
                        tx_Param[Avr::PAR_READ_EXT_FUSE_INST ]->label(),
                        tx_Param[Avr::PAR_WRITE_EXT_FUSE_INST]->label()
                    );
                }
                tx_Param[Avr::PAR_READ_EXT_FUSE_INST ]->color(FL_YELLOW);
                tx_Param[Avr::PAR_WRITE_EXT_FUSE_INST]->color(FL_YELLOW);
            }
            tx_Param[Avr::PAR_READ_EXT_FUSE_INST ]->redraw();
            tx_Param[Avr::PAR_WRITE_EXT_FUSE_INST]->redraw();
        }
    }
    // Checking bit names
    {
    int i;
    int separators;

        for (i=0; i<Avr::LAST_BIT_NAME; i++) {
            const char *name = tx_bitNames[i]->value();
            separators=0;
            for (int j=0; j<strlen(name); j++) {
                if (name[j]==',') {
                    separators++;
                }
            }
            if (separators != (Avr::BITS_IN_FUSES-1)) {
                ok = false;
                if (verbose) {
                    fl_alert (
                        "%s must match the number of supported bits (%d)\n"
                        "Currently named bits are: %d",
                        tx_bitNames[i]->label(),
                        Avr::BITS_IN_FUSES,
                        separators+1
                    );
                }
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

    device.get("powerOffAfterWriteFuse",i,0);
    device.set("powerOffAfterWriteFuse",i);

    for (i=Avr::PAR_VOLTAGES_START; i<Avr::PAR_VOLTAGES_END; i++) {
        AvrUI::Float::init(i, device);
    }

    for (i=Avr::PAR_WAIT_DELAYS_START; i<Avr::PAR_WAIT_DELAYS_END; i++) {
        AvrUI::Int::init(i, device);
    }

    for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
        AvrUI::Int::init(i, device);
    }
    AvrUI::Int::init(Avr::PAR_CALIBRATION_BYTES, device);

    AvrUI::String::init(Avr::PAR_SIGN_B0, device);
    AvrUI::String::init(Avr::PAR_SIGN_B1, device);
    AvrUI::String::init(Avr::PAR_SIGN_B2, device);

    AvrUI::String::init(Avr::PAR_FLASH_READ_BACK_P1, device);
    AvrUI::String::init(Avr::PAR_FLASH_READ_BACK_P2, device);
    AvrUI::String::init(Avr::PAR_EEPROM_READ_BACK_P1, device);
    AvrUI::String::init(Avr::PAR_EEPROM_READ_BACK_P2, device);

    for (i=Avr::PAR_MANDATORY_INSTRUCTIONS_START; i<Avr::PAR_MANDATORY_INSTRUCTIONS_END; i++) {
        AvrUI::String::init(i, device);
    }

    for (i=Avr::PAR_OPTIONAL_INSTRUCTIONS_START; i<Avr::PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        AvrUI::String::init(i, device);
    }

    for (i=0; i<Avr::LAST_BIT_NAME;i++) {
        for (j=0;j<Avr::BITS_IN_FUSES;j++) {
            device.get(Avr::sbitNames[(i*j)].name,buf,(char *)Avr::sbitNames[(i*j)].defv,sizeof(buf)-1);
            device.set(Avr::sbitNames[(i*j)].name,buf);
        }
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
int                 new_val, cur_val, i, j, level;
const Fl_Menu_Item *mitem;
char               *mdata;
char                report[1024];
char                scur_val[FL_PATH_MAX];
char                snew_val[FL_PATH_MAX];

    if (!match(vendor, spec)) {
        return false;
    }

    if ( Avr::verifyConfig(from) ) {
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

            from.get("powerOffAfterWriteFuse",new_val,0);
              to.get("powerOffAfterWriteFuse",cur_val,0);

            if (new_val != cur_val) {
                ls_report->add("@powerOffAfterWriteFuse:");
                sprintf(report,"  cur=%d",cur_val);
                ls_report->add(report);
                sprintf(report,"@_  new=%d",new_val);
                ls_report->add(report);
                different = true;
            }

            for (i=Avr::PAR_VOLTAGES_START; i<Avr::PAR_VOLTAGES_END; i++) {
                AvrUI::Float::compare(i, from, to, different);
            }

            for (i=Avr::PAR_WAIT_DELAYS_START; i<Avr::PAR_WAIT_DELAYS_END; i++) {
                AvrUI::Int::compare(i, from, to, different);
            }

            for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
                AvrUI::Int::compare(i, from, to, different);
            }
            AvrUI::Int::compare(Avr::PAR_CALIBRATION_BYTES, from, to, different);

            AvrUI::String::compare(Avr::PAR_SIGN_B0, from, to, different);
            AvrUI::String::compare(Avr::PAR_SIGN_B1, from, to, different);
            AvrUI::String::compare(Avr::PAR_SIGN_B2, from, to, different);

            AvrUI::String::compare(Avr::PAR_FLASH_READ_BACK_P1, from, to, different);
            AvrUI::String::compare(Avr::PAR_FLASH_READ_BACK_P2, from, to, different);
            AvrUI::String::compare(Avr::PAR_EEPROM_READ_BACK_P1, from, to, different);
            AvrUI::String::compare(Avr::PAR_EEPROM_READ_BACK_P2, from, to, different);

            for (i=Avr::PAR_MANDATORY_INSTRUCTIONS_START; i<Avr::PAR_MANDATORY_INSTRUCTIONS_END; i++) {
                AvrUI::String::compare(i, from, to, different);
            }

            for (i=Avr::PAR_OPTIONAL_INSTRUCTIONS_START; i<Avr::PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
                AvrUI::String::compare(i, from, to, different);
            }

            for (i=0; i<Avr::LAST_BIT_NAME; i++) {
                for (j=0; j<Avr::BITS_IN_FUSES; j++) {
                    from.get(Avr::sbitNames[(i*j)].name,snew_val,(char *)Avr::sbitNames[(i*j)].defv,sizeof(snew_val)-1);
                      to.get(Avr::sbitNames[(i*j)].name,scur_val,(char *)Avr::sbitNames[(i*j)].defv,sizeof(scur_val)-1);

                    if (strcmp(scur_val,snew_val)) {
                        sprintf(report,"@%s:",Avr::sbitNames[(i*j)].name);
                        ls_report->add(report);
                        sprintf(report,"  cur=%s",cur_val);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%s",new_val);
                        ls_report->add(report);
                        different = true;
                    }
                }
            }

            sprintf(report,"Device: %s",path);
            if (different && show_report_window(report)) {
                store = true;
            }
        }
        if (store || !compare) {
            from.get("experimental",new_val,1);
              to.set("experimental",new_val);

            from.get("powerOffAfterWriteFuse",new_val,0);
              to.set("powerOffAfterWriteFuse",new_val);

            for (i=Avr::PAR_VOLTAGES_START; i<Avr::PAR_VOLTAGES_END; i++) {
                AvrUI::Float::clone(i, from, to);
            }

            for (i=Avr::PAR_WAIT_DELAYS_START; i<Avr::PAR_WAIT_DELAYS_END; i++) {
                AvrUI::Int::clone(i, from, to);
            }

            for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
                AvrUI::Int::clone(i, from, to);
            }
            AvrUI::Int::clone(Avr::PAR_CALIBRATION_BYTES, from, to);

            AvrUI::String::clone(Avr::PAR_SIGN_B0, from, to);
            AvrUI::String::clone(Avr::PAR_SIGN_B1, from, to);
            AvrUI::String::clone(Avr::PAR_SIGN_B2, from, to);

            AvrUI::String::clone(Avr::PAR_FLASH_READ_BACK_P1, from, to);
            AvrUI::String::clone(Avr::PAR_FLASH_READ_BACK_P2, from, to);
            AvrUI::String::clone(Avr::PAR_EEPROM_READ_BACK_P1, from, to);
            AvrUI::String::clone(Avr::PAR_EEPROM_READ_BACK_P2, from, to);

            for (i=Avr::PAR_MANDATORY_INSTRUCTIONS_START; i<Avr::PAR_MANDATORY_INSTRUCTIONS_END; i++) {
                AvrUI::String::clone(i, from, to);
            }

            for (i=0; i<Avr::LAST_BIT_NAME; i++) {
                for (j=0; j<Avr::BITS_IN_FUSES; j++) {
                    from.get(Avr::sbitNames[(i*j)].name,snew_val,(char *)Avr::sbitNames[(i*j)].defv,sizeof(snew_val)-1);
                      to.set(Avr::sbitNames[(i*j)].name,snew_val);
                }
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
char bit_names[Avr::BITS_IN_FUSES*(Avr::BIT_NAME_MAX_SIZE+1)];
int i;

    for (i=0;i<Avr::LAST_PARAM;i++) {
        tx_Param[i]->value("");
        tx_Param[i]->color(FL_WHITE);
    }
    tx_Param[Avr::PAR_VCC_MIN]->value("0.0");
    tx_Param[Avr::PAR_VCC_MAX]->value("0.0");
    for (i=Avr::PAR_WAIT_DELAYS_START;i<Avr::PAR_WAIT_DELAYS_END;i++) {
        tx_Param[i]->value("0");
    }
    for (i=Avr::PAR_SIZE_BOUNDARIES_START;i<Avr::PAR_SIZE_BOUNDARIES_END;i++) {
        tx_Param[i]->value("0");
    }
    tx_Param[Avr::PAR_CALIBRATION_BYTES]->value("0");
    tb_Experimental->value(1);
    tb_powerOffAfterWrite->value(0);

    for (i=0; i<Avr::LAST_BIT_NAME; i++) {
        bit_names[0]='\0';
        for (int j=0; j<Avr::BITS_IN_FUSES; j++) {
            lb_bits[i][j]->label("-");
            lb_bits[i][j]->deactivate();
            strcat(bit_names,"-,");
        }
        bit_names[strlen(bit_names)-1]='\0';
        tx_bitNames[i]->value(bit_names);
        g_fuses[i]->deactivate();
    }
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

    device.get("powerOffAfterWriteFuse",i,0);
    tb_powerOffAfterWrite->value(i ? 1 : 0);

    for (i=Avr::PAR_VOLTAGES_START; i<Avr::PAR_VOLTAGES_END; i++) {
        AvrUI::Float::reset(i, device);
    }

    for (i=Avr::PAR_WAIT_DELAYS_START; i<Avr::PAR_WAIT_DELAYS_END; i++) {
        AvrUI::Int::reset(i, device);
    }

    for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
        AvrUI::Int::reset(i, device);
    }
    AvrUI::Int::reset(Avr::PAR_CALIBRATION_BYTES, device);

    AvrUI::String::reset(Avr::PAR_SIGN_B0, device);
    AvrUI::String::reset(Avr::PAR_SIGN_B1, device);
    AvrUI::String::reset(Avr::PAR_SIGN_B2, device);

    AvrUI::String::reset(Avr::PAR_FLASH_READ_BACK_P1, device);
    AvrUI::String::reset(Avr::PAR_FLASH_READ_BACK_P2, device);
    AvrUI::String::reset(Avr::PAR_EEPROM_READ_BACK_P1, device);
    AvrUI::String::reset(Avr::PAR_EEPROM_READ_BACK_P2, device);

    for (i=Avr::PAR_MANDATORY_INSTRUCTIONS_START; i<Avr::PAR_MANDATORY_INSTRUCTIONS_END; i++) {
        AvrUI::String::reset(i, device);
    }

    for (i=Avr::PAR_OPTIONAL_INSTRUCTIONS_START; i<Avr::PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        AvrUI::String::reset(i, device);
    }

    {
    char bit_names[Avr::BITS_IN_FUSES*(Avr::BIT_NAME_MAX_SIZE+1)];
    char buf[11];

        for (int i=0; i<Avr::LAST_BIT_NAME; i++) {
            bit_names[0]='\0';
            for (int j=0; j<Avr::BITS_IN_FUSES; j++) {
                device.get(Avr::sbitNames[(i*j)].name,buf,(char *)Avr::sbitNames[(i*j)].defv,sizeof(buf)-1);
                buf[10]='\0';
                lb_bits[i][j]->label(buf);
                if (buf[0]=='-') {
                    lb_bits[i][j]->deactivate();
                } else {
                    lb_bits[i][j]->activate();
                }
                strncat(bit_names,buf,10);
                strcat(bit_names,",");
            }
            bit_names[strlen(bit_names)-1]='\0';
            tx_bitNames[i]->value(bit_names);
        }
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

    device.set("powerOffAfterWriteFuse", tb_powerOffAfterWrite->value() ? 1 : 0);

    for (i=Avr::PAR_VOLTAGES_START; i<Avr::PAR_VOLTAGES_END; i++) {
        AvrUI::Float::save(i, device);
    }

    for (i=Avr::PAR_WAIT_DELAYS_START; i<Avr::PAR_WAIT_DELAYS_END; i++) {
        AvrUI::Int::save(i, device);
    }

    for (i=Avr::PAR_SIZE_BOUNDARIES_START; i<Avr::PAR_SIZE_BOUNDARIES_END; i++) {
        AvrUI::Int::save(i, device);
    }
    AvrUI::Int::save(Avr::PAR_CALIBRATION_BYTES, device);

    AvrUI::String::save(Avr::PAR_SIGN_B0, device);
    AvrUI::String::save(Avr::PAR_SIGN_B1, device);
    AvrUI::String::save(Avr::PAR_SIGN_B2, device);

    AvrUI::String::save(Avr::PAR_FLASH_READ_BACK_P1, device);
    AvrUI::String::save(Avr::PAR_FLASH_READ_BACK_P2, device);
    AvrUI::String::save(Avr::PAR_EEPROM_READ_BACK_P1, device);
    AvrUI::String::save(Avr::PAR_EEPROM_READ_BACK_P2, device);

    for (i=Avr::PAR_MANDATORY_INSTRUCTIONS_START; i<Avr::PAR_MANDATORY_INSTRUCTIONS_END; i++) {
        AvrUI::String::save(i, device);
    }

    for (i=Avr::PAR_OPTIONAL_INSTRUCTIONS_START; i<Avr::PAR_OPTIONAL_INSTRUCTIONS_END; i++) {
        AvrUI::String::save(i, device);
    }

    {
    const char *bit_name;
    char        buf[11];
    int         bit=0;
    int         pos=0;

        for (int i=0; i<Avr::LAST_BIT_NAME; i++) {
            bit_name = tx_bitNames[i]->value();
            bit = 0;
            pos = 0;
            for (int j=0; j<strlen(bit_name); j++) {
                if ((bit_name[j] != ',') && (bit_name[j] != '\0')) {
                   if (pos<10) {
                       buf[pos] = bit_name[j];
                       pos++;
                   }
                } else {
                   buf[pos] = '\0';
                   device.set(Avr::sbitNames[(i*bit)].name,buf);
                   bit++;
                   pos=0;
                }
            }
        }
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
    vdd_min  = atof(tx_Param[Avr::PAR_VCC_MIN ]->value());
    vdd_max  = atof(tx_Param[Avr::PAR_VCC_MAX ]->value());

    return false; // for now - TODO
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

void AvrUI::initFusesAndLocks(Avr *chip, DataBuffer& buf)
{
uint32_t word;

    if (!chip) {
        return;
    }
    for (int i=0; i<Avr::LAST_BIT_NAME; i++) {
        word = 0xffffffff;
        for (int j=0; j<Avr::BITS_IN_FUSES; j++) {
            word &= ~( lb_bits[i][j]->value() << j );
        }
        chip->set_config_word(i,word,buf);
    }
}

//-----------------------------------------------------------------------------------
//                                FLOAT UTILITIES
//-----------------------------------------------------------------------------------
void AvrUI::Float::compare(int i, Preferences &from, Preferences &to, bool &different)
{
double cur_val, new_val;
char   report[1024];

    from.get(Avr::sparams[(i)].name,new_val,(double)((int)Avr::sparams[(i)].defv));
      to.get(Avr::sparams[(i)].name,cur_val,(double)((int)Avr::sparams[(i)].defv));

    if (cur_val != new_val) {
        sprintf(report,"@%s:",Avr::sparams[(i)].name);
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

    from.get(Avr::sparams[(i)].name,new_val,(double)((int)Avr::sparams[(i)].defv));
      to.set(Avr::sparams[(i)].name,new_val);
}

void AvrUI::Float::reset(int i, Preferences &device)
{
    double d;
    char buf[FL_PATH_MAX];
    device.get(Avr::sparams[(i)].name,d,(double)((int)Avr::sparams[(i)].defv));
    sprintf(buf,"%.2lf",d);
    tx_Param[i]->value(buf);
}

void AvrUI::Float::init(int i, Preferences &device)
{
double d;

    device.get(Avr::sparams[(i)].name,d,(double)((int)Avr::sparams[(i)].defv));
    device.set(Avr::sparams[(i)].name,d);
}

void AvrUI::Float::save(int i, Preferences &device)
{
double d;

    if (sscanf(tx_Param[i]->value(),"%lf",&d)) {
        device.set(Avr::sparams[i].name,d);
    }
}

//-----------------------------------------------------------------------------------
//                                INT UTILITIES
//-----------------------------------------------------------------------------------
void AvrUI::Int::compare(int i, Preferences &from, Preferences &to, bool &different)
{
int  cur_val, new_val;
char report[1024];

    from.get(Avr::sparams[(i)].name,new_val,(int)Avr::sparams[(i)].defv);
      to.get(Avr::sparams[(i)].name,cur_val,(int)Avr::sparams[(i)].defv);

    if (cur_val != new_val) {
        sprintf(report,"@%s:",Avr::sparams[(i)].name);
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

    from.get(Avr::sparams[(i)].name,new_val,(int)Avr::sparams[(i)].defv);
      to.set(Avr::sparams[(i)].name,new_val);
}

void AvrUI::Int::reset(int i, Preferences &device)
{
    int j;
    char buf[FL_PATH_MAX];
    device.get(Avr::sparams[(i)].name,j,(int)Avr::sparams[(i)].defv);
    sprintf(buf,"%d",j);
    tx_Param[i]->value(buf);
}

void AvrUI::Int::init(int i, Preferences &device)
{
int j;

    device.get(Avr::sparams[(i)].name,j,(int)Avr::sparams[(i)].defv);
    device.set(Avr::sparams[(i)].name,j);
}

void AvrUI::Int::save(int i, Preferences &device)
{
int j;

    if (sscanf(tx_Param[i]->value(),"%d",&j)) {
        device.set(Avr::sparams[i].name,j);
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

    from.get(Avr::sparams[(i)].name,new_val,(char *)Avr::sparams[(i)].defv,sizeof(new_val)-1);
      to.get(Avr::sparams[(i)].name,cur_val,(char *)Avr::sparams[(i)].defv,sizeof(cur_val)-1);

    if (strcmp(cur_val,new_val)) {
        sprintf(report,"@%s:",Avr::sparams[(i)].name);
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

    from.get(Avr::sparams[(i)].name,new_val,(char *)Avr::sparams[(i)].defv,sizeof(new_val)-1);
      to.set(Avr::sparams[(i)].name,new_val);
}

void AvrUI::String::reset(int i, Preferences &device)
{
    char buf[FL_PATH_MAX];
    device.get(Avr::sparams[(i)].name,buf,(char *)Avr::sparams[(i)].defv,sizeof(buf)-1); 
    tx_Param[i]->value(buf);
}

void AvrUI::String::init(int i, Preferences &device)
{
char buf[FL_PATH_MAX];

    device.get(Avr::sparams[(i)].name,buf,(char *)Avr::sparams[(i)].defv,sizeof(buf)-1);
    device.set(Avr::sparams[(i)].name,buf);
}

void AvrUI::String::save(int i, Preferences &device)
{
    device.set(Avr::sparams[(i)].name,tx_Param[i]->value());
}
