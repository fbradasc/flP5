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
 * $Id: Device.cxx,v 1.13 2002/11/06 22:42:37 marka Exp $
 */
#include <string.h>
#include <stdexcept>

using namespace std;

#include "Device.h"
#include "devices/Microchip/Microchip.h"
#include "Util.h"

Preferences *Device::config = NULL;

Device *Device::load(Preferences *config, char *name)
{
Device *d = NULL;
char *vendor, *spec;

    Device::config = config;
    vendor = name;
    spec = strchr(name,'/');
    if (spec) {
        *spec='\0';
        spec++;
        if (strncasecmp(vendor,"Microchip",sizeof("Microchip")) == 0) { 
            d = Microchip::load(spec);
        } else {
            throw runtime_error (
                (const char *)Preferences::Name (
                    "Unknown vendor %s",
                    vendor
                )
            );
        }
    } else {
        throw runtime_error (
            (const char *)Preferences::Name (
                "Unknown device %s",
                name
            )
        );
    }
    return d;
}

Device::Device(char *name)
{
    this->wordsize = 8;
    this->set_iodevice(NULL);
    this->set_progress_cb(NULL);
    this->set_dump_cb(NULL);
    this->progress_count = 0;
    this->progress_total = 1;
    this->name = string(name);
}

Device::~Device()
{
}

void Device::dump(DataBuffer& buf)
{
    if (!this->dump_cb) {
        return;
    }

int start, len, addr, bytes, data, i;
bool writable;
int bufwordsize = ((buf.get_wordsize() + 7) & ~7) / 8;
static char line[256], ascii[9];

IntPairVector::iterator n = memmap.begin();

    this->dump_cb(this->dump_cb_data,(const char *)0,-1);
    for (; n != memmap.end(); n++) {
        start = n->first;
        len   = n->second;
        addr  = start;
        while (addr < (unsigned long)start+len) {
            sprintf(line,"%07x)", addr);
            writable=false;
            for (i=0, bytes=0; bytes<8; bytes+=bufwordsize) {
                if (!buf.isblank(addr)) {
                    writable=true;
                }
                switch (bufwordsize) {
                    case 1:
                        data   = buf[addr] & 0xff;
                        sprintf(line,"%s %02x", line, data);
                        ascii[i++] = (isprint(data)) ? data : '.';
                    break;
                    case 2:
                        /* Print low byte, then high byte */
                        data   = buf[addr] & 0xff;
                        sprintf(line,"%s %02x", line, data);
                        ascii[i++] = (isprint(data)) ? data : '.';
                        data   = (buf[addr] >> 8) & 0xff;
                        sprintf(line,"%s %02x", line, data);
                        ascii[i++] = (isprint(data)) ? data : '.';
                    break;
                }
                addr++;
            }
            ascii[i] = '\0';
            if (writable) {
                sprintf(line,"%s |%s|", line, ascii);
                this->dump_cb (
                    this->dump_cb_data,
                    (const char *)line,
                    -1
                );
            }
        }
    }
}

string Device::get_name(void)
{
    return this->name;
}

IntPairVector& Device::get_mmap(void)
{
    return this->memmap;
}

int Device::get_wordsize(void)
{
    return this->wordsize;
}

unsigned int Device::get_clearvalue(size_t addr)
{
    return 0x00ff;
}

void Device::set_iodevice(IO *iodev)
{
    this->io = iodev;
}

void Device::set_progress_cb (
    bool (*cb)(void *data, long addr, int percent),
    void *data
) {
    this->progress_cb = cb;
    this->progress_cb_data = data;
}

bool Device::progress(unsigned long addr)
{
    if (this->progress_cb) {
        return this->progress_cb (
            this->progress_cb_data,
            addr,
            (100*this->progress_count)/this->progress_total
        );
    }
    return true;
}

void Device::set_dump_cb (
    bool (*cb)(void *data, const char *line, int percent),
    void *data
) {
    this->dump_cb = cb;
    this->dump_cb_data = data;
}
