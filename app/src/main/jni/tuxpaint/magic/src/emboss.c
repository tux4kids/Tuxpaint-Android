/*
  emboss.c

  Emboss Magic Tool Plugin
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

  Last updated: February 12, 2023
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk *emboss_snd;
static int emboss_radius = 16;

// Prototypes
Uint32 emboss_api_version(void);
int emboss_init(magic_api * api, Uint32 disabled_features);
int emboss_get_tool_count(magic_api * api);
SDL_Surface *emboss_get_icon(magic_api * api, int which);
char *emboss_get_name(magic_api * api, int which);
int emboss_get_group(magic_api * api, int which);
char *emboss_get_description(magic_api * api, int which, int mode);

void emboss_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void emboss_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void emboss_release(magic_api * api, int which,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void emboss_shutdown(magic_api * api);
void emboss_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int emboss_requires_colors(magic_api * api, int which);
void emboss_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void emboss_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int emboss_modes(magic_api * api, int which);
Uint8 emboss_accepted_sizes(magic_api * api, int which, int mode);
Uint8 emboss_default_size(magic_api * api, int which, int mode);
void emboss_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                     SDL_Rect * update_rect);


Uint32 emboss_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


// No setup required:
int emboss_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/emboss.ogg", api->data_directory);
  emboss_snd = Mix_LoadWAV(fname);

  return (1);
}

// We have multiple tools:
int emboss_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}

// Load our icons:
SDL_Surface *emboss_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/emboss.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *emboss_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Emboss")));
}

// Return our groups:
int emboss_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_DISTORTS;
}

// Return our descriptions, localized:
char *emboss_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return (strdup(gettext_noop("Click and drag the mouse to emboss the picture.")));
  else
    return (strdup(gettext_noop("Click to emboss the entire picture.")));
}


// Do the effect (single pixel; used by do_emboss() (painted circle)
// and emboss_click() when in fullscreen mode):
static void emboss_pixel(void *ptr, SDL_Surface * last, int x, int y, SDL_Surface * canvas)
{
  magic_api *api = (magic_api *) ptr;
  Uint8 r1, g1, b1, r2, g2, b2;
  int r;
  float h, s, v;
  int avg1, avg2;

  SDL_GetRGB(api->getpixel(last, x, y), last->format, &r1, &g1, &b1);
  SDL_GetRGB(api->getpixel(last, x + 2, y + 2), last->format, &r2, &g2, &b2);

  avg1 = (r1 + g1 + b1) / 3;
  avg2 = (r2 + g2 + b2) / 3;

  api->rgbtohsv(r1, g1, b1, &h, &s, &v);

  r = 128 + (((avg1 - avg2) * 3) / 2);
  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  v = (r / 255.0);

  api->hsvtorgb(h, s, v, &r1, &g1, &b1);

  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, r1, g1, b1));
}


// Do the effect (a circle around a touch point):
static void do_emboss(void *ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;

  for (yy = -emboss_radius; yy < emboss_radius; yy++)
  {
    for (xx = -emboss_radius; xx < emboss_radius; xx++)
    {
      if (api->in_circle(xx, yy, emboss_radius))
      {
        if (!api->touched(x + xx, y + yy))
        {
          emboss_pixel(api, last, x + xx, y + yy, canvas);
        }
      }
    }
  }
}

// Affect the canvas on drag:
void emboss_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_emboss);

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

  update_rect->x = ox - emboss_radius;
  update_rect->y = oy - emboss_radius;
  update_rect->w = (x + emboss_radius) - update_rect->x;
  update_rect->h = (y + emboss_radius) - update_rect->y;

  api->playsound(emboss_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void emboss_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
  {
    emboss_drag(api, which, canvas, last, x, y, x, y, update_rect);
  }
  else
  {
    for (y = 0; y < canvas->h; y++)
    {
      if (y % 10 == 0)
      {
        api->update_progress_bar();
      }

      for (x = 0; x < canvas->w; x++)
      {
        emboss_pixel(api, last, x, y, canvas);
      }
    }
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
    api->playsound(emboss_snd, 128, 255);
  }
}

// Affect the canvas on release:
void emboss_release(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED,
                    SDL_Surface * last ATTRIBUTE_UNUSED,
                    int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void emboss_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (emboss_snd != NULL)
    Mix_FreeChunk(emboss_snd);
}

// Record the color from Tux Paint:
void emboss_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                      Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// Use colors:
int emboss_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void emboss_switchin(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void emboss_switchout(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int emboss_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 emboss_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 8;
  else
    return 0;
}

Uint8 emboss_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void emboss_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                     SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  emboss_radius = size * 4;
}
