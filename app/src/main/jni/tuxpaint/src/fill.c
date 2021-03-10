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

/* math.h makes y1 an obscure function! */
#define y1 evil_y1
#include <math.h>
#undef y1

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
  simulate_flood_fill(canvas, x, y, cur_colr, old_colr, x1, y1, x2, y2, NULL);
}


void simulate_flood_fill(SDL_Surface * canvas, int x, int y, Uint32 cur_colr, Uint32 old_colr, int * x1, int * y1, int * x2, int * y2, Uint8 * touched)
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
      if (touched != NULL) {
        touched[(y * canvas->w) + fillL] = 1;
      }

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
      if (touched != NULL) {
        touched[(y * canvas->w) + fillR] = 1;
      }
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
        simulate_flood_fill(canvas, i, y - 1, cur_colr, old_colr, x1, y1, x2, y2, touched);

      if (y < canvas->h && colors_close(canvas, getpixels[canvas->format->BytesPerPixel] (canvas, i, y + 1), old_colr))
        simulate_flood_fill(canvas, i, y + 1, cur_colr, old_colr, x1, y1, x2, y2, touched);
    }
}


void draw_linear_gradient(SDL_Surface * canvas, SDL_Surface * last,
  int x_left, int y_top, int x_right, int y_bottom,
  int x1, int y1, int x2, int y2, Uint32 draw_color, Uint8 * touched
) {
  Uint32 old_colr, new_colr;
  int xx, yy;
  float xd, yd;
  Uint8 draw_r, draw_g, draw_b, old_r, old_g, old_b, new_r, new_g, new_b;
  float A, B, C, C1, C2, ratio;

  /* Get our target color */
  SDL_GetRGB(draw_color, canvas->format, &draw_r, &draw_g, &draw_b);

  A = (x2 - x1);
  B = (y2 - y1);
  C1 = (A * x1) + (B * y1);
  C2 = (A * x2) + (B * y2);
  /* FIXME: C2 should be larger than C1? */

  for (yy = y_top; yy <= y_bottom; yy++) {
    for (xx = x_left; xx <= x_right; xx++) {
      if (touched[(yy * canvas->w) + xx]) {
        /* Get the old color, and blend it (with a distance-based ratio) with the target color */
        old_colr = getpixels[last->format->BytesPerPixel] (last, xx, yy);
        SDL_GetRGB(old_colr, last->format, &old_r, &old_g, &old_b);

        /* (h/t David Z on StackOverflow for how to quickly compute this:
           https://stackoverflow.com/questions/521493/creating-a-linear-gradient-in-2d-array) */
        C = (A * xx) + (B * yy);

        if (C < C1) {
          /* At/beyond the click spot (opposite direction of mouse); solid color */
          putpixels[canvas->format->BytesPerPixel] (canvas, xx, yy, draw_color);
        } else if (C >= C2) {
          /* At/beyond the mouse; completely faded out */
          putpixels[canvas->format->BytesPerPixel] (canvas, xx, yy, old_colr);
        } else {
          /* The actual gradient... */

          ratio = (C - C1) / (C2 - C1);

          new_r = (Uint8) (((float) old_r) * ratio + ((float) draw_r * (1.00 - ratio)));
          new_g = (Uint8) (((float) old_g) * ratio + ((float) draw_g * (1.00 - ratio)));
          new_b = (Uint8) (((float) old_b) * ratio + ((float) draw_b * (1.00 - ratio)));

          new_colr = SDL_MapRGB(canvas->format, new_r, new_g, new_b);
          putpixels[canvas->format->BytesPerPixel] (canvas, xx, yy, new_colr);
        }
      }
    }
  }
}

void draw_radial_gradient(SDL_Surface * canvas, int x_left, int y_top, int x_right, int y_bottom,
  int x, int y, Uint32 draw_color, Uint8 * touched
) {
  Uint32 old_colr, new_colr;
  int xx, yy;
  float xd, yd, dist, rad, ratio;
  Uint8 draw_r, draw_g, draw_b, old_r, old_g, old_b, new_r, new_g, new_b;

  /* Calculate the max radius of the filled area */
  xd = (x_right - x_left + 1);
  yd = (y_bottom - y_top + 1);
  rad = sqrt(xd * xd + yd * yd);
  if (rad == 0) {
    return;
  }

  /* Get our target color */
  SDL_GetRGB(draw_color, canvas->format, &draw_r, &draw_g, &draw_b);

  /* Traverse the flood-filled zone */
  for (yy = y_top; yy <= y_bottom; yy++) {
    for (xx = x_left; xx <= x_right; xx++) {
      /* Only alter the pixels within the flood itself */
      if (touched[(yy * canvas->w) + xx]) {
        /* Determine the distance from the click point */
        xd = (float) abs(xx - x);
        yd = (float) abs(yy - y);
        dist = sqrt(xd * xd + yd * yd);
        ratio = (dist / rad);

        /* Get the old color, and blend it (with a distance-based ratio) with the target color */
        old_colr = getpixels[canvas->format->BytesPerPixel] (canvas, xx, yy);
        SDL_GetRGB(old_colr, canvas->format, &old_r, &old_g, &old_b);

        new_r = (Uint8) (((float) old_r) * ratio + ((float) draw_r * (1.00 - ratio)));
        new_g = (Uint8) (((float) old_g) * ratio + ((float) draw_g * (1.00 - ratio)));
        new_b = (Uint8) (((float) old_b) * ratio + ((float) draw_b * (1.00 - ratio)));

        new_colr = SDL_MapRGB(canvas->format, new_r, new_g, new_b);
        putpixels[canvas->format->BytesPerPixel] (canvas, xx, yy, new_colr);
      }
    }
  }
}

