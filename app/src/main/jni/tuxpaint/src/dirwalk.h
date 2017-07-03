/*
  dirwalk.h

  Copyright (c) 2009
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

  $Id: dirwalk.h,v 1.5 2009/11/23 07:45:25 albert Exp $
*/

#ifndef DIRWALK_H
#define DIRWALK_H

#include "SDL.h"

#include "compiler.h"

/////////////////////////////// directory tree walking /////////////////////

#define TP_FTW_UNKNOWN 1
#define TP_FTW_DIRECTORY 2
#define TP_FTW_NORMAL 0

#define TP_FTW_PATHSIZE 400

typedef struct tp_ftw_str
{
  char *str;
  unsigned char len;
//  unsigned char is_rsrc;
} tp_ftw_str;


void loadfont_callback(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, const char *restrict const dir,
		       unsigned dirlen, tp_ftw_str * files, unsigned i, const char *restrict const locale);
int compare_ftw_str(const void *v1, const void *v2);
void tp_ftw(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, char *restrict const dir, unsigned dirlen,
	    int rsrc, void (*fn) (SDL_Surface * screen,
				  SDL_Texture * texture,
				  SDL_Renderer * renderer,
				  const char *restrict const dir,
				  unsigned dirlen, tp_ftw_str * files,
				  unsigned count, const char *restrict const locale),
            const char *restrict const locale);

#endif
