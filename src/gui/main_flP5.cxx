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
#include "flP5.h"
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

using namespace std;

#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>

#include "Util.h"
#include "ParallelPort.h"

DataBuffer buf(16);
HexFile *hexFile = NULL;
Device *chip = NULL;
IO *io = NULL;

int currentDevice = -1;
int currentProgrammer = -1;

Preferences app(Preferences::USER,"flP5","flP5");
Preferences programmers(Preferences::USER,"flP5","programmers");
Preferences devices(Preferences::USER,"flP5","devices");

char *copyrightText =
"<HTML><BODY><CENTER>"
"<B>flP5 1.1.3</B><BR>"
"the Fast Light Parallel Port Production PIC Programmer.<BR>"
"Copyright (C) 2003 by Francesco Bradascio</CENTER><P>"
"</P>"
"This program is free software; you can redistribute it and/or modify "
"it under the terms of the <I>GNU General Public License</I> as published by "
"the Free Software Foundation; either version 2 of the License, or "
"(at your option) any later version.<P>"
"</P>"
"This program is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
"GNU General Public License for more details.<P>"
"</P>"
"You should have received a copy of the <I>GNU General Public License</I> "
"along with this program; if not, write to the Free Software "
"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA<P>"
"</P>"
"<CENTER>Please report bugs to fbradasc@yahoo.it.</CENTER></BODY></HTML>";

static char *cwNames[] = {
    "cw_mask", "cw_save",
    "cp_mask", "cp_all_", "cp_none",
    "dp_mask", "dp_on__", "dp_off_",
    "bd_mask", "bd_on__", "bd_off_"
};

static char *paramNames[] = {
    "wordSize" , "codeSize" , "eepromSize",
    "progCount", "progMult" ,
    "progTime" , "eraseTime",
    "vppMin"   , "vppMax"   ,
    "vddMin"   , "vddMax"   ,
    "vddpMin"  , "vddpMax"
};

static char *pinNames[] = {
    "icspClock",
    "icspDataIn",
    "icspDataOut",
    "icspVddOn",
    "icspVppOn",
    "selMinVdd",
    "selProgVdd",
    "selMaxVdd",
    "selVihhVpp"
};

static char *portAccess[] = { "DirectPP", "LinuxPPDev" };

bool verifyDeviceConfig(bool verbose)
{
const char *name;
int i, v;
double d[6];
bool ok = true;
const char *paramNames[] = {
    "Vpp Min" , "Vpp Max" ,
    "Vdd Min" , "Vdd Max" ,
    "Vddp Min", "Vddp Max"
};

    name = tx_devName->value();
    if (!name || strlen(name)==0) {
        if (verbose) {
            fl_alert("You must specify the device name.");
            return false;
        }
    } else {
        for (i=0;i<strlen(name);i++) {
            if (!isspace(name[i]) && !iscntrl(name[i])) {
                break;
            }
        }
        if (verbose && name[i]=='\0') {
            fl_alert("The device name must be a valid UNIX file name.");
            return false;
        }
        tx_devName->value(&name[i]);
    }
    for (v=0,i=LAST_PARAM-6;verbose && i<LAST_PARAM;i++,v++) {
        tx_devParam[i]->color(FL_WHITE);
        if (sscanf(tx_devParam[i]->value(),"%lf",&d[v])!=1 || d[v]<1.0) {
            ok = false;
            fl_alert (
                "The %s value must be a positive (>0) float.",
                paramNames[v]
            );
            tx_devParam[i]->color(FL_YELLOW);
        }
        tx_devParam[i]->redraw();
    }
    if (ok) {
        for (v=0;v<3;v++) {
            if (d[(2*v)]>d[(2*v)+1]) {
                fl_alert (
                    "The %s value must be greater or equal to the %s value.",
                    paramNames[(2*v)+1],paramNames[(2*v)]
                );
                tx_devParam[(2*v)+LAST_PARAM-6]->color(FL_YELLOW);
                tx_devParam[(2*v)+LAST_PARAM-6+1]->color(FL_YELLOW);
                ok = false;
            }
        }
    }
    for (i=0;verbose && i<LAST_PARAM-6;i++) {
        tx_devParam[i]->color(FL_WHITE);
        if (sscanf(tx_devParam[i]->value(),"%d",&v)!=1 || v<0) {
            ok = false;
            fl_alert (
                "The %s value must be a non negative (>=0) integer.",
                tx_devParam[i]->label()
            );
            tx_devParam[i]->color(FL_YELLOW);
        }
        tx_devParam[i]->redraw();
    }
    return ok;
}

bool loadDevicesSettings(const char *fname)
{
int i,j,k,level;
int v[12],v1[12];
double d, d1;
const Fl_Menu_Item *mitem;
char *mdata;
char buf[FL_PATH_MAX];
char buf1[FL_PATH_MAX];
char report[1024];
const char *pext, *pfname;
char path[FL_PATH_MAX];
char application[FL_PATH_MAX];
int settings;
bool loaded = false;
bool compare = false;
bool different = false;
bool store = false;

    if (fname && strlen(fname)) {
        pext   = fl_filename_ext(fname);
        pfname = fl_filename_name(fname);
        strncpy(path,fname,(pfname-fname));
        path[(pfname-fname)] = '\0';
        if (pext) {
            strncpy(application,pfname,(pext-pfname));
            application[(pext-pfname)] = '\0';
        } else {
            strcpy(application,pfname);
        }
        Preferences imports(path,"flP5",application);
        for (int vendor=0;vendor<imports.groups();vendor++) {
            Preferences vendors(imports,imports.group(vendor));
            for (int spec=0;spec<vendors.groups();spec++) {
                Preferences specs(vendors,vendors.group(spec));
                for (int device=0;device<specs.groups();device++) {
                    sprintf(
                        path,
                        "%s/%s/%s",
                        imports.group(vendor),
                        vendors.group(spec),
                        specs.group(device)
                    );
                    Preferences from(specs,specs.group(device));
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
                    for (i=0;i<LAST_PARAM-6;i++) {
                        settings += from.get(paramNames[i],v[0],0) ? 1 : 0;
                    }
                    for (;i<LAST_PARAM;i++) {
                        settings += from.get(paramNames[i],d,0.0) ? 1 : 0;
                    }
                    settings += from.get("configWords",v[0],0) ? 1 : 0;
                    for (i=0;i<v[0];i++) {
                        for (j=0;j<11;j++) {
                            settings += from.get (
                                Preferences::Name (
                                    "%s_%02d",
                                    cwNames[j],i
                                ),
                                buf,
                                "0x0000",
                                sizeof(buf)-1
                            ) ? 1 : 0;
                        }
                    }
                    store = false;
                    if (settings==(/* 1+ */ 1+LAST_PARAM+1+(11*v[0]))) {
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
                                    tb_devExperimental->value() ? FL_BLUE
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
                            for (i=0;i<LAST_PARAM-6;i++) {
                                from.get(paramNames[i],v[0],0);
                                  to.get(paramNames[i],v1[0],0);

                                if (v[0] != v1[0]) {
                                    sprintf(report,"@b%s:",paramNames[i]);
                                    ls_report->add(report);
                                    sprintf(report,"  cur=%d",v1[0]);
                                    ls_report->add(report);
                                    sprintf(report,"@_  new=%d",v[0]);
                                    ls_report->add(report);
                                    different = true;
                                }
                            }
                            for (;i<LAST_PARAM;i++) {
                                from.get(paramNames[i],d,0.0);
                                  to.get(paramNames[i],d1,0.0);

                                if (d != d1) {
                                    sprintf(report,"@b%s:",paramNames[i]);
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
                                for (j=0;j<11;j++) {
                                    from.get (
                                        Preferences::Name (
                                            "%s_%02d",
                                            cwNames[j],i
                                        ),
                                        buf,
                                        "0x0000",
                                        sizeof(buf)-1
                                    );
                                    to.get (
                                        Preferences::Name (
                                            "%s_%02d",
                                            cwNames[j],i
                                        ),
                                        buf1,
                                        "0x0000",
                                        sizeof(buf1)-1
                                    );
                                    if (strcmp(buf,buf1)) {
                                        ls_report->add (
                                            Preferences::Name (
                                                "@b%s_%02d:",
                                                cwNames[j],i
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
                            for (i=0;i<LAST_PARAM-6;i++) {
                                from.get(paramNames[i],v[0],0);
                                    to.set(paramNames[i],v[0]);
                            }
                            for (;i<LAST_PARAM;i++) {
                                from.get(paramNames[i],d,0.0);
                                    to.set(paramNames[i],d);
                            }
                            from.get("configWords",v[0],0);
                                to.set("configWords",v[0]);
                            for (i=0;i<v[0];i++) {
                                for (j=0;j<11;j++) {
                                    from.get (
                                        Preferences::Name (
                                            "%s_%02d",
                                            cwNames[j],i
                                        ),
                                        buf,
                                        "0x0000",
                                        sizeof(buf)-1
                                    );
                                        to.set (
                                            Preferences::Name (
                                                "%s_%02d",
                                                cwNames[j],i
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
                    }
                }
            }
        }
        loaded = true;
    }
    return loaded;
}
bool deviceConfigCB(CfgOper oper)
{
static int lastOper=-1;
int i,j,k,selected;
int v[12];
double d;
const Fl_Menu_Item *mitem;
char *mdata;
char buf[FL_PATH_MAX];
const char *pfname;
char path[FL_PATH_MAX];

    if (oper==CFG_DELETE) {
        if (lastOper==CFG_NEW || lastOper==CFG_EDIT || lastOper==CFG_COPY) {
            i = fl_ask (
                "Are you sure you want to revert the current device settings ?"
            );
        } else {
            i = fl_ask (
                "Are you sure you want to delete the current device"
                " configuration ?"
            );
        }
        if (!i) {
            return false;
        }
    }
    if (oper==CFG_LOAD || oper==CFG_NEW || oper==CFG_DELETE) {
        tx_devName->value("");
        ch_devMemType->value(0);
        for (i=0;i<LAST_PARAM;i++) {
            tx_devParam[i]->value("0");
        }
        if (
            oper==CFG_DELETE &&
            (
                lastOper!=CFG_NEW  &&
                lastOper!=CFG_EDIT &&
                lastOper!=CFG_COPY
            ) &&
            (mitem = ch_devices->mvalue()) &&
            (mdata = (char *)mitem->user_data())
        ) {
            ((Fl_Menu_Item*)mitem)->labelcolor(FL_BLACK);
            devices.deleteGroup((const char *)mdata);
            ch_devices->remove(ch_devices->value());
            if (
                ch_devices->value() &&
                ch_devices->value()==(ch_devices->size()-1)
            ) {
                ch_devices->value(ch_devices->value()-1);
            }
            ch_devices->set_changed();
            ch_devices->redraw();

            delete mdata;
        }
        for (i=0;i<11;i++) {
            tx_devCfgWord[i]->value("");
        }
        for (i=0;i<LAST_PARAM;i++) {
            tx_devParam[i]->color(FL_WHITE);
        }
        for (i=ls_devConfigWords->size();i>1;i--) {
            ls_devConfigWords->remove(i);
        }
        ls_devConfigWords->do_callback();
        tb_devExperimental->value(1);
    }
    if (oper==CFG_LOAD || oper==CFG_DELETE) {
        if (
            ch_devices->size() > 1 &&
            (mitem = ch_devices->mvalue()) &&
            (mdata = (char *)mitem->user_data()) &&
            strlen(mdata)
        ) {
            Preferences device(devices,(const char *)mdata);
            tx_devName->value(ch_devices->text());
            for (i=0;i<LAST_PARAM-6;i++) {
                device.get(paramNames[i],v[0],0);
                    sprintf(buf,"%d",v[0]);
                    tx_devParam[i]->value(buf);
            }
            for (;i<LAST_PARAM;i++) {
                device.get(paramNames[i],d,0.0);
                    sprintf(buf,"%.2lf",d);
                    tx_devParam[i]->value(buf);
            }
            device.get("configWords",v[0],0);
            for (i=0;i<v[0];i++) {
                ls_devConfigWords->add("");
                ls_devConfigWords->select(ls_devConfigWords->size());
                selected=ls_devConfigWords->value();
                sprintf(buf," %02d ",selected-2);
                for (j=0;j<11;j++) {
                    device.get (
                        Preferences::Name("%s_%02d",cwNames[j],i),
                        path,
                        "0x0000",
                        sizeof(path)-1
                    );
                    sprintf(buf,"%s| %s ",buf,path);
                    tx_devCfgWord[j]->value("");
                }
                ls_devConfigWords->text(selected,buf);
            }
            ls_devConfigWords->do_callback();

            device.get("experimental",v[0],1);
            tb_devExperimental->value(v[0] ? 1 : 0);

            strcpy(path,mdata);
            pfname = fl_filename_name((const char *)mdata);
            strncpy(buf,mdata,(pfname-mdata));
            buf[(pfname-mdata)] = '\0';
            if ((pfname-mdata)>0 && buf[(pfname-mdata)-1]=='/') {
                buf[(pfname-mdata)-1]='\0';
            }
            for (i=0;i<ch_devProgSpec->size();i++) {
                 ch_devProgSpec->value(i);
                 if (
                     (mitem=ch_devProgSpec->mvalue()) &&
                     (mdata=(char*)mitem->user_data()) &&
                     !strcmp(buf,mdata)
                 ) {
                     break;
                 }
            }
            device.get("memType",buf,"rom",sizeof(buf)-1);
            for (i=0;i<ch_devMemType->size();i++) {
                 ch_devMemType->value(i);
                 if (
                     (mitem=ch_devMemType->mvalue()) &&
                     (mdata=(char*)mitem->user_data()) &&
                     !strcmp(buf,mdata)
                 ) {
                     break;
                 }
            }
            if (chip) {
                delete chip;
                chip = NULL;
            }
            try {
                chip = Device::load(&device,(char *)path);
            } catch (std::exception &e) {
                fl_alert("Device init: %s", e.what());
                chip = NULL;
                return false;
            }
            if (chip) {
                chip->set_dump_cb(showMemoryDumpCB);
            }
        } else {
            return false;
        }
    } else if (oper==CFG_COPY) {
        tx_devName->value("");
    } else if (oper==CFG_EDIT) {
        /* do nothing */
    } else if (
        oper==CFG_IMPORT &&
        loadDevicesSettings (
            fl_file_chooser (
                "Device Settings file selection",
                "*.prefs",
                NULL,
                0
            )
        )
    ) {
        ch_devices->sort();

    } else if (oper==CFG_SAVE && verifyDeviceConfig(true)) {
        mitem = ch_devProgSpec->mvalue();
        mdata = (char *)mitem->user_data();
        sprintf(buf,"%s/%s",mdata,tx_devName->value());
        if (lastOper==CFG_EDIT) {
            ch_devices->replace(ch_devices->value(),tx_devName->value());
            if ((mitem = ch_devices->mvalue())) {
                ((Fl_Menu_Item*)mitem)->labelcolor (
                    tb_devExperimental->value() ? FL_BLUE : FL_BLACK
                );
                mdata = (char *)((Fl_Menu_Item*)mitem)->user_data();
                if (mdata) {
                    delete mdata;
                }
                ((Fl_Menu_Item*)mitem)->user_data((void*)strdup(buf));
                ch_devices->set_changed();
            }
        }
        if (lastOper==CFG_NEW || lastOper==CFG_COPY) {
            i = ch_devices->add (
                buf,
                (const char *)0,
                (Fl_Callback *)0,
                (void *)strdup(buf),
                0
            );
            ch_devices->value(i);
            ch_devices->set_changed();
            if ((mitem = ch_devices->mvalue())) {
                ((Fl_Menu_Item*)mitem)->labelcolor (
                    tb_devExperimental->value() ? FL_BLUE : FL_BLACK
                );
            }
        }
        while (!devices.groupExists(buf)) {
            Preferences device(devices,buf);
        }
        Preferences device(devices,buf);

        device.set("experimental",tb_devExperimental->value() ? 1 : 0);

        device.set (
            "memType",
            (const char*)ch_devMemType->mvalue()->user_data()
        );
        for (i=0;i<LAST_PARAM-6;i++) {
            if (sscanf(tx_devParam[i]->value(),"%d",&v[0])) {
                device.set(paramNames[i],v[0]);
            }
        }
        for (;i<LAST_PARAM;i++) {
            if (sscanf(tx_devParam[i]->value(),"%lf",&d)) {
                device.set(paramNames[i],d);
            }
        }
        device.set("configWords",ls_devConfigWords->size()-1);
        for (i=2;i<=ls_devConfigWords->size();i++) {
            if (
                sscanf(
                    strcpy(buf,ls_devConfigWords->text(i)),
                    " %d | %x | %x | %x | %x | %x |"
                    " %x | %x | %x | %x | %x | %x ",
                    &j,
                    &v[0],&v[1],&v[2],&v[3],&v[4],&v[5],
                    &v[6],&v[7],&v[8],&v[9],&v[10]
                ) == 12
            ) {
                for (j=0;j<11;j++) {
                    device.set (
                        Preferences::Name("%s_%02d",cwNames[j],i-2),
                        Preferences::Name("0x%04x",v[j])
                    );
                }
            }
        }
        ch_devices->sort();

    } else if (oper!=CFG_NEW) {
        t_devcfg->redraw();
        return false;
    }
    lastOper = oper;

    if (
        (mitem = ch_devices->mvalue()) &&
        (mdata = (char *)((Fl_Menu_Item*)mitem)->user_data())
    ) {
        app.set("device",(const char*)mdata);
    } else {
        app.set("device","");
    }

    t_devcfg->redraw();

    return true;
}

bool verifyProgrammerConfig(bool verbose)
{
const char *name;
int i;
char pins[26];
bool ok = true;

    name = tx_programmerName->value();
    if (!name || strlen(name)==0) {
        if (verbose) {
            fl_alert("You must specify the programmer name.");
            return false;
        }
    } else {
        for (i=0;i<strlen(name);i++) {
            if (!isspace(name[i]) && !iscntrl(name[i])) {
                break;
            }
        }
        if (verbose && name[i]=='\0') {
            fl_alert("The programmer name must be a valid UNIX file name.");
            return false;
        }
        tx_programmerName->value(&name[i]);
    }
    for (i=0;i<26;i++) {
        pins[i] = -1;
    }
    for (i=0;i<LAST_PIN;i++) {
        bx_pinName[i]->color(FL_WHITE);
        if (
            ch_pinNumber[i]->value() &&
            pins[ch_pinNumber[i]->value()] >= 0
        ) {
            if (verbose) {
                fl_alert (
                    "The pin #%d connected to %s\nis already connected to %s",
                    ch_pinNumber[i]->value(),
                    bx_pinName[i]->label(),
                    bx_pinName[pins[ch_pinNumber[i]->value()]]->label()
                );
            }
            bx_pinName[i]->color(FL_YELLOW);
        } else {
            pins[ch_pinNumber[i]->value()] = i;
        }
        bx_pinName[i]->redraw();
    }
    if (verbose) {
        i=0;
        i += (ch_pinNumber[ICSP_CLOCK]->value()) ? 1 : 0;
        i += (ch_pinNumber[ICSP_DATA_IN]->value()) ? 1 : 0;
        i += (ch_pinNumber[ICSP_DATA_OUT]->value()) ? 1 : 0;
        i += (ch_pinNumber[ICSP_VPP_ON]->value()) ? 1 : 0;

        if (i<4) {
            fl_alert (
                "ICSP Clock\n"
                "ICSP Data In\n"
                "ICSP Data Out\n"
                "ICSP Vpp On\n"
                "must be connected."
            );
            return false;
        }
    }
    return true;
}

bool loadProgrammersSettings(const char *fname)
{
const char *gname, *pext, *pfname;
char path[FL_PATH_MAX];
char application[FL_PATH_MAX];
char report[1024];
int settings, i, j;
int v[3], v1[3];
bool loaded = false;
bool compare = false;
bool different = false;
bool store = false;

    if (fname && strlen(fname)) {
        pext   = fl_filename_ext(fname);
        pfname = fl_filename_name(fname);
        strncpy(path,fname,(pfname-fname));
        path[(pfname-fname)] = '\0';
        if (pext) {
            strncpy(application,pfname,(pext-pfname));
            application[(pext-pfname)] = '\0';
        } else {
            strcpy(application,pfname);
        }
        Preferences imports(path,"flP5",application);
        for (i=0;i<imports.groups();i++) {
            gname = imports.group(i);
            Preferences from(imports,gname);
            //
            // check if it's a valid programmer settings group
            //
            settings = 0;
            for (j=0;j<LAST_PIN;j++) {
                settings += from.get(pinNames[j]           ,v[0],0)? 1 : 0;
            }
            settings += from.get("vddMinCond"              ,v[0],0)? 1 : 0;
            settings += from.get("vddProgCond"             ,v[0],0)? 1 : 0;
            settings += from.get("vddMaxCond"              ,v[0],0)? 1 : 0;
            settings += from.get("vppOffCond"              ,v[0],0)? 1 : 0;
            /* - Optional -
            settings += from.get("independentVddVppControl",v[0],0)? 1 : 0;
            */
            store = false;
            if (settings==(4 /* 5 */ + LAST_PIN)) {
                compare = false;
                if (!programmers.groupExists(gname)) {
                    j = ch_programmers->add(gname);
                    ch_programmers->value(j);
                    ch_programmers->set_changed();
                    ch_programmers->redraw();
                } else {
                    compare = true;
                }
                while (!programmers.groupExists(gname)) {
                    Preferences to(programmers,gname);
                }
                Preferences to(programmers,gname);
                if (compare) {
                    different = false;
                    ls_report->clear();

                    for (j=0;j<LAST_PIN;j++) {
                        from.get(pinNames[j],v[0] ,0);
                          to.get(pinNames[j],v1[0],0);
                        if (v[0] != v1[0]) {
                            sprintf (
                                report,
                                "@b%s:",
                                pinNames[j]
                            );
                            ls_report->add(report);
                            sprintf(report,"  cur=%d",v1[0]);
                            ls_report->add(report);
                            sprintf(report,"@_  new=%d",v[0]);
                            ls_report->add(report);
                            different = true;
                        }
                    }
                    from.get("vddMinCond",v[0] ,0);
                      to.get("vddMinCond",v1[0],0);

                    if (v[0] != v1[0]) {
                        ls_report->add("@bvddMinCond:");
                        sprintf(report,"  cur=%d",v1[0]);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%d",v[0]);
                        ls_report->add(report);
                        different = true;
                    }
                    from.get("vddProgCond",v[0] ,0);
                      to.get("vddProgCond",v1[0],0);

                    if (v[0] != v1[0]) {
                        ls_report->add("@bvddProgCond:");
                        sprintf(report,"  cur=%d",v1[0]);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%d",v[0]);
                        ls_report->add(report);
                        different = true;
                    }
                    from.get("vddMaxCond",v[0] ,0);
                      to.get("vddMaxCond",v1[0],0);

                    if (v[0] != v1[0]) {
                        ls_report->add("@bvddMaxCond:");
                        sprintf(report,"  cur=%d",v1[0]);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%d",v[0]);
                        ls_report->add(report);
                        different = true;
                    }
                    from.get("vppOffCond",v[0] ,0);
                      to.get("vppOffCond",v1[0],0);

                    if (v[0] != v1[0]) {
                        ls_report->add("@bvppOffCond:");
                        sprintf(report,"  cur=%d",v1[0]);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%d",v[0]);
                        ls_report->add(report);
                        different = true;
                    }
                    from.get("independentVddVppControl",v[0] ,0);
                      to.get("independentVddVppControl",v1[0],0);

                    if (v[0] != v1[0]) {
                        ls_report->add("@bindependentVddVppControl:");
                        sprintf(report,"  cur=%d",v1[0]);
                        ls_report->add(report);
                        sprintf(report,"@_  new=%d",v[0]);
                        ls_report->add(report);
                        different = true;
                    }
                    sprintf(report,"Programmer: %s",gname);
                    if (different && show_report_window(report)) {
                        store = true;
                    }
                }
                if (store || !compare) {
                    for (j=0;j<LAST_PIN;j++) {
                        from.get(pinNames[j]   ,v[0],0);
                          to.set(pinNames[j]   ,v[0]  );
                    }
                    from.get("vddMinCond"              ,v[0],0);
                      to.set("vddMinCond"              ,v[0]  );
                    from.get("vddProgCond"             ,v[0],0);
                      to.set("vddProgCond"             ,v[0]  );
                    from.get("vddMaxCond"              ,v[0],0);
                      to.set("vddMaxCond"              ,v[0]  );
                    from.get("vppOffCond"              ,v[0],0);
                      to.set("vppOffCond"              ,v[0]  );
                    from.get("independentVddVppControl",v[0],0);
                      to.set("independentVddVppControl",v[0]  );
                }
            }
        }
        loaded = true;
    }
    return loaded;
}
bool programmerConfigCB(CfgOper oper)
{
static int lastOper=-1;
int i,j;
int v[3];
char pins[26];

    if (oper==CFG_DELETE) {
        if (lastOper==CFG_NEW || lastOper==CFG_EDIT || lastOper==CFG_COPY) {
            i = fl_ask (
                "Are you sure you want to revert the current programmer"
                " settings ?"
            );
        } else {
            i = fl_ask (
                "Are you sure you want to delete the current programmer"
                " configuration ?"
            );
        }
        if (!i) {
            return false;
        }
    }
    if (oper==CFG_LOAD || oper==CFG_NEW || oper==CFG_DELETE) {
        tx_programmerName->value("");
        for (i=0;i<LAST_PIN;i++) {
            tb_pinInvert[i]->value(0);
            ch_pinNumber[i]->value(0);
        }
        for (i=0;i<3;i++) {
            tb_vddMinCond [i]->value(0);
            tb_vddProgCond[i]->value(0);
            tb_vddMaxCond [i]->value(0);
            tb_vddMinCond [i]->deactivate();
            tb_vddProgCond[i]->deactivate();
            tb_vddMaxCond [i]->deactivate();
            tb_vddMinCond [i]->redraw();
            tb_vddProgCond[i]->redraw();
            tb_vddMaxCond [i]->redraw();
        }
        tb_vppOffCond->value(0);
        tb_vppOffCond->deactivate();
        tb_vppOffCond->redraw();

        tb_saVddVppControl->value(0);

        if (
            oper==CFG_DELETE &&
            (lastOper!=CFG_NEW && lastOper!=CFG_EDIT && lastOper!=CFG_COPY)
        ) {
            programmers.deleteGroup(ch_programmers->text());
            ch_programmers->remove(ch_programmers->value());
            if (
                ch_programmers->value() &&
                ch_programmers->value()==(ch_programmers->size()-1)
            ) {
                ch_programmers->value(ch_programmers->value()-1);
            }
            ch_programmers->set_changed();
            ch_programmers->redraw();
        }
        for (i=0;i<LAST_PIN;i++) {
            bx_pinName[i]->color(FL_WHITE);
            bx_pinName[i]->redraw();
        }
    }
    if (oper==CFG_LOAD || oper==CFG_DELETE) {
        if (
            ch_programmers->size() > 1 &&
            ch_programmers->text()     &&
            strlen(ch_programmers->text())
        ) {
            Preferences programmer(programmers,ch_programmers->text());
            tx_programmerName->value(ch_programmers->text());
            // to format the name of the programmer:
            verifyProgrammerConfig(false);
            for (i=0;i<LAST_PIN;i++) {
                programmer.get(pinNames[i],v[0],0);
                    tb_pinInvert[i]->value(v[0]<0);
                    ch_pinNumber[i]->value(abs(v[0]));
                    ch_pinNumber[i]->set_changed();
            }
            programmer.get("vddMinCond" ,v[0],0);
            programmer.get("vddProgCond",v[1],0);
            programmer.get("vddMaxCond" ,v[2],0);
                for (i=0;i<3;i++) {
                    tb_vddMinCond [i]->value((v[0]>>i) & 0x01);
                    tb_vddProgCond[i]->value((v[1]>>i) & 0x01);
                    tb_vddMaxCond [i]->value((v[2]>>i) & 0x01);
                }
            programmer.get("vppOffCond",v[0],0);
                tb_vppOffCond->value(v[0]>0);

            programmer.get("independentVddVppControl",v[0],0);
                tb_saVddVppControl->value(v[0]>0);

            if (ch_pinNumber[SEL_MIN_VDD]->value()) {
                tb_vddMinCond [0]->deactivate();
                tb_vddProgCond[0]->activate();
                tb_vddMaxCond [0]->activate();
            } else {
                tb_vddMinCond [0]->deactivate();
                tb_vddProgCond[0]->deactivate();
                tb_vddMaxCond [0]->deactivate();
            }
            if (ch_pinNumber[SEL_PRG_VDD]->value()) {
                tb_vddMinCond [1]->activate();
                tb_vddProgCond[1]->deactivate();
                tb_vddMaxCond [1]->activate();
            } else {
                tb_vddMinCond [1]->deactivate();
                tb_vddProgCond[1]->deactivate();
                tb_vddMaxCond [1]->deactivate();
            }
            if (ch_pinNumber[SEL_MAX_VDD]->value()) {
                tb_vddMinCond [2]->activate();
                tb_vddProgCond[2]->activate();
                tb_vddMaxCond [2]->deactivate();
            } else {
                tb_vddMinCond [2]->deactivate();
                tb_vddProgCond[2]->deactivate();
                tb_vddMaxCond [2]->deactivate();
            }
            if (ch_pinNumber[SEL_VIHH_VPP]->value()) {
                tb_vppOffCond->activate();
            } else {
                tb_vppOffCond->deactivate();
            }
            verifyProgrammerConfig(false);

            app.get("portNumber",i,0);
            app.get("portAccessMethod",j,0);
            if (io) {
                delete io;
                io = NULL;
            }
            try {
                io = IO::acquire(&programmer,portAccess[j],i);
            } catch (std::exception& e) {
                fl_alert("I/O init: %s", e.what());
                io = NULL;
                return false;
            }
        } else {
            return false;
        }
    } else if (oper==CFG_COPY) {
        tx_programmerName->value("");
    } else if (oper==CFG_EDIT) {
        /* do nothing */
    } else if (
        oper==CFG_IMPORT &&
        loadProgrammersSettings (
            fl_file_chooser (
                "Programmer Settings file selection",
                "*.prefs",
                NULL,
                0
            )
        )
    ) {
        ch_programmers->sort();
    } else if (oper==CFG_SAVE && verifyProgrammerConfig(true)) {
        if (lastOper==CFG_EDIT) {
            ch_programmers->replace (
                ch_programmers->value(),
                tx_programmerName->value()
            );
            ch_programmers->set_changed();
        }
        if (lastOper==CFG_NEW || lastOper==CFG_COPY) {
            i = ch_programmers->add(tx_programmerName->value());
            ch_programmers->value(i);
            ch_programmers->set_changed();
        }
        Preferences programmer (
            programmers,
            ch_programmers->text(ch_programmers->value())
        );
        for (i=0;i<LAST_PIN;i++) {
            v[0] = abs(ch_pinNumber[i]->value()) *
                   ((tb_pinInvert[i]->value())?-1:1);
            programmer.set(pinNames[i],v[0]);
        }
        v[0] = v[1] = v[2] = 0;
        for (i=0;i<3;i++) {
            v[0] |= ((tb_vddMinCond [i]->value() & 0x01)<<i);
            v[1] |= ((tb_vddProgCond[i]->value() & 0x01)<<i);
            v[2] |= ((tb_vddMaxCond [i]->value() & 0x01)<<i);
        }
        programmer.set("vddMinCond" ,v[0]);
        programmer.set("vddProgCond",v[1]);
        programmer.set("vddMaxCond" ,v[2]);

        v[0] = (tb_vppOffCond->value()>0);
        programmer.set("vppOffCond",v[0]);

        v[0] = (tb_saVddVppControl->value()>0);
        programmer.set("independentVddVppControl",v[0]);

        ch_programmers->sort();

    } else if (oper!=CFG_NEW) {
        t_progcfg->redraw();
        return false;
    }
    lastOper = oper;

    app.set("programmer",ch_programmers->value());
    t_progcfg->redraw();

    return true;
}

void loadPreferences(void)
{
int groups,vendors,specs,devs;
int i,j,k,level,devnum;
int available, experimental;
char buf[256];
const Fl_Menu_Item *mitem;
const char *mdata, *cfgFile;

    for (i=1; i<=ParallelPort::ports.count; i++) {
#ifdef WIN32
        sprintf(buf,"lpt%d: address %X",i,ParallelPort::ports.address[i-1]);
#else
        sprintf(buf,"lp%d: address %X",i-1,ParallelPort::ports.address[i-1]);
#endif
        sm_ppNumber[i].label(strdup(buf));
        sm_ppNumber[i].show();
    }
    for (;i<=MAX_LPTPORTS;i++) {
        sm_ppNumber[i].hide();
    }
    app.get("portNumber",i,0);
    sm_ppNumber[((i<0 || i>=ParallelPort::ports.count)?1:(i+1))].setonly();

#if defined(linux) && defined(ENABLE_LINUX_PPDEV)
    sm_ppAccessMethod->show();
    linux_pp_dev->show();
    app.get("portAccessMethod",i,0);
#else
    linux_pp_dev->hide();
    sm_ppAccessMethod->hide();
    i=0;
#endif

    sm_ppAccessMethod[i+1].setonly();

    available=0;
    if (!(groups = programmers.groups())) {
        // load default programmers settings
        available += loadProgrammersSettings (
            Util::getDistFile("/data/programmers.prefs")
        ) ? 1 : 0;
    }
    for (i=0;i<groups;i++) {
        ch_programmers->add(programmers.group(i));
        available++;
    }
    available += app.get("programmer",i,0);
    ch_programmers->sort();
    ch_programmers->value(i);
    ch_programmers->set_changed();
    if (available) {
        ch_programmers->do_callback();
    }
    available = 0;
    if (!(vendors = devices.groups())) {
        // load default devices settings
        available += loadDevicesSettings (
            Util::getDistFile("/data/devices.prefs")
        ) ? 1 : 0;
    }
    for (i=0;i<vendors;i++) {
        Preferences vendor(devices,devices.group(i));
        specs = vendor.groups();
        for (j=0;j<specs;j++) {
            Preferences spec(vendor,vendor.group(j));
            devs = spec.groups();
            for (k=0;k<devs;k++) {
                sprintf(
                    buf,
                    "%s/%s/%s",
                    devices.group(i),
                    vendor.group(j),
                    spec.group(k)
                );
                ch_devices->add (
                    buf,
                    (const char*)0,
                    (Fl_Callback*)0,
                    (void*)strdup(buf),
                    0
                );
            }
        }
    }
    ch_devices->sort();

    available += app.get("device",buf,"",sizeof(buf));
    for (
        devnum = -1, i = 0, level = 0, mitem = ch_devices->menu();
        // null label at level 0 means end of menu
        mitem && ( level || mitem->label() );
        mitem++, i++
    ) {
        if (mitem->submenu()) { // submenu: down one level
            level++;
        } else if (!mitem->label()) { // null label: up one level
            level--;
        } else {
            if ((mdata = (const char *)mitem->user_data())) {
                Preferences dev(devices,mdata);
                dev.get("experimental",experimental,1);
                ((Fl_Menu_Item*)mitem)->labelcolor (
                    experimental ? FL_BLUE : FL_BLACK
                );
                if (
                    available &&
                    ( buf[0]=='\0' || strcmp(buf,(const char *)mdata)==0 )
                ) {
                    devnum=i;
                    available=0;
                }
            }
        }
    }
    if (devnum>=0) {
        ch_devices->value(devnum);
        ch_devices->set_changed();
        ch_devices->redraw();
        ch_devices->do_callback();
    }
}

bool cfgWordsCB(CfgOper oper)
{
char line[105];
int i, j, selected;
unsigned int cfgWords[11];
const char *lsel;
const char *cfgWordNames[] = {
    "Configuration Word Mask",
    "Configuration Word Base",
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
            for (i=0;i<11;i++) {
                //
                // elimino i caratteri non alfanumerici
                //
                strncpy(line,tx_devCfgWord[i]->value(),sizeof(line)-1);
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
                    tx_devCfgWord[i]->color(FL_YELLOW);
                } else {
                    tx_devCfgWord[i]->color(FL_WHITE);
                }
            }
            if (ok) {
                if (lastOper==CFG_NEW || lastOper==CFG_COPY) {
                    ls_devConfigWords->add("");
                    ls_devConfigWords->select(ls_devConfigWords->size());
                }
                if ((selected=ls_devConfigWords->value())>1) {
                    sprintf(line," %02d ",selected-2);
                    for (i=0;i<11;i++) {
                        sprintf(line,"%s| 0x%04x ",line,cfgWords[i]);
                        tx_devCfgWord[i]->value("");
                    }
                    ls_devConfigWords->text(selected,line);
                } else {
                    ok = false;
                }
            }
        } else {
            ok = false;
        }
    } else if (
        (oper==CFG_LOAD || oper==CFG_EDIT || oper==CFG_COPY) &&
        (selected = ls_devConfigWords->value())
    ) {
        if (selected>1) {
            lsel = ls_devConfigWords->text(selected);
            if (
                sscanf (
                    lsel,
                    " %d | %x | %x | %x | %x | %x |"
                    " %x | %x | %x | %x | %x | %x ",
                    &i,
                    &cfgWords[0],&cfgWords[1],&cfgWords[2],
                    &cfgWords[3],&cfgWords[4],&cfgWords[5],
                    &cfgWords[6],&cfgWords[7],&cfgWords[8],
                    &cfgWords[9],&cfgWords[10]
                ) == 12
            ) {
                for (i=0;i<11;i++) {
                    sprintf(line,"0x%04x",cfgWords[i]);
                    tx_devCfgWord[i]->value(line);
                    tx_devCfgWord[i]->color(FL_WHITE);
                }
            }
        } else {
            for (i=0;i<11;i++) {
                tx_devCfgWord[i]->value("");
            }
            ls_devConfigWords->deselect();
            ok = false;
        }
    } else if (oper==CFG_DELETE && ls_devConfigWords->size()>1) {
        for (i=0;i<11;i++) {
            tx_devCfgWord[i]->value("");
        }
        ls_devConfigWords->remove(ls_devConfigWords->size());
    } else if (oper==CFG_NEW) {
        for (i=0;i<11;i++) {
            tx_devCfgWord[i]->value("");
        }
    }
    if (ok) {
        lastOper = oper;
    }
    g_devcfgwords->redraw();

    return ok;
}

bool showMemoryDumpCB(void *data,const char *line,int progress)
{
    if (line) {
        ls_memdump->add(line);
    } else {
        ls_memdump->clear();
    }
    return true;
}

void dumpHexFile(bool set_wordsize=false)
{
    if (chip) {
        if (set_wordsize) {
            buf.set_wordsize(chip->get_wordsize());
        }
        chip->dump(buf);
    }
}

static bool progressOperation(void *data, long addr, int percent)
{
static int last_percent = -1;
static char msg[256];
static char oper[256];

#ifdef WIN32
  static LARGE_INTEGER last, start;
  LARGE_INTEGER now;
#else
  static struct timeval last, start;
  struct timeval now;
#endif

static double freq = 1000000.0;

double remaining, elaphsed, estimated;
int rmin, emin, smin;
double rsec, esec, ssec;


    if  (percent<0) {
        oper[0] = '\0';
        if (data) {
            strcpy(oper,(char*)data);
        }
        last_percent = 101;
        p_progress->label("");
        p_progress->value(0);
        p_progress->minimum(0);
        p_progress->maximum(100);
        flP5->redraw();
        Fl::flush();
#ifdef WIN32
        if (QueryPerformanceFrequency(&start)) {
            freq = (double)start.QuadPart / 2.0; // circa ;-<
        }
        QueryPerformanceCounter(&last);
#else
        gettimeofday(&last,NULL);
#endif
        start = last;
    } else {
        if (percent<=100 && percent != last_percent) {
#ifdef WIN32
            QueryPerformanceCounter(&now);
            elaphsed  = (double)(now.QuadPart - start.QuadPart);
            remaining = (double)(now.QuadPart - last.QuadPart);
#else
            gettimeofday(&now, NULL);
            elaphsed  = (double)(now.tv_usec - start.tv_usec);
            remaining = (double)(now.tv_usec - last.tv_usec);
#endif
            if (percent>last_percent) {
                remaining /= (double)(percent-last_percent);
            }
            remaining /= (double)freq;

            remaining *= (double)(100 - percent);
            rmin = (int)( remaining / 60 );
            rsec = (double)( remaining - (double)( rmin * 60 ) );

            elaphsed  /= (double)freq;
            emin = (int)( elaphsed / 60 );
            esec = (double)( elaphsed - (double)( emin * 60 ) );

            estimated = ( elaphsed / ( (percent>0) ? percent : 1 ) ) * 100;
            smin = (int)( estimated / 60 );
            ssec = (double)( estimated - (double)( smin * 60 ) );

            sprintf (
                msg,
                // " Address: 0x%06lx, %3d%% done",
                // " %s: [%3d%%] %2d'%2d\" /%2d'%2d\" -%2d'%2d\"",
                " %s: [%3d%%] %2d'%4.1lf\" /%2d'%4.1lf\"",
                oper,
                percent,
                rmin, rsec,
                smin, ssec //,
                // rmin, rsec
            );
            p_progress->label(msg);
            p_progress->value(percent);
            p_progress->redraw();
            Fl::flush();
            last_percent = percent;
            last = now;
        }
    }
    return true;
}

bool processOperation(ChipOper oper)
{
static int lastDevice=-1;
static int lastProgrammer=-1;
double vppMin, vppMax;
double vddMin, vddMax;
double vddpMin, vddpMax;
bool proceed = true, forceCalibration = true;
char *soper[] = {
    /* CHIP_READ           */ "Read",
    /* CHIP_ERASE          */ "Erase",
    /* CHIP_BLANCK_CHECK   */ "Blank Chk",
    /* CHIP_WRITE          */ "Write",
    /* CHIP_VERIFY         */ "Verify @@ VDD prg.",
    /* CHIP_VERIFY_MIN     */ "Verify @@ VDD min.",
    /* CHIP_VERIFY_MAX     */ "Verify @@ VDD max.",
    /* CHIP_TEST_ON        */ "On Circuit Test Mode Active",
    /* CHIP_TEST_OFF       */ "",
    /* CHIP_TEST_RESET     */ "Resetting Device",
    /* CHIP_TEST_RESET_ON  */ "Resetting Device",
    /* CHIP_TEST_RESET_OFF */ "",
    /* CHIP_CALIBRATE      */ "",
    ""
};

    if (
        currentDevice>=0 &&
        currentProgrammer>=0 &&
        chip &&
        io
    ) {
        if (oper == CHIP_TEST_RESET) {
            io->vpp(IO::VPP_TO_GND);
            io->usleep(100000);     // 0.1" reset time
            io->vpp(IO::VPP_TO_VDD);
        } else if (oper == CHIP_TEST_RESET_ON) {
            io->vpp(IO::VPP_TO_GND);
        } else if (oper == CHIP_TEST_RESET_OFF) {
            io->vpp(IO::VPP_TO_VDD);
        } else if (oper == CHIP_TEST_OFF) {
            io->vpp(IO::VPP_TO_GND);
            io->vdd(IO::VDD_TO_OFF);
        } else {
            make_regcheck_window();
            if (
                ( oper == CHIP_ERASE || oper == CHIP_WRITE ) && 
                !fl_ask (
                    "Are you sure you want to %s the memory of the %s ?",
                    (oper == CHIP_ERASE) ? "erase" : "overwrite",
                    chip->get_name().c_str()
                )
            ) {
                return false;
            }
            io->clock(false);
            io->data(false);
            io->vpp(IO::VPP_TO_GND);
            io->vdd(IO::VDD_TO_OFF);
            forceCalibration = (
                currentDevice     != lastDevice ||
                currentProgrammer != lastProgrammer
            );
            if (forceCalibration || oper == CHIP_CALIBRATE) {
               proceed = make_calibration_window (
                   forceCalibration,
                   chip->get_name().c_str(),
                   atof(tx_devParam[PAR_VPP_MIN]->value()),
                   atof(tx_devParam[PAR_VPP_MAX]->value()),
                   atof(tx_devParam[PAR_VDDP_MIN]->value()),
                   atof(tx_devParam[PAR_VDDP_MAX]->value()),
                   atof(tx_devParam[PAR_VDD_MIN]->value()),
                   atof(tx_devParam[PAR_VDD_MAX]->value())
               );
            }
            if (!proceed) {
                return false;
            }
            lastDevice     = currentDevice;
            lastProgrammer = currentProgrammer;
        
            if (oper != CHIP_CALIBRATE) {
                io->vdd(IO::VDD_TO_PRG);
            }
            chip->set_iodevice(io);
            chip->set_progress_cb(progressOperation);

            // init progress bar & time remaining calculations
            progressOperation((void*)soper[oper],0,-1);
        
            switch (oper) {
                case CHIP_READ:
                    buf.set_wordsize(chip->get_wordsize());
                    try {
                        chip->read(buf);
                    } catch(std::exception& e) {
                        fl_alert("%s: %s",chip->get_name().c_str(),e.what());
                    }
                    chip->dump(buf);
                break;
                case CHIP_ERASE:
                    try {
                        chip->erase();
                    } catch(std::exception& e) {
                        fl_alert("%s: %s",chip->get_name().c_str(),e.what());
                    }
                break;
                case CHIP_BLANCK_CHECK: {
                    DataBuffer lbuf(chip->get_wordsize());
                    try {
                        chip->read(lbuf,true);
                    } catch(std::exception& e) {
                        fl_message("%s\nDevice is not blank.",e.what());
                        progressOperation((void*)0,0,-1);
                        return false;
                    }
                    fl_message("Device is blank.");
                } break;
                case CHIP_WRITE:
                    buf.set_wordsize(chip->get_wordsize());
                    try {
                        chip->program(buf);
                    } catch(std::exception& e) {
                        fl_alert("%s: %s",chip->get_name().c_str(),e.what());
                    }
                break;
                case CHIP_VERIFY: {
                    for (int i=0; i < ((io->production())?3:1); i++) {
                        if (i==1) {
                            io->vdd(IO::VDD_TO_MIN);
                        } else if (i==2) {
                            io->vdd(IO::VDD_TO_MAX);
                        }
                        progressOperation((void*)soper[oper+i],0,-1);
                        try {
                            chip->read(buf,true);
                        } catch(std::exception& e) {
                            fl_message (
                                "%s\n\n%s",
                                (i==0) ? "Verify @@ VDD prog."
                                       : (i==1) ? "Verify @@ VDD min."
                                                : "Verify @@ VDD max.",
                                e.what()
                            );
                        }
                    }
                } break;
                case CHIP_TEST_ON:
                    io->vdd(IO::VDD_TO_PRG);
                    io->vpp(IO::VPP_TO_GND);
                    io->vdd(IO::VDD_TO_ON);
                    io->usleep(100000);      // 0.1" reset time
                    io->vpp(IO::VPP_TO_VDD); // let's go
                break;
                default:
                break;
            }
            progressOperation((void*)0,0,-1);
        }
    }
    return true;
}

void loadHexFile(void)
{
char *fname;

    if (hexFile) {
        delete hexFile;
    }
    hexFile = 0;

    fname = fl_file_chooser (
        "HEX file selection",
        "*.hex",
        NULL,
        0
    );
    if (fname && strlen(fname)) {
        try {
            /* Read the hex file into the data buffer */
            hexFile = HexFile::load(fname);
        } catch (std::exception& e) {
            fl_message("%s: %s\n",fname,e.what());
            hexFile = 0;
            return;
        }
        if (chip) {
            /* Clear the data buffer */
            buf.clear();
            /* reads the hex file into memory */
            try {
                hexFile->read(buf);
            } catch (std::exception& e) {
                fl_message("%s\n",e.what());
                return;
            }
        }
        dumpHexFile();
    }
}

void saveHexFile(void)
{
char *fname;
HexFile_ihx8 *hf;

    if (chip) {
        fname = fl_file_chooser (
            "HEX file selection for saving",
            "*.hex",
            "",
            0
        );
        if (fname && strlen(fname)) {
            try {
                hf = new HexFile_ihx8(fname);
            } catch (std::exception& e) {
                fl_message("%s: %s\n",fname,e.what());
            }
            /* Write the data buffer into the hex file */
            try {
                /* Get the device memory map so we know what
                 * parts of the buffer are valid and save
                 * those parts to the hex file.
                 */
                IntPairVector mmap = chip->get_mmap();
                for (
                    IntPairVector::iterator n = mmap.begin();
                    n != mmap.end();
                    n++
                ) {
                    hf->write(buf,n->first,n->second);
                }
            } catch (std::exception& e) {
                fl_message("%s: %s\n",fname,e.what());
            }
            delete hf;
        }
    }
}

void showManual(void)
{
const char *pdocf = Util::getDistFile("/doc/flP5.pdf");

    if (pdocf && pdocf[0]!='\0') {
#ifdef WIN32
        ShellExecute(NULL,"open",pdocf,NULL,NULL,SW_SHOWNORMAL);
#else
        char str[256];
        sprintf(str,"acroread %s &",pdocf);
        system(str);
#endif
    }
}

int handle(int e)
{
    // this is used to stop Esc from exiting the program:
    return ((e==FL_SHORTCUT)&&(Fl::event_key()==FL_Escape));
}

static void sighandler(int sig)
{
    fprintf(stderr, "Caught signal %d.\n", sig);
    if (io != NULL) {
        delete io;
        io = NULL;
    }
    exit(1);
}

#if defined(CHECK_REGISTRATION)
bool checkRegistration(unsigned int prodCode, unsigned int regKey)
{
unsigned int a=prodCode, b=regKey;

    return CHECK_REGISTRATION;
}
#endif

int main(int argc, char **argv)
{
    io = NULL;

#ifndef WIN32
    /* Set UID back to user if running setuid */
    Util::setUser(getuid());

    /* Catch some signals to properly shut down the hardware */
    signal( SIGHUP , sighandler );
    signal( SIGINT , sighandler );
    signal( SIGQUIT, sighandler );
    signal( SIGPIPE, sighandler );
    signal( SIGTERM, sighandler );
#endif

    Util::setProgramPath(argv[0]);

    fl_register_images();
    Fl::add_handler(handle);
    if (make_flP5()) {
        flP5->show(argc, argv);
    }
    Fl::run();
    app.flush();

    if (io != NULL) {
        delete io;
    }
    return 0;
}
