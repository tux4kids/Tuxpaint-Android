/*
  alien.c
//
  alien, Modifies the colours of the image.
  Tux Paint - A simple drawing program for children.

  Credits: Andrew Corcoran <akanewbie@gmail.com> inspired by the Alien Map GIMP plugin

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

static const double alien_ANGLE[] = { 0, 0, 0 };
static const double alien_FREQUENCY[] = { 1, 1, 1 };

static const int alien_RADIUS = 16;

enum
{
  TOOL_alien,
  alien_NUM_TOOLS
};

static Mix_Chunk *alien_snd_effect[alien_NUM_TOOLS];

const char *alien_snd_filenames[alien_NUM_TOOLS] = {
  "alien.ogg",
};

const char *alien_icon_filenames[alien_NUM_TOOLS] = {
  "alien.png",
};

const char *alien_names[alien_NUM_TOOLS] = {
  gettext_noop("Color Shift"),
};

const char *alien_descs[alien_NUM_TOOLS][2] = {
  {gettext_noop("Click and drag the mouse to change the colors in parts of your picture."),
   gettext_noop("Click to change the colors in your entire picture."),},
};

// Prototypes
Uint32 alien_api_version(void);
int alien_init(magic_api * api);
int alien_get_tool_count(magic_api * api);
SDL_Surface *alien_get_icon(magic_api * api, int which);
char *alien_get_name(magic_api * api, int which);
char *alien_get_description(magic_api * api, int which, int mode);
void alien_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
Mix_Chunk *magic_loadsound(char *file);
void alien_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void alien_release(magic_api * api, int which,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void alien_shutdown(magic_api * api);
void alien_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int alien_requires_colors(magic_api * api, int which);
void alien_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void alien_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int alien_modes(magic_api * api, int which);


Uint32 alien_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

//Load sounds
int alien_init(magic_api * api)
{
  int i;
  char fname[1024];

  srand(time(0));

  for (i = 0; i < alien_NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, alien_snd_filenames[i]);
      alien_snd_effect[i] = Mix_LoadWAV(fname);
    }
  return (1);
}

int alien_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (alien_NUM_TOOLS);
}

// Load our icons:
SDL_Surface *alien_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, alien_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *alien_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(alien_names[which])));
}

// Return our descriptions, localized:
char *alien_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return (strdup(gettext_noop(alien_descs[which][mode - 1])));
}

//Do the effect for one pixel
static void do_alien_pixel(void *ptr, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  Uint8 temp[3];
  double temp2[3];
  int k;

  SDL_GetRGB(api->getpixel(canvas, x, y), canvas->format, &temp[0], &temp[1], &temp[2]);
  for (k = 0; k < 3; k++)
    {
//EP      temp2[k] = clamp(0,127.5 * (1.0 + sin (((temp[k] / 127.5 - 1.0) * alien_FREQUENCY[k] + alien_ANGLE[k] / 180.0) * M_PI)),255);
      temp2[k] = clamp(0.0,
                       127.5 * (1.0 +
                                sin(((temp[k] / 127.5 - 1.0) * alien_FREQUENCY[k] + alien_ANGLE[k] / 180.0) * M_PI)),
                       255.0);
    }
  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, temp2[0], temp2[1], temp2[2]));

}

// Do the effect for the full image
static void do_alien_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which)
{

  magic_api *api = (magic_api *) ptr;

  int x, y;

  for (y = 0; y < last->h; y++)
    {
      for (x = 0; x < last->w; x++)
        {
          do_alien_pixel(ptr, which, canvas, last, x, y);
        }
    }
}

//do the effect for the brush
static void do_alien_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - alien_RADIUS; yy < y + alien_RADIUS; yy++)
    {
      for (xx = x - alien_RADIUS; xx < x + alien_RADIUS; xx++)
        {
          if (api->in_circle(xx - x, yy - y, alien_RADIUS) && !api->touched(xx, yy))
            {
              do_alien_pixel(api, which, canvas, last, xx, yy);
            }
        }
    }
}

// Affect the canvas on drag:
void alien_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_alien_brush);

  api->playsound(alien_snd_effect[which], (x * 255) / canvas->w, 255);

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

  update_rect->x = ox - alien_RADIUS;
  update_rect->y = oy - alien_RADIUS;
  update_rect->w = (x + alien_RADIUS) - update_rect->x;
  update_rect->h = (y + alien_RADIUS) - update_rect->y;
}

int use_sound = 1;

Mix_Chunk *magic_loadsound(char *file)
{
  Mix_Chunk *temp;

  if (!use_sound)
    {
      return (Mix_Chunk *) - 1;
    }
  temp = Mix_LoadWAV(file);
  return temp;
}

// Affect the canvas on click:
void alien_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    alien_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
    {
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;
      do_alien_full(api, canvas, last, which);
      api->playsound(alien_snd_effect[which], 128, 255);
    }
}

// Affect the canvas on release:
void alien_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                   int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void alien_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;

  for (i = 0; i < alien_NUM_TOOLS; i++)
    {
      if (alien_snd_effect[i] != NULL)
        {
          Mix_FreeChunk(alien_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void alien_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED,
                     Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int alien_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void alien_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void alien_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int alien_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_FULLSCREEN | MODE_PAINT);
}
