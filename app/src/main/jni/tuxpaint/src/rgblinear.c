/*
  rgblinear.c

  For Tux Paint
  RGB to Linear and Linear to RGB functions

  Copyright (c) 2002-2006 by Bill Kendrick and others
  bill@newbreedsoftware.com
  http://www.newbreedsoftware.com/tuxpaint/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  (See COPYING.txt)

  June 14, 2002 - February 18, 2006
  $Id$
*/

#include "rgblinear.h"
#include "debug.h"

/**
 * Return sRGB mapping (0-255 byte) of a linear (0.0 to 1.0) value
 * (see rgblinear.h)
 *
 * @param linear Linear (float) value
 * @return sRGB (byte) value
 */
unsigned char linear_to_sRGB(float linear)
{
  unsigned slot;

  slot = linear * 4096.0 + 0.5;
  if (slot > 4095)
    {
      if (linear > 0.5)
        slot = 4095;
      else
        slot = 0;
    }
  return linear_to_sRGB_table[slot];
}
