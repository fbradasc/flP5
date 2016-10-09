         flP5 - Fast Light Parallel Port Production PIC Programmer
         ---------------------------------------------------------
               by Francesco Bradascio <fbradasc@yahoo.it>

What is flP5 ?
--------------

flP5 is a frontend for the Mark Aikens's "Odyssey - PIC programming software"
and it's based on the FLTK 1.1.3 or later GUI toolkit.

Features
--------

- can support all the parallel port programmers supported by Odyssey, plus
  the 3 voltages "production" PIC programmers (on parallel port), as specified
  by Microchips.

  New programmers can be easily configured on the fly.

- can support all the PIC devices supported by Odyssey, here's a summery:

  Tested:
    PIC16F84, PIC16C84, PIC16C76, PIC16F74, PIC16C765, PIC16F877, PIC18F252,
    PIC18F452, PIC12F629, PIC12F675, PIC16LF628

  Untested but should work:
    PIC16C61, PIC16C62, PIC16C62A, PIC16C62B, PIC16C63, PIC16C63A,
      PIC16C64, PIC16C64A, PIC16C65, PIC16C65A, PIC16C65B, PIC16C66,
      PIC16C67, PIC16C71, PIC16C72, PIC16C72A, PIC16C73, PIC16C73A,
      PIC16C73B, PIC16C74, PIC16C74A, PIC16C74B, PIC16C77, PIC16C620,
      PIC16C620A, PIC16C621, PIC16C621A, PIC16C622, PIC16C622A, PIC16C710,
      PIC16C711, PIC16C712, PIC16C716, PIC16C745, PIC16C773, PIC16C774,
      PIC16C923, PIC16C924,
    PIC16F83, PIC16F84A, PIC16F870, PIC16F871, PIC16F872, PIC16F873,
      PIC16F874, PIC16F876,
    PIC16F73, PIC16F76, PIC16F77,
    PIC18F248, PIC18F252, PIC18F258, PIC18F442, PIC18F448, PIC18F458,
    PIC16F627, PIC16LF627, PIC16F628

  New PIC devices can be easily added on the fly, but they must folow the
  Microchip's specifications listed below:

    DS30034D - PIC16F62X
    DS30228J - PIC16C6XX/7XX/9XX
    DS30262C - PIC16F8X
    DS30324B - PIC16F7X
    DS39025F - PIC16F87X
    DS39576A - PIC18FXX2/XX8
    DS41173B - PIC12F6XX

- As Mark Aikens says, thank to his "Odyssey - PIC programming software":

  << This program has the potential to program any serially programmed device
     and I would like to be able to program any PIC in existence (under Linux
     of course). >>

  Well, now it runs even under Windows (for all that mads who still play with
  that broken toy).

Copying policy
--------------

GPL, of course.
While it is copyright, (C) 2003 by Francesco Bradascio, the program is also
free software; you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by  the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.  You will receive a copy of the GNU General Public License
along with this program; for more information, write to the Free Software
oundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Questions and comments
----------------------

Sent any questions and comments to Francesco Bradascio <fbradasc@yahoo.it>
