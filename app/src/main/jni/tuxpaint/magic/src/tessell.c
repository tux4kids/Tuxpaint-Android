/*
  tessell.c

  Repeats what you draw in a tessellating pattern around
  the canvas.

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

  Last updated: October 9, 2024
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#define REPEAT_CNT 3
#define SIN_60DEG 0.866025403784439

enum
{
  TOOL_TESSELL_POINTY_TOP,
  TOOL_TESSELL_FLAT_TOP,
  NUM_TOOLS
};

static Mix_Chunk *tessell_snd;
static int tessell_radius = 16, tessell_width, tessell_height;
static Uint32 tessell_color;

Uint32 tessell_api_version(void);
int tessell_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int tessell_get_tool_count(magic_api * api);
SDL_Surface *tessell_get_icon(magic_api * api, int which);
char *tessell_get_name(magic_api * api, int which);
int tessell_get_group(magic_api * api, int which);
int tessell_get_order(int which);
char *tessell_get_description(magic_api * api, int which, int mode);

void tessell_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void tessell_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void tessell_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void tessell_shutdown(magic_api * api);
void tessell_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int tessell_requires_colors(magic_api * api, int which);
void tessell_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void tessell_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int tessell_modes(magic_api * api, int which);
Uint8 tessell_accepted_sizes(magic_api * api, int which, int mode);
Uint8 tessell_default_size(magic_api * api, int which, int mode);
void tessell_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                      SDL_Rect * update_rect);


Uint32 tessell_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int tessell_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/tessellation.ogg", api->data_directory);
  tessell_snd = Mix_LoadWAV(fname);

  return (1);
}

int tessell_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

SDL_Surface *tessell_get_icon(magic_api *api, int which)
{
  char fname[1024];

  if (which == TOOL_TESSELL_POINTY_TOP)
    snprintf(fname, sizeof(fname), "%simages/magic/tessellation-pointy.png", api->data_directory);
  else                          // which == TOOL_TESSELL_FLAT_TOP
    snprintf(fname, sizeof(fname), "%simages/magic/tessellation-flat.png", api->data_directory);

  return (IMG_Load(fname));
}

char *tessell_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  if (which == TOOL_TESSELL_POINTY_TOP)
    return (strdup(gettext("Tessellation Pointy")));
  else                          // which == TOOL_TESSELL_TOP_TOP
    return (strdup(gettext("Tessellation Flat")));
}

int tessell_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PATTERN_PAINTING;
}

int tessell_get_order(int which)
{
  return 300 + which;
}

char *tessell_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  if (which == TOOL_TESSELL_POINTY_TOP)
    return (strdup(gettext("Click and drag to draw a repeating tessellating pattern of pointy-topped hexagons.")));
  else                          // which == TOOL_TESSELL_FLAT_TOP
    return (strdup(gettext("Click and drag to draw a repeating tessellating pattern of flat-topped hexagons.")));
}

static void do_tessell_circle(void *ptr, int which,
                              SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y)
{
  int xx, yy, rx, ry, sx, sy;
  magic_api *api = (magic_api *) ptr;

  for (yy = -tessell_radius; yy <= tessell_radius; yy++)
  {
    for (xx = -tessell_radius; xx <= tessell_radius; xx++)
    {
      if (api->in_circle(xx, yy, tessell_radius))
      {
        for (ry = -REPEAT_CNT; ry <= REPEAT_CNT; ry++)
        {
          for (rx = -REPEAT_CNT; rx <= REPEAT_CNT; rx++)
          {
            if (which == TOOL_TESSELL_POINTY_TOP)
            {
              sx = rx * tessell_width;
              if (abs(ry) % 2 == 1)
                sx += tessell_width / 2;
              sy = ry * tessell_height;
            }
            else                // which == TOOL_TESSELL_FLAT_TOP
            {
              sx = rx * tessell_width;
              sy = ry * tessell_height;
              if (abs(rx) % 2 == 1)
                sy += tessell_height / 2;
            }

            api->putpixel(canvas, x + xx + sx, y + yy + sy, tessell_color);
          }
        }
      }
    }
  }
}

void tessell_drag(magic_api *api, int which, SDL_Surface *canvas,
                  SDL_Surface *last, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_tessell_circle);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(tessell_snd, (x * 255) / canvas->w, 255);
}

void tessell_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  tessell_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void tessell_release(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED,
                     SDL_Surface *last ATTRIBUTE_UNUSED, int x ATTRIBUTE_UNUSED,
                     int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
}

void tessell_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (tessell_snd != NULL)
    Mix_FreeChunk(tessell_snd);
}

void tessell_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                       Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  tessell_color = SDL_MapRGB(canvas->format, r, g, b);
}

int tessell_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void tessell_switchin(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
  int tessell_mult;

  if (canvas->w > canvas->h)
    tessell_mult = canvas->w / (REPEAT_CNT + 1);
  else
    tessell_mult = canvas->h / (REPEAT_CNT + 1);

  if (which == TOOL_TESSELL_POINTY_TOP)
  {
    tessell_width = tessell_mult;
    tessell_height = tessell_mult * SIN_60DEG;
  }
  else                          // which == TOOL_TESSELL_FLAT_TOP
  {
    tessell_width = tessell_mult * SIN_60DEG;
    tessell_height = tessell_mult;
  }
}

void tessell_switchout(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int tessell_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 tessell_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 8;
}

Uint8 tessell_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 2;
}

void tessell_set_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED,
                      Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  tessell_radius = size;
}
