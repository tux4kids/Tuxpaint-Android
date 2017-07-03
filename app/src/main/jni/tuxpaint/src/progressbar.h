/*
  progressbar.h

  For Tux Paint
  Progress bar functions

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

  June 14, 2002 - February 17, 2006
  $Id: progressbar.h,v 1.2 2006/08/27 21:00:55 wkendrick Exp $
*/


#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "SDL.h"

extern SDL_Surface *img_progress;
extern int progress_bar_disabled, prog_bar_ctr;

void show_progress_bar_(SDL_Surface * screen, SDL_Texture *texture, SDL_Renderer *renderer);

#endif
