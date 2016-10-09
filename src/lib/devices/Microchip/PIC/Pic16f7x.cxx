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
 * $Id: Pic16f7x.cxx,v 1.3 2002/10/19 17:53:54 marka Exp $
 */
#include "Microchip.h"


Pic16f7x::Pic16f7x(char *vendor, char *spec, char *device) : Pic16(vendor, spec, device)
{
}

Pic16f7x::~Pic16f7x()
{
}

void Pic16f7x::disable_codeprotect(void)
{
    this->bulk_erase();
}
