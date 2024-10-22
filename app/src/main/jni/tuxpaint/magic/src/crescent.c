/*
  crescent.c

  Draws crescent shapes

  Tux Paint - A simple drawing program for children.

  Copyright (c) 2024 by Bill Kendrick

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

  Last updated: October 7, 2024
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

static Mix_Chunk *crescent_snd;
static int crescent_neg_size;
Uint32 crescent_color;
int crescent_cx, crescent_cy;

#define NUM_SIZES 6
#define DEFAULT_SIZE 3


Uint32 crescent_api_version(void);
int crescent_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int crescent_get_tool_count(magic_api * api);
SDL_Surface *crescent_get_icon(magic_api * api, int which);
char *crescent_get_name(magic_api * api, int which);
int crescent_get_group(magic_api * api, int which);
int crescent_get_order(int which);
char *crescent_get_description(magic_api * api, int which, int mode);

void crescent_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void crescent_click(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void crescent_release(magic_api * api, int which,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void crescent_shutdown(magic_api * api);
void crescent_set_color(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int crescent_requires_colors(magic_api * api, int which);
void crescent_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void crescent_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int crescent_modes(magic_api * api, int which);
Uint8 crescent_accepted_sizes(magic_api * api, int which, int mode);
Uint8 crescent_default_size(magic_api * api, int which, int mode);
void crescent_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                       SDL_Rect * update_rect);

void do_crescent(magic_api * api, SDL_Surface * canvas, int x, int y, SDL_Rect * update_rect, int final);


Uint32 crescent_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int crescent_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/crescent.ogg", api->data_directory);
  crescent_snd = Mix_LoadWAV(fname);

  return (1);
}

int crescent_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (1);
}

SDL_Surface *crescent_get_icon(magic_api *api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/crescent.png", api->data_directory);

  return (IMG_Load(fname));
}

char *crescent_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Crescent")));
}

int crescent_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

int crescent_get_order(int which ATTRIBUTE_UNUSED)
{
  return 1250;
}

char *crescent_get_description(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Click and drag to draw a crescent shape. Use the size option to change the shape.")));
}

void do_crescent(magic_api *api, SDL_Surface *canvas, int x, int y, SDL_Rect *update_rect, int final)
{
  float angle;
  int radius, neg_radius;
  int xr, yr, xx, yy, spacing, xr2, yr2;

  /* Live preview (while dragging/adjusting) vs. final mode (click release) */
  if (final)
    spacing = 1;
  else
    spacing = 2;

  /* Distance to center controls overall radius */
  radius = sqrt(pow(x - crescent_cx, 2) + pow(y - crescent_cy, 2));
  if (radius < 32)
  {
    x = crescent_cx + 32;
    radius = 32;
  }

  /* Overall position (Up/Down/Left/Right) relative to center
     controls angle of the inner "negative" circle */
  angle = -atan2(y - crescent_cy, x - crescent_cx);

  /* Size options control the radius of the inner "negative" circle */
  neg_radius = (radius / 2) + ((radius * crescent_neg_size) / NUM_SIZES) + 4;

  /* Scan from top-to-bottom, left-to-right, within
     the square encompassing the overall circle, and
     decide when to place pixels */
  for (yr = -radius; yr <= radius; yr += spacing)
  {
    for (xr = -radius; xr <= radius; xr += spacing)
    {
      xx = crescent_cx + xr;
      yy = crescent_cy + yr;

      /* Within the canvas? */
      if (xx >= 0 && xx < canvas->w && yy >= 0 && yy < canvas->h)
      {
        /* Within the overall circle? */
        if (api->in_circle(xr, yr, radius))
        {
          xr2 = xr + cos(angle) * (neg_radius / 2);
          yr2 = yr - sin(angle) * (neg_radius / 2);

          /* But NOT within the inner "negative" circle? */
          if (!api->in_circle(xr2, yr2, neg_radius))
          {
            api->putpixel(canvas, xx, yy, crescent_color);
          }
        }
      }
    }
  }

  /* FIXME: Want to encompass both the new area we just drew,
     and (if dragging) the old area being removed, if it
     was bigger (i.e., the radius just shrunk) */
  /*
     update_rect->x = crescent_cx - radius - 1;
     update_rect->y = crescent_cy - radius - 1;
     update_rect->w = (radius * 2) + 2;
     update_rect->h = (radius * 2) + 2;
   */

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void crescent_drag(magic_api *api, int which ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                   SDL_Surface *last, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED,
                   int x, int y, SDL_Rect *update_rect)
{
  SDL_BlitSurface(last, NULL, canvas, NULL);    // FIXME

  do_crescent(api, canvas, x, y, update_rect, 0);

  api->playsound(crescent_snd, (x * 255) / canvas->w, 255);
}

void crescent_click(magic_api *api, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  crescent_cx = x;
  crescent_cy = y;

  do_crescent(api, canvas, x, y, update_rect, 0);

  api->playsound(crescent_snd, (x * 255) / canvas->w, 255);
}

void crescent_release(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  do_crescent(api, canvas, x, y, update_rect, 1);
}

void crescent_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (crescent_snd != NULL)
    Mix_FreeChunk(crescent_snd);
}

void crescent_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                        SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                        Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  crescent_color = SDL_MapRGB(canvas->format, r, g, b);
}

int crescent_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void crescent_switchin(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

void crescent_switchout(magic_api *api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int crescent_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 crescent_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return NUM_SIZES;
}

Uint8 crescent_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return DEFAULT_SIZE;
}

void crescent_set_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED,
                       Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  crescent_neg_size = size;
}
