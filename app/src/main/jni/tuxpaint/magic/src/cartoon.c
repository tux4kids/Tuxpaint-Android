/*
  cartoon.c

  Cartoon Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

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

  Last updated: May 16, 2023
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"


/* Our globals: */

static Mix_Chunk *cartoon_snd;
SDL_Surface *result_surf;
static int cartoon_radius = 16;

#define OUTLINE_THRESH 48

/* Local function prototypes: */
int cartoon_init(magic_api * api, Uint32 disabled_features);
Uint32 cartoon_api_version(void);
int cartoon_get_tool_count(magic_api * api);
SDL_Surface *cartoon_get_icon(magic_api * api, int which);
char *cartoon_get_name(magic_api * api, int which);
int cartoon_get_group(magic_api * api, int which);
char *cartoon_get_description(magic_api * api, int which, int mode);
void cartoon_apply_colors(magic_api * api, SDL_Surface * surf, int xx, int yy);
void cartoon_apply_outline(magic_api * api, int xx, int yy);
static void do_cartoon(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void cartoon_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void cartoon_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                   SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void cartoon_release(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void cartoon_shutdown(magic_api * api);
void cartoon_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int cartoon_requires_colors(magic_api * api, int which);
void cartoon_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void cartoon_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int cartoon_modes(magic_api * api, int which);
Uint8 cartoon_accepted_sizes(magic_api * api, int which, int mode);
Uint8 cartoon_default_size(magic_api * api, int which, int mode);
void cartoon_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                      SDL_Rect * update_rect);


// No setup required:
int cartoon_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/cartoon.wav", api->data_directory);
  cartoon_snd = Mix_LoadWAV(fname);

  return (1);
}

Uint32 cartoon_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// We have multiple tools:
int cartoon_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}

// Load our icons:
SDL_Surface *cartoon_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/cartoon.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *cartoon_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Cartoon")));
}

// Return our groups
int cartoon_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}

// Return our descriptions, localized:
char *cartoon_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
  {
    return (strdup(gettext_noop("Click and drag the mouse around to turn the picture into a cartoon.")));
  }
  else
  {
    return (strdup(gettext_noop("Click to turn the entire picture into a cartoon.")));
  }
}

// Do the effect:

void cartoon_apply_colors(magic_api * api, SDL_Surface * surf, int xx, int yy)
{
  Uint8 r, g, b;
  float hue, sat, val;

  SDL_GetRGB(api->getpixel(surf, xx, yy), surf->format, &r, &g, &b);
  api->rgbtohsv(r, g, b, &hue, &sat, &val);

  val = val - 0.5;
  val = val * 4;
  val = val + 0.5;

  if (val < 0)
    val = 0;
  else if (val > 1.0)
    val = 1.0;

  val = floor(val * 4) / 4;
  hue = floor(hue * 4) / 4;
  sat = floor(sat * 4) / 4;

  api->hsvtorgb(hue, sat, val, &r, &g, &b);
  api->putpixel(result_surf, xx, yy, SDL_MapRGB(result_surf->format, r, g, b));
}


void cartoon_apply_outline(magic_api * api, int xx, int yy)
{
  Uint8 r, g, b;
  Uint8 r1, g1, b1, r2, g2, b2;

  SDL_GetRGB(api->getpixel(result_surf, xx, yy), result_surf->format, &r, &g, &b);
  SDL_GetRGB(api->getpixel(result_surf, xx + 1, yy), result_surf->format, &r1, &g1, &b1);
  SDL_GetRGB(api->getpixel(result_surf, xx + 1, yy + 1), result_surf->format, &r2, &g2, &b2);

  if (abs(((r + g + b) / 3) - (r1 + g1 + b1) / 3) > OUTLINE_THRESH
      || abs(((r + g + b) / 3) - (r2 + g2 + b2) / 3) >
      OUTLINE_THRESH || abs(r - r1) > OUTLINE_THRESH
      || abs(g - g1) > OUTLINE_THRESH
      || abs(b - b1) > OUTLINE_THRESH
      || abs(r - r2) > OUTLINE_THRESH || abs(g - g2) > OUTLINE_THRESH || abs(b - b2) > OUTLINE_THRESH)
  {
    api->putpixel(result_surf, xx - 1, yy, SDL_MapRGB(result_surf->format, 0, 0, 0));
    api->putpixel(result_surf, xx, yy - 1, SDL_MapRGB(result_surf->format, 0, 0, 0));
    api->putpixel(result_surf, xx - 1, yy - 1, SDL_MapRGB(result_surf->format, 0, 0, 0));
  }
}


static void do_cartoon(void *ptr, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;

  for (yy = y - cartoon_radius; yy < y + cartoon_radius; yy = yy + 1)
  {
    for (xx = x - cartoon_radius; xx < x + cartoon_radius; xx = xx + 1)
    {
      if (api->in_circle(xx - x, yy - y, cartoon_radius))
      {
        api->putpixel(canvas, xx, yy, api->getpixel(result_surf, xx, yy));
      }
    }
  }
}

// Affect the canvas on drag:
void cartoon_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_cartoon);

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

  update_rect->x = ox - cartoon_radius;
  update_rect->y = oy - cartoon_radius;
  update_rect->w = (x + cartoon_radius) - update_rect->x;
  update_rect->h = (y + cartoon_radius) - update_rect->y;

  api->playsound(cartoon_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void cartoon_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  int effect_x, effect_y;

  for (effect_y = 0; effect_y < canvas->h; effect_y++)
  {
    if (effect_y % 10 == 0)
    {
      api->update_progress_bar();
    }

    for (effect_x = 0; effect_x < canvas->w; effect_x++)
    {
      cartoon_apply_colors(api, last, effect_x, effect_y);
    }
  }
  for (effect_y = 0; effect_y < canvas->h; effect_y++)
  {
    if (effect_y % 10 == 0)
    {
      api->update_progress_bar();
    }

    for (effect_x = 0; effect_x < canvas->w; effect_x++)
    {
      cartoon_apply_outline(api, effect_x, effect_y);
    }
  }

  if (mode == MODE_PAINT)
  {
    cartoon_drag(api, which, canvas, last, x, y, x, y, update_rect);
  }
  else
  {
    api->playsound(cartoon_snd, 128, 255);

    SDL_BlitSurface(result_surf, NULL, canvas, NULL);
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}

// Affect the canvas on release:
void cartoon_release(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED,
                     SDL_Surface * last ATTRIBUTE_UNUSED,
                     int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void cartoon_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (cartoon_snd != NULL)
    Mix_FreeChunk(cartoon_snd);
}

// Record the color from Tux Paint:
void cartoon_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// Use colors:
int cartoon_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void cartoon_switchin(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  Uint32 amask;

  amask = ~(canvas->format->Rmask | canvas->format->Gmask | canvas->format->Bmask);

  result_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                     canvas->w,
                                     canvas->h,
                                     canvas->format->BitsPerPixel,
                                     canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, amask);
}

void cartoon_switchout(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  if (result_surf != NULL)
    SDL_FreeSurface(result_surf);
}

int cartoon_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 cartoon_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 8;
  else
    return 0;
}

Uint8 cartoon_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void cartoon_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  cartoon_radius = size * 4;
}
