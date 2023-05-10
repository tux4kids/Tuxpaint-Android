/*
  stretch.c

  "Stretch" Magic Tools Plugin
  Tux Paint - A simple drawing program for children.

  By Bill Kendrick
  Some parts based on "Blind" Magic Tool by Pere Pujal Carabantes

  Copyright (c) 2021-2023
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

  Last updated: April 19, 2023
*/

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

int stretch_side, stretch_start_x, stretch_start_y;

enum stretch_sides
{
  STRETCH_DIRECTION_VERT,
  STRETCH_DIRECTION_HORIZ
};

enum stretch_tools
{
  STRETCH_TOOL_STRETCH,
  STRETCH_NUMTOOLS
};

Mix_Chunk *stretch_snd;

// Prototypes
Uint32 stretch_api_version(void);
void stretch_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int stretch_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED);
int stretch_get_tool_count(magic_api * api);
SDL_Surface *stretch_get_icon(magic_api * api, int which);
char *stretch_get_name(magic_api * api, int which);
int stretch_get_group(magic_api * api, int which);
char *stretch_get_description(magic_api * api, int which, int mode);
int stretch_requires_colors(magic_api * api, int which);
void stretch_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void stretch_shutdown(magic_api * api);
void stretch_paint_stretch(void *ptr_to_api, int which_tool,
                           SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void stretch_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void stretch_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                   SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void stretch_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void stretch_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int stretch_modes(magic_api * api, int which);
Uint8 stretch_accepted_sizes(magic_api * api, int which, int mode);
Uint8 stretch_default_size(magic_api * api, int which, int mode);
void stretch_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                      SDL_Rect * update_rect);

// Housekeeping functions
Uint32 stretch_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

void stretch_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

int stretch_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/stretch.ogg", api->data_directory);
  stretch_snd = Mix_LoadWAV(fname);

  return (1);
}

int stretch_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return STRETCH_NUMTOOLS;
}

SDL_Surface *stretch_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/stretch.png", api->data_directory);

  return (IMG_Load(fname));
}

char *stretch_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Stretch"));
}

int stretch_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PICTURE_WARPS;
}

char *stretch_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Click and drag to stretch part of your picture vertically or horizontally."));
}

int stretch_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void stretch_release(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED,
                     SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                     int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void stretch_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Mix_FreeChunk(stretch_snd);
}

// Interactivity functions

void stretch_drag(magic_api * api, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * snapshot,
                  int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  SDL_Rect src, dest;
  float xx, yy;
  float divisor1, divisor2;

  SDL_BlitSurface(snapshot, NULL, canvas, NULL);

  switch (stretch_side)
  {
  case STRETCH_DIRECTION_VERT:
    {
      if (y != stretch_start_y)
      {
        divisor1 = (float)y / (float)stretch_start_y;
        divisor2 = (float)(canvas->h - y) / (float)(canvas->h - stretch_start_y);

        for (yy = 0; yy < y; yy++)
        {
          src.x = 0;
          src.y = (yy / divisor1);
          src.w = canvas->w;
          src.h = 1;

          dest.x = 0;
          dest.y = yy;
          dest.w = canvas->w;
          dest.h = 1;

          SDL_BlitSurface(snapshot, &src, canvas, &dest);
        }

        for (yy = y; yy < canvas->h; yy++)
        {
          src.x = 0;
          src.y = stretch_start_y + ((yy - y) / divisor2);
          src.w = canvas->w;
          src.h = 1;

          dest.x = 0;
          dest.y = yy;
          dest.w = canvas->w;
          dest.h = 1;

          SDL_BlitSurface(snapshot, &src, canvas, &dest);
        }

        api->playsound(stretch_snd, 128, 255);
      }
      break;
    }

  case STRETCH_DIRECTION_HORIZ:
    {
      if (x != stretch_start_x)
      {
        divisor1 = (float)x / (float)stretch_start_x;
        divisor2 = (float)(canvas->w - x) / (float)(canvas->w - stretch_start_x);

        for (xx = 0; xx < x; xx++)
        {
          src.x = (xx / divisor1);
          src.y = 0;
          src.w = 1;
          src.h = canvas->h;

          dest.x = xx;
          dest.y = 0;
          dest.w = 1;
          dest.h = canvas->h;

          SDL_BlitSurface(snapshot, &src, canvas, &dest);
        }

        for (xx = x; xx < canvas->w; xx++)
        {
          src.x = stretch_start_x + ((xx - x) / divisor2);
          src.y = 0;
          src.w = 1;
          src.h = canvas->h;

          dest.x = xx;
          dest.y = 0;
          dest.w = 1;
          dest.h = canvas->h;

          SDL_BlitSurface(snapshot, &src, canvas, &dest);
        }
        api->playsound(stretch_snd, (x * 255) / canvas->w, 255);
      }

      break;
    }
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void stretch_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (y < canvas->h / 2)
  {
    if (x < y)
      stretch_side = STRETCH_DIRECTION_HORIZ;
    else if (canvas->w - x < y)
      stretch_side = STRETCH_DIRECTION_HORIZ;
    else
      stretch_side = STRETCH_DIRECTION_VERT;
  }
  else
  {
    if (x < canvas->h - y)
      stretch_side = STRETCH_DIRECTION_HORIZ;
    else if (canvas->w - x < canvas->h - y)
      stretch_side = STRETCH_DIRECTION_HORIZ;
    else
      stretch_side = STRETCH_DIRECTION_VERT;
  }

  stretch_start_x = x;
  stretch_start_y = y;

  stretch_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void stretch_switchin(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{

}

void stretch_switchout(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{

}

int stretch_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 stretch_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

Uint8 stretch_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

void stretch_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                      Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}
