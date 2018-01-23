/*
  smudge.c

  Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Smudge by Albert Cahalan <albert@users.sf.net>
  Wet Paint addition by Bill Kendrick <bill@newbreedsoftware.com>

  Copyright (c) 2002-2011
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

  Last updated: Oconter 8, 2009
  $Id$
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk *smudge_snd;
static Uint8 smudge_r, smudge_g, smudge_b;

int smudge_init(magic_api * api);
Uint32 smudge_api_version(void);
SDL_Surface *smudge_get_icon(magic_api * api, int which);
char *smudge_get_name(magic_api * api, int which);
char *smudge_get_description(magic_api * api, int which, int mode);
static void do_smudge(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void smudge_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void smudge_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void smudge_release(magic_api * api, int which,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void smudge_shutdown(magic_api * api);
void smudge_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int smudge_requires_colors(magic_api * api, int which);
void smudge_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void smudge_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int smudge_modes(magic_api * api, int which);
int smudge_get_tool_count(magic_api * api);

// No setup required:
int smudge_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/smudge.wav", api->data_directory);
  smudge_snd = Mix_LoadWAV(fname);

  return (1);
}

Uint32 smudge_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// We have multiple tools:
int smudge_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (2);
}

// Load our icons:
SDL_Surface *smudge_get_icon(magic_api * api, int which)
{
  char fname[1024];

  if (which == 0)
    snprintf(fname, sizeof(fname), "%s/images/magic/smudge.png", api->data_directory);
  else                          /* if (which == 1) */
    snprintf(fname, sizeof(fname), "%s/images/magic/wetpaint.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *smudge_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == 0)
    return (strdup(gettext_noop("Smudge")));
  else                          /* if (which == 1) */
    return (strdup(gettext_noop("Wet Paint")));
}

// Return our descriptions, localized:
char *smudge_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  if (which == 0)
    return (strdup(gettext_noop("Click and drag the mouse around to smudge the picture.")));
  else                          /* if (which == 1) */
    return (strdup(gettext_noop("Click and drag the mouse around to draw with wet, smudgy paint.")));
}

// Do the effect:

static void do_smudge(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  static double state[32][32][3];
  unsigned i = 32 * 32;
  double rate = api->button_down()? 0.5 : 0.0;
  Uint8 r, g, b;
  int xx, yy, strength;

  if (which == 1)
    {
      /* Wet paint */
      for (yy = -8; yy < 8; yy++)
        for (xx = -8; xx < 8; xx++)
          if (api->in_circle(xx, yy, 8))
            {
              SDL_GetRGB(api->getpixel(last, x + xx, y + yy), last->format, &r, &g, &b);
              //strength = (abs(xx * yy) / 8) + 6;
              strength = (abs(xx * yy) / 8) + 1;
              api->putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format,
                                                               (smudge_r + r * strength) / (strength + 1),
                                                               (smudge_g + g * strength) / (strength + 1),
                                                               (smudge_b + b * strength) / (strength + 1)));
            }
    }

  while (i--)
    {
      int iy = i >> 5;
      int ix = i & 0x1f;

      // is it not on the circle of radius sqrt(120) at location 16,16?
      if ((ix - 16) * (ix - 16) + (iy - 16) * (iy - 16) > 120)
        continue;
      // it is on the circle, so grab it

      SDL_GetRGB(api->getpixel(canvas, x + ix - 16, y + iy - 16), last->format, &r, &g, &b);
      state[ix][iy][0] = rate * state[ix][iy][0] + (1.0 - rate) * api->sRGB_to_linear(r);
      state[ix][iy][1] = rate * state[ix][iy][1] + (1.0 - rate) * api->sRGB_to_linear(g);
      state[ix][iy][2] = rate * state[ix][iy][2] + (1.0 - rate) * api->sRGB_to_linear(b);

      // opacity 100% --> new data not blended w/ existing data
      api->putpixel(canvas, x + ix - 16, y + iy - 16,
                    SDL_MapRGB(canvas->format, api->linear_to_sRGB(state[ix][iy][0]),
                               api->linear_to_sRGB(state[ix][iy][1]), api->linear_to_sRGB(state[ix][iy][2])));
    }
}

// Affect the canvas on drag:
void smudge_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
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

  update_rect->x = ox - 16;
  update_rect->y = oy - 16;
  update_rect->w = (x + 16) - update_rect->x;
  update_rect->h = (y + 16) - update_rect->y;
}

// Affect the canvas on click:
void smudge_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  smudge_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

// Affect the canvas on click:
void smudge_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                    int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void smudge_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (smudge_snd != NULL)
    Mix_FreeChunk(smudge_snd);
}

// Record the color from Tux Paint:
void smudge_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  smudge_r = r;
  smudge_g = g;
  smudge_b = b;
}

// Use colors:
int smudge_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  if (which == 0)
    return 0;
  else                          /* if (which == 1) */
    return 1;
}

void smudge_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void smudge_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int smudge_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}
