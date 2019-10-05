/*
  fill.c

  Fill tool
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

  Last updated: September 14, 2019
  $Id$
*/

#include <stdio.h>
#include <string.h>
#include "fill.h"
#include "rgblinear.h"
#include "playsound.h"
#include "pixels.h"


/* Local function prototypes: */

int colors_close(SDL_Surface * canvas, Uint32 c1, Uint32 c2);


int colors_close(SDL_Surface * canvas, Uint32 c1, Uint32 c2)
{
  Uint8 r1, g1, b1, r2, g2, b2;

  if (c1 == c2)
    {
      /* Get it over with quick, if possible! */

      return 1;
    }
  else
    {
      double r, g, b;

      SDL_GetRGB(c1, canvas->format, &r1, &g1, &b1);
      SDL_GetRGB(c2, canvas->format, &r2, &g2, &b2);

      // use distance in linear RGB space
      r = sRGB_to_linear_table[r1] - sRGB_to_linear_table[r2];
      r *= r;
      g = sRGB_to_linear_table[g1] - sRGB_to_linear_table[g2];
      g *= g;
      b = sRGB_to_linear_table[b1] - sRGB_to_linear_table[b2];
      b *= b;

      // easy to confuse:
      //   dark grey, brown, purple
      //   light grey, tan
      //   red, orange
      return r + g + b < 0.04;
    }
}


int would_flood_fill(SDL_Surface * canvas, Uint32 cur_colr, Uint32 old_colr)
{
  if (cur_colr == old_colr || colors_close(canvas, cur_colr, old_colr))
    {
      return 0;
    } else {
      return 1;
    }
}

void do_flood_fill(SDL_Surface * canvas, int x, int y, Uint32 cur_colr, Uint32 old_colr, int * x1, int * y1, int * x2, int * y2)
{
  int fillL, fillR, i, in_line;
  static unsigned char prog_anim;


  if (cur_colr == old_colr || colors_close(canvas, cur_colr, old_colr))
    return;

  if (y < *y1)
    {
      *y1 = y;
    }
  if (y > *y2)
    {
      *y2 = y;
    }


  fillL = x;
  fillR = x;

  prog_anim++;
  if ((prog_anim % 4) == 0)
    {
      /* FIXME: api->update_progress_bar(); */
      playsound(canvas, 1, SND_FILL, 1, x, SNDDIST_NEAR);
    }


  /* Find left side, filling along the way */

  in_line = 1;

  while (in_line)
    {
      putpixels[canvas->format->BytesPerPixel] (canvas, fillL, y, cur_colr);
      fillL--;

      in_line = (fillL < 0) ? 0 : colors_close(canvas, getpixels[canvas->format->BytesPerPixel] (canvas, fillL, y), old_colr);
    }

  if (fillL < *x1)
    {
      *x1 = fillL;
    }

  fillL++;

  /* Find right side, filling along the way */

  in_line = 1;
  while (in_line)
    {
      putpixels[canvas->format->BytesPerPixel] (canvas, fillR, y, cur_colr);
      fillR++;

      in_line = (fillR >= canvas->w) ? 0 : colors_close(canvas, getpixels[canvas->format->BytesPerPixel] (canvas, fillR, y), old_colr);
    }

  if (fillR > *x2)
    {
      *x2 = fillR;
    }

  fillR--;


  /* Search top and bottom */

  for (i = fillL; i <= fillR; i++)
    {
      if (y > 0 && colors_close(canvas, getpixels[canvas->format->BytesPerPixel] (canvas, i, y - 1), old_colr))
        do_flood_fill(canvas, i, y - 1, cur_colr, old_colr, x1, y1, x2, y2);

      if (y < canvas->h && colors_close(canvas, getpixels[canvas->format->BytesPerPixel] (canvas, i, y + 1), old_colr))
        do_flood_fill(canvas, i, y + 1, cur_colr, old_colr, x1, y1, x2, y2);
    }
}

