/*
  cursor.h

  For Tux Paint
  Bitmapped mouse pointer (cursor)

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

  June 14, 2002 - May 15, 2007
  $Id: cursor.h,v 1.3 2007/05/16 01:11:34 wkendrick Exp $
*/

#ifndef CURSOR_H
#define CURSOR_H

#include "SDL.h"

#include "watch.xbm"
#include "watch-mask.xbm"

#include "hand.xbm"
#include "hand-mask.xbm"

#include "wand.xbm"
#include "wand-mask.xbm"

#include "insertion.xbm"
#include "insertion-mask.xbm"

#include "brush.xbm"
#include "brush-mask.xbm"

#include "crosshair.xbm"
#include "crosshair-mask.xbm"

#include "rotate.xbm"
#include "rotate-mask.xbm"

#include "up.xbm"
#include "up-mask.xbm"

#include "down.xbm"
#include "down-mask.xbm"

#include "tiny.xbm"
#include "tiny-mask.xbm"

#include "arrow.xbm"
#include "arrow-mask.xbm"

extern SDL_Cursor *cursor_hand, *cursor_arrow, *cursor_watch,
  *cursor_up, *cursor_down, *cursor_tiny, *cursor_crosshair,
  *cursor_brush, *cursor_wand, *cursor_insertion, *cursor_rotate;

extern int no_fancy_cursors, hide_cursor;

void do_setcursor(SDL_Cursor * c);
void free_cursor(SDL_Cursor ** cursor);


#endif
