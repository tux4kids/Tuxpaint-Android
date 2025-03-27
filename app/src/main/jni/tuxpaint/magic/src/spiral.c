/*
  spiral.c

  Draws a spiral shape, or a concentric shape -- circular or square --
  centered around the mouse click point.  Size setting changes thickness
  and spacing.

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

enum
{
  TOOL_SPIRAL_CIRCLE,
  TOOL_SPIRAL_SQUARE,
  TOOL_CONCENTRIC_CIRCLE,
  TOOL_CONCENTRIC_SQUARE,
  NUM_TOOLS
};

const char *spiral_names[NUM_TOOLS] = {
  gettext_noop("Spiral"),
  gettext_noop("Square Spiral"),
  gettext_noop("Concentric Circles"),
  gettext_noop("Concentric Squares"),
};

const char *spiral_descrs[NUM_TOOLS] = {
  gettext_noop("Click and drag to create a spiral."),
  gettext_noop("Click and drag to create a square spiral."),
  gettext_noop("Click and drag to create concentric circles."),
  gettext_noop("Click and drag to create concentric squares."),
};

const char *spiral_sounds[NUM_TOOLS] = {
  "spiral-circle.ogg",
  "spiral-square.ogg",
  "concentric-circle.ogg",
  "concentric-square.ogg",
};

const char *spiral_icons[NUM_TOOLS] = {
  "spiral-circle.png",
  "spiral-square.png",
  "concentric-circle.png",
  "concentric-square.png",
};

static Mix_Chunk *spiral_snd[NUM_TOOLS];
static int spiral_thickness = 2;
Uint32 spiral_color;
int spiral_cx, spiral_cy, spiral_has_dragged;

/* When first clicking, and when click-and-release-ing
   (without dragging), we'll draw a default spiral that
   is `MIN_RADIUS` * `sprial_thickness` in radius. */
#define MIN_RADIUS 32

/* Angle will be the radius multiplied by `ITER` divided
   by `spiral_thickness`; the larger the thickness, the
   fewer the iterations (the slower the spiral will go around) */
#define ITER 50

Uint32 spiral_api_version(void);
int spiral_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int spiral_get_tool_count(magic_api * api);
SDL_Surface *spiral_get_icon(magic_api * api, int which);
char *spiral_get_name(magic_api * api, int which);
int spiral_get_group(magic_api * api, int which);
int spiral_get_order(int which);
char *spiral_get_description(magic_api * api, int which, int mode);

void spiral_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void spiral_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void spiral_release(magic_api * api, int which,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void spiral_shutdown(magic_api * api);
void spiral_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int spiral_requires_colors(magic_api * api, int which);
void spiral_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void spiral_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int spiral_modes(magic_api * api, int which);
Uint8 spiral_accepted_sizes(magic_api * api, int which, int mode);
Uint8 spiral_default_size(magic_api * api, int which, int mode);
void spiral_set_size(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);

void do_spiral(magic_api * api, int which, SDL_Surface * canvas,
               SDL_Surface * last, int x, int y, SDL_Rect * update_rect, int final);

Uint32 spiral_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int spiral_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
  {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s", api->data_directory, spiral_sounds[i]);
    spiral_snd[i] = Mix_LoadWAV(fname);
  }

  return (1);
}

int spiral_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

SDL_Surface *spiral_get_icon(magic_api *api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, spiral_icons[which]);

  return (IMG_Load(fname));
}

char *spiral_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext(spiral_names[which])));
}

int spiral_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

int spiral_get_order(int which)
{
  return 1310 + which;
}

char *spiral_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext(spiral_descrs[which])));
}

static void do_spiral_render(void *ptr, int which,
                             SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  SDL_Rect dest;
  int thick;

  thick = spiral_thickness * 4;

  if (which == TOOL_SPIRAL_CIRCLE || which == TOOL_CONCENTRIC_CIRCLE)
  {
    int xx, yy;

    for (yy = -thick / 2; yy <= thick / 2; yy++)
      for (xx = -thick / 2; xx <= thick / 2; xx++)
        if (api->in_circle(xx, yy, thick / 2))
          api->putpixel(canvas, x + xx, y + yy, spiral_color);
  }
  else if (which == TOOL_SPIRAL_SQUARE || which == TOOL_CONCENTRIC_SQUARE)
  {
    dest.x = x - thick / 2;
    dest.y = y - thick / 2;
    dest.w = thick;
    dest.h = thick;

    SDL_FillRect(canvas, &dest, spiral_color);
  }
}

void do_spiral(magic_api *api, int which, SDL_Surface *canvas,
               SDL_Surface *last, int x, int y, SDL_Rect *update_rect, int final)
{
  float radius = 0, i, xx, yy, xsgn, ysgn, stp, oxx, oyy;
  int vol;

  SDL_BlitSurface(last, NULL, canvas, NULL);

  if (x < spiral_cx)
    xsgn = -1;
  else
    xsgn = 1;

  if (y < spiral_cy)
    ysgn = -1;
  else
    ysgn = 1;

  if (which == TOOL_SPIRAL_CIRCLE)
  {
    if (final)
      stp = 0.1;
    else
      stp = 0.5;

    radius = sqrt(pow((x - spiral_cx), 2) + pow((y - spiral_cy), 2));

    oxx = 0;
    oyy = 0;

    for (i = 0; i < radius; i += stp)
    {
      xx = (i * cos((i * (ITER / spiral_thickness)) / 180.0 * M_PI)) * xsgn;
      yy = (i * sin((i * (ITER / spiral_thickness)) / 180.0 * M_PI)) * ysgn;
      if (final)
      {
        api->line((void *)api, which, canvas, NULL,
                  ((int)oxx) + spiral_cx, ((int)oyy) + spiral_cy,
                  ((int)xx) + spiral_cx, ((int)yy) + spiral_cy, 1, do_spiral_render);
        oxx = xx;
        oyy = yy;
      }
      else
      {
        do_spiral_render(api, which, canvas, NULL, ((int)xx) + spiral_cx, ((int)yy) + spiral_cy);
      }
    }
  }
  else if (which == TOOL_SPIRAL_SQUARE)
  {
    int dir, oi, ooi;

    radius = max(abs(x - spiral_cx), abs(y - spiral_cy));

    dir = 0;
    ooi = 0;
    oi = 0;
    for (i = spiral_thickness; i < radius; i += spiral_thickness * 2)
    {
      if (dir == 0)             // right
        api->line((void *)api, which, canvas, NULL,
                  spiral_cx - (ooi * xsgn), spiral_cy - (oi * ysgn),
                  spiral_cx + (i * xsgn), spiral_cy - (oi * ysgn), 1, do_spiral_render);
      else if (dir == 1)        // down
        api->line((void *)api, which, canvas, NULL,
                  spiral_cx + (oi * xsgn), spiral_cy - (ooi * ysgn),
                  spiral_cx + (oi * xsgn), spiral_cy + (i * ysgn), 1, do_spiral_render);
      else if (dir == 2)        // left
        api->line((void *)api, which, canvas, NULL,
                  spiral_cx + (ooi * xsgn), spiral_cy + (oi * ysgn),
                  spiral_cx - (i * xsgn), spiral_cy + (oi * ysgn), 1, do_spiral_render);
      else if (dir == 3)        // up
        api->line((void *)api, which, canvas, NULL,
                  spiral_cx - (oi * xsgn), spiral_cy + (ooi * ysgn),
                  spiral_cx - (oi * xsgn), spiral_cy - (i * ysgn), 1, do_spiral_render);

      dir = (dir + 1) % 4;

      ooi = oi;
      oi = i;
    }
  }
  else if (which == TOOL_CONCENTRIC_CIRCLE)
  {
    float ang, ang_stp;

    radius = sqrt(pow((x - spiral_cx), 2) + pow((y - spiral_cy), 2));

    if (final)
      ang_stp = 1;
    else
      ang_stp = 5;

    for (i = 0; i < radius; i += spiral_thickness * 8)
    {
      oxx = (i * cos(0));
      oyy = (i * sin(0));
      for (ang = 0; ang < 360; ang += ang_stp)
      {
        xx = (i * cos(ang / 180.0 * M_PI));
        yy = (i * sin(ang / 180.0 * M_PI));
        if (final)
        {
          api->line((void *)api, which, canvas, NULL,
                    ((int)oxx) + spiral_cx, ((int)oyy) + spiral_cy,
                    ((int)xx) + spiral_cx, ((int)yy) + spiral_cy, 1, do_spiral_render);
          oxx = xx;
          oyy = yy;
        }
        else
        {
          do_spiral_render(api, which, canvas, NULL, ((int)xx) + spiral_cx, ((int)yy) + spiral_cy);
        }
      }
    }
  }
  else if (which == TOOL_CONCENTRIC_SQUARE)
  {
    radius = max(abs(x - spiral_cx), abs(y - spiral_cy));

    for (i = spiral_thickness; i < radius; i += spiral_thickness * 8)
    {
      api->line((void *)api, which, canvas, NULL,
                spiral_cx - i, spiral_cy - i, spiral_cx + i, spiral_cy - i, 1, do_spiral_render);
      api->line((void *)api, which, canvas, NULL, spiral_cx + i,
                spiral_cy - i, spiral_cx + i, spiral_cy + i, 1, do_spiral_render);
      api->line((void *)api, which, canvas, NULL, spiral_cx + i,
                spiral_cy + i, spiral_cx - i, spiral_cy + i, 1, do_spiral_render);
      api->line((void *)api, which, canvas, NULL, spiral_cx - i,
                spiral_cy + i, spiral_cx - i, spiral_cy - i, 1, do_spiral_render);
    }
  }

  /* FIXME */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  vol = (radius * 255) / max(canvas->w, canvas->h);
  if (vol > 255)
    vol = 255;
  else if (vol < 32)
    vol = 32;

  api->playsound(spiral_snd[which], (spiral_cx * 255) / canvas->w, vol);
}

void spiral_drag(magic_api *api, int which, SDL_Surface *canvas,
                 SDL_Surface *last, int ox ATTRIBUTE_UNUSED,
                 int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  spiral_has_dragged = 1;
  do_spiral(api, which, canvas, last, x, y, update_rect, 0);
}

void spiral_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  spiral_cx = x;
  spiral_cy = y;

  api->stopsound();
  spiral_drag(api, which, canvas, last, x, y, x + (spiral_thickness * MIN_RADIUS), y, update_rect);

  spiral_has_dragged = 0;
}

void spiral_release(magic_api *api, int which,
                    SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  float radius;

  radius = sqrt(pow((x - spiral_cx), 2) + pow((y - spiral_cy), 2));
  if (radius < spiral_thickness * MIN_RADIUS && spiral_has_dragged == 0)
    x = spiral_cx + (spiral_thickness * MIN_RADIUS);

  do_spiral(api, which, canvas, last, x, y, update_rect, 1);
}

void spiral_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
    if (spiral_snd[i] != NULL)
      Mix_FreeChunk(spiral_snd[i]);
}

void spiral_set_color(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                      SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                      Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  spiral_color = SDL_MapRGB(canvas->format, r, g, b);
}

int spiral_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void spiral_switchin(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

void spiral_switchout(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int spiral_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 spiral_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 8;
}

Uint8 spiral_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void spiral_set_size(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED,
                     SDL_Surface *last ATTRIBUTE_UNUSED,
                     Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  spiral_thickness = size;
}
