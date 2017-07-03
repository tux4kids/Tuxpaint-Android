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

  June 14, 2002 - February 18, 2006
  $Id: progressbar.c,v 1.3 2006/08/27 21:00:55 wkendrick Exp $
*/


#include "progressbar.h"
#include "debug.h"

SDL_Surface *img_progress;
int progress_bar_disabled, prog_bar_ctr;

void show_progress_bar_(SDL_Surface * screen, SDL_Texture *texture, SDL_Renderer *renderer)
{
  SDL_Rect dest, src, r;
  int x;
  static Uint32 oldtime;
  Uint32 newtime;

  if (progress_bar_disabled)
    return;

  newtime = SDL_GetTicks();
  if (newtime > oldtime + 15)	// trying not to eat some serious CPU time!
  {
    for (x = 0; x < screen->w; x = x + 65)
    {
      src.x = 65 - (prog_bar_ctr % 65);
      src.y = 0;
      src.w = 65;
      src.h = 24;

      dest.x = x;
      dest.y = screen->h - 24;

      SDL_BlitSurface(img_progress, &src, screen, &dest);
    }

    prog_bar_ctr++;

    // FIXME SDL2
    //    SDL_UpdateRect(screen, 0, screen->h - 24, screen->w, 24);
    r.x = 0;
    r.y = screen->h - 24;
    r.w = screen->w;
    r.h = 24;

    SDL_UpdateTexture(texture, &r, screen->pixels + ((screen->h - 24) * screen->pitch), screen->pitch);

#if !defined (__ANDROID__)
    //  NOTE docs says one should clear the renderer, however this means a refresh of the whole thing.
    //  SDL_RenderClear(renderer);
    //  SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderCopy(renderer, texture, &r, &r);
#else
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
#endif

    SDL_RenderPresent(renderer);
  }
  oldtime = newtime;


  /* FIXME: RESURRECT THIS (bjk 2006.02.18) */
  //eat_sdl_events();
}
