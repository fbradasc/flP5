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
#else
#  include <windows.h>
#endif
#include <exception>
#include <iterator>
#include <cmath>

using namespace std;

#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>
#include <FL/Fl_Tooltip.H>

#include "DeviceUI.h"
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

const char *copyrightText =
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

static t_NamedSettings spropDly[] = {
    "signalDelay.default"               , (void *)0,
    "signalDelay.additional"            , (void *)0,
    "signalDelay.read"                  , (void *)0,
    "signalDelay.read.clk"              , (void *)0,
    "signalDelay.read.data"             , (void *)0,
    "signalDelay.read.vpp"              , (void *)0,
    "signalDelay.read.vdd"              , (void *)0,
    "signalDelay.write"                 , (void *)0,
    "signalDelay.write.clk"             , (void *)0,
    "signalDelay.write.data"            , (void *)0,
    "signalDelay.write.vpp"             , (void *)0,
    "signalDelay.write.vdd"             , (void *)0,
    "signalDelay.write.clk.high_to_low" , (void *)0,
    "signalDelay.write.data.high_to_low", (void *)0,
    "signalDelay.write.vpp.high_to_low" , (void *)0,
    "signalDelay.write.vdd.high_to_low" , (void *)0,
    "signalDelay.write.clk.low_to_high" , (void *)0,
    "signalDelay.write.data.low_to_high", (void *)0,
    "signalDelay.write.vpp.low_to_high" , (void *)0,
    "signalDelay.write.vdd.low_to_high" , (void *)0
};

static t_NamedSettings spins[] = {
    "icspClock"  , (void *)0,
    "icspDataIn" , (void *)0,
    "icspDataOut", (void *)0,
    "icspVddOn"  , (void *)0,
    "icspVppOn"  , (void *)0,

    "selMinVdd"  , (void *)0,
    "selProgVdd" , (void *)0,
    "selMaxVdd"  , (void *)0,
    "selVihhVpp" , (void *)0,

    "ispVcc"     , (void *)0,
    "ispReset"   , (void *)0,
    "ispMiso"    , (void *)0,
    "ispMosi"    , (void *)0,
    "ispSck"     , (void *)0,
};

static const char *portAccess[] = { "DirectPP", "LinuxPPDev" };

bool verifyDeviceConfig(bool verbose)
{
const char *name;
int i;
bool ok = true;

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
    const Fl_Menu_Item *mitem = ch_devProgSpec->mvalue();
    const char         *mdata = (const char *)mitem->user_data();

    return DeviceUI::verifyConfig(mdata,verbose);
}

void initDevicesSettings(void)
{
int i,j,v;
double d;
char buf[FL_PATH_MAX];

    for (int vendor=0;vendor<devices.groups();vendor++) {
        Preferences vendors(devices,devices.group(vendor));
        for (int spec=0;spec<vendors.groups();spec++) {
            Preferences specs(vendors,vendors.group(spec));
            for (int device=0;device<specs.groups();device++) {
                Preferences dev(specs,specs.group(device));

                DeviceUI::initDevicesSettings(devices.group(vendor),vendors.group(spec),dev);
            }
        }
    }
}

bool loadDevicesSettings(const char *fname)
{
const char *pext, *pfname;
char path[FL_PATH_MAX];
char application[FL_PATH_MAX];
bool loaded = false;

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

                    DeviceUI::loadSettings (
                        imports.group(vendor),
                        vendors.group(spec),
                        path,from
                    );
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
double d;
const Fl_Menu_Item *mitem;
char *mdata;
char buf[FL_PATH_MAX];
const char *pfname;
char path[FL_PATH_MAX];

    if (oper==CFG_DELETE) {
        if (lastOper==CFG_NEW || lastOper==CFG_EDIT || lastOper==CFG_COPY) {
            i = fl_choice (
                "Are you sure you want to revert the current device settings ?",
                "Cancel", "Revert", NULL
            );
        } else {
            i = fl_choice (
                "Are you sure you want to delete the current device",
                "Cancel", "Delete", "No op"
            );
        }
        if (!i) {
            return false;
        }
    }
    if (oper==CFG_LOAD || oper==CFG_NEW || oper==CFG_DELETE) {
        tx_devName->value("");
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
        DeviceUI::cleanConfigFields();
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
                     mitem->do_callback(ch_devProgSpec);
                     break;
                 }
            }

            bool reset = DeviceUI::resetConfigFields(device, buf, path);

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

        bool isExperimental = DeviceUI::isExperimental(mdata);

        if (lastOper==CFG_EDIT) {
            ch_devices->replace(ch_devices->value(),tx_devName->value());
            if ((mitem = ch_devices->mvalue())) {
                ((Fl_Menu_Item*)mitem)->labelcolor (
                    isExperimental ? FL_BLUE : FL_BLACK
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
                    isExperimental ? FL_BLUE : FL_BLACK
                );
            }
        }

        DeviceUI::saveConfig(mdata, buf);

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
    if (verbose) {
        int j=0, k=0;
        for (i=0;i<PARPORT_PINS;i++) {
            if ( ch_pinConnection[i]->value() == ParallelPort::SCK       ||
                 ch_pinConnection[i]->value() == ParallelPort::SDI_MISO  ||
                 ch_pinConnection[i]->value() == ParallelPort::SDO_MOSI  ||
                 ch_pinConnection[i]->value() == ParallelPort::VDD_VCC   ||
                 ch_pinConnection[i]->value() == ParallelPort::VPP_RESET  )
            {
                j++;
            }
        }
        if (j<5) {
            fl_alert (
                "CLK/SCK\n"
                "DIN/MISO\n"
                "DOUT/MOSI\n"
                "VDD/VCC\n"
                "VPP/RESET\n"
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
            settings += from.get("vddMinCond" ,v[0],0)? 1 : 0;
            settings += from.get("vddProgCond",v[0],0)? 1 : 0;
            settings += from.get("vddMaxCond" ,v[0],0)? 1 : 0;
            settings += from.get("vppOffCond" ,v[0],0)? 1 : 0;

            for (j=0;j<PARPORT_PINS;j++) {
                settings += from.get(Preferences::Name("parportPin_%02d",j),v[0],0)? 1 : 0;
                // make sure all pins exist
                from.set(Preferences::Name("parportPin_%02d",j),v[0]);
            }
            if (settings<PARPORT_PINS) {
                for (j=1;j<ParallelPort::LAST_PIN;j++) {
                    from.get(spins[j-1].name,v[0],(int)spins[j-1].defv);
                    if (v[0]!=0) {
                        from.set(Preferences::Name("parportPin_%02d", abs(v[0])-1),
                                 (v[0]<0)? j+ParallelPort::LAST_PIN : j);
                    }
                }
            }
            /* - Optional -
            settings += from.get("independentVddVppControl",v[0],0)? 1 : 0;
            */
            store = false;
            if (settings==(4 /* 5 */ + PARPORT_PINS)) {
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

                    for (j=0;j<PARPORT_PINS;j++) {
                        from.get(Preferences::Name("parportPin_%02d",j),v[0],ParallelPort::OFF);
                          to.get(Preferences::Name("parportPin_%02d",j),v1[0],ParallelPort::OFF);
                        if (v[0] != v1[0]) {
                            sprintf (
                                report,
                                "@bparportPin_%d:",
                                j
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
                    for (j=0;j<PARPORT_PINS;j++) {
                        from.get(Preferences::Name("parportPin_%02d",j),v[0],ParallelPort::OFF);
                          to.get(Preferences::Name("parportPin_%02d",j),v1[0],ParallelPort::OFF);
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

int searchPin(ParallelPort::PpPins connection)
{
    int pin=-1;
    for (int i=0; i<PARPORT_PINS; i++) {
        if ((ch_pinConnection[i]->value() == connection) ||
            (ch_pinConnection[i]->value() == connection+ParallelPort::LAST_PIN)) {
            pin = i;
            break;
        }
    }
    return pin;
}

bool programmerConfigCB(CfgOper oper)
{
static int lastOper=-1;
int i,j;
int v[3];

    if (oper==CFG_DELETE) {
        if (lastOper==CFG_NEW || lastOper==CFG_EDIT || lastOper==CFG_COPY) {
            i = fl_choice (
                "Are you sure you want to revert the current programmer"
                " settings ?",
                "Cancel", "Revert", NULL
            );
        } else {
            i = fl_choice (
                "Are you sure you want to delete the current programmer"
                " configuration ?",
                "Cancel", "Delete", NULL
            );
        }
        if (!i) {
            return false;
        }
    }
    if (oper==CFG_LOAD || oper==CFG_NEW || oper==CFG_DELETE) {
        tx_programmerName->value("");
        for (i=0;i<PARPORT_PINS;i++) {
            tb_pinInvert[i]->value(0);
            ch_pinConnection[i]->value(0);
        }
        for (i=0;i<3;i++) {
            tb_vddMinCond[i]->value(0);
            tb_vddPrgCond[i]->value(0);
            tb_vddMaxCond[i]->value(0);
            tb_vddMinCond[i]->deactivate();
            tb_vddPrgCond[i]->deactivate();
            tb_vddMaxCond[i]->deactivate();
            tb_vddMinCond[i]->redraw();
            tb_vddPrgCond[i]->redraw();
            tb_vddMaxCond[i]->redraw();
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

            // check whether all new parport settings have been set
            i=0;
            for (j=0;j<PARPORT_PINS;j++) {
                i += programmer.get(Preferences::Name("parportPin_%02d",j),v[0],0)? 1 : 0;
                // make sure all pins exist
                programmer.set(Preferences::Name("parportPin_%02d",j),v[0]);
            }
            if (i<PARPORT_PINS) {
                // not all new settings vave been set, load deprecated parport configuration
                for (j=1;j<ParallelPort::LAST_PIN;j++) {
                    programmer.get(spins[j-1].name,v[0],(int)spins[j-1].defv);
                    if (v[0]!=0) {
                        programmer.set(Preferences::Name("parportPin_%02d", abs(v[0])-1),
                                       (v[0]<0)? j+ParallelPort::LAST_PIN : j);
                    }
                }
            }
            // remove deprecated configuration settings
            for (j=1;j<ParallelPort::LAST_PIN;j++) {
                programmer.deleteEntry(spins[j-1].name);
            }
            // new parport configuration overrides old configuration
            for (i=0;i<PARPORT_PINS;i++) {
                if (programmer.get(Preferences::Name("parportPin_%02d",i),
                                   v[0],(int)ch_pinConnection[i]->value())) {
                    tb_pinInvert[i]->value((v[0]>=ParallelPort::LAST_PIN));
                    ch_pinConnection[i]->value((v[0]>=ParallelPort::LAST_PIN)?(v[0]-ParallelPort::LAST_PIN):(v[0]));
                    ch_pinConnection[i]->set_changed();
                }
            }
            programmer.get("vddMinCond" ,v[0],0);
            programmer.get("vddProgCond",v[1],0);
            programmer.get("vddMaxCond" ,v[2],0);
                for (i=0;i<3;i++) {
                    tb_vddMinCond[i]->value((v[0]>>i) & 0x01);
                    tb_vddPrgCond[i]->value((v[1]>>i) & 0x01);
                    tb_vddMaxCond[i]->value((v[2]>>i) & 0x01);
                }
            programmer.get("vppOffCond",v[0],0);
                tb_vppOffCond->value(v[0]>0);

            programmer.get("independentVddVppControl",v[0],0);
                tb_saVddVppControl->value(v[0]>0);

#if 0
            if (searchPin(ParallelPort::SEL_MIN_VDD)>=0) {
                tb_vddMinCond[0]->deactivate();
                tb_vddPrgCond[0]->activate();
                tb_vddMaxCond[0]->activate();
            } else {
                tb_vddMinCond[0]->deactivate();
                tb_vddPrgCond[0]->deactivate();
                tb_vddMaxCond[0]->deactivate();
            }
            if (searchPin(ParallelPort::SEL_PRG_VDD)>=0) {
                tb_vddMinCond[1]->activate();
                tb_vddPrgCond[1]->deactivate();
                tb_vddMaxCond[1]->activate();
            } else {
                tb_vddMinCond[1]->deactivate();
                tb_vddPrgCond[1]->deactivate();
                tb_vddMaxCond[1]->deactivate();
            }
            if (searchPin(ParallelPort::SEL_MAX_VDD)>=0) {
                tb_vddMinCond[2]->activate();
                tb_vddPrgCond[2]->activate();
                tb_vddMaxCond[2]->deactivate();
            } else {
                tb_vddMinCond[2]->deactivate();
                tb_vddPrgCond[2]->deactivate();
                tb_vddMaxCond[2]->deactivate();
            }
#else
            if (searchPin(ParallelPort::SEL_MIN_VDD)>=0) {
                tb_vddMinCond[0]->activate();
                tb_vddPrgCond[0]->activate();
                tb_vddMaxCond[0]->activate();
            } else {
                tb_vddMinCond[0]->deactivate();
                tb_vddPrgCond[0]->deactivate();
                tb_vddMaxCond[0]->deactivate();
            }
            if (searchPin(ParallelPort::SEL_PRG_VDD)>=0) {
                tb_vddMinCond[1]->activate();
                tb_vddPrgCond[1]->activate();
                tb_vddMaxCond[1]->activate();
            } else {
                tb_vddMinCond[1]->deactivate();
                tb_vddPrgCond[1]->deactivate();
                tb_vddMaxCond[1]->deactivate();
            }
            if (searchPin(ParallelPort::SEL_MAX_VDD)>=0) {
                tb_vddMinCond[2]->activate();
                tb_vddPrgCond[2]->activate();
                tb_vddMaxCond[2]->activate();
            } else {
                tb_vddMinCond[2]->deactivate();
                tb_vddPrgCond[2]->deactivate();
                tb_vddMaxCond[2]->deactivate();
            }
#endif
            if (searchPin(ParallelPort::SEL_VIHH_VPP)>=0) {
                tb_vppOffCond->activate();
            } else {
                tb_vppOffCond->deactivate();
            }
            verifyProgrammerConfig(false);

            app.get("portNumber",i,0);
#if 0
            app.get("portAccessMethod",j,0);
#else
            j = ParallelPort::ports[i].access;
#endif
            if (io) {
                delete io;
                io = NULL;
            }
            try {
printf("port device : %s\n", ParallelPort::ports[i].device);
printf("port access : %s\n", portAccess[j]);
printf("port address: %0X\n", ParallelPort::ports[i].address);
printf("port regs   : %d\n", ParallelPort::ports[i].regs);
fflush(stdout);
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
        for (i=0;i<PARPORT_PINS;i++) {
            v[0] = ch_pinConnection[i]->value() + ((tb_pinInvert[i]->value())?ParallelPort::LAST_PIN:0);
            programmer.set(Preferences::Name("parportPin_%02d",i),v[0]);
        }
        v[0] = v[1] = v[2] = 0;
        for (i=0;i<3;i++) {
            v[0] |= ((tb_vddMinCond[i]->value() & 0x01)<<i);
            v[1] |= ((tb_vddPrgCond[i]->value() & 0x01)<<i);
            v[2] |= ((tb_vddMaxCond[i]->value() & 0x01)<<i);
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

bool loadGeneralSettings(const char *fname)
{
const char *pext, *pfname;
char path[FL_PATH_MAX];
char application[FL_PATH_MAX];
char report[1024];
int i;
int v[3], v1[3];
bool loaded = false;
bool different = false;

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

        different = false;
        ls_report->clear();

        for (i=0;i<LAST_PROP_DLY;i++) {
            imports.get(spropDly[i].name,v[0] ,(int)spropDly[i].defv);
                app.get(spropDly[i].name,v1[0],(int)spropDly[i].defv);
            if (v[0] != v1[0]) {
                sprintf (
                    report,
                    "@b%s:",
                    spropDly[i].name
                );
                ls_report->add(report);
                sprintf(report,"  cur=%d",v1[0]);
                ls_report->add(report);
                sprintf(report,"@_  new=%d",v[0]);
                ls_report->add(report);
                different = true;
            }
        }
        if (different && show_report_window("General Settings")) {
            for (i=0;i<LAST_PROP_DLY;i++) {
                imports.get(spropDly[i].name,v[0],(int)spropDly[i].defv);
                    app.set(spropDly[i].name,v[0]);
            }
        }
        loaded = true;
    }
    return loaded;
}

bool generalSettingsCB(CfgOper oper)
{
static int lastOper=-1;
int i,j;
int v[3];
char buf[FL_PATH_MAX];

    if (oper==CFG_DELETE) {
        if (lastOper==CFG_NEW || lastOper==CFG_EDIT || lastOper==CFG_COPY) {
            i = fl_choice (
                "Are you sure you want to revert the current general"
                " settings ?",
                "Cancel", "Revert", NULL
            );
        } else {
            i = fl_choice (
                "Are you sure you want to reset the current general"
                " settings ?",
                "Cancel", "Reset", NULL
            );
        }
        if (!i) {
            return false;
        }
    }
    if (oper==CFG_LOAD || oper==CFG_NEW || oper==CFG_DELETE) {
        for (i=0;i<LAST_PROP_DLY;i++) {
            tx_propDelay[i]->value("0");
        }
        if (
            oper==CFG_DELETE &&
            (lastOper!=CFG_NEW && lastOper!=CFG_EDIT && lastOper!=CFG_COPY)
        ) {
            for (i=0;i<LAST_PROP_DLY;i++) {
                app.set(spropDly[i].name,(int)spropDly[i].defv);
            }
        }
    }
    if (oper==CFG_LOAD || oper==CFG_DELETE) {
        for (i=0;i<LAST_PROP_DLY;i++) {
            app.get(spropDly[i].name,j,(int)spropDly[i].defv);
                sprintf(buf,"%d",j);
                tx_propDelay[i]->value(buf);
        }
    } else if (oper==CFG_COPY) {
        /* for now, do nothing */
    } else if (oper==CFG_EDIT) {
        /* do nothing */
    } else if (
        oper==CFG_IMPORT &&
        loadGeneralSettings (
            fl_file_chooser (
                "General Settings file selection",
                "*.prefs",
                NULL,
                0
            )
        )
    ) {
        /* for now, do nothing */
    } else if (oper==CFG_SAVE) {
        for (i=0;i<LAST_PROP_DLY;i++) {
            if (sscanf(tx_propDelay[i]->value(),"%d",&j)) {
                app.set(spropDly[i].name,j);
            }
        }
    } else if (oper!=CFG_NEW) {
        t_settings->redraw();
        return false;
    }
    lastOper = oper;

    t_settings->redraw();

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

    for (i=0; i<ParallelPort::ports.count; i++) {
         ch_parports->add (
             ParallelPort::ports[i].device,
             (const char *)0,
             (Fl_Callback *)0,
             (void *)strdup(ParallelPort::ports[i].device),
             0
         );
    }
    app.get("portNumber",i,0);
    ch_parports->value(i);

#if 0 && defined(linux) && defined(ENABLE_LINUX_PPDEV)
    g_ppAccessMethod->show();
    linux_pp_dev->show();
    app.get("portAccessMethod",i,0);

    linux_pp_dev->value((i==0));
    linux_direct_pp->value((i!=0));
#else
    linux_pp_dev->hide();
    g_ppAccessMethod->hide();
#endif

    for (i=0;i<LAST_PROP_DLY;i++) {
        app.get(spropDly[i].name,j,(int)spropDly[i].defv);
            sprintf(buf,"%d",j);
            tx_propDelay[i]->value(buf);
    }

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
    } else {
        initDevicesSettings();
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
            elaphsed  = fabs( (double)(now.QuadPart - start.QuadPart) );
            remaining = fabs( (double)(now.QuadPart - last.QuadPart) );
#else
            gettimeofday(&now, NULL);
            elaphsed  = fabs( (double)(now.tv_usec - start.tv_usec) );
            remaining = fabs( (double)(now.tv_usec - last.tv_usec) );
#endif
            if (percent>last_percent) {
                remaining /= (double)(percent-last_percent);
            }
            remaining /= (double)freq;

            remaining *= (double)(100 - percent);
            rmin = abs( (int)( remaining / 60 ) );
            rsec = fabs( (double)( remaining - (double)( rmin * 60 ) ) );

            elaphsed  /= (double)freq;
            emin = abs( (int)( elaphsed / 60 ) );
            esec = fabs( (double)( elaphsed - (double)( emin * 60 ) ) );

            estimated = ( elaphsed / ( (percent>0) ? percent : 1 ) ) * 100;
            smin = abs( (int)( estimated / 60 ) );
            ssec = fabs( (double)( estimated - (double)( smin * 60 ) ) );

            sprintf (
                msg,
                // " Address: 0x%06lx, %3d%% done",
                // " %s: [%3d%%] %2d'%2d\" /%2d'%2d\" -%2d'%2d\"",
                // " %s: [%3d%%] %2d'%4.1lf\" /%2d'%4.1lf\"",
                " %s: [%3d%%]",
                oper,
                percent //,
                // rmin, rsec,
                // smin, ssec //,
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
const char *soper[] = {
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
            io->reset(true);
            io->usleep(100000);     // 0.1" reset time
            io->reset(false);
        } else if (oper == CHIP_TEST_RESET_ON) {
            io->reset(true);
        } else if (oper == CHIP_TEST_RESET_OFF) {
            io->reset(false);
        } else if (oper == CHIP_TEST_OFF) {
            io->vpp(IO::VPP_TO_GND);
            io->vdd(IO::VDD_TO_OFF);
        } else {
            make_regcheck_window();
            if (
                ( oper == CHIP_ERASE || oper == CHIP_WRITE ) && 
                !fl_choice (
                    "Are you sure you want to %s the memory of the %s ?",
                    "Cancel",
                    (oper == CHIP_ERASE) ? "Erase" : "Overwrite", NULL,
                    (oper == CHIP_ERASE) ? "erase" : "overwrite",
                    chip->get_name().c_str() )
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
               proceed = DeviceUI::getVoltageRanges (
                   chip->get_fullname().c_str(),
                   vppMin , vppMax ,
                   vddpMin, vddpMax,
                   vddMin , vddMax
               );
               if ( proceed ) {
                   proceed = make_calibration_window (
                       forceCalibration,
                       chip->get_name().c_str(),
                       vppMin , vppMax ,
                       vddpMin, vddpMax,
                       vddMin , vddMax
                   );
               }
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
                    chip->set_config_default(lbuf);
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
                    proceed = true;
                    buf.set_wordsize(chip->get_wordsize());

                    if (chip->get_spec() == "AVR") {
                        proceed = show_avr_fuses_window();
                        if (proceed) {
                            AvrUI::initFusesAndLocks((Avr*)chip,buf);
                        }
                    }
                    if (proceed) {
                        try {
                            chip->program(buf);
                        } catch(std::exception& e) {
                            fl_alert("%s: %s",chip->get_name().c_str(),e.what());
                        }
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
                    io->reset(true);
                    io->vdd(IO::VDD_TO_ON);
                    io->usleep(100000);      // 0.1" reset time
                    io->reset(false);        // let's go
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

    if (chip && chip->get_spec() == "AVR") {
        if (show_open_avr_window()) {
            /* Clear the data buffer */
            buf.clear();

            if (strlen(tx_flash_hex_file->value())) {
                if (hexFile) {
                    delete hexFile;
                }
                hexFile = 0;

                try {
                    /* Read the hex file into the data buffer */
                    hexFile = HexFile::load(fname);
                    if (chip) {
                        /* reads the hex file into memory */
                        try {
                            IntPair p(0,0);
                            p = chip->get_code_extent();
                            hexFile->read(buf,p.first,p.second);
                        } catch (std::exception& e) {
                            fl_message("%s\n",e.what());
                        }
                    }
                } catch (std::exception& e) {
                    fl_message("%s: %s\n",fname,e.what());
                    hexFile = 0;
                }
            }
            if (strlen(tx_eeprom_hex_file->value())) {
                if (hexFile) {
                    delete hexFile;
                }
                hexFile = 0;

                try {
                    /* Read the hex file into the data buffer */
                    hexFile = HexFile::load(fname);
                    if (chip) {
                        /* reads the hex file into memory */
                        try {
                            IntPair p(0,0);
                            p = chip->get_data_extent();
                            hexFile->read(buf,p.first,p.second);
                        } catch (std::exception& e) {
                            fl_message("%s\n",e.what());
                        }
                    }
                } catch (std::exception& e) {
                    fl_message("%s: %s\n",fname,e.what());
                    hexFile = 0;
                }
            }
            dumpHexFile();
        }
    } else {
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

    Fl_Tooltip::font(FL_SCREEN);

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
