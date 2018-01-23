/*
  tint.c

  Tint, Convert the image into differant shades of a user specified colour.

  Seperate Colours, Convert the image into white and the user specified colour. 
    This does not use differant shades of the user colour like tint does.

  Tux Paint - A simple drawing program for children.

  Credits: Andrew Corcoran <akanewbie@gmail.com>

  Copyright (c) 2002-2009 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: May 6, 2009
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

#ifndef gettext_noop
#define gettext_noop(String) String
#endif

enum
{
  TOOL_TINT,
  TOOL_THRESHOLD,
  tint_NUM_TOOLS
};
static Uint8 tint_r, tint_g, tint_b;
static int tint_min = INT_MAX;
static int tint_max = 0;

static const int tint_RADIUS = 16;

static Mix_Chunk *tint_snd_effect[tint_NUM_TOOLS];

const char *tint_snd_filenames[tint_NUM_TOOLS] = {
  "tint.wav",
  "fold.ogg"                    /* FIXME */
};

const char *tint_icon_filenames[tint_NUM_TOOLS] = {
  "tint.png",
  "colornwhite.png"
};

const char *tint_names[tint_NUM_TOOLS] = {
  gettext_noop("Tint"),
  gettext_noop("Color & White") // It does more than this but more intuitive than threshold.
};

const char *tint_descs[tint_NUM_TOOLS][2] = {
  {gettext_noop("Click and drag the mouse around to change the color of parts of your picture."),
   gettext_noop("Click to change the color of your entire picture."),},
  {gettext_noop("Click and drag the mouse around to turn parts of your picture into white and a color you choose."),
   gettext_noop("Click to turn your entire picture into white and a color you choose.")}
};

int tint_init(magic_api * api);
Uint32 tint_api_version(void);
int tint_get_tool_count(magic_api * api);
SDL_Surface *tint_get_icon(magic_api * api, int which);
char *tint_get_name(magic_api * api, int which);
char *tint_get_description(magic_api * api, int which, int mode);
static int tint_grey(Uint8 r1, Uint8 g1, Uint8 b1);
static void do_tint_pixel(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void do_tint_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which);
static void do_tint_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void tint_drag(magic_api * api, int which, SDL_Surface * canvas,
               SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void tint_click(magic_api * api, int which, int mode,
                SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void tint_release(magic_api * api, int which,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void tint_shutdown(magic_api * api);
void tint_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int tint_requires_colors(magic_api * api, int which);
void tint_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void tint_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int tint_modes(magic_api * api, int which);

Uint32 tint_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

//Load sounds
int tint_init(magic_api * api)
{
  int i;
  char fname[1024];

  for (i = 0; i < tint_NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, tint_snd_filenames[i]);
      tint_snd_effect[i] = Mix_LoadWAV(fname);
    }
  return (1);
}

int tint_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (tint_NUM_TOOLS);
}

// Load our icons:
SDL_Surface *tint_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, tint_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *tint_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(tint_names[which])));
}

// Return our descriptions, localized:
char *tint_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return (strdup(gettext_noop(tint_descs[which][mode - 1])));
}

//Calculates the grey scale value for a rgb pixel
static int tint_grey(Uint8 r1, Uint8 g1, Uint8 b1)
{
  return 0.3 * r1 + .59 * g1 + 0.11 * b1;
}

static void do_tint_pixel(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{

  magic_api *api = (magic_api *) ptr;
  Uint8 r, g, b;
  float h, s, v;

  SDL_GetRGB(api->getpixel(last, x, y), last->format, &r, &g, &b);
  {

    int greyValue = tint_grey(r, g, b);

    if (which == TOOL_TINT)
      {
        api->rgbtohsv(tint_r, tint_g, tint_b, &h, &s, &v);
        api->hsvtorgb(h, s, greyValue / 255.0, &r, &g, &b);
        api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, r, g, b));
      }
    else if (which == TOOL_THRESHOLD)
      {
        int thresholdValue = (tint_max - tint_min) / 2;

        if (greyValue < thresholdValue)
          {
            api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, tint_r, tint_g, tint_b));
          }
        else
          {
            api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, 255, 255, 255));
          }
      }
  }
}

// Do the effect:
static void do_tint_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which)
{
  int x, y;

  for (y = 0; y < last->h; y++)
    {
      for (x = 0; x < last->w; x++)
        {
          do_tint_pixel(ptr, which, canvas, last, x, y);
        }
    }
}

static void do_tint_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - tint_RADIUS; yy < y + tint_RADIUS; yy++)
    {
      for (xx = x - tint_RADIUS; xx < x + tint_RADIUS; xx++)
        {
          if (api->in_circle(xx - x, yy - y, tint_RADIUS) && !api->touched(xx, yy))
            {
              do_tint_pixel(api, which, canvas, last, xx, yy);
            }
        }
    }
}

// Affect the canvas on drag:
void tint_drag(magic_api * api, int which, SDL_Surface * canvas,
               SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_tint_brush);

  api->playsound(tint_snd_effect[which], (x * 255) / canvas->w, 255);

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

  update_rect->x = ox - tint_RADIUS;
  update_rect->y = oy - tint_RADIUS;
  update_rect->w = (x + tint_RADIUS) - update_rect->x;
  update_rect->h = (y + tint_RADIUS) - update_rect->y;
}

// Affect the canvas on click:
void tint_click(magic_api * api, int which, int mode,
                SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    tint_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
    {
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;
      do_tint_full(api, canvas, last, which);
      api->playsound(tint_snd_effect[which], 128, 255);
    }
}

// Affect the canvas on release:
void tint_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void tint_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;

  for (i = 0; i < tint_NUM_TOOLS; i++)
    {
      if (tint_snd_effect[i] != NULL)
        {
          Mix_FreeChunk(tint_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void tint_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  tint_r = r;
  tint_g = g;
  tint_b = b;
}

// Use colors:
int tint_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void tint_switchin(magic_api * api, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{

  int x, y;
  Uint8 r1, g1, b1;

  for (y = 0; y < canvas->h; y++)
    {
      for (x = 0; x < canvas->w; x++)
        {
          SDL_GetRGB(api->getpixel(canvas, x, y), canvas->format, &r1, &g1, &b1);
          {
            int greyValue = tint_grey(r1, g1, b1);

            if (greyValue < tint_min)
              {
                tint_min = greyValue;
              }
            if (greyValue > tint_max)
              {
                tint_max = greyValue;
              }
          }
        }
    }
}

void tint_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int tint_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_FULLSCREEN | MODE_PAINT);
}
