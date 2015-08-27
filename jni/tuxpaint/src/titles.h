/*
  titles.h

  For Tux Paint
  List of available titles

  Copyright (c) 2002-2007 by Bill Kendrick and others
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

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

  June 14, 2002 - October 9, 2009
  $Id: titles.h,v 1.9 2009/10/10 06:33:38 wkendrick Exp $
*/



/* What titles are available: */

enum
{
  TITLE_NONE,
  TITLE_NOCOLORS,
  TITLE_TOOLS,
  TITLE_COLORS,
  TITLE_BRUSHES,
  TITLE_ERASERS,
  TITLE_STAMPS,
  TITLE_SHAPES,
  TITLE_LETTERS,
  TITLE_MAGIC,
  NUM_TITLES
};


/* Title names: */

const char *const title_names[NUM_TITLES] = {
  "",
  "",
  // Title of tool selector (buttons down the left)
  gettext_noop("Tools"),

  // Title of color palette (buttons across the bottom)
  gettext_noop("Colors"),

  // Title of brush selector (buttons down the right for paint and line tools)
  gettext_noop("Brushes"),

  // Title of eraser selector (buttons down the right for eraser tool)
  gettext_noop("Erasers"),

  // Title of stamp selector (buttons down the right for stamps tool)
  gettext_noop("Stamps"),

  // Title of shape selector (buttons down the right for shapes tool)
  gettext_noop("Shapes"),

  // Title of font selector (buttons down the right for text and label tools)
  gettext_noop("Letters"),

  // Title of magic tool selector (buttons down the right for magic (effect plugin) tool)
  gettext_noop("Magic")
};
