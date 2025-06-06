/*
  smudge.c

  Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Smudge by Albert Cahalan <albert@users.sf.net>
  Wet Paint addition by Bill Kendrick <bill@newbreedsoftware.com>

  Copyright (c) 2002-2024
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

  Last updated: October 7, 2024

  FIXME: "Wet Paint" doesn't smudge enough -bjk 2023.04.23
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk *smudge_snd;
static Uint8 smudge_r, smudge_g, smudge_b;
static int smudge_radius = 16;

int smudge_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
Uint32 smudge_api_version(void);
SDL_Surface *smudge_get_icon(magic_api * api, int which);
char *smudge_get_name(magic_api * api, int which);
int smudge_get_group(magic_api * api, int which);
int smudge_get_order(int which);
char *smudge_get_description(magic_api * api, int which, int mode);
static void do_smudge(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void smudge_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void smudge_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                  SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void smudge_release(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void smudge_shutdown(magic_api * api);
void smudge_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int smudge_requires_colors(magic_api * api, int which);
void smudge_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void smudge_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int smudge_modes(magic_api * api, int which);
int smudge_get_tool_count(magic_api * api);
Uint8 smudge_accepted_sizes(magic_api * api, int which, int mode);
Uint8 smudge_default_size(magic_api * api, int which, int mode);
void smudge_set_size(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);


int smudge_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/smudge.wav", api->data_directory);
  smudge_snd = Mix_LoadWAV(fname);

  return (1);
}

Uint32 smudge_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// We have multiple tools:
int smudge_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (2);
}

// Load our icons:
SDL_Surface *smudge_get_icon(magic_api *api, int which)
{
  char fname[1024];

  if (which == 0)
    snprintf(fname, sizeof(fname), "%simages/magic/smudge.png", api->data_directory);
  else                          /* if (which == 1) */
    snprintf(fname, sizeof(fname), "%simages/magic/wetpaint.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *smudge_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  if (which == 0)
    return (strdup(gettext("Smudge")));
  else                          /* if (which == 1) */
    return (strdup(gettext("Wet Paint")));
}

// Return our groups
int smudge_get_group(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  if (which == 0)
    return MAGIC_TYPE_DISTORTS; /* Smudge */
  else
    return MAGIC_TYPE_PAINTING; /* Wet Paint */
}

// Return our order
int smudge_get_order(int which)
{
  if (which == 0)
    return 3;                   /* within MAGIC_TYPE_DISTORTS */
  else
    return 2500;                /* within MAGIC_TYPE_PAINTING */
}

// Return our descriptions, localized:
char *smudge_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  if (which == 0)
    return (strdup(gettext("Click and drag the mouse around to smudge the picture.")));
  else                          /* if (which == 1) */
    return (strdup(gettext("Click and drag the mouse around to draw with wet, smudgy paint.")));
}

// Do the effect:

static void do_smudge(void *ptr, int which, SDL_Surface *canvas, SDL_Surface *last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  static double state[256][256][3];
  unsigned i = (smudge_radius * 2) * (smudge_radius * 2);
  double rate = api->button_down()? 0.5 : 0.0;
  Uint8 r, g, b;
  int xx, yy, strength;

  if (which == 1)
  {
    /* Wet paint */
    for (yy = -(smudge_radius / 2); yy < (smudge_radius / 2); yy++)
      for (xx = -(smudge_radius / 2); xx < (smudge_radius / 2); xx++)
        if (api->in_circle(xx, yy, (smudge_radius / 2)))
        {
          SDL_GetRGB(api->getpixel(last, x + xx, y + yy), last->format, &r, &g, &b);
          strength = (abs(xx * yy) / (smudge_radius / 2)) + 1;
          api->putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format,
                                                           (smudge_r +
                                                            r * strength) /
                                                           (strength + 1),
                                                           (smudge_g +
                                                            g * strength) /
                                                           (strength + 1), (smudge_b + b * strength) / (strength + 1)));
        }
  }

  while (i--)
  {
    int iy = i / (smudge_radius * 2);
    int ix = i % (smudge_radius * 2);
    int radius_check;

    radius_check = (smudge_radius * 75) / 10;   /* For 16 radius, we'll use 120 */

    // is it not on the circle of radius sqrt(radius_check) at location (smudge_radius,smudge_radius)?
    if ((ix - smudge_radius) * (ix - smudge_radius) + (iy - smudge_radius) * (iy - smudge_radius) > radius_check)
      continue;
    // it is on the circle, so grab it

    SDL_GetRGB(api->getpixel(canvas, x + ix - smudge_radius, y + iy - smudge_radius), last->format, &r, &g, &b);
    state[ix][iy][0] = rate * state[ix][iy][0] + (1.0 - rate) * api->sRGB_to_linear(r);
    state[ix][iy][1] = rate * state[ix][iy][1] + (1.0 - rate) * api->sRGB_to_linear(g);
    state[ix][iy][2] = rate * state[ix][iy][2] + (1.0 - rate) * api->sRGB_to_linear(b);

    // opacity 100% --> new data not blended w/ existing data
    api->putpixel(canvas, x + ix - smudge_radius, y + iy - smudge_radius,
                  SDL_MapRGB(canvas->format,
                             api->linear_to_sRGB(state[ix][iy][0]),
                             api->linear_to_sRGB(state[ix][iy][1]), api->linear_to_sRGB(state[ix][iy][2])));
  }
}

// Affect the canvas on drag:
void smudge_drag(magic_api *api, int which, SDL_Surface *canvas,
                 SDL_Surface *last, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_smudge);

  api->playsound(smudge_snd, (x * 255) / canvas->w, 255);

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

  update_rect->x = ox - smudge_radius;
  update_rect->y = oy - smudge_radius;
  update_rect->w = (x + smudge_radius) - update_rect->x;
  update_rect->h = (y + smudge_radius) - update_rect->y;
}

// Affect the canvas on click:
void smudge_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  smudge_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

// Affect the canvas on click:
void smudge_release(magic_api *api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED,
                    SDL_Surface *canvas ATTRIBUTE_UNUSED,
                    SDL_Surface *last ATTRIBUTE_UNUSED,
                    int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void smudge_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (smudge_snd != NULL)
    Mix_FreeChunk(smudge_snd);
}

// Record the color from Tux Paint:
void smudge_set_color(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED,
                      SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                      Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  smudge_r = r;
  smudge_g = g;
  smudge_b = b;
}

// Use colors:
int smudge_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  if (which == 0)
    return 0;
  else                          /* if (which == 1) */
    return 1;
}

void smudge_switchin(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

void smudge_switchout(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int smudge_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}



Uint8 smudge_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 8;
}

Uint8 smudge_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void smudge_set_size(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED,
                     SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 size, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  smudge_radius = size * 4;
}
