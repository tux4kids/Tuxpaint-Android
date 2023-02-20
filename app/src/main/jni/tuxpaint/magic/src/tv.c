/*
  tv.c

  TV Magic Tools Plugin
  Tux Paint - A simple drawing program for children.

  Credits: Adam 'foo-script' Rakowski <foo-script@o2.pl>
  and Bill Kendrick <bill@newbreedsoftware.com>

  Copyright (c) 2002-2023 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: February 12, 2023
*/

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <math.h>

int RADIUS = 16;

Mix_Chunk *tv_snd;

Uint32 tv_api_version(void);
void tv_set_color(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int tv_init(magic_api * api);
int tv_get_tool_count(magic_api * api);
SDL_Surface *tv_get_icon(magic_api * api, int which);
char *tv_get_name(magic_api * api, int which);
int tv_get_group(magic_api * api, int which);
char *tv_get_description(magic_api * api, int which, int mode);
int tv_requires_colors(magic_api * api, int which);
void tv_release(magic_api * api, int which,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect);
void tv_shutdown(magic_api * api);
void tv_paint_tv(void *ptr_to_api, int which_tool, SDL_Surface * canvas,
                 SDL_Surface * snapshot, int x, int y);
void tv_do_tv(void *ptr_to_api, int which_tool, SDL_Surface * canvas,
              SDL_Surface * snapshot, int x, int y);
void tv_drag(magic_api * api, int which, SDL_Surface * canvas,
             SDL_Surface * snapshot, int ox, int oy, int x, int y,
             SDL_Rect * update_rect);
void tv_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
              SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void tv_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void tv_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int tv_modes(magic_api * api, int which);

//                              Housekeeping functions

Uint32 tv_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

void tv_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
                  SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  //get the colors from API and store it in structure
}

int tv_init(magic_api * api ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/tv.ogg",
           api->data_directory);
  tv_snd = Mix_LoadWAV(fname);

  return (1);
}

int tv_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

SDL_Surface *tv_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/tv.png",
           api->data_directory);

  return (IMG_Load(fname));
}

char *tv_get_name(magic_api * api ATTRIBUTE_UNUSED,
                  int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("TV"));
}

int tv_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_DISTORTS;
}

char *tv_get_description(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return
      strdup(gettext_noop
             ("Click and drag to make parts of your picture look like they are on television."));

  else
    return
      strdup(gettext_noop
             ("Click to make your picture look like it's on television."));

}

int tv_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void tv_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                SDL_Surface * canvas ATTRIBUTE_UNUSED,
                SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
                SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void tv_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Mix_FreeChunk(tv_snd);
}

// Interactivity functions

void tv_do_tv(void *ptr_to_api, int which_tool ATTRIBUTE_UNUSED,
              SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
              int x, int y)
{
  magic_api *api = (magic_api *) ptr_to_api;
  Uint8 r, g, b, i;

  for (i = 0; i < 2; i++)
  {
    /* Convert the line below to their red/green/blue elements */
    SDL_GetRGB(api->getpixel(snapshot, x, y + i), snapshot->format, &r, &g,
               &b);
    if (x % 3 == 0)
    {
      /* Red */
      g = 0;
      b = 0;
    }
    else if (x % 3 == 1)
    {
      /* Green */
      r = 0;
      b = 0;
    }
    else
    {
      /* Blue */
      r = 0;
      g = 0;
    }

    r = r / (i + 1);
    g = g / (i + 1);
    b = b / (i + 1);

    api->putpixel(canvas, x, y + i, SDL_MapRGB(canvas->format, r, g, b));
  }
}

void tv_paint_tv(void *ptr_to_api, int which_tool ATTRIBUTE_UNUSED,
                 SDL_Surface * canvas,
                 SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  int i, j;
  magic_api *api = (magic_api *) ptr_to_api;

  y = (y - (y % 2));

  for (i = x - RADIUS; i < x + RADIUS; i++)
  {
    for (j = y - RADIUS; j < y + RADIUS; j += 2)
    {
      if (api->in_circle(i - x, j - y, RADIUS) && !api->touched(i, j))
      {
        tv_do_tv(api, 0, canvas, snapshot, i, j);
      }
    }
  }
}

void tv_drag(magic_api * api, int which, SDL_Surface * canvas,
             SDL_Surface * snapshot, int ox, int oy, int x, int y,
             SDL_Rect * update_rect)
{
  api->line(api, which, canvas, snapshot, ox, oy, x, y, 1, tv_paint_tv);

  update_rect->x = min(ox, x) - RADIUS;
  update_rect->y = min(oy, y) - RADIUS;
  update_rect->w = abs(ox - x) + RADIUS * 2;
  update_rect->h = abs(oy - y) + RADIUS * 2;
  api->playsound(tv_snd, (x * 255) / canvas->w, 255);
}

void tv_click(magic_api * api, int which, int mode,
              SDL_Surface * canvas, SDL_Surface * last, int x, int y,
              SDL_Rect * update_rect)
{
  if (mode == MODE_FULLSCREEN)
  {
    for (y = 0; y < canvas->h; y += 2)
    {
      for (x = 0; x < canvas->w; x++)
      {
        tv_do_tv(api, which, canvas, last, x, y);
      }
    }

    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
    update_rect->x = update_rect->y = 0;
    api->playsound(tv_snd, 128, 255);
  }
  else
  {
    tv_drag(api, which, canvas, last, x, y, x, y, update_rect);
  }
}

void tv_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                 int mode ATTRIBUTE_UNUSED,
                 SDL_Surface * canvas ATTRIBUTE_UNUSED)
{

}

void tv_switchout(magic_api * api ATTRIBUTE_UNUSED,
                  int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas ATTRIBUTE_UNUSED)
{

}

int tv_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_FULLSCREEN | MODE_PAINT);
}
