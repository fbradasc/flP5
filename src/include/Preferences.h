/* Copyright (C) 2003-2010  Francesco Bradascio <fbradasc@gmail.com>
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
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <FL/Fl_Preferences.H>

class Preferences: public Fl_Preferences
{
public:
    Preferences (
        Fl_Preferences::Root root,
        const char *vendor,
        const char *application
    );
    Preferences (
        const char *path,
        const char *vendor,
        const char *application
    );
    Preferences ( Preferences&, const char *group );
    Preferences ( Preferences*, const char *group );

    char getHex(const char *entry, int &value, int defaultValue);
};

#endif // PREFERENCES_H
