/*
  pixels.c

  Pixel art paintbrush Magic Tools Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2023 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: April 23, 2023
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* What tools we contain: */

enum
{
  TOOL_PIXELS,
  NUM_TOOLS
};


/* Our globals: */

static Mix_Chunk *pixel_snd;
static Uint8 pixels_r, pixels_g, pixels_b;
static int pixel_size = 8;


/* Local function prototype: */

int pixels_init(magic_api * api, Uint32 disabled_features);
Uint32 pixels_api_version(void);
int pixels_get_tool_count(magic_api * api);
SDL_Surface *pixels_get_icon(magic_api * api, int which);
char *pixels_get_name(magic_api * api, int which);
int pixels_get_group(magic_api * api, int which);
char *pixels_get_description(magic_api * api, int which, int mode);
void pixels_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void pixels_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                  SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void pixels_release(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void pixels_shutdown(magic_api * api);
void pixels_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int pixels_requires_colors(magic_api * api, int which);
void pixels_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void pixels_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int pixels_modes(magic_api * api, int which);
Uint8 pixels_accepted_sizes(magic_api * api, int which, int mode);
Uint8 pixels_default_size(magic_api * api, int which, int mode);
void pixels_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                     SDL_Rect * update_rect);


// No setup required:
int pixels_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/pixels.ogg", api->data_directory);
  pixel_snd = Mix_LoadWAV(fname);

  return (1);
}

Uint32 pixels_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// We have multiple tools:
int pixels_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

// Load our icons:
SDL_Surface *pixels_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/pixels.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *pixels_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Pixels")));
}

// Return our group (both the same):
int pixels_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

// Return our descriptions, localized:
char *pixels_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Click and drag to draw large pixels.")));

  return (NULL);
}

// Do the effect:

static void do_pixels(void *ptr ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  SDL_Rect dest;

  dest.x = (x / pixel_size) * pixel_size;
  dest.y = (y / pixel_size) * pixel_size;
  dest.w = pixel_size;
  dest.h = pixel_size;

  SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, pixels_r, pixels_g, pixels_b));
}

// Affect the canvas on drag:
void pixels_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_pixels);

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

  ox = (ox / pixel_size) * pixel_size;
  oy = (oy / pixel_size) * pixel_size;
  x = (x / pixel_size) * pixel_size;
  y = (y / pixel_size) * pixel_size;

  update_rect->x = ox - pixel_size * 2;
  update_rect->y = oy - pixel_size * 2;
  update_rect->w = (x + pixel_size * 2) - update_rect->x;
  update_rect->h = (y + pixel_size * 2) - update_rect->y;

  api->playsound(pixel_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void pixels_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  pixels_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void pixels_release(magic_api * api, int which ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED,
                    SDL_Surface * last ATTRIBUTE_UNUSED,
                    int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  api->stopsound();
}

// No setup happened:
void pixels_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (pixel_snd != NULL)
    Mix_FreeChunk(pixel_snd);
}

// Record the color from Tux Paint:
void pixels_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                      Uint8 b, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  pixels_r = r;
  pixels_g = g;
  pixels_b = b;
}

// Use colors:
int pixels_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void pixels_switchin(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void pixels_switchout(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int pixels_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 pixels_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

Uint8 pixels_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 1;
}

void pixels_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                     SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  int canv_area_scale;

  canv_area_scale = sqrt(canvas->w * canvas->h) / 144;

  pixel_size = pow(2, size) * canv_area_scale;
}
