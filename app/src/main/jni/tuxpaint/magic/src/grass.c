/*
  grass.c 

  Grass Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  by Albert Cahalan <albert@users.sf.net>
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
#include <stdlib.h>             /* for RAND_MAX */
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk *grass_snd;
static Uint8 grass_r, grass_g, grass_b;
static SDL_Surface *img_grass;

// Prototypes
int grass_init(magic_api * api);
Uint32 grass_api_version(void);
int grass_get_tool_count(magic_api * api);
SDL_Surface *grass_get_icon(magic_api * api, int which);
char *grass_get_name(magic_api * api, int which);
char *grass_get_description(magic_api * api, int which, int mode);
void grass_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void grass_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void grass_release(magic_api * api, int which,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void grass_shutdown(magic_api * api);
void grass_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int grass_requires_colors(magic_api * api, int which);
static void do_grass(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static int log2int(int x);
void grass_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void grass_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int grass_modes(magic_api * api, int which);


// No setup required:
int grass_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/grass.wav", api->data_directory);
  grass_snd = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/images/magic/grass_data.png", api->data_directory);
  img_grass = IMG_Load(fname);

  return (1);
}



Uint32 grass_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// We have multiple tools:
int grass_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}

// Load our icons:
SDL_Surface *grass_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/grass.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *grass_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Grass")));
}

// Return our descriptions, localized:
char *grass_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Click and drag to draw grass. Don’t forget the dirt!")));
}


// Affect the canvas on drag:
void grass_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 4, do_grass);

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

  update_rect->x = ox - 64;
  update_rect->y = oy - 64;
  update_rect->w = 128;
  update_rect->h = 192;

  api->playsound(grass_snd, (x * 255) / canvas->w, (y * 255) / canvas->h);
}

// Affect the canvas on click:
void grass_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  grass_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void grass_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                   int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void grass_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (grass_snd != NULL)
    Mix_FreeChunk(grass_snd);
}

// Record the color from Tux Paint:
void grass_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  grass_r = r;
  grass_g = g;
  grass_b = b;
}

// Use colors:
int grass_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

static void do_grass(void *ptr, int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;

  // grass color: 82,180,17
  static int bucket;
  double tmp_red, tmp_green, tmp_blue;
  Uint8 r, g, b, a;
  SDL_Rect src, dest;

  if (!api->button_down())
    bucket = 0;
  bucket += (3.5 + (rand() / (double)RAND_MAX)) * 7.0;
  while (bucket >= 0)
    {
      int rank = log2int(((double)y / canvas->h) * (0.99 + (rand() / (double)RAND_MAX)) * 64);
      int ah = 1 << rank;

      bucket -= ah;
      src.x = (rand() % 4) * 64;
      src.y = ah;
      src.w = 64;
      src.h = ah;

      dest.x = x - 32;
      dest.y = y - 30 + (int)((rand() / (double)RAND_MAX) * 30);

      tmp_red = api->sRGB_to_linear(grass_r) * 2.0 + (rand() / (double)RAND_MAX);
      tmp_green = api->sRGB_to_linear(grass_g) * 2.0 + (rand() / (double)RAND_MAX);
      tmp_blue = api->sRGB_to_linear(grass_b) * 2.0 + api->sRGB_to_linear(17);

      for (yy = 0; yy < ah; yy++)
        {
          for (xx = 0; xx < 64; xx++)
            {
              double rd, gd, bd;

              SDL_GetRGBA(api->getpixel(img_grass, xx + src.x, yy + src.y), img_grass->format, &r, &g, &b, &a);

              rd = api->sRGB_to_linear(r) * 8.0 + tmp_red;
              rd = rd * (a / 255.0) / 11.0;
              gd = api->sRGB_to_linear(g) * 8.0 + tmp_green;
              gd = gd * (a / 255.0) / 11.0;
              bd = api->sRGB_to_linear(b) * 8.0 + tmp_blue;
              bd = bd * (a / 255.0) / 11.0;

              SDL_GetRGB(api->getpixel(canvas, xx + dest.x, yy + dest.y), canvas->format, &r, &g, &b);

              r = api->linear_to_sRGB(api->sRGB_to_linear(r) * (1.0 - a / 255.0) + rd);
              g = api->linear_to_sRGB(api->sRGB_to_linear(g) * (1.0 - a / 255.0) + gd);
              b = api->linear_to_sRGB(api->sRGB_to_linear(b) * (1.0 - a / 255.0) + bd);

              api->putpixel(canvas, xx + dest.x, yy + dest.y, SDL_MapRGB(canvas->format, r, g, b));
            }
        }
    }
}

// this one rounds down
static int log2int(int x)
{
  int y = 0;

  if (x <= 1)
    return 0;
  x >>= 1;
  while (x)
    {
      x >>= 1;
      y++;
    }
  return y;
}

void grass_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void grass_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int grass_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}
