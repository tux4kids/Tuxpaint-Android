/*
  fill_tools.h

  Fill tool -- tool variations (for selector)
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2019 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

  Flood fill code based on Wikipedia example:
  http://www.wikipedia.org/wiki/Flood_fill/C_example
  by Damian Yerrick - http://www.wikipedia.org/wiki/Damian_Yerrick

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

  Last updated: February 20, 2021
  $Id$
*/

#ifndef FILL_TOOLS_H
#define FILL_TOOLS_H

#ifndef gettext_noop
#define gettext_noop(String) String
#endif

enum {
  FILL_FLOOD,
  FILL_GRADIENT_LINEAR,
  FILL_GRADIENT_RADIAL,
  NUM_FILLS
};

const char *const fill_names[NUM_FILLS] = {
  gettext_noop("Solid"),
  gettext_noop("Linear"),
  gettext_noop("Radial")
};

const char *const fill_tips[NUM_FILLS] = {
  gettext_noop("Click to fill an area with a solid color."),
  gettext_noop("Click and drag to fill an area with a linear gradient (from the chosen color to transparent)."),
  gettext_noop("Click to fill an area with a radial gradient (from the chosen color to transparent).")
};

const char *const fill_img_fnames[NUM_FILLS] = {
  DATA_PREFIX "images/fills/solid.png",
  DATA_PREFIX "images/fills/gradient_linear.png",
  DATA_PREFIX "images/fills/gradient_radial.png"
};

#endif

