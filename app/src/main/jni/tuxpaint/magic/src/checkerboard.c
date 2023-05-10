/*
  checkeroard.c

  "Checkerboard" Magic Tools Plugin
  Tux Paint - A simple drawing program for children.

  By Bill Kendrick
  Based on `blind.c` by Pere Pujal Carabantes

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

  Last updated: April 18, 2023
*/

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

static Uint8 checkerboard_r, checkerboard_g, checkerboard_b;
int checkerboard_start_x, checkerboard_start_y;

Mix_Chunk *checkerboard_snd;

// Prototypes
Uint32 checkerboard_api_version(void);
void checkerboard_set_color(magic_api * api, int which, SDL_Surface * canvas,
                            SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int checkerboard_init(magic_api * api, Uint32 disabled_features);
int checkerboard_get_tool_count(magic_api * api);
SDL_Surface *checkerboard_get_icon(magic_api * api, int which);
char *checkerboard_get_name(magic_api * api, int which);
int checkerboard_get_group(magic_api * api, int which);
char *checkerboard_get_description(magic_api * api, int which, int mode);
int checkerboard_requires_colors(magic_api * api, int which);
void checkerboard_release(magic_api * api, int which,
                          SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void checkerboard_shutdown(magic_api * api);
void checkerboard_paint_checkerboard(void *ptr_to_api, int which_tool,
                                     SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void checkerboard_drag(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void checkerboard_click(magic_api * api, int which, int mode,
                        SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void checkerboard_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void checkerboard_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int checkerboard_modes(magic_api * api, int which);
Uint8 checkerboard_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                                  int mode ATTRIBUTE_UNUSED);
Uint8 checkerboard_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                                int mode ATTRIBUTE_UNUSED);
void checkerboard_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                           Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED);

//                              Housekeeping functions

Uint32 checkerboard_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

void checkerboard_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                            SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r,
                            Uint8 g, Uint8 b, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  //get the colors from API and store it in structure
  checkerboard_r = r;
  checkerboard_g = g;
  checkerboard_b = b;
}

int checkerboard_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/checkerboard.ogg", api->data_directory);
  checkerboard_snd = Mix_LoadWAV(fname);

  return (1);
}

int checkerboard_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

SDL_Surface *checkerboard_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/checkerboard.png", api->data_directory);

  return (IMG_Load(fname));
}

char *checkerboard_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Checkerboard"));
}

int checkerboard_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PICTURE_DECORATIONS;
}

char *checkerboard_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                   int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Click and drag to fill the canvas with a checkerboard pattern."));
}

int checkerboard_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void checkerboard_release(magic_api * api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED,
                          SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                          int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void checkerboard_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Mix_FreeChunk(checkerboard_snd);
}

// Interactivity functions

void checkerboard_drag(magic_api * api, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas, SDL_Surface * snapshot,
                       int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  int sz, xx, yy;
  Uint8 draw_start, draw_row, draw_cell;
  Uint32 colr;
  SDL_Rect dest;

  SDL_BlitSurface(snapshot, NULL, canvas, NULL);

  sz = max(10, max(abs(x - checkerboard_start_x), abs(y - checkerboard_start_y)));

  colr = SDL_MapRGB(canvas->format, checkerboard_r, checkerboard_g, checkerboard_b);

  draw_start = 1;
  if (x < checkerboard_start_x)
    draw_start = !draw_start;
  if (y < checkerboard_start_y)
    draw_start = !draw_start;

  /* From the mouse Y position down... */
  draw_row = draw_start;
  for (yy = checkerboard_start_y; yy <= canvas->h; yy += sz)
  {
    /* From the mouse X position right... */
    draw_cell = draw_row;
    for (xx = checkerboard_start_x; xx <= canvas->w; xx += sz)
    {
      if (draw_cell)
      {
        dest.x = xx;
        dest.y = yy;
        dest.w = sz;
        dest.h = sz;
        SDL_FillRect(canvas, &dest, colr);
      }
      draw_cell = !draw_cell;
    }

    /* From the mouse X position left... */
    draw_cell = !draw_row;
    for (xx = checkerboard_start_x - sz; xx > -sz; xx -= sz)
    {
      if (draw_cell)
      {
        dest.x = xx;
        dest.y = yy;
        dest.w = sz;
        dest.h = sz;
        SDL_FillRect(canvas, &dest, colr);
      }
      draw_cell = !draw_cell;
    }

    draw_row = !draw_row;
  }

  /* From the mouse Y position up... */
  draw_row = !draw_start;
  for (yy = checkerboard_start_y - sz; yy > -sz; yy -= sz)
  {
    /* From the mouse X position right... */
    draw_cell = draw_row;
    for (xx = checkerboard_start_x; xx <= canvas->w; xx += sz)
    {
      if (draw_cell)
      {
        dest.x = xx;
        dest.y = yy;
        dest.w = sz;
        dest.h = sz;
        SDL_FillRect(canvas, &dest, colr);
      }
      draw_cell = !draw_cell;
    }

    /* From the mouse X position left... */
    draw_cell = !draw_row;
    for (xx = checkerboard_start_x - sz; xx > -sz; xx -= sz)
    {
      if (draw_cell)
      {
        dest.x = xx;
        dest.y = yy;
        dest.w = sz;
        dest.h = sz;
        SDL_FillRect(canvas, &dest, colr);
      }
      draw_cell = !draw_cell;
    }

    draw_row = !draw_row;
  }

  /* Always applies to the whole screen! */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(checkerboard_snd, 128, 255);
}

void checkerboard_click(magic_api * api, int which ATTRIBUTE_UNUSED,
                        int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas,
                        SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  checkerboard_start_x = x;
  checkerboard_start_y = y;
  checkerboard_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void checkerboard_switchin(magic_api * api ATTRIBUTE_UNUSED,
                           int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void checkerboard_switchout(magic_api * api ATTRIBUTE_UNUSED,
                            int which ATTRIBUTE_UNUSED,
                            int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int checkerboard_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}

Uint8 checkerboard_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                                  int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

Uint8 checkerboard_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

void checkerboard_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                           Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}
