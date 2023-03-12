/*
  fade_darken.c

  Fade and Darken Magic Tools Plugin
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

  Last updated: March 7, 2023
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
  TOOL_DESATURATE,
  TOOL_SATURATE,
  TOOL_REMOVE,
  TOOL_KEEP,
  NUM_TOOLS
};

char * tool_names[NUM_TOOLS] = {
  gettext_noop("Lighten"),
  gettext_noop("Darken"),
  gettext_noop("Desaturate"),
  gettext_noop("Saturate"),
  gettext_noop("Remove Color"),
  gettext_noop("Keep Color"),
};

char * tool_descriptions[NUM_TOOLS][2] = {
  {
   gettext_noop("Click and drag the mouse to lighten parts of your picture."),
   gettext_noop("Click to lighten your entire picture.")
  },
  {
   gettext_noop("Click and drag the mouse to darken parts of your picture."),
   gettext_noop("Click to darken your entire picture.")
  },
  {
   gettext_noop("Click and drag the mouse to desaturate parts of your picture."),
   gettext_noop("Click to desaturate your entire picture.")
  },
  {
   gettext_noop("Click and drag the mouse to saturate parts of your picture."),
   gettext_noop("Click to saturate your entire picture.")
  },
  {
   gettext_noop("Click and drag the mouse to entirely desaturate parts of your picture that match the chosen color."),
   gettext_noop("Click to entirely desaturate your the parts of your picture that match the chosen color."),
  },
  {
   gettext_noop("Click and drag the mouse to entirely desaturate parts of your picture that don't match the chosen color."),
   gettext_noop("Click to entirely desaturate your the parts of your picture that don't match the chosen color."),
  },
};

char * sfx_filenames[NUM_TOOLS] = {
  "fade.wav",
  "darken.wav",
  "desaturate.ogg",
  "saturate.ogg",
  "remove_color.ogg",
  "keep_color.ogg",
};

char * icon_filenames[NUM_TOOLS] = {
  "fade.png",
  "darken.png",
  "desaturate.png",
  "saturate.png",
  "remove_color.png",
  "keep_color.png",
};

static Mix_Chunk *snd_effects[NUM_TOOLS];
float chosen_h, chosen_s;

#define KEEP_REMOVE_HUE_THRESH 30.0
// #define KEEP_REMOVE_VALUE_THRESH 0.4

#define SAT_DESAT_RATIO_NUM 3
#define SAT_DESAT_RATIO_DENOM 4


/* Local function prototypes: */

int fade_darken_init(magic_api * api);
Uint32 fade_darken_api_version(void);
int fade_darken_get_tool_count(magic_api * api);
SDL_Surface *fade_darken_get_icon(magic_api * api, int which);
int fade_darken_get_group(magic_api * api, int which);
char *fade_darken_get_name(magic_api * api, int which);
char *fade_darken_get_description(magic_api * api, int which, int mode);
static void do_fade_darken(void *ptr, int which, SDL_Surface * canvas,
                           SDL_Surface * last, int x, int y);
static void do_fade_darken_paint(void *ptr, int which, SDL_Surface * canvas,
                                 SDL_Surface * last, int x, int y);
void fade_darken_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y,
                      SDL_Rect * update_rect);
void fade_darken_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y,
                       SDL_Rect * update_rect);
void fade_darken_release(magic_api * api, int which, SDL_Surface * canvas,
                         SDL_Surface * last, int x, int y,
                         SDL_Rect * update_rect);
void fade_darken_shutdown(magic_api * api);
void fade_darken_set_color(magic_api * api, int which, SDL_Surface * canvas,
                           SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int fade_darken_requires_colors(magic_api * api, int which);
void fade_darken_switchin(magic_api * api, int which, int mode,
                          SDL_Surface * canvas);
void fade_darken_switchout(magic_api * api, int which, int mode,
                           SDL_Surface * canvas);
int fade_darken_modes(magic_api * api, int which);


int fade_darken_init(magic_api * api)
{
  int i;
  char fname[1024];

  for (i = 0; i < NUM_TOOLS; i++) {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s",
             api->data_directory, sfx_filenames[i]);
    snd_effects[i] = Mix_LoadWAV(fname);
  }

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

  snprintf(fname, sizeof(fname), "%simages/magic/%s",
           api->data_directory, icon_filenames[which]);

  return (IMG_Load(fname));
}

// Return our name, localized:
char *fade_darken_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return strdup(gettext(tool_names[which]));
}

// Return our group (all the same):
int fade_darken_get_group(magic_api * api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}

// Return our description, localized:
char *fade_darken_get_description(magic_api * api ATTRIBUTE_UNUSED, int which,
                                  int mode)
{
  return strdup(gettext(tool_descriptions[which][mode - 1]));
}

static void do_fade_darken(void *ptr, int which, SDL_Surface * canvas,
                           SDL_Surface * last, int x, int y)
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
  else
  {
    float h, s, v;

    api->rgbtohsv(r, g, b, &h, &s, &v);

    if (which == TOOL_DESATURATE) {
      s = (s * SAT_DESAT_RATIO_NUM) / SAT_DESAT_RATIO_DENOM;
    } else if (which == TOOL_SATURATE) {
      if (s > 0.1) { /* don't saturate things w/o undefined color! */
        s = (s * SAT_DESAT_RATIO_DENOM) / SAT_DESAT_RATIO_NUM;
        if (s > 1.0) {
          s = 1.0;
        }
      }
    } else if (which == TOOL_REMOVE) {
      if (fabs(h - chosen_h) <= KEEP_REMOVE_HUE_THRESH
 /* && fabs(s - chosen_s) <= KEEP_REMOVE_VALUE_THRESH */
) {
        s = 0.0;
      }
    } else if (which == TOOL_KEEP) {
      if (fabs(h - chosen_h) > KEEP_REMOVE_HUE_THRESH
 /* || fabs(s - chosen_s) > KEEP_REMOVE_VALUE_THRESH */
) {
        s = 0.0;
      }
    }

    api->hsvtorgb(h, s, v, &r, &g, &b);
  }

  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, r, g, b));
}


// Callback that does the fade_darken color effect on a circle centered around x,y
static void do_fade_darken_paint(void *ptr, int which, SDL_Surface * canvas,
                                 SDL_Surface * last, int x, int y)
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
                      SDL_Surface * last, int ox, int oy, int x, int y,
                      SDL_Rect * update_rect)
{
  SDL_LockSurface(last);
  SDL_LockSurface(canvas);

  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1,
            do_fade_darken_paint);

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
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y,
                       SDL_Rect * update_rect)
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
void fade_darken_release(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED,
                         SDL_Surface * last ATTRIBUTE_UNUSED,
                         int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
                         SDL_Rect * update_rect ATTRIBUTE_UNUSED)
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

void fade_darken_set_color(magic_api * api, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
                           SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  float tmp;

  api->rgbtohsv(r, g, b, &chosen_h, &chosen_s, &tmp);
}

// We don't use colors
int fade_darken_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                                int which ATTRIBUTE_UNUSED)
{
  if (which == TOOL_REMOVE || which == TOOL_KEEP)
    return 1;

  return 0;
}

void fade_darken_switchin(magic_api * api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED,
                          int mode ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void fade_darken_switchout(magic_api * api ATTRIBUTE_UNUSED,
                           int which ATTRIBUTE_UNUSED,
                           int mode ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int fade_darken_modes(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}
