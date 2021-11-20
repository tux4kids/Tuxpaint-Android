/*
  fill.c

  Fill tool
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2021 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: November 20, 2021
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
#include "progressbar.h"

#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif /* ATTRIBUTE_UNUSED */


/* How close colors need to be to match all the time */
#define COLOR_MATCH_NARROW 0.04

/* How close colors can be to match for a few pixels */
#define COLOR_MATCH_WIDE 0.60

/* How many pixels can we allow a wide match before stopping? */
#define WIDE_MATCH_THRESHOLD 3

// #define DEBUG
// #define DEBUG_WATCH

#define QUEUE_SIZE_CHUNK 1024

typedef struct queue_s {
  int x, y, y_outside;
} queue_t;

queue_t * queue;
int queue_size = 0, queue_end = 0;

/* Local function prototypes: */

SDL_Surface * global_screen, * global_last, * global_canvas;
Uint32 global_old_colr, global_cur_colr;
Uint8 * global_touched;
int global_extent_x1, global_extent_y1, global_extent_x2, global_extent_y2;
int global_prog_anim;

double colors_close(SDL_Surface * canvas, Uint32 c1, Uint32 c2);
Uint32 blend(SDL_Surface * canvas, Uint32 draw_colr, Uint32 old_colr, double pct);
void simulate_flood_fill_outside_check(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, int x, int y, int y_outside);
void draw_brush_fill_single(SDL_Surface * canvas, int x, int y, Uint32 draw_color, Uint8 * touched);

void init_queue(void);
void add_to_queue(int x, int y, int y_outside);
int remove_from_queue(int * x, int * y, int * y_outside);
void cleanup_queue(void);

void init_queue(void) {
  queue_size = 0;
  queue_end = 0;

  queue = (queue_t *) malloc(sizeof(queue_t) * QUEUE_SIZE_CHUNK);
  if (queue == NULL)
    {
      fprintf(stderr, "Fill queue cannot be malloc()'d\n");
      return;
    }

  queue_size = QUEUE_SIZE_CHUNK;
}

void add_to_queue(int x, int y, int y_outside) {
  /* Reallocate if we need more space */
  if (queue_end + 1 > queue_size)
    {
      queue_t * tmp;
      tmp = (queue_t *) realloc(queue, sizeof(queue_t) * (queue_size + QUEUE_SIZE_CHUNK));
      if (tmp == NULL)
        {
          fprintf(stderr, "Fill queue cannot be realloc()'d\n");
          return;
        }
      queue_size += QUEUE_SIZE_CHUNK;
#ifdef DEBUG
      printf("queue_size = %d\n", queue_size);
      fflush(stdout);
#endif
      queue = tmp;
    }

  queue[queue_end].x = x;
  queue[queue_end].y = y;
  queue[queue_end].y_outside = y_outside;

  queue_end++;

#ifdef DEBUG
  if (queue_end % 100 == 0)
    {
      printf("queue_end = %d\n", queue_end);
      fflush(stdout);
    }
#endif
}

int remove_from_queue(int * x, int * y, int * y_outside) {
  if (queue_end == 0)
    return 0;

  queue_end--;

  *x = queue[queue_end].x;
  *y = queue[queue_end].y;
  *y_outside = queue[queue_end].y_outside;

#ifdef DEBUG
  if (queue_end % 100 == 0)
    {
      printf("queue_end = %d\n", queue_end);
      fflush(stdout);
    }
#endif

  return 1;
}

void cleanup_queue(void) {
  if (queue != NULL)
    free(queue);

#ifdef DEBUG
  printf("Final size was %d\n", queue_size);
#endif

  queue_size = 0;
  queue_end = 0;
}


/* Returns how similar colors 'c1' and 'c2' are */
double colors_close(SDL_Surface * canvas, Uint32 c1, Uint32 c2)
{
  Uint8 r1, g1, b1, r2, g2, b2;

  if (c1 == c2)
    {
      /* Get it over with quick, if possible! */

      return 0.0;
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
      return (r + g + b);
    }
}

int would_flood_fill(SDL_Surface * canvas, Uint32 cur_colr, Uint32 old_colr)
{
  if (colors_close(canvas, cur_colr, old_colr) < COLOR_MATCH_NARROW)
    {
      return 0;
    } else {
      return 1;
    }
}

void do_flood_fill(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, SDL_Surface * last, SDL_Surface * canvas, int x, int y, Uint32 cur_colr, Uint32 old_colr, int * x1, int * y1, int * x2, int * y2, Uint8 * touched)
{
  simulate_flood_fill(screen, texture, renderer, last, canvas, x, y, cur_colr, old_colr, x1, y1, x2, y2, touched);
}


Uint32 blend(SDL_Surface * canvas, Uint32 draw_colr, Uint32 old_colr, double pct) {
  Uint8 old_r, old_g, old_b, draw_r, draw_g, draw_b, new_r, new_g, new_b;

  SDL_GetRGB(draw_colr, canvas->format, &draw_r, &draw_g, &draw_b);
  SDL_GetRGB(old_colr, canvas->format, &old_r, &old_g, &old_b);

  new_r = (Uint8) (((float) old_r) * (1.00 - pct) + ((float) draw_r * pct));
  new_g = (Uint8) (((float) old_g) * (1.00 - pct) + ((float) draw_g * pct));
  new_b = (Uint8) (((float) old_b) * (1.00 - pct) + ((float) draw_b * pct));

  return SDL_MapRGB(canvas->format, draw_r, draw_g, draw_b);
  return SDL_MapRGB(canvas->format, new_r, new_g, new_b);
}

void simulate_flood_fill(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, SDL_Surface * last, SDL_Surface * canvas, int x, int y, Uint32 cur_colr, Uint32 old_colr, int * extent_x1, int * extent_y1, int * extent_x2, int * extent_y2, Uint8 * touched) {
  int y_outside;

  /* Get ready */
  global_screen = screen;
  global_last = last;
  global_canvas = canvas;
  global_old_colr = old_colr;
  global_cur_colr = cur_colr;
  global_touched = touched;
  global_extent_x1 = x;
  global_extent_y1 = y;
  global_extent_x2 = x;
  global_extent_y2 = y;
  global_prog_anim = 0;

  /* Queue up the first things to work on: */
  init_queue();
  add_to_queue(x, y, 0);

  /* Do the work (possibly queuing more, as we go) */
  while (remove_from_queue(&x, &y, &y_outside))
    {
      simulate_flood_fill_outside_check(screen, texture, renderer, x, y, y_outside);
    }
  cleanup_queue();

  *extent_x1 = global_extent_x1;
  *extent_y1 = global_extent_y1;
  *extent_x2 = global_extent_x2;
  *extent_y2 = global_extent_y2;
}

void simulate_flood_fill_outside_check(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, int x, int y, int y_outside)
{
  int fillL, fillR, narrowFillL, narrowFillR, i, outside, just_queued;
  double in_line, closeness;
  Uint32 px_colr;
  Uint8 touch_byt;

  /* "Same" color?  No need to fill */
  if (!would_flood_fill(global_canvas, global_cur_colr, global_old_colr))
    return;

  if (x < 0 || x >= global_canvas->w || y < 0 || y >= global_canvas->h)
    return;

  /* Don't re-visit the same pixel */
  if (global_touched && global_touched[(y * global_canvas->w) + x])
    return;

  if (y < global_extent_y1)
    {
      global_extent_y1 = y;
    }
  if (y > global_extent_y2)
    {
      global_extent_y2 = y;
    }


  fillL = x;
  fillR = x;
  narrowFillL = x;
  narrowFillR = x;

  global_prog_anim++;
  if ((global_prog_anim % 8) == 0)
    {
      show_progress_bar_(screen, texture, renderer);

    }

  if ((global_prog_anim % 800) == 1) /* Always lay sound _once_ */
    playsound(global_canvas, 1, SND_FILL, 1, x, SNDDIST_NEAR);

#ifdef DEBUG_WATCH
  if (global_prog_anim % 100 == 0)
    {
      SDL_BlitSurface(global_canvas, NULL, global_screen, NULL);
      SDL_Flip(global_screen);
    }
#endif

  /* Find left side, filling along the way */

  px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillL /* - 1 */, y);
  in_line = colors_close(global_canvas, px_colr, global_old_colr);
  outside = 0;
  while (in_line < COLOR_MATCH_WIDE && outside < WIDE_MATCH_THRESHOLD)
    {
      if (in_line > COLOR_MATCH_NARROW) {
        outside++;
      } else {
        narrowFillL = fillL;
      }

      if (global_touched != NULL) {
        touch_byt = (255 - ((Uint8) (in_line * 85)));
        if (touch_byt == 0)
          touch_byt = 1;

        global_touched[(y * global_canvas->w) + fillL] = touch_byt;
      }

      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillL, y);
      putpixels[global_canvas->format->BytesPerPixel] (global_canvas, fillL, y, blend(global_canvas, global_cur_colr, px_colr, (1.0 - in_line)));
      fillL--;

      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillL, y);

      if (fillL >= 0)
        {
          in_line = colors_close(global_canvas, px_colr, global_old_colr);
        }
      else
        {
          in_line = 3.0;
        }
    }

  if (fillL >= 0)
    {
      if (global_touched != NULL)
        {
          touch_byt = (255 - ((Uint8) (in_line * 85)));
          if (touch_byt == 0)
            touch_byt = 1;

          global_touched[(y * global_canvas->w) + fillL] = touch_byt;
        }

      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillL, y);
      putpixels[global_canvas->format->BytesPerPixel] (global_canvas, fillL, y, blend(global_canvas, global_cur_colr, px_colr, (1.0 - in_line)));
    }


  if (fillL < global_extent_x1)
    {
      global_extent_x1 = fillL;
    }

  fillL++;


  /* Find right side, filling along the way */

  px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillR + 1, y);
  in_line = colors_close(global_canvas, px_colr, global_old_colr);
  outside = 0;
  while (in_line < COLOR_MATCH_WIDE && outside < WIDE_MATCH_THRESHOLD)
    {
      if (in_line > COLOR_MATCH_NARROW) {
        outside++;
      } else {
        narrowFillR = fillR;
      }

      if (global_touched != NULL) {
        touch_byt = (255 - ((Uint8) (in_line * 85)));
        if (touch_byt == 0)
          touch_byt = 1;

        global_touched[(y * global_canvas->w) + fillR] = touch_byt;
      }

      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillR, y);
      putpixels[global_canvas->format->BytesPerPixel] (global_canvas, fillR, y, blend(global_canvas, global_cur_colr, px_colr, (1.0 - in_line)));
      fillR++;

      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillR, y);

      if (fillR < global_canvas->w)
        {
          in_line = colors_close(global_canvas, px_colr, global_old_colr);
        }
      else
        {
          in_line = 3.0;
        }
    }

  if (fillR < global_canvas->w)
    {
      if (global_touched != NULL)
        {
          touch_byt = (255 - ((Uint8) (in_line * 85)));
          if (touch_byt == 0)
            touch_byt = 1;

          global_touched[(y * global_canvas->w) + fillR] = touch_byt;
        }

      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, fillR, y);
      putpixels[global_canvas->format->BytesPerPixel] (global_canvas, fillR, y, blend(global_canvas, global_cur_colr, px_colr, (1.0 - in_line)));
    }

  if (fillR > global_extent_x2)
    {
      global_extent_x2 = fillR;
    }

  fillR--;


  /* Continue filling upwards from this scanline */

  just_queued = 0;
  for (i = narrowFillL; i <= narrowFillR; i++)
    {
      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, i, y - 1);
      closeness = colors_close(global_canvas, px_colr, global_old_colr);
      if (y > 0 &&
          (
           closeness < COLOR_MATCH_NARROW ||
           (closeness < COLOR_MATCH_WIDE && y_outside < WIDE_MATCH_THRESHOLD)
          )
         )
        {
          if (!just_queued && (global_touched == NULL || !global_touched[((y - 1) * global_canvas->w) + i]))
            {
              add_to_queue(i, y - 1, y_outside + 1);
              just_queued = 1;
            }
          else
            {
              just_queued = 0;
            }
        }
      else
        {
          just_queued = 0;
        }
    }


  /* Continue filling downwards from this scanline */

  just_queued = 0;
  for (i = narrowFillL; i <= narrowFillR; i++)
    {
      px_colr = getpixels[global_last->format->BytesPerPixel] (global_last, i, y + 1);
      closeness = colors_close(global_canvas, px_colr, global_old_colr);
      if (y < global_canvas->h &&
          (
           closeness < COLOR_MATCH_NARROW ||
           (closeness < COLOR_MATCH_WIDE && y_outside < WIDE_MATCH_THRESHOLD)
          )
         )
        {
          if (!just_queued && (global_touched == NULL || !global_touched[((y + 1) * global_canvas->w) + i]))
            {
              add_to_queue(i, y + 1, y_outside + 1);
              just_queued = 1;
            }
          else
            {
              just_queued = 0;
            }
        }
      else
        {
          just_queued = 0;
        }
    }
}


void draw_linear_gradient(SDL_Surface * canvas, SDL_Surface * last,
  int x_left, int y_top, int x_right, int y_bottom,
  int x1, int y1, int x2, int y2, Uint32 draw_color, Uint8 * touched
) {
  Uint32 old_colr, new_colr;
  int xx, yy;
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
          ratio = 0.0;
        } else if (C >= C2) {
          /* At/beyond the mouse; completely faded out */
          ratio = 1.0;
        } else {
          /* The actual gradient... */
          ratio = (C - C1) / (C2 - C1);
        }

        /* Apply fuzziness at any antialiased edges we detected */
        ratio = (ratio * ((float) touched[yy * canvas->w + xx] / 255.0));

        new_r = (Uint8) (((float) old_r) * ratio + ((float) draw_r * (1.0 - ratio)));
        new_g = (Uint8) (((float) old_g) * ratio + ((float) draw_g * (1.0 - ratio)));
        new_b = (Uint8) (((float) old_b) * ratio + ((float) draw_b * (1.0 - ratio)));

        new_colr = SDL_MapRGB(canvas->format, new_r, new_g, new_b);
        putpixels[canvas->format->BytesPerPixel] (canvas, xx, yy, new_colr);
      }
    }
  }
}

void draw_brush_fill_single(SDL_Surface * canvas, int x, int y, Uint32 draw_color, Uint8 * touched) {
  int xx, yy;

  for (yy = -16; yy < 16; yy++)
    {
      for (xx = -16; xx < 16; xx++)
        {
          if ((xx * xx) + (yy * yy) < (16 * 16) &&
              touched[((y + yy) * canvas->w) + (x + xx)])
            {
              putpixels[canvas->format->BytesPerPixel] (canvas, x + xx, y + yy, draw_color);
            }
        }
    }
}

void draw_brush_fill(SDL_Surface * canvas,
  int x_left ATTRIBUTE_UNUSED, int y_top ATTRIBUTE_UNUSED, int x_right ATTRIBUTE_UNUSED, int y_bottom ATTRIBUTE_UNUSED,
  int x1, int y1, int x2, int y2, Uint32 draw_color, Uint8 * touched,
  int * up_x1, int * up_y1, int * up_x2, int * up_y2
) {
  int dx, dy;
  int y;
  int orig_x1, orig_y1, orig_x2, orig_y2, tmp;
  float m, b;

  orig_x1 = x1;
  orig_y1 = y1;

  orig_x2 = x2;
  orig_y2 = y2;

  dx = x2 - x1;
  dy = y2 - y1;

  if (dx != 0)
    {
      m = ((float)dy) / ((float)dx);
      b = y1 - m * x1;

      if (x2 >= x1)
        dx = 1;
      else
        dx = -1;

      while (x1 != x2)
        {
          y1 = m * x1 + b;
          y2 = m * (x1 + dx) + b;

          if (y1 > y2)
            {
              for (y = y1; y >= y2; y--)
                draw_brush_fill_single(canvas, x1, y, draw_color, touched);
            }
          else
            {
              for (y = y1; y <= y2; y++)
                draw_brush_fill_single(canvas, x1, y, draw_color, touched);
            }

          x1 = x1 + dx;
        }
    }
  else
    {
      if (y1 > y2)
        {
          y = y1;
          y1 = y2;
          y2 = y;
        }

      for (y = y1; y <= y2; y++)
        draw_brush_fill_single(canvas, x1, y, draw_color, touched);
    }

  if (orig_x1 > orig_x2)
    {
      tmp = orig_x1;
      orig_x1 = orig_x2;
      orig_x2 = tmp;
    }

  if (orig_y1 > orig_y2)
    {
      tmp = orig_y1;
      orig_y1 = orig_y2;
      orig_y2 = tmp;
    }

  *up_x1 = orig_x1 - 16;
  *up_y1 = orig_y1 - 16;
  *up_x2 = orig_x2 + 16;
  *up_y2 = orig_y2 + 16;
}

void draw_radial_gradient(SDL_Surface * canvas, int x_left, int y_top, int x_right, int y_bottom,
  int x, int y, Uint32 draw_color, Uint8 * touched
) {
  Uint32 old_colr, new_colr;
  int xx, yy;
  float xd, yd, dist, rad, ratio;
  Uint8 draw_r, draw_g, draw_b, old_r, old_g, old_b, new_r, new_g, new_b;

  /* Calculate the max radius of the filled area */
  xd = max(abs(x - x_right), abs(x - x_left));
  yd = max(abs(y - y_bottom), abs(y - y_top));
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
        xd = fabs((float) (xx - x));
        yd = fabs((float) (yy - y));
        dist = sqrt(xd * xd + yd * yd);
        if (dist < rad) {
          ratio = (dist / rad);

          /* Get the old color, and blend it (with a distance-based ratio) with the target color */
          old_colr = getpixels[canvas->format->BytesPerPixel] (canvas, xx, yy);
          SDL_GetRGB(old_colr, canvas->format, &old_r, &old_g, &old_b);

          /* Apply fuzziness at any antialiased edges we detected */
          ratio = (ratio * ((float) touched[yy * canvas->w + xx] / 255.0));

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

