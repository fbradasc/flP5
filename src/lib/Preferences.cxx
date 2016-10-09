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
#include "Preferences.h"

Preferences::Preferences (
    Fl_Preferences::Root root,
    const char *vendor,
    const char *application
)
: Fl_Preferences(root, vendor, application)
{
}

Preferences::Preferences (
    const char *path,
    const char *vendor,
    const char *application
)
: Fl_Preferences(path, vendor, application)
{
}

Preferences::Preferences (
    Preferences &pref,
    const char  *group
)
: Fl_Preferences((Fl_Preferences&)pref, group)
{
}

Preferences::Preferences (
    Preferences *pref,
    const char  *group
)
: Fl_Preferences((Fl_Preferences*)pref, group)
{
}

char Preferences::getHex(const char *entry, int &value, int defaultValue)
{
char buf[256];
char ret = 0;

    value = 0;
    ret   = get(
       entry,
       buf,
       Fl_Preferences::Name("0x%x",defaultValue),
       sizeof(buf)-1
    );
    if (ret) {
        ret = (char)sscanf(buf,"%x",&value);
    }
    return ret;
}
