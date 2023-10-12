/*
  ribbon.c

  Ribbon Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2023-2023 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: May 23, 2023
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

#define MAX_LENGTH 256
#define NUM_LENGTH_OPTIONS 4

#define ribbon_radius 8

static int ribbon_max_length = (MAX_LENGTH / 2);
static Uint8 ribbon_r, ribbon_g, ribbon_b;
static Uint32 ribbon_segment_color;
static int ribbon_x[MAX_LENGTH], ribbon_y[MAX_LENGTH];
static int ribbon_tail = 0, ribbon_head = 0;
static double ribbon_old_angle;
static Mix_Chunk *ribbon_snd;

int ribbon_init(magic_api * api, Uint32 disabled_features);
Uint32 ribbon_api_version(void);
int ribbon_get_tool_count(magic_api * api);
SDL_Surface *ribbon_get_icon(magic_api * api, int which);
char *ribbon_get_name(magic_api * api, int which);
int ribbon_get_group(magic_api * api, int which);
char *ribbon_get_description(magic_api * api, int which, int mode);
static void ribbon_linecb(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);

void ribbon_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void ribbon_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void ribbon_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);


void ribbon_shutdown(magic_api * api);
void ribbon_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int ribbon_requires_colors(magic_api * api, int which);
void ribbon_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void ribbon_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int ribbon_modes(magic_api * api, int which);
Uint8 ribbon_accepted_sizes(magic_api * api, int which, int mode);
Uint8 ribbon_default_size(magic_api * api, int which, int mode);
void ribbon_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                      SDL_Rect * update_rect);


Uint32 ribbon_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// Load our sfx:
int ribbon_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/ribbon.ogg", api->data_directory);
  ribbon_snd = Mix_LoadWAV(fname);

  return (1);
}

// We have multiple tools:
int ribbon_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

// Load our icons:
SDL_Surface *ribbon_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/ribbon.png", api->data_directory);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *ribbon_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Ribbon")));
}

// Return our group:
int ribbon_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

// Return our descriptions, localized:
char *ribbon_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Add a flowing ribbon to your picture.")));
}

// Do the effect:

static void ribbon_linecb(void *ptr, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int i;

  for (i = - ribbon_radius; i < ribbon_radius; i++)
  {
    api->putpixel(canvas, x + i, y + i, ribbon_segment_color);
    api->putpixel(canvas, x + i, y + i + 1, ribbon_segment_color);
    api->putpixel(canvas, x + i + 1, y + i, ribbon_segment_color);
  }
}

// Affect the canvas on drag:
void ribbon_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  int pt, pt2, first_click;
  Uint8 r, g, b;
  int angle;

  SDL_BlitSurface(last, NULL, canvas, NULL);

  ribbon_x[ribbon_head] = x;
  ribbon_y[ribbon_head] = y;

  first_click = (ribbon_head == 0 && ribbon_tail == 0);

  ribbon_head = (ribbon_head + 1) % ribbon_max_length;

  if (ribbon_head == ribbon_tail)
    ribbon_tail = (ribbon_tail + 1) % ribbon_max_length;


  angle = 0.0;

  if (!first_click) {
    double x_angle;

    if (sqrt((x - ox) * (x - ox) + (y - oy) * (y - oy)) > 16)
    {
      /* Play swooshing sfx if we're moving quickly and making a big angle */
      x_angle = (fabs(atan2((double)(y - oy), (double)(x - ox))) * 2.0);
      if (fabs(x_angle - ribbon_old_angle) > (M_PI / 4.0))
        api->playsound(ribbon_snd, (x * 255) / canvas->w, 255);
      ribbon_old_angle = x_angle;
    }

    pt = ribbon_tail;
    do {
      int brt;

      pt2 = ((pt + 1) % ribbon_max_length);
      ox = ribbon_x[pt];
      oy = ribbon_y[pt];
      x = ribbon_x[pt2];
      y = ribbon_y[pt2];

      x_angle = (fabs(atan2((double)(y - oy), (double)(x - ox))) * 2.0);
      if (pt == ribbon_tail)
        angle = x_angle;
      else
        angle = (angle + x_angle) / 2.0;

      brt = ((angle - (M_PI / 4)) * 128.0) / M_PI;

      r = max(min(ribbon_r + brt, 255), 0);
      g = max(min(ribbon_g + brt, 255), 0);
      b = max(min(ribbon_b + brt, 255), 0);

      ribbon_segment_color = SDL_MapRGB(canvas->format, r, g, b);
      api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, ribbon_linecb);
      pt = pt2;
    } while (((pt + 1) % ribbon_max_length) != ribbon_head);
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

// Affect the canvas on click:
void ribbon_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  ribbon_head = ribbon_tail = ribbon_old_angle = 0;
  ribbon_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void ribbon_release(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED,
                     SDL_Surface * last ATTRIBUTE_UNUSED,
                     int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// Clean up
void ribbon_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (ribbon_snd != NULL)
    Mix_FreeChunk(ribbon_snd);
}

// Record the color from Tux Paint:
void ribbon_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       Uint8 r, Uint8 g, Uint8 b,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  ribbon_r = r;
  ribbon_g = g;
  ribbon_b = b;
}

// Use colors:
int ribbon_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void ribbon_switchin(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void ribbon_switchout(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int ribbon_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 ribbon_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return NUM_LENGTH_OPTIONS;
}

Uint8 ribbon_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (NUM_LENGTH_OPTIONS / 2);
}

void ribbon_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  ribbon_max_length = (size * MAX_LENGTH) / NUM_LENGTH_OPTIONS;
}
