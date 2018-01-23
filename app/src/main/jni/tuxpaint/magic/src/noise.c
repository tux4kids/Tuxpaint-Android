/*
  noise.c

  noise,Add noise the whole image.
  Tux Paint - A simple drawing program for children.

  Credits: Andrew Corcoran <akanewbie@gmail.com>

  Copyright (c) 2002-2007 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
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

  Last updated: June 6, 2008
  $Id$
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <math.h>
#include <limits.h>
#include <time.h>

#ifndef gettext_noop
#define gettext_noop(String) String
#endif

static const int noise_AMOUNT = 100.0;
static const int noise_RADIUS = 16;

enum
{
  TOOL_NOISE,
  noise_NUM_TOOLS
};

static Mix_Chunk *noise_snd_effect[noise_NUM_TOOLS];

const char *noise_snd_filenames[noise_NUM_TOOLS] = {
  "noise.ogg",
};

const char *noise_icon_filenames[noise_NUM_TOOLS] = {
  "noise.png",
};

const char *noise_names[noise_NUM_TOOLS] = {
  gettext_noop("Noise"),
};

const char *noise_descs[noise_NUM_TOOLS][2] = {
  {gettext_noop("Click and drag the mouse to add noise to parts of your picture."),
   gettext_noop("Click to add noise to your entire picture."),},
};

Uint32 noise_api_version(void);
int noise_init(magic_api * api);
SDL_Surface *noise_get_icon(magic_api * api, int which);
char *noise_get_name(magic_api * api, int which);
char *noise_get_description(magic_api * api, int which, int mode);
static void do_noise_pixel(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void do_noise_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which);
static void do_noise_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void noise_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void noise_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void noise_release(magic_api * api, int which,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void noise_shutdown(magic_api * api);
void noise_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int noise_requires_colors(magic_api * api, int which);
void noise_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void noise_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int noise_modes(magic_api * api, int which);
int noise_get_tool_count(magic_api * api ATTRIBUTE_UNUSED);

Uint32 noise_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

//Load sounds
int noise_init(magic_api * api)
{
  int i;
  char fname[1024];

  srand(time(0));

  for (i = 0; i < noise_NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, noise_snd_filenames[i]);
      noise_snd_effect[i] = Mix_LoadWAV(fname);
    }
  return (1);
}

int noise_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (noise_NUM_TOOLS);
}

// Load our icons:
SDL_Surface *noise_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, noise_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *noise_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(noise_names[which])));
}

// Return our descriptions, localized:
char *noise_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return (strdup(gettext_noop(noise_descs[which][mode - 1])));
}

//Do the effect for one pixel
static void do_noise_pixel(void *ptr, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  Uint8 temp[3];
  double temp2[3];
  int k;

  SDL_GetRGB(api->getpixel(canvas, x, y), canvas->format, &temp[0], &temp[1], &temp[2]);
  for (k = 0; k < 3; k++)
    {
      temp2[k] = clamp(0.0, (int)temp[k] - (rand() % noise_AMOUNT) + noise_AMOUNT / 2.0, 255.0);
    }
  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, temp2[0], temp2[1], temp2[2]));

}

// Do the effect for the full image
static void do_noise_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which)
{
  int x, y;

  for (y = 0; y < last->h; y++)
    {
      for (x = 0; x < last->w; x++)
        {
          do_noise_pixel(ptr, which, canvas, last, x, y);
        }
    }
}

//do the effect for the brush
static void do_noise_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - noise_RADIUS; yy < y + noise_RADIUS; yy++)
    {
      for (xx = x - noise_RADIUS; xx < x + noise_RADIUS; xx++)
        {
          if (api->in_circle(xx - x, yy - y, noise_RADIUS) && !api->touched(xx, yy))
            {
              do_noise_pixel(api, which, canvas, last, xx, yy);
            }
        }
    }
}

// Affect the canvas on drag:
void noise_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_noise_brush);

  api->playsound(noise_snd_effect[which], (x * 255) / canvas->w, 255);

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

  update_rect->x = ox - noise_RADIUS;
  update_rect->y = oy - noise_RADIUS;
  update_rect->w = (x + noise_RADIUS) - update_rect->x;
  update_rect->h = (y + noise_RADIUS) - update_rect->y;
}

// Affect the canvas on click:
void noise_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    noise_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
    {
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;
      do_noise_full(api, canvas, last, which);
      api->playsound(noise_snd_effect[which], 128, 255);
    }
}

// Affect the canvas on release:
void noise_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                   int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void noise_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;

  for (i = 0; i < noise_NUM_TOOLS; i++)
    {
      if (noise_snd_effect[i] != NULL)
        {
          Mix_FreeChunk(noise_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void noise_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED,
                     Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int noise_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void noise_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void noise_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int noise_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_FULLSCREEN | MODE_PAINT);
}
