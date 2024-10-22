/*
  spraypaint.c

  A spraypaint- / airbrush-like painting tool.

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

static Mix_Chunk *spraypaint_snd_spray, *spraypaint_snd_shake;
static int spraypaint_radius = 16;
static Uint8 spraypaint_r, spraypaint_g, spraypaint_b;
static int spraypaint_cnt = 0;

Uint32 spraypaint_api_version(void);
int spraypaint_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int spraypaint_get_tool_count(magic_api * api);
SDL_Surface *spraypaint_get_icon(magic_api * api, int which);
char *spraypaint_get_name(magic_api * api, int which);
int spraypaint_get_group(magic_api * api, int which);
int spraypaint_get_order(int which);
char *spraypaint_get_description(magic_api * api, int which, int mode);

void spraypaint_drag(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void spraypaint_click(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void spraypaint_release(magic_api * api, int which,
                        SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void spraypaint_shutdown(magic_api * api);
void spraypaint_set_color(magic_api * api, int which, SDL_Surface * canvas,
                          SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int spraypaint_requires_colors(magic_api * api, int which);
void spraypaint_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void spraypaint_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int spraypaint_modes(magic_api * api, int which);
Uint8 spraypaint_accepted_sizes(magic_api * api, int which, int mode);
Uint8 spraypaint_default_size(magic_api * api, int which, int mode);
void spraypaint_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                         SDL_Rect * update_rect);


Uint32 spraypaint_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int spraypaint_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/spraypaint-spray.ogg", api->data_directory);
  spraypaint_snd_spray = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%ssounds/magic/spraypaint-shake.ogg", api->data_directory);
  spraypaint_snd_shake = Mix_LoadWAV(fname);

  return (1);
}

int spraypaint_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (1);
}

SDL_Surface *spraypaint_get_icon(magic_api *api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/spraypaint.png", api->data_directory);

  return (IMG_Load(fname));
}

char *spraypaint_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Spray Paint")));
}

int spraypaint_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

int spraypaint_get_order(int which ATTRIBUTE_UNUSED)
{
  return 800;
}

char *spraypaint_get_description(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Click and drag to add a random spray of color onto your image.")));
}

static void do_spraypaint(magic_api *api, SDL_Surface *canvas, int x, int y, int max_intensity)
{
  Uint8 r, g, b, intensity;
  Uint32 pixel;

  SDL_GetRGB(api->getpixel(canvas, x, y), canvas->format, &r, &g, &b);
  intensity = (rand() % max_intensity) / 4;

  r = (((spraypaint_r * intensity) + (r * (255 - intensity))) / 255);
  g = (((spraypaint_g * intensity) + (g * (255 - intensity))) / 255);
  b = (((spraypaint_b * intensity) + (b * (255 - intensity))) / 255);
  pixel = SDL_MapRGB(canvas->format, r, g, b);
  api->putpixel(canvas, x, y, pixel);
}

static void do_spraypaint_circle(void *ptr, int which ATTRIBUTE_UNUSED,
                                 SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy, dist;
  int max_dist;

  max_dist = sqrt((spraypaint_radius * spraypaint_radius) * 2);

  for (yy = -spraypaint_radius; yy < spraypaint_radius; yy++)
  {
    for (xx = -spraypaint_radius; xx < spraypaint_radius; xx++)
    {
      dist = sqrt((xx * xx) + (yy * yy));
      if (dist <= spraypaint_radius)
      {
        if ((rand() % (dist * 2 + 1)) == 0)
        {
          do_spraypaint(api, canvas, x + xx, y + yy, 256 - ((dist * 255) / max_dist));
        }
      }
    }
  }
  spraypaint_cnt++;
}

void spraypaint_drag(magic_api *api, int which, SDL_Surface *canvas,
                     SDL_Surface *last ATTRIBUTE_UNUSED, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, (spraypaint_radius / 8) + 1, do_spraypaint_circle);

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

  update_rect->x = ox - spraypaint_radius;
  update_rect->y = oy - spraypaint_radius;
  update_rect->w = (x + spraypaint_radius) - update_rect->x;
  update_rect->h = (y + spraypaint_radius) - update_rect->y;

  api->playsound(spraypaint_snd_spray, (x * 255) / canvas->w, 255);
}

void spraypaint_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  spraypaint_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void spraypaint_release(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                        SDL_Surface *canvas ATTRIBUTE_UNUSED,
                        SDL_Surface *last ATTRIBUTE_UNUSED, int x ATTRIBUTE_UNUSED,
                        int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  api->stopsound();
  if (spraypaint_cnt >= 1000)
  {
    spraypaint_cnt = 0;
    api->playsound(spraypaint_snd_shake, (x * 255) / canvas->w, 255);
  }
}

void spraypaint_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (spraypaint_snd_spray != NULL)
    Mix_FreeChunk(spraypaint_snd_spray);
  if (spraypaint_snd_shake != NULL)
    Mix_FreeChunk(spraypaint_snd_shake);
}

void spraypaint_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                          SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                          Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  spraypaint_r = r;
  spraypaint_g = g;
  spraypaint_b = b;
}

int spraypaint_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void spraypaint_switchin(magic_api *api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
  spraypaint_cnt = 0;
}

void spraypaint_switchout(magic_api *api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int spraypaint_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 spraypaint_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 12;
}

Uint8 spraypaint_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void spraypaint_set_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                         SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED,
                         Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  spraypaint_radius = size * 8;
}
