/*
  negative.c

  Negative Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2023 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: February 12, 2023
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

static Mix_Chunk *negative_snd;
static int negative_radius = 16;

int negative_init(magic_api * api, Uint32 disabled_features);
Uint32 negative_api_version(void);
int negative_get_tool_count(magic_api * api);
SDL_Surface *negative_get_icon(magic_api * api, int which);
char *negative_get_name(magic_api * api, int which);
int negative_get_group(magic_api * api, int which);
char *negative_get_description(magic_api * api, int which, int mode);
static void do_negative(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void negative_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void negative_click(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void negative_release(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void negative_shutdown(magic_api * api);
void negative_set_color(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int negative_requires_colors(magic_api * api, int which);
void negative_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void negative_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int negative_modes(magic_api * api, int which);
Uint8 negative_accepted_sizes(magic_api * api, int which, int mode);
Uint8 negative_default_size(magic_api * api, int which, int mode);
void negative_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                       SDL_Rect * update_rect);

enum
{
  TOOL_NEGATIVE,
  TOOL_COMPLEMENTARY,
  negative_NUM_TOOLS
};

const char *negative_icon_filenames[negative_NUM_TOOLS] = {
  "negative.png",
  "opposite.png"
};

const char *negative_names[negative_NUM_TOOLS] = {
  gettext_noop("Negative"),
  gettext_noop("Opposite")
};

const char *negative_descs[negative_NUM_TOOLS][2] = {
  {
   gettext_noop("Click and drag the mouse around to make your painting negative."),
   gettext_noop("Click to turn your painting into its negative.")},
  {
   gettext_noop("Click and drag the mouse around to change colors to their opposites -- their complementary colors."),
   gettext_noop("Click to turn all colors in your painting into their opposites -- their complementary colors.")},
};


int negative_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/negative.wav", api->data_directory);

  negative_snd = Mix_LoadWAV(fname);

  return (1);
}

Uint32 negative_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int negative_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (negative_NUM_TOOLS);
}

// Load our icon:
SDL_Surface *negative_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, negative_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our name, localized:
char *negative_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(negative_names[which])));
}

// Return our group (both the same):
int negative_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}

// Return our description, localized:
char *negative_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  int mode_idx;

  if (mode == MODE_PAINT)
  {
    mode_idx = 0;
  }
  else if (mode == MODE_FULLSCREEN)
  {
    mode_idx = 1;
  }
  else
  {
    return NULL;
  }

  return (strdup(gettext_noop(negative_descs[which][mode_idx])));
}

static void negative_calc(void *ptr, int which, Uint8 r, Uint8 g, Uint8 b, Uint8 * new_r, Uint8 * new_g, Uint8 * new_b)
{
  float h, s, v, new_h;
  magic_api *api = (magic_api *) ptr;

  if (which == TOOL_NEGATIVE)
  {
    *new_r = 0xFF - r;
    *new_g = 0xFF - g;
    *new_b = 0xFF - b;
  }
  else
  {
    api->rgbtohsv(r, g, b, &h, &s, &v);
    new_h = h + 180.0;
    if (new_h >= 360.0)
    {
      new_h = new_h - 360.0;
    }
    api->hsvtorgb(new_h, s, v, new_r, new_g, new_b);
  }
}

// Callback that does the negative color effect on a circle centered around x,y
static void do_negative(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  Uint8 r, g, b, new_r, new_g, new_b;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - negative_radius; yy < y + negative_radius; yy++)
  {
    for (xx = x - negative_radius; xx < x + negative_radius; xx++)
    {
      if (api->in_circle(xx - x, yy - y, negative_radius))
      {
        SDL_GetRGB(api->getpixel(last, xx, yy), last->format, &r, &g, &b);
        negative_calc(api, which, r, g, b, &new_r, &new_g, &new_b);
        api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, new_r, new_g, new_b));
      }
    }
  }
}

// Ask Tux Paint to call our 'do_negative()' callback over a line
void negative_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  SDL_LockSurface(last);
  SDL_LockSurface(canvas);

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_negative);

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

  update_rect->x = ox - negative_radius;
  update_rect->y = oy - negative_radius;
  update_rect->w = (x + negative_radius) - update_rect->x;
  update_rect->h = (y + negative_radius) - update_rect->y;

  api->playsound(negative_snd, (x * 255) / canvas->w, 255);


  SDL_UnlockSurface(canvas);
  SDL_UnlockSurface(last);
}

// Ask Tux Paint to call our 'do_negative()' callback at a single point
void negative_click(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    negative_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
  {
    int xx, yy;
    Uint8 r, g, b, new_r, new_g, new_b;

    for (yy = 0; yy < canvas->h; yy++)
    {
      for (xx = 0; xx < canvas->w; xx++)
      {
        SDL_GetRGB(api->getpixel(last, xx, yy), last->format, &r, &g, &b);
        negative_calc(api, which, r, g, b, &new_r, &new_g, &new_b);
        api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, new_r, new_g, new_b));
      }
    }

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;

    api->playsound(negative_snd, (x * 255) / canvas->w, 255);
  }
}


void negative_release(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED,
                      SDL_Surface * last ATTRIBUTE_UNUSED,
                      int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void negative_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (negative_snd != NULL)
    Mix_FreeChunk(negative_snd);
}

// We don't use colors
void negative_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                        Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                        SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// We don't use colors
int negative_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void negative_switchin(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void negative_switchout(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int negative_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 negative_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 8;
  else
    return 0;
}

Uint8 negative_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void negative_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  negative_radius = size * 4;
}
