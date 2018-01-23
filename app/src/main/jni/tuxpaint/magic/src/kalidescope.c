/*
  kalidescope.c

  Kaleidoscope Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

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
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk *kalidescope_snd;
static Uint8 kalidescope_r, kalidescope_g, kalidescope_b;
static int square_size = 128;


enum
{
  KAL_UD,
  KAL_LR,
  KAL_BOTH,
  KAL_PATTERN,
  KAL_TILES,
  KAL_COUNT
};

char *kal_icon_names[KAL_COUNT] = {
  "symmetric_updown.png",
  "symmetric_leftright.png",
  "kalidescope.png",
  "kal_pattern.png",
  "kal_tiles.png"
};

/* Function Declarations: */

Uint32 kalidescope_api_version(void);
int kalidescope_init(magic_api * api);
int kalidescope_get_tool_count(magic_api * api);
SDL_Surface *kalidescope_get_icon(magic_api * api, int which);
char *kalidescope_get_name(magic_api * api, int which);
char *kalidescope_get_description(magic_api * api, int which, int mode);
static void do_kalidescope(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void kalidescope_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void kalidescope_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void kalidescope_release(magic_api * api, int which,
                         SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void kalidescope_shutdown(magic_api * api);
int kalidescope_requires_colors(magic_api * api, int which);
void kalidescope_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void kalidescope_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void kalidescope_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int kalidescope_modes(magic_api * api, int which);

Uint32 kalidescope_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// No setup required:
int kalidescope_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/kaleidoscope.ogg", api->data_directory);
  kalidescope_snd = Mix_LoadWAV(fname);

  return (1);
}

int kalidescope_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (KAL_COUNT);
}

// Load our icons:
SDL_Surface *kalidescope_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/%s", api->data_directory, kal_icon_names[which]);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *kalidescope_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == KAL_LR)
    {
      return (strdup(gettext_noop("Symmetric Left/Right")));
    }
  else if (which == KAL_UD)
    {
      return (strdup(gettext_noop("Symmetric Up/Down")));
    }
  else if (which == KAL_PATTERN)
    {
      return (strdup(gettext_noop("Pattern")));
    }
  else if (which == KAL_TILES)
    {
      return (strdup(gettext_noop("Tiles")));
    }
  else
    {                           /* KAL_BOTH */
      return (strdup(gettext_noop("Kaleidoscope")));
    }
}

// Return our descriptions, localized:
char *kalidescope_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  if (which == KAL_LR)
    {
      return (strdup
              (gettext_noop
               ("Click and drag the mouse to draw with two brushes that are symmetric across the left and right of your picture.")));
    }
  else if (which == KAL_UD)
    {
      return (strdup
              (gettext_noop
               ("Click and drag the mouse to draw with two brushes that are symmetric across the top and bottom of your picture.")));
    }
  else if (which == KAL_PATTERN)
    {
      return (strdup(gettext_noop("Click and drag the mouse to draw a pattern across the picture.")));
    }
  else if (which == KAL_TILES)
    {
      return (strdup(gettext_noop("Click and drag the mouse to draw a pattern that is symmetric across the picture.")));
    }
  else
    {                           /* KAL_BOTH */
      return (strdup(gettext_noop("Click and drag the mouse to draw with symmetric brushes (a kaleidoscope).")));
    }
}

// Do the effect:

static void do_kalidescope(void *ptr, int which, SDL_Surface * canvas,
                           SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;
  int i, j;
  Uint32 colr;

  colr = SDL_MapRGB(canvas->format, kalidescope_r, kalidescope_g, kalidescope_b);

  for (yy = -8; yy < 8; yy++)
    {
      for (xx = -8; xx < 8; xx++)
        {
          if (api->in_circle(xx, yy, 8))
            {
              api->putpixel(canvas, x + xx, y + yy, colr);

              if (which == KAL_LR || which == KAL_BOTH)
                {
                  api->putpixel(canvas, canvas->w - 1 - x + xx, y + yy, colr);

                  if (which == KAL_BOTH)
                    {
                      api->putpixel(canvas, canvas->w - 1 - x + xx, canvas->h - 1 - y + yy, colr);
                    }
                }
              if (which == KAL_UD || which == KAL_BOTH)
                {
                  api->putpixel(canvas, x + xx, canvas->h - 1 - y + yy, colr);
                }
              if (which == KAL_PATTERN || which == KAL_TILES)
                {
                  for (i = 0; i <= canvas->w; i += square_size)
                    for (j = 0; j <= canvas->h; j += square_size)
                      {
                        api->putpixel(canvas, i + xx + x % square_size, j + yy + y % square_size, colr);
                        if (which == KAL_TILES)
                          api->putpixel(canvas, i + yy + y % square_size, j + xx + x % square_size, colr);
                      }
                }
            }
        }
    }
}

// Affect the canvas on drag:
void kalidescope_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_kalidescope);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(kalidescope_snd, 128, 255);
}

// Affect the canvas on click:
void kalidescope_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  kalidescope_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

// Affect the canvas on release:
void kalidescope_release(magic_api * api, int which ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                         int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  api->stopsound();
}

// No setup happened:
void kalidescope_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (kalidescope_snd != NULL)
    Mix_FreeChunk(kalidescope_snd);
}

// Record the color from Tux Paint:
void kalidescope_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  kalidescope_r = r;
  kalidescope_g = g;
  kalidescope_b = b;
}

// Use colors:
int kalidescope_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void kalidescope_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                          int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void kalidescope_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                           int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int kalidescope_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}
