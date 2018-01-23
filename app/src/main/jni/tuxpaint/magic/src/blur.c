/*
  blur.c
//
  blur, Blur tool
  Tux Paint - A simple drawing program for children.

  Credits: Bill Kendrick<bill@newbreedsoftware.com> & Andrew Corcoran <akanewbie@gmail.com>

  Copyright (c) 2002-2008 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: July 8, 2008
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

// Prototypes
Uint32 blur_api_version(void);
int blur_init(magic_api * api);
int blur_get_tool_count(magic_api * api);
SDL_Surface *blur_get_icon(magic_api * api, int which);
char *blur_get_name(magic_api * api, int which);
char *blur_get_description(magic_api * api, int which, int mode);
void blur_drag(magic_api * api, int which, SDL_Surface * canvas,
               SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void blur_click(magic_api * api, int which, int mode,
                SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void blur_release(magic_api * api, int which,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void blur_shutdown(magic_api * api);
void blur_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int blur_requires_colors(magic_api * api, int which);
void blur_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void blur_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int blur_modes(magic_api * api, int which);

enum
{
  TOOL_blur,
  blur_NUM_TOOLS
};

static const int blur_RADIUS = 16;

static Mix_Chunk *blur_snd_effect[blur_NUM_TOOLS];

const char *blur_snd_filenames[blur_NUM_TOOLS] = {
  "blur.wav",
};

const char *blur_icon_filenames[blur_NUM_TOOLS] = {
  "blur.png",
};

const char *blur_names[blur_NUM_TOOLS] = {
  gettext_noop("Blur"),
};

const char *blur_descs[blur_NUM_TOOLS][2] = {
  {gettext_noop("Click and drag the mouse around to blur the image."),
   gettext_noop("Click to blur the entire image.")},
};

Uint32 blur_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

//Load sounds
int blur_init(magic_api * api)
{

  int i;
  char fname[1024];

  for (i = 0; i < blur_NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, blur_snd_filenames[i]);
      blur_snd_effect[i] = Mix_LoadWAV(fname);
    }
  return (1);
}

int blur_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (blur_NUM_TOOLS);
}

// Load our icons:
SDL_Surface *blur_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, blur_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *blur_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(blur_names[which])));
}

// Return our descriptions, localized:
char *blur_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return (strdup(gettext_noop(blur_descs[which][mode - 1])));
}

//Do the effect for one pixel
static void do_blur_pixel(void *ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int i, j, k;
  Uint8 temp[3];
  double blurValue[3];

  //5x5 gaussiann weighting window
  const int weight[5][5] = { {1, 4, 7, 4, 1},
  {4, 16, 26, 16, 4},
  {7, 26, 41, 26, 7},
  {4, 16, 26, 16, 4},
  {1, 4, 7, 4, 1}
  };

  for (k = 0; k < 3; k++)
    {
      blurValue[k] = 0;
    }

  for (i = -2; i < 3; i++)
    {
      for (j = -2; j < 3; j++)
        {
          //Add the pixels around the current one wieghted 
          SDL_GetRGB(api->getpixel(last, x + i, y + j), last->format, &temp[0], &temp[1], &temp[2]);
          for (k = 0; k < 3; k++)
            {
              blurValue[k] += temp[k] * weight[i + 2][j + 2];
            }
        }
    }
  for (k = 0; k < 3; k++)
    {
      blurValue[k] /= 273;
    }
  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, blurValue[0], blurValue[1], blurValue[2]));
}

// Do the effect for the full image
static void do_blur_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which)
{

  //magic_api * api = (magic_api *) ptr;

  int x, y;

  for (y = 0; y < last->h; y++)
    {
      for (x = 0; x < last->w; x++)
        {
          do_blur_pixel(ptr, which, canvas, last, x, y);
        }
    }
}

//do the effect for the brush
static void do_blur_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - blur_RADIUS; yy < y + blur_RADIUS; yy++)
    {
      for (xx = x - blur_RADIUS; xx < x + blur_RADIUS; xx++)
        {
          if (api->in_circle(xx - x, yy - y, blur_RADIUS) && !api->touched(xx, yy))
            {
              do_blur_pixel(api, which, canvas, last, xx, yy);
            }
        }
    }
}

// Affect the canvas on drag:
void blur_drag(magic_api * api, int which, SDL_Surface * canvas,
               SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_blur_brush);

  api->playsound(blur_snd_effect[which], (x * 255) / canvas->w, 255);

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

  update_rect->x = ox - blur_RADIUS;
  update_rect->y = oy - blur_RADIUS;
  update_rect->w = (x + blur_RADIUS) - update_rect->x;
  update_rect->h = (y + blur_RADIUS) - update_rect->y;
}

// Affect the canvas on click:
void blur_click(magic_api * api, int which, int mode,
                SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    blur_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
    {
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;
      do_blur_full(api, canvas, last, which);
      api->playsound(blur_snd_effect[which], 128, 255);
    }
}

// Affect the canvas on release:
void blur_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void blur_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;

  for (i = 0; i < blur_NUM_TOOLS; i++)
    {
      if (blur_snd_effect[i] != NULL)
        {
          Mix_FreeChunk(blur_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void blur_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED,
                    Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int blur_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void blur_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void blur_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int blur_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_FULLSCREEN | MODE_PAINT);
}
