/*
  sharpen.c

  Sharpen, Trace Contour and Silhouette Magic Tool Plugin
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

/* Our globals: */

enum
{
  TOOL_TRACE,
  TOOL_SHARPEN,
  TOOL_SILHOUETTE,
  sharpen_NUM_TOOLS
};

static const int THRESHOLD = 50;

static const int sharpen_RADIUS = 16;

static const double SHARPEN = 0.5;

static Mix_Chunk *sharpen_snd_effect[sharpen_NUM_TOOLS];

const char *sharpen_snd_filenames[sharpen_NUM_TOOLS] = {
  "edges.ogg",
  "sharpen.ogg",
  "silhouette.ogg"
};

const char *sharpen_icon_filenames[sharpen_NUM_TOOLS] = {
  "edges.png",
  "sharpen.png",
  "silhouette.png"
};

const char *sharpen_names[sharpen_NUM_TOOLS] = {
  gettext_noop("Edges"),
  gettext_noop("Sharpen"),
  gettext_noop("Silhouette")
};

const char *sharpen_descs[sharpen_NUM_TOOLS][2] = {
  {gettext_noop("Click and drag the mouse to trace edges in parts of your picture."),
   gettext_noop("Click to trace edges in your entire picture."),},
  {gettext_noop("Click and drag the mouse to sharpen parts of your picture."),
   gettext_noop("Click to sharpen the entire picture."),},
  {gettext_noop("Click and drag the mouse to create a black and white silhouette."),
   gettext_noop("Click to create a black and white silhouette of your entire picture.")},
};

Uint32 sharpen_api_version(void);
int sharpen_init(magic_api * api);
int sharpen_get_tool_count(magic_api * api);
SDL_Surface *sharpen_get_icon(magic_api * api, int which);
char *sharpen_get_name(magic_api * api, int which);
char *sharpen_get_description(magic_api * api, int which, int mode);
static int sharpen_grey(Uint8 r1, Uint8 g1, Uint8 b1);
static void do_sharpen_pixel(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void do_sharpen_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which);
static void do_sharpen_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void sharpen_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void sharpen_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void sharpen_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void sharpen_shutdown(magic_api * api);
void sharpen_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int sharpen_requires_colors(magic_api * api, int which);
void sharpen_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void sharpen_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int sharpen_modes(magic_api * api, int which);

Uint32 sharpen_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


// No setup required:
int sharpen_init(magic_api * api)
{

  int i;
  char fname[1024];

  for (i = 0; i < sharpen_NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, sharpen_snd_filenames[i]);
      sharpen_snd_effect[i] = Mix_LoadWAV(fname);
    }

  return (1);
}

// We have multiple tools:
int sharpen_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (sharpen_NUM_TOOLS);
}

// Load our icons:
SDL_Surface *sharpen_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, sharpen_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *sharpen_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(sharpen_names[which])));
}

// Return our descriptions, localized:
char *sharpen_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return (strdup(gettext_noop(sharpen_descs[which][mode - 1])));
}

//Calculates the grey scale value for a rgb pixel
static int sharpen_grey(Uint8 r1, Uint8 g1, Uint8 b1)
{
  return 0.3 * r1 + .59 * g1 + 0.11 * b1;
}

// Do the effect:
static void do_sharpen_pixel(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{

  magic_api *api = (magic_api *) ptr;

  Uint8 r1, g1, b1;
  int grey;
  int i, j;
  double sobel_1 = 0, sobel_2 = 0;
  double temp;

  //Sobel weighting masks
  const int sobel_weights_1[3][3] = { {1, 2, 1},
  {0, 0, 0},
  {-1, -2, -1}
  };
  const int sobel_weights_2[3][3] = { {-1, 0, 1},
  {-2, 0, 2},
  {-1, 0, 1}
  };

  sobel_1 = 0;
  sobel_2 = 0;
  for (i = -1; i < 2; i++)
    {
      for (j = -1; j < 2; j++)
        {
          //No need to check if inside canvas, getpixel does it for us.
          SDL_GetRGB(api->getpixel(last, x + i, y + j), last->format, &r1, &g1, &b1);
          grey = sharpen_grey(r1, g1, b1);
          sobel_1 += grey * sobel_weights_1[i + 1][j + 1];
          sobel_2 += grey * sobel_weights_2[i + 1][j + 1];
        }
    }

  temp = sqrt(sobel_1 * sobel_1 + sobel_2 * sobel_2);
  temp = (temp / 1443) * 255.0;

  // set image to white where edge value is below THRESHOLD
  if (which == TOOL_TRACE)
    {
      if (temp < THRESHOLD)
        {
          api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, 255, 255, 255));
        }
    }
  //Simply display the edge values - provides a nice black and white silhouette image
  else if (which == TOOL_SILHOUETTE)
    {
      api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, temp, temp, temp));
    }
  //Add the edge values to the original image, creating a more distinct jump in contrast at edges
  else if (which == TOOL_SHARPEN)
    {
      SDL_GetRGB(api->getpixel(last, x, y), last->format, &r1, &g1, &b1);
      api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, clamp(0.0, r1 + SHARPEN * temp, 255.0),
                                             clamp(0.0, g1 + SHARPEN * temp, 255.0),
                                             clamp(0.0, b1 + SHARPEN * temp, 255.0)));
    }
}

// Do the effect for the full image
static void do_sharpen_full(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int which)
{

  // magic_api * api = (magic_api *) ptr;

  int x, y;

  for (y = 0; y < last->h; y++)
    {
      for (x = 0; x < last->w; x++)
        {
          do_sharpen_pixel(ptr, which, canvas, last, x, y);
        }
    }
}

//do the effect for the brush
static void do_sharpen_brush(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - sharpen_RADIUS; yy < y + sharpen_RADIUS; yy++)
    {
      for (xx = x - sharpen_RADIUS; xx < x + sharpen_RADIUS; xx++)
        {
          if (api->in_circle(xx - x, yy - y, sharpen_RADIUS) && !api->touched(xx, yy))
            {
              do_sharpen_pixel(api, which, canvas, last, xx, yy);
            }
        }
    }
}

// Affect the canvas on drag:
void sharpen_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_sharpen_brush);

  api->playsound(sharpen_snd_effect[which], (x * 255) / canvas->w, 255);

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

  update_rect->x = ox - sharpen_RADIUS;
  update_rect->y = oy - sharpen_RADIUS;
  update_rect->w = (x + sharpen_RADIUS) - update_rect->x;
  update_rect->h = (y + sharpen_RADIUS) - update_rect->y;
}

// Affect the canvas on click:
void sharpen_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    sharpen_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
    {
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;
      do_sharpen_full(api, canvas, last, which);
      api->playsound(sharpen_snd_effect[which], 128, 255);
    }
}

// Affect the canvas on release:
void sharpen_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                     int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void sharpen_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;

  for (i = 0; i < sharpen_NUM_TOOLS; i++)
    {
      if (sharpen_snd_effect[i] != NULL)
        {
          Mix_FreeChunk(sharpen_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void sharpen_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED,
                       Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int sharpen_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void sharpen_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void sharpen_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int sharpen_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_FULLSCREEN | MODE_PAINT);
}
