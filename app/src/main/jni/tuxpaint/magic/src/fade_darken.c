/*
  fade_darken.c

  Fade and Darken Magic Tools Plugin
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

  Last updated: July 9, 2008
  $Id$
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

enum
{
  TOOL_FADE,
  TOOL_DARKEN,
  NUM_TOOLS
};

static Mix_Chunk *snd_effects[NUM_TOOLS];


/* Local function prototypes: */

int fade_darken_init(magic_api * api);
Uint32 fade_darken_api_version(void);
int fade_darken_get_tool_count(magic_api * api);
SDL_Surface *fade_darken_get_icon(magic_api * api, int which);
char *fade_darken_get_name(magic_api * api, int which);
char *fade_darken_get_description(magic_api * api, int which, int mode);
static void do_fade_darken(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void do_fade_darken_paint(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void fade_darken_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void fade_darken_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void fade_darken_release(magic_api * api, int which,
                         SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void fade_darken_shutdown(magic_api * api);
void fade_darken_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int fade_darken_requires_colors(magic_api * api, int which);
void fade_darken_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void fade_darken_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int fade_darken_modes(magic_api * api, int which);

int fade_darken_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/fade.wav", api->data_directory);
  snd_effects[TOOL_FADE] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/sounds/magic/darken.wav", api->data_directory);
  snd_effects[TOOL_DARKEN] = Mix_LoadWAV(fname);

  return (1);
}

Uint32 fade_darken_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// Multiple tools:
int fade_darken_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

// Load our icon:
SDL_Surface *fade_darken_get_icon(magic_api * api, int which)
{
  char fname[1024];

  if (which == TOOL_FADE)
    {
      snprintf(fname, sizeof(fname), "%s/images/magic/fade.png", api->data_directory);
    }
  else if (which == TOOL_DARKEN)
    {
      snprintf(fname, sizeof(fname), "%s/images/magic/darken.png", api->data_directory);
    }

  return (IMG_Load(fname));
}

// Return our name, localized:
char *fade_darken_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == TOOL_FADE)
    return (strdup(gettext_noop("Lighten")));
  else if (which == TOOL_DARKEN)
    return (strdup(gettext_noop("Darken")));

  return (NULL);
}

// Return our description, localized:
char *fade_darken_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  if (which == TOOL_FADE)
    {
      if (mode == MODE_PAINT)
        return (strdup(gettext_noop("Click and drag the mouse to lighten parts of your picture.")));
      else if (mode == MODE_FULLSCREEN)
        return (strdup(gettext_noop("Click to lighten your entire picture.")));
    }
  else if (which == TOOL_DARKEN)
    {
      if (mode == MODE_PAINT)
        return (strdup(gettext_noop("Click and drag the mouse to darken parts of your picture.")));
      else if (mode == MODE_FULLSCREEN)
        return (strdup(gettext_noop("Click to darken your entire picture.")));
    }

  return (NULL);
}

static void do_fade_darken(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  Uint8 r, g, b;
  magic_api *api = (magic_api *) ptr;

  SDL_GetRGB(api->getpixel(last, x, y), last->format, &r, &g, &b);

  if (which == TOOL_FADE)
    {
      r = min(r + 48, 255);
      g = min(g + 48, 255);
      b = min(b + 48, 255);
    }
  else if (which == TOOL_DARKEN)
    {
      r = max(r - 48, 0);
      g = max(g - 48, 0);
      b = max(b - 48, 0);
    }

  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, r, g, b));
}

// Callback that does the fade_darken color effect on a circle centered around x,y
static void do_fade_darken_paint(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  int xx, yy;
  magic_api *api = (magic_api *) ptr;

  for (yy = y - 16; yy < y + 16; yy++)
    {
      for (xx = x - 16; xx < x + 16; xx++)
        {
          if (api->in_circle(xx - x, yy - y, 16) && !api->touched(xx, yy))
            {
              do_fade_darken(api, which, canvas, last, xx, yy);
            }
        }
    }
}

// Ask Tux Paint to call our 'do_fade_darken_paint()' callback over a line
void fade_darken_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  SDL_LockSurface(last);
  SDL_LockSurface(canvas);

  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_fade_darken_paint);

  SDL_UnlockSurface(canvas);
  SDL_UnlockSurface(last);

  api->playsound(snd_effects[which], (x * 255) / canvas->w, 255);

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

// Ask Tux Paint to call our 'do_fade_darken_paint()' callback at a single point,
// or 'do_fade_darken()' on the entire image
void fade_darken_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
    fade_darken_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
    {
      int xx, yy;

      for (yy = 0; yy < canvas->h; yy++)
        for (xx = 0; xx < canvas->w; xx++)
          do_fade_darken(api, which, canvas, last, xx, yy);

      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;

      /* FIXME: Play sfx */
    }
}

// Release
void fade_darken_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                         int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


// No setup happened:
void fade_darken_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (snd_effects[0] != NULL)
    Mix_FreeChunk(snd_effects[0]);
  if (snd_effects[1] != NULL)
    Mix_FreeChunk(snd_effects[1]);
}

// We don't use colors
void fade_darken_set_color(magic_api * api ATTRIBUTE_UNUSED,
                           Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// We don't use colors
int fade_darken_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void fade_darken_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void fade_darken_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int fade_darken_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}
