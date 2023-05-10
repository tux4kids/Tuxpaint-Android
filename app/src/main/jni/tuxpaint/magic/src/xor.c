/*
  xor.c

  Draws pixels which color depends on previous hue value
  (in HSV model) and coordinates

  Tux Paint - A simple drawing program for children.

  Copyright (c) 2013-2023 by Lukasz Dmitrowski

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

  Last updated: April 22, 2023
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

static Mix_Chunk *xor_snd;
static int xor_radius = 16;

Uint32 xor_api_version(void);
int xor_init(magic_api * api, Uint32 disabled_features);
int xor_get_tool_count(magic_api * api);
SDL_Surface *xor_get_icon(magic_api * api, int which);
char *xor_get_name(magic_api * api, int which);
int xor_get_group(magic_api * api, int which);
char *xor_get_description(magic_api * api, int which, int mode);

void xor_drag(magic_api * api, int which, SDL_Surface * canvas,
              SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void xor_click(magic_api * api, int which, int mode,
               SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void xor_release(magic_api * api, int which,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void xor_shutdown(magic_api * api);
void xor_set_color(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int xor_requires_colors(magic_api * api, int which);
void xor_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void xor_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int xor_modes(magic_api * api, int which);
Uint8 xor_accepted_sizes(magic_api * api, int which, int mode);
Uint8 xor_default_size(magic_api * api, int which, int mode);
void xor_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                  SDL_Rect * update_rect);


Uint32 xor_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int xor_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/xor.ogg", api->data_directory);
  xor_snd = Mix_LoadWAV(fname);

  return (1);
}

int xor_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}

SDL_Surface *xor_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/xor.png", api->data_directory);

  return (IMG_Load(fname));
}

char *xor_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Xor Colors")));
}

int xor_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}

char *xor_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return (strdup(gettext_noop("Click and drag to draw a XOR effect")));
  else
    return (strdup(gettext_noop("Click to draw a XOR effect on the whole picture")));
}

static void do_xor(void *ptr, int which ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  Uint8 r, g, b, xor;
  float hue, sat, val;
  Uint32 pixel;

  SDL_GetRGB(api->getpixel(canvas, x, y), canvas->format, &r, &g, &b);
  api->rgbtohsv(r, g, b, &hue, &sat, &val);
  if (sat == 0)
    xor = (2 * (int)hue + (x ^ y)) % 360;
  else
    xor = ((int)hue + (x ^ y)) % 360;
  api->hsvtorgb(xor, 1, 1, &r, &g, &b);
  pixel = SDL_MapRGB(canvas->format, r, g, b);
  api->putpixel(canvas, x, y, pixel);
}

static void do_xor_circle(void *ptr, int which ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;

  for (yy = -xor_radius; yy < xor_radius; yy++)
  {
    for (xx = -xor_radius; xx < xor_radius; xx++)
    {
      if (api->in_circle(xx, yy, xor_radius))
      {
        if (!api->touched(xx + x, yy + y))
          do_xor(api, which, canvas, last, x + xx, y + yy);
      }
    }
  }
}

void xor_drag(magic_api * api, int which, SDL_Surface * canvas,
              SDL_Surface * last ATTRIBUTE_UNUSED, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_xor_circle);

  if (ox > x)
  {
    int tmp = ox;

    ox = x;
    x = tmp;
  }
  if (oy > y)
  {
    int tmp = oy;

    oy = y;
    y = tmp;
  }

  update_rect->x = ox - xor_radius;
  update_rect->y = oy - xor_radius;
  update_rect->w = (x + xor_radius) - update_rect->x;
  update_rect->h = (y + xor_radius) - update_rect->y;

  api->playsound(xor_snd, (x * 255) / canvas->w, 255);
}

void xor_click(magic_api * api, int which, int mode,
               SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    xor_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
  {
    int xx, yy;

    for (yy = 0; yy < canvas->h; yy++)
      for (xx = 0; xx < canvas->w; xx++)
        do_xor(api, which, canvas, last, xx, yy);

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
    api->playsound(xor_snd, (x * 255) / canvas->w, 255);
  }
}

void xor_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                 SDL_Surface * canvas ATTRIBUTE_UNUSED,
                 SDL_Surface * last ATTRIBUTE_UNUSED, int x ATTRIBUTE_UNUSED,
                 int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void xor_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (xor_snd != NULL)
    Mix_FreeChunk(xor_snd);
}

void xor_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
                   SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED,
                   Uint8 b ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

int xor_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void xor_switchin(magic_api * api ATTRIBUTE_UNUSED,
                  int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void xor_switchout(magic_api * api ATTRIBUTE_UNUSED,
                   int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int xor_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 xor_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 8;
  else
    return 0;
}

Uint8 xor_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void xor_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                  Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  xor_radius = size * 4;
}
