/*
  tip_tux.h

  For Tux Paint
  List of tux images for tips.

  Copyright (c) 2002 by Bill Kendrick
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

  June 17, 2002 - June 27, 2002
  $Id: tip_tux.h,v 1.5 2009/06/03 20:46:07 wkendrick Exp $
*/


#ifndef TIP_TUX_H
#define TIP_TUX_H

/* What tuxes are available: */

enum
{
  TUX_DEFAULT,
  TUX_KISS,
  TUX_BORED,
  TUX_GREAT,
  TUX_OOPS,
  TUX_WAIT,
  NUM_TIP_TUX
};


/* Tux filenames: */

const char *const tux_img_fnames[NUM_TIP_TUX] = {
  DATA_PREFIX "images/tux/default.png",
  DATA_PREFIX "images/tux/kiss.png",
  DATA_PREFIX "images/tux/bored.png",
  DATA_PREFIX "images/tux/great.png",
  DATA_PREFIX "images/tux/oops.png",
  DATA_PREFIX "images/tux/wait.png"
};

#endif /* TIP_TUX_H */
