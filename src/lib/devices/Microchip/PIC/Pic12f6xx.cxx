/* Copyright (C) 2002-2003  Mark Andrew Aikens <marka@desert.cx>
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
 * $Id: Pic12f6xx.cxx,v 1.3 2003/03/25 20:31:33 marka Exp $
 */
#include <stdio.h>
#include <stdexcept>

using namespace std;

#include "Microchip.h"
#include "Util.h"


Pic12f6xx::Pic12f6xx(char *name) : Pic16(name)
{
    this->flags |= PIC_HAS_OSCAL;
}

Pic12f6xx::~Pic12f6xx()
{
}

void Pic12f6xx::disable_codeprotect(void)
{
    this->bulk_erase();
}
