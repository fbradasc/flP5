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
#include "PicUI.h"

#include <ctype.h>
#ifndef WIN32
#  include <sys/types.h>
#  include <sys/select.h>
#  include <sys/time.h>
#  include <signal.h>
#  include <unistd.h>
#endif
#include <exception>
#include <iterator>
#include <cmath>

using namespace std;

#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>

#include "Util.h"
#include "ParallelPort.h"
#include "flP5.h"

static t_NamedSettings scfgWords[] = {
    "cw_mask", (void *)"0xffff",
    "cw_save", (void *)"0x0000",
    "cw_defs", (void *)"0xffff",
    "cp_mask", (void *)"0x0000",
    "cp_all_", (void *)"0x0000",
    "cp_none", (void *)"0x0000",
    "dp_mask", (void *)"0x0000",
    "dp_on__", (void *)"0x0000",
    "dp_off_", (void *)"0x0000",
    "bd_mask", (void *)"0x0000",
    "bd_on__", (void *)"0x0000",
    "bd_off_", (void *)"0x0000"
};
#define CONFIG_WORD_SETTINGS sizeof(scfgWords) / sizeof(t_NamedSettings)

static t_NamedSettings sparams[] = {
    "wordSize"       , (void *)0,
    "codeSize"       , (void *)0,
    "eepromSize"     , (void *)0,
    "progCount"      , (void *)0,
    "progMult"       , (void *)0,
    "progTime"       , (void *)0,
    "eraseTime"      , (void *)0,
    "writeBufferSize", (void *)32,
    "eraseBufferSize", (void *)64,
    "deviceID"       , (void *)"0x0000",
    "deviceIDMask"   , (void *)"0x0000",
    "vppMin"         , (void *)0,
    "vppMax"         , (void *)0,
    "vddMin"         , (void *)0,
    "vddMax"         , (void *)0,
    "vddpMin"        , (void *)0,
    "vddpMax"        , (void *)0
};

Fl_Input        *PicUI::tx_Param[PicUI::LAST_PARAM]         = { 0 };
Fl_Input        *PicUI::tx_CfgWord[PicUI::LAST_CONFIG_WORD] = { 0 };
Fl_Tabs         *PicUI::g_Settings                          = 0;
Fl_Group        *PicUI::g_gencfg                            = 0;
Fl_Choice       *PicUI::ch_MemType                          = 0;
Fl_Group        *PicUI::g_cfgwords                          = 0;
Fl_Group        *PicUI::g_ConfigWordsEdit                   = 0;
Fl_Group        *PicUI::g_ConfigWordsToolBar                = 0;
Fl_Group        *PicUI::g_ConfigWordsNewEditCopy            = 0;
Fl_Browser      *PicUI::ls_ConfigWords                      = 0;
Fl_Group        *PicUI::g_miscellanea                       = 0;
Fl_Check_Button *PicUI::tb_Experimental                     = 0;

#define DEVICE_VENDOR "Microchip"
#define DEVICE_SPEC   "PIC"
#define DEVICE_TYPE   "Microchip/PIC"

bool PicUI::match(const char *deviceSpec)
{
    return ( strncmp(deviceSpec, DEVICE_TYPE, sizeof(DEVICE_TYPE)-1) == 0 );
}

bool PicUI::match(const char *vendor, const char *spec)
{
    return ( ( strncmp(vendor, DEVICE_VENDOR, sizeof(DEVICE_VENDOR)-1) == 0 ) &&
             ( strncmp(spec  , DEVICE_SPEC  , sizeof(DEVICE_SPEC  )-1) == 0 ) );
}

bool PicUI::verifyConfig(const char *device, bool verbose)
{
int i, v;
double d[6];
bool ok = false;
const char *voltageNames[] = {
    "Vpp Min" , "Vpp Max" ,
    "Vdd Min" , "Vdd Max" ,
    "Vddp Min", "Vddp Max"
};

    if (!match(device)) {
        return false;
    }
    ok = true;
    for (v=0,i=LAST_PARAM-6;verbose && i<LAST_PARAM;i++,v++) {
        tx_Param[i]->color(FL_WHITE);
        if (sscanf(tx_Param[i]->value(),"%lf",&d[v])!=1 || d[v]<1.0) {
            ok = false;
            fl_alert (
                "The %s value must be a positive (>0) float.",
                voltageNames[v]
            );
            tx_Param[i]->color(FL_YELLOW);
        }
        tx_Param[i]->redraw();
    }
    if (ok) {
        for (v=0;v<3;v++) {
            if (d[(2*v)]>d[(2*v)+1]) {
                fl_alert (
                    "The %s value must be greater or equal to the %s value.",
                    voltageNames[(2*v)+1],voltageNames[(2*v)]
                );
                tx_Param[(2*v)+LAST_PARAM-6]->color(FL_YELLOW);
                tx_Param[(2*v)+LAST_PARAM-6+1]->color(FL_YELLOW);
                ok = false;
            }
        }
    }
    for (i=0;verbose && i<LAST_PARAM-8;i++) {
        tx_Param[i]->color(FL_WHITE);
        if (sscanf(tx_Param[i]->value(),"%d",&v)!=1 || v<0) {
            ok = false;
            fl_alert (
                "The %s value must be a non negative (>=0) integer.",
                tx_Param[i]->label()
            );
            tx_Param[i]->color(FL_YELLOW);
        }
        tx_Param[i]->redraw();
    }
    return ok;
}

bool PicUI::initDevicesSettings (
    const char *vendor,
    const char *spec,
    Preferences &device
) {
int i,j,v;
double d;
char buf[FL_PATH_MAX];

    if (!match(vendor, spec)) {
        return false;
    }

    device.get("experimental",v,1);
    device.set("experimental",v);

    device.get("memType",buf,"rom",sizeof(buf)-1);
    device.set("memType",buf);

    for (i=0;i<LAST_PARAM-8;i++) {
        device.get(sparams[i].name,v,(int)sparams[i].defv);
        device.set(sparams[i].name,v);
    }
    for (;i<LAST_PARAM-6;i++) { /* device id & mask */
        device.get (
            sparams[i].name,
            buf,
            (char *)sparams[i].defv,
            sizeof(buf)-1
        );
        device.set(sparams[i].name,buf);
    }
    for (;i<LAST_PARAM;i++) {
        device.get(sparams[i].name,d,(double)((int)sparams[i].defv));
        device.set(sparams[i].name,d);
    }

    device.get("configWords",v,0);
    device.set("configWords",v);

    for (i=0;i<v;i++) {
        for (j=0;j<CONFIG_WORD_SETTINGS;j++) {
            device.get (
                Preferences::Name (
                    "%s_%02d",
                    scfgWords[j].name,i
                ),
                buf,
                (char*)scfgWords[j].defv,
                sizeof(buf)-1
            );
            device.set (
                Preferences::Name (
                    "%s_%02d",
                    scfgWords[j].name,i
                ),
                buf
            );
        }
    }
    return true;
}

bool PicUI::loadSettings (
    const char  *vendor,
    const char  *spec  ,
    char        *path  ,
    Preferences &from
) {
bool loaded   = false;
int  settings = 0;
int i,j,k,level;
int v[CONFIG_WORD_SETTINGS],v1[CONFIG_WORD_SETTINGS];
double d, d1;
const Fl_Menu_Item *mitem;
char *mdata;
char buf[FL_PATH_MAX];
char buf1[FL_PATH_MAX];
char report[1024];
bool compare   = false;
bool different = false;
bool store     = false;

    if (!match(vendor, spec)) {
        return false;
    }
    //
    // check if it's a valid device settings group
    //
    settings = 0;
    /* - Optional -
    settings += from.get("experimental",v[0],1) ? 1 : 0;
    */
    settings += from.get (
        "memType",buf,"rom",sizeof(buf)-1
    ) ? 1 : 0;
    for (i=0;i<LAST_PARAM-8;i++) {
        settings += from.get(sparams[i].name,v[0],(int)sparams[i].defv) ? 1 : 0;
    }
    for (;i<LAST_PARAM-6;i++) { /* device id & mask */
        settings += from.get (
            sparams[i].name,
            buf,
            (char*)sparams[i].defv,
            sizeof(buf)-1
        ) ? 1 : 0;
    }
    for (;i<LAST_PARAM;i++) {
        settings += from.get(sparams[i].name,d,(double)((int)sparams[i].defv)) ? 1 : 0;
    }
    settings += from.get("configWords",v[0],0) ? 1 : 0;
    for (i=0;i<v[0];i++) {
        for (j=0;j<CONFIG_WORD_SETTINGS;j++) {
            settings += from.get (
                Preferences::Name (
                    "%s_%02d",
                    scfgWords[j].name,i
                ),
                buf,
                (char*)scfgWords[j].defv,
                sizeof(buf)-1
            ) ? 1 : 0;
        }
    }
    if (settings==(/* 1+ */ 1+LAST_PARAM+1+(CONFIG_WORD_SETTINGS*v[0]))) {
        store   = false;
        compare = false;
        if (!devices.groupExists(path)) {
            j = ch_devices->add (
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
            different = false;
            ls_report->clear();

            from.get("experimental",v[0],1);
              to.get("experimental",v1[0],1);

            if (v[0] != v1[0]) {
                ls_report->add("@bexperimental:");
                sprintf(report,"  cur=%d",v1[0]);
                ls_report->add(report);
                sprintf(report,"@_  new=%d",v[0]);
                ls_report->add(report);
                different = true;
            }
            from.get("memType",buf,"rom",sizeof(buf)-1);
              to.get("memType",buf1,"rom",sizeof(buf1)-1);

            if (strcmp(buf,buf1)) {
                ls_report->add("@bmemType:");
                sprintf(report,"  cur=%s",buf1);
                ls_report->add(report);
                sprintf(report,"@_  new=%s",buf);
                ls_report->add(report);
                different = true;
            }
            for (i=0;i<LAST_PARAM-8;i++) {
                from.get(sparams[i].name,v[0],(int)sparams[i].defv);
                  to.get(sparams[i].name,v1[0],(int)sparams[i].defv);

                if (v[0] != v1[0]) {
                    sprintf(report,"@b%s:",sparams[i].name);
                    ls_report->add(report);
                    sprintf(report,"  cur=%d",v1[0]);
                    ls_report->add(report);
                    sprintf(report,"@_  new=%d",v[0]);
                    ls_report->add(report);
                    different = true;
                }
            }
            for (;i<LAST_PARAM-6;i++) { /* device id & mask */
                from.get (
                    sparams[i].name,
                    buf,
                    (char*)sparams[i].defv,
                    sizeof(buf)-1
                );
                to.get (
                    sparams[i].name,
                    buf1,
                    (char*)sparams[i].defv,
                    sizeof(buf1)-1
                );
                if (strcmp(buf,buf1)) {
                    sprintf(report,"@b%s:",sparams[i].name);
                    ls_report->add(report);
                    sprintf(report,"  cur=%s",buf1);
                    ls_report->add(report);
                    sprintf(report,"@_  new=%s",buf);
                    ls_report->add(report);
                    different = true;
                }
            }
            for (;i<LAST_PARAM;i++) {
                from.get(sparams[i].name,d,(double)((int)sparams[i].defv));
                  to.get(sparams[i].name,d1,(double)((int)sparams[i].defv));

                if (d != d1) {
                    sprintf(report,"@b%s:",sparams[i].name);
                    ls_report->add(report);
                    sprintf(report,"  cur=%lf",d1);
                    ls_report->add(report);
                    sprintf(report,"@_  new=%lf",d);
                    ls_report->add(report);
                    different = true;
                }
            }
            from.get("configWords",v[0],0);
              to.get("configWords",v1[0],0);

            if (v[0] != v1[0]) {
                ls_report->add("@bconfigWords:");
                sprintf(report,"  cur=%d",v1[0]);
                ls_report->add(report);
                sprintf(report,"@_  new=%d",v[0]);
                ls_report->add(report);
                different = true;
            }
            for (i=0;i<v[0];i++) {
                for (j=0;j<CONFIG_WORD_SETTINGS;j++) {
                    from.get (
                        Preferences::Name (
                            "%s_%02d",
                            scfgWords[j].name,i
                        ),
                        buf,
                        (char*)scfgWords[j].defv,
                        sizeof(buf)-1
                    );
                    to.get (
                        Preferences::Name (
                            "%s_%02d",
                            scfgWords[j].name,i
                        ),
                        buf1,
                        (char*)scfgWords[j].defv,
                        sizeof(buf1)-1
                    );
                    if (strcmp(buf,buf1)) {
                        ls_report->add (
                            Preferences::Name (
                                "@b%s_%02d:",
                                scfgWords[j].name,i
                            )
                        );
                        sprintf(report,"  cur=%s",buf1);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%s",buf);
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
            from.get("experimental",v[0],1);
                to.set("experimental",v[0]);
            from.get("memType",buf,"rom",sizeof(buf)-1);
                to.set("memType",buf);
            for (i=0;i<LAST_PARAM-8;i++) {
                from.get(sparams[i].name,v[0],(int)sparams[i].defv);
                    to.set(sparams[i].name,v[0]);
            }
            for (;i<LAST_PARAM-6;i++) { /* device id & mask */
                from.get (
                    sparams[i].name,
                    buf,
                    (char*)sparams[i].defv,
                    sizeof(buf)-1
                );
                    to.set(sparams[i].name,buf);
            }
            for (;i<LAST_PARAM;i++) {
                from.get(sparams[i].name,d,(double)((int)sparams[i].defv));
                    to.set(sparams[i].name,d);
            }
            from.get("configWords",v[0],0);
                to.set("configWords",v[0]);
            for (i=0;i<v[0];i++) {
                for (j=0;j<CONFIG_WORD_SETTINGS;j++) {
                    from.get (
                        Preferences::Name (
                            "%s_%02d",
                            scfgWords[j].name,i
                        ),
                        buf,
                        (char*)scfgWords[j].defv,
                        sizeof(buf)-1
                    );
                        to.set (
                            Preferences::Name (
                                "%s_%02d",
                                scfgWords[j].name,i
                            ),
                            buf
                        );
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
                    if ((mdata =
                         (char *)mitem->user_data()
                    )) {
                        Preferences dev(devices,mdata);
                        dev.get("experimental",v[0],1);
                        ((Fl_Menu_Item*)mitem)->labelcolor (
                            v[0] ? FL_BLUE : FL_BLACK
                        );
                    }
                }
            }
        }
        loaded = true;
    }
    return loaded;
}

void PicUI::cleanConfigFields()
{
int i;

    ch_MemType->value(0);
    for (i=0;i<LAST_PARAM-8;i++) {
        tx_Param[i]->value("0");
    }
    for (;i<LAST_PARAM-6;i++) {
        tx_Param[i]->value("0x0000");
    }
    for (;i<LAST_PARAM;i++) {
        tx_Param[i]->value("0");
    }
    for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
        tx_CfgWord[i]->value("");
    }
    for (i=0;i<LAST_PARAM;i++) {
        tx_Param[i]->color(FL_WHITE);
    }
    for (i=ls_ConfigWords->size();i>1;i--) {
        ls_ConfigWords->remove(i);
    }
    ls_ConfigWords->do_callback();
    tb_Experimental->value(1);
}

bool PicUI::resetConfigFields (
    Preferences &device,
    const char *deviceSpec,
    const char *path
) {
int i,j,selected;
int v[CONFIG_WORD_SETTINGS];
double d;
const Fl_Menu_Item *mitem;
char *mdata;
char buf[FL_PATH_MAX];
char buf2[FL_PATH_MAX];

    if (!match(deviceSpec)) {
        return false;
    }
    for (i=0;i<LAST_PARAM-8;i++) {
        device.get(sparams[i].name,v[0],(int)sparams[i].defv);
            sprintf(buf,"%d",v[0]);
            tx_Param[i]->value(buf);
    }
    for (;i<LAST_PARAM-6;i++) {
        device.get (
            sparams[i].name,
            buf,
            (char*)sparams[i].defv,
            sizeof(buf)-1
        );
        tx_Param[i]->value(buf);
    }
    for (;i<LAST_PARAM;i++) {
        device.get(sparams[i].name,d,(double)((int)sparams[i].defv));
            sprintf(buf,"%.2lf",d);
            tx_Param[i]->value(buf);
    }
    device.get("configWords",v[0],0);
    for (i=0;i<v[0];i++) {
        ls_ConfigWords->add("");
        ls_ConfigWords->select(ls_ConfigWords->size());
        selected=ls_ConfigWords->value();
        sprintf(buf," %02d ",selected-2);
        for (j=0;j<CONFIG_WORD_SETTINGS;j++) {
            device.get (
                Preferences::Name("%s_%02d",scfgWords[j].name,i),
                buf2,
                (const char*)scfgWords[j].defv,
                sizeof(buf2)-1
            );
            strcat(buf, Preferences::Name("| %s ",buf2));
            tx_CfgWord[j]->value("");
        }
        ls_ConfigWords->text(selected,buf);
    }
    ls_ConfigWords->do_callback();

    device.get("experimental",v[0],1);
    tb_Experimental->value(v[0] ? 1 : 0);

    device.get("memType",buf,"rom",sizeof(buf)-1);
    for (i=0;i<ch_MemType->size();i++) {
         ch_MemType->value(i);
         if (
             (mitem=ch_MemType->mvalue()) &&
             (mdata=(char*)mitem->user_data()) &&
             !strcmp(buf,mdata)
         ) {
             break;
         }
    }
    return true;
}

bool PicUI::isExperimental(const char *deviceSpec)
{
    if (!match(deviceSpec)) {
        return true; // by default is experimental
    }
    return tb_Experimental->value();
}

bool PicUI::saveConfig(const char *deviceSpec, const char *deviceName)
{
int i,j;
int v[CONFIG_WORD_SETTINGS];
double d;
char buf[FL_PATH_MAX];

    if (!match(deviceSpec)) {
        return false;
    }
    while (!devices.groupExists(deviceName)) {
        Preferences device(devices,deviceName);
    }
    Preferences device(devices,deviceName);

    device.set("experimental",tb_Experimental->value() ? 1 : 0);

    device.set (
        "memType",
        (const char*)ch_MemType->mvalue()->user_data()
    );
    for (i=0;i<LAST_PARAM-8;i++) {
        if (sscanf(tx_Param[i]->value(),"%d",&v[0])) {
            device.set(sparams[i].name,v[0]);
        }
    }
    for (;i<LAST_PARAM-6;i++) { /* device id & mask  */
        if (sscanf(tx_Param[i]->value(),"%x",&v[0])) {
            device.set (
                sparams[i].name,
                Preferences::Name("0x%04x",v[0])
            );
        }
    }
    for (;i<LAST_PARAM;i++) {
        if (sscanf(tx_Param[i]->value(),"%lf",&d)) {
            device.set(sparams[i].name,d);
        }
    }
    device.set("configWords",ls_ConfigWords->size()-1);
    for (i=2;i<=ls_ConfigWords->size();i++) {
        if (
            sscanf(
                strcpy(buf,ls_ConfigWords->text(i)),
                " %d | %x | %x | %x | %x | %x | %x |"
                     " %x | %x | %x | %x | %x | %x ",
                &j,
                &v[ 0],&v[ 1],&v[ 2],&v[ 3],&v[ 4],&v[ 5],
                &v[ 6],&v[ 7],&v[ 8],&v[ 9],&v[10],&v[11]
            ) == CONFIG_WORD_SETTINGS + 1
        ) {
            for (j=0;j<CONFIG_WORD_SETTINGS;j++) {
                device.set (
                    Preferences::Name("%s_%02d",scfgWords[j].name,i-2),
                    Preferences::Name("0x%04x",v[j])
                );
            }
        }
    }
    return true;
}

bool PicUI::cfgWordsCB(CfgOper oper)
{
char line[105];
int i, j, selected;
unsigned int cfgWords[CONFIG_WORD_SETTINGS];
const char *lsel;
const char *cfgWordNames[] = {
    "Configuration Word Mask",
    "Configuration Word Base",
    "Configuration Word Default",
    "Code Protection Mask",
    "Code Protection On",
    "Code Protection Off",
    "Data Protection Mask",
    "Data Protection On",
    "Data Protection Off",
    "Background Debug Mask",
    "Background Debug On",
    "Background Debug Off"
};
bool ok=true;
static CfgOper lastOper=CFG_NEW;

    if (oper==CFG_SAVE){
        if (lastOper!=CFG_SAVE && lastOper!=CFG_DELETE) {
            for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
                //
                // elimino i caratteri non alfanumerici
                //
                strncpy(line,tx_CfgWord[i]->value(),sizeof(line)-1);
                line[sizeof(line)-1]='\0';
                for (j=0;j<strlen(line);j++) {
                    if (!isxdigit(line[j]) && line[j]!='x' && line[j]!='X') {
                        line[0]='\0';
                        break;
                    }
                }
                if (sscanf(line,"%x",&cfgWords[i])!=1 || cfgWords[i]>0xffff) {
                    fl_alert (
                        "Please insert a valid 4 digit hexadecimal number"
                        " for the field:\n%s.",
                        cfgWordNames[i]
                    );
                    ok = false;
                    tx_CfgWord[i]->color(FL_YELLOW);
                } else {
                    tx_CfgWord[i]->color(FL_WHITE);
                }
            }
            if (ok) {
                if (lastOper==CFG_NEW || lastOper==CFG_COPY) {
                    ls_ConfigWords->add("");
                    ls_ConfigWords->select(ls_ConfigWords->size());
                }
                if ((selected=ls_ConfigWords->value())>1) {
                    sprintf(line," %02d ",selected-2);
                    for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
                        strcat(line, Preferences::Name("| 0x%04x ",cfgWords[i]));
                        tx_CfgWord[i]->value("");
                    }
                    ls_ConfigWords->text(selected,line);
                } else {
                    ok = false;
                }
            }
        } else {
            ok = false;
        }
    } else if (
        (oper==CFG_LOAD || oper==CFG_EDIT || oper==CFG_COPY) &&
        (selected = ls_ConfigWords->value())
    ) {
        if (selected>1) {
            lsel = ls_ConfigWords->text(selected);
            if (
                sscanf (
                    lsel,
                    " %d | %x | %x | %x | %x | %x | %x |"
                         " %x | %x | %x | %x | %x | %x ",
                    &i,
                    &cfgWords[ 0],&cfgWords[ 1],&cfgWords[ 2],
                    &cfgWords[ 3],&cfgWords[ 4],&cfgWords[ 5],
                    &cfgWords[ 6],&cfgWords[ 7],&cfgWords[ 8],
                    &cfgWords[ 9],&cfgWords[10],&cfgWords[11]
                ) == CONFIG_WORD_SETTINGS + 1
            ) {
                for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
                    sprintf(line,"0x%04x",cfgWords[i]);
                    tx_CfgWord[i]->value(line);
                    tx_CfgWord[i]->color(FL_WHITE);
                }
            }
        } else {
            for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
                tx_CfgWord[i]->value("");
            }
            ls_ConfigWords->deselect();
            ok = false;
        }
    } else if (oper==CFG_DELETE && ls_ConfigWords->size()>1) {
        for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
            tx_CfgWord[i]->value("");
        }
        ls_ConfigWords->remove(ls_ConfigWords->size());
    } else if (oper==CFG_NEW) {
        for (i=0;i<CONFIG_WORD_SETTINGS;i++) {
            tx_CfgWord[i]->value("");
        }
    }
    if (ok) {
        lastOper = oper;
    }
    g_cfgwords->redraw();

    return ok;
}

void PicUI::deactivate()
{
    g_gencfg->deactivate();
    g_cfgwords->deactivate();
    g_miscellanea->deactivate();
}

void PicUI::activate()
{
    g_gencfg->activate();
    g_cfgwords->activate();
    g_miscellanea->activate();
}

bool PicUI::getVoltageRanges (
    const char *deviceSpec,
    double &vpp_min , double &vpp_max ,
    double &vddp_min, double &vddp_max,
    double &vdd_min , double &vdd_max
) {
    if (!match(deviceSpec)) {
        return false;
    }
    vpp_min  = atof(tx_Param[PicUI::PAR_VPP_MIN ]->value());
    vpp_max  = atof(tx_Param[PicUI::PAR_VPP_MAX ]->value());
    vddp_min = atof(tx_Param[PicUI::PAR_VDDP_MIN]->value());
    vddp_max = atof(tx_Param[PicUI::PAR_VDDP_MAX]->value());
    vdd_min  = atof(tx_Param[PicUI::PAR_VDD_MIN ]->value());
    vdd_max  = atof(tx_Param[PicUI::PAR_VDD_MAX ]->value());

    return true;
}
