/*
  reflection.c

  Reflection Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2021-2023 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  https://tuxpaint.org/

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

  Last updated: April 19, 2023
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"


#define REFLECTION_XOR_SIZE 10

/* Our globals: */

static Mix_Chunk *reflection_snd;
int reflection_x1, reflection_y1;
enum reflection_sides
{
  REFLECTION_SIDE_TOP,
  REFLECTION_SIDE_LEFT,
  REFLECTION_SIDE_BOTTOM,
  REFLECTION_SIDE_RIGHT
};
int reflection_side_old;

/* Local function prototypes: */
int reflection_init(magic_api * api, Uint32 disabled_features);
Uint32 reflection_api_version(void);
int reflection_get_tool_count(magic_api * api);
SDL_Surface *reflection_get_icon(magic_api * api, int which);
char *reflection_get_name(magic_api * api, int which);
int reflection_get_group(magic_api * api, int which);
char *reflection_get_description(magic_api * api, int which, int mode);
void reflection_drag(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void do_reflection(magic_api * api, SDL_Surface * canvas, SDL_Surface * last,
                   int x, int y, SDL_Rect * update_rect, int show_origin);
void reflection_click(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void reflection_release(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void reflection_shutdown(magic_api * api);
void reflection_set_color(magic_api * api, int which, SDL_Surface * canvas,
                          SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int reflection_requires_colors(magic_api * api, int which);
void reflection_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void reflection_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int reflection_modes(magic_api * api, int which);
Uint8 reflection_accepted_sizes(magic_api * api, int which, int mode);
Uint8 reflection_default_size(magic_api * api, int which, int mode);
void reflection_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                         SDL_Rect * update_rect);



int reflection_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/reflection.ogg", api->data_directory);
  reflection_snd = Mix_LoadWAV(fname);

  return (1);
}

Uint32 reflection_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int reflection_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}

SDL_Surface *reflection_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/reflection.png", api->data_directory); /* FIXME */

  return (IMG_Load(fname));
}

char *reflection_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Reflection")));
}

int reflection_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PICTURE_WARPS;
}

char *reflection_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                 int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Click and drag the mouse around to add a reflection to your picture.")));
}

void reflection_drag(magic_api * api, int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas, SDL_Surface * last,
                     int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  do_reflection(api, canvas, last, x, y, update_rect, 1);
}

void do_reflection(magic_api * api, SDL_Surface * canvas,
                   SDL_Surface * last, int x, int y, SDL_Rect * update_rect, int show_origin)
{
  float scale;
  int xx, yy;
  SDL_Rect src, dest;
  int reflection_side;
  int update_all = 0;

  if (x <= 0)
    x = 1;
  else if (x >= canvas->w)
    x = canvas->w - 1;

  if (y <= 0)
    y = 1;
  else if (y >= canvas->h)
    y = canvas->h - 1;


  /* Determine what direction to go */
  if (abs(x - reflection_x1) < 32)
  {
    /* +/-32 pixels of wiggle room before we switch from vertical to horizontal */
    if (y > reflection_y1)
      reflection_side = REFLECTION_SIDE_BOTTOM;
    else
      reflection_side = REFLECTION_SIDE_TOP;
  }
  else
  {
    if (x < reflection_x1)
      reflection_side = REFLECTION_SIDE_LEFT;
    else
      reflection_side = REFLECTION_SIDE_RIGHT;
  }

  /* If we've changed direction, reset the canvas back
     to the snapshot before proceeding */
  if (reflection_side != reflection_side_old)
  {
    SDL_BlitSurface(last, NULL, canvas, NULL);
    reflection_side_old = reflection_side;
    update_all = 1;
  }


  /* Note: This isn't very good, and I basically
     brute-forced the code until it seemed to work
     well enough.  There's a ton of room for improvement!
     -bjk 2021.11.05 */
  if (reflection_side == REFLECTION_SIDE_BOTTOM)
  {
    /* Starting from `reflection_y1` and moving down,
       we'll copy from `reflection_y1` and moving up */

    scale = (float)reflection_y1 / (float)y;

    for (yy = reflection_y1; yy < canvas->h; yy++)
    {
      dest.x = 0;
      dest.y = yy;
      dest.w = canvas->w;
      dest.h = 1;

      src.x = 0;
      src.y = (reflection_y1 * scale) + ((y - yy) * scale);
      src.w = canvas->w;
      src.h = 1;

      if (src.y < 0)
      {
        src.y = yy;
      }

      SDL_BlitSurface(last, &src, canvas, &dest);
    }

    update_rect->x = 0;
    update_rect->y = reflection_y1;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h - reflection_y1 + 1;
  }
  else if (reflection_side == REFLECTION_SIDE_TOP)
  {
    /* Starting from `reflection_y1` and moving up,
       we'll copy from `reflection_y1` and moving down */

    scale = ((float)reflection_y1 / (float)y);

    for (yy = reflection_y1; yy >= 0; yy--)
    {
      dest.x = 0;
      dest.y = yy;
      dest.w = canvas->w;
      dest.h = 1;

      src.x = 0;
      src.y = (reflection_y1 / scale) + (y * scale) - (yy / scale);
      src.w = canvas->w;
      src.h = 1;

      if (src.y >= canvas->h)
      {
        src.y = yy;
      }

      SDL_BlitSurface(last, &src, canvas, &dest);
    }

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = reflection_y1;
  }
  else if (reflection_side == REFLECTION_SIDE_RIGHT)
  {
    /* Starting from `reflection_x1` and moving right,
       we'll copy from `reflection_x1` and moving left */

    scale = (float)reflection_x1 / (float)x;

    for (xx = reflection_x1; xx < canvas->w; xx++)
    {
      dest.x = xx;
      dest.y = 0;
      dest.w = 1;
      dest.h = canvas->h;

      src.x = (reflection_x1 * scale) + ((x - xx) * scale);
      src.y = 0;
      src.w = 1;
      src.h = canvas->h;

      if (src.x < 0)
      {
        src.x = xx;
      }

      SDL_BlitSurface(last, &src, canvas, &dest);
    }

    update_rect->x = reflection_x1;
    update_rect->y = 0;
    update_rect->w = canvas->w - reflection_x1 + 1;
    update_rect->h = canvas->h;
  }
  else if (reflection_side == REFLECTION_SIDE_LEFT)
  {
    /* Starting from `reflection_x1` and left up,
       we'll copy from `reflection_x1` and right down */

    scale = (float)reflection_x1 / (float)x;

    for (xx = reflection_x1; xx >= 0; xx--)
    {
      dest.x = xx;
      dest.y = 0;
      dest.w = 1;
      dest.h = canvas->h;

      src.x = (reflection_x1 / scale) + (x * scale) - (xx / scale);
      src.y = 0;
      src.w = 1;
      src.h = canvas->h;

      if (src.x >= canvas->w)
      {
        src.x = xx;
      }

      SDL_BlitSurface(last, &src, canvas, &dest);
    }

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = reflection_x1;
    update_rect->h = canvas->h;
  }

  /* TODO: Support reflecting at arbitrary angles!
     (a la linear gradient fill tool) */

  if (update_all)
  {
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
  else
  {
    for (yy = reflection_y1 - REFLECTION_XOR_SIZE; yy < reflection_y1 + REFLECTION_XOR_SIZE; yy++)
    {
      if (show_origin)
        api->xorpixel(canvas, reflection_x1, yy);
      else
        api->putpixel(canvas, reflection_x1, yy, api->getpixel(last, reflection_x1, yy));
    }

    for (xx = reflection_x1 - REFLECTION_XOR_SIZE; xx < reflection_x1 + REFLECTION_XOR_SIZE; xx++)
    {
      if (show_origin)
        api->xorpixel(canvas, xx, reflection_y1);
      else
        api->putpixel(canvas, xx, reflection_y1, api->getpixel(last, xx, reflection_y1));
    }

    update_rect->x -= REFLECTION_XOR_SIZE;
    update_rect->w += (REFLECTION_XOR_SIZE * 2);
    update_rect->y -= REFLECTION_XOR_SIZE;
    update_rect->h += (REFLECTION_XOR_SIZE * 2);
  }

  api->playsound(reflection_snd, (x * 255) / canvas->w, (y * 255) / canvas->h);
}

void reflection_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (x <= 0)
    x = 1;
  else if (x >= canvas->w)
    x = canvas->w - 1;

  if (y <= 0)
    y = 1;
  else if (y >= canvas->h)
    y = canvas->h - 1;

  reflection_x1 = x;
  reflection_y1 = y - 1;
  reflection_side_old = REFLECTION_SIDE_BOTTOM;

  reflection_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void reflection_release(magic_api * api, int which ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  do_reflection(api, canvas, last, x, y, update_rect, 0);
}

void reflection_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (reflection_snd != NULL)
    Mix_FreeChunk(reflection_snd);
}

void reflection_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                          Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                          SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

int reflection_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void reflection_switchin(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  reflection_x1 = canvas->w / 2;
  reflection_y1 = canvas->h / 2;
}

void reflection_switchout(magic_api * api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int reflection_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 reflection_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

Uint8 reflection_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

void reflection_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                         Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}
