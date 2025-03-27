/*
  comicdot.c

  Paints a pattern of circular dots, simulating the effect of
  older comic books that used the "Ben Day Process"
  (https://en.wikipedia.org/wiki/Ben_Day_process)

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

static Mix_Chunk *comicdot_snd;
static int comicdot_radius = 16;
static int comicdot_r, comicdot_g, comicdot_b;

enum
{
  COMICDOT_LG,
  COMICDOT_SM,
  NUM_COMICDOT_SIZES
};

SDL_Surface *comicdot_pattern[NUM_COMICDOT_SIZES];

Uint32 comicdot_api_version(void);
int comicdot_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int comicdot_get_tool_count(magic_api * api);
SDL_Surface *comicdot_get_icon(magic_api * api, int which);
char *comicdot_get_name(magic_api * api, int which);
int comicdot_get_group(magic_api * api, int which);
int comicdot_get_order(int which);
char *comicdot_get_description(magic_api * api, int which, int mode);

void comicdot_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void comicdot_click(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void comicdot_release(magic_api * api, int which,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void comicdot_shutdown(magic_api * api);
void comicdot_set_color(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int comicdot_requires_colors(magic_api * api, int which);
void comicdot_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void comicdot_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int comicdot_modes(magic_api * api, int which);
Uint8 comicdot_accepted_sizes(magic_api * api, int which, int mode);
Uint8 comicdot_default_size(magic_api * api, int which, int mode);
void comicdot_set_size(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);

Uint32 comicdot_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int comicdot_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];
  int i;

  snprintf(fname, sizeof(fname), "%ssounds/magic/comic_dots.ogg", api->data_directory);
  comicdot_snd = Mix_LoadWAV(fname);

  /* Load base pattern image */
  snprintf(fname, sizeof(fname), "%simages/magic/comicdot-pattern.png", api->data_directory);
  comicdot_pattern[0] = IMG_Load(fname);
  if (comicdot_pattern[0] == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  /* Create the scaled versions */
  for (i = 1; i < NUM_COMICDOT_SIZES; i++)
  {
    int size;

    size = (100 * (NUM_COMICDOT_SIZES - i)) / NUM_COMICDOT_SIZES;

    comicdot_pattern[i] = api->scale(comicdot_pattern[0],
                                     (comicdot_pattern[0]->w * size) / 100, (comicdot_pattern[0]->h * size) / 100, 1);
    if (comicdot_pattern[i] == NULL)
    {
      fprintf(stderr, "Can't scale %s (%d%%)\n", fname, size);
      return 0;
    }
  }

  return (1);
}

int comicdot_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (NUM_COMICDOT_SIZES);
}

SDL_Surface *comicdot_get_icon(magic_api *api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/comic_dots_%d.png", api->data_directory, which);

  return (IMG_Load(fname));
}

char *comicdot_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Comic Dots")));
}

int comicdot_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

int comicdot_get_order(int which)
{
  return 1910 + which;
}

char *comicdot_get_description(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return (strdup(gettext("Click and drag to draw an a dot pattern on your picture")));
  else
    return (strdup(gettext("Click to apply a dot pattern on your entire picture")));
}

static void do_comicdot(void *ptr, int which, SDL_Surface *canvas, SDL_Surface *last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  Uint8 r1, g1, b1, r, g, b, n, _;

  // Uint8 nr, ng, nb;
  Uint32 pixel;
  int offx, offy;
  SDL_Surface *pat;

  pat = comicdot_pattern[which];

  offx = (((comicdot_r + comicdot_g) / 2) * pat->w) / 255;
  offy = (((comicdot_b - comicdot_g) / 2) * pat->h) / 255;

  SDL_GetRGB(api->getpixel(last, x, y), last->format, &r1, &g1, &b1);
  SDL_GetRGB(api->getpixel(pat, (x + offx) % pat->w, (y + offy) % pat->h), pat->format, &n, &_, &_);
  r = ((r1 * n) + (comicdot_r * (255 - n))) / 255;
  g = ((g1 * n) + (comicdot_g * (255 - n))) / 255;
  b = ((b1 * n) + (comicdot_b * (255 - n))) / 255;

  /* N.B. This caused an unwanted outline around the effect
     when drawing on a non-white background */
  //nr = (r1 * r) / 255;
  //ng = (g1 * g) / 255;
  //nb = (b1 * b) / 255;
  //pixel = SDL_MapRGB(canvas->format, nr, ng, nb);

  pixel = SDL_MapRGB(canvas->format, r, g, b);

  api->putpixel(canvas, x, y, pixel);
}

static void do_comicdot_circle(void *ptr, int which ATTRIBUTE_UNUSED,
                               SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;

  for (yy = -comicdot_radius; yy < comicdot_radius; yy++)
  {
    for (xx = -comicdot_radius; xx < comicdot_radius; xx++)
    {
      if (api->in_circle(xx, yy, comicdot_radius))
      {
        if (!api->touched(xx + x, yy + y))
          do_comicdot(api, which, canvas, last, x + xx, y + yy);
      }
    }
  }
}

void comicdot_drag(magic_api *api, int which, SDL_Surface *canvas,
                   SDL_Surface *last ATTRIBUTE_UNUSED, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_comicdot_circle);

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

  update_rect->x = ox - comicdot_radius;
  update_rect->y = oy - comicdot_radius;
  update_rect->w = (x + comicdot_radius) - update_rect->x;
  update_rect->h = (y + comicdot_radius) - update_rect->y;

  if (api->playingsound())
    api->unpausesound();
  else
    api->playsound(comicdot_snd, 64 + ((x * 127) / canvas->w), 255);
}

void comicdot_click(magic_api *api, int which, int mode,
                    SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  if (mode == MODE_PAINT)
    comicdot_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
  {
    int xx, yy;

    for (yy = 0; yy < canvas->h; yy++)
      for (xx = 0; xx < canvas->w; xx++)
        do_comicdot(api, which, canvas, last, xx, yy);

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
    api->playsound(comicdot_snd, (x * 255) / canvas->w, 255);
  }
}

void comicdot_release(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED,
                      SDL_Surface *last ATTRIBUTE_UNUSED,
                      int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  api->pausesound();
}

void comicdot_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  if (comicdot_snd != NULL)
    Mix_FreeChunk(comicdot_snd);

  for (i = 0; i < NUM_COMICDOT_SIZES; i++)
  {
    if (comicdot_pattern[i] != NULL)
    {
      SDL_FreeSurface(comicdot_pattern[i]);
      comicdot_pattern[i] = NULL;
    }
  }
}

void comicdot_set_color(magic_api *api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED,
                        SDL_Surface *canvas ATTRIBUTE_UNUSED,
                        SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                        Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  comicdot_r = r;
  comicdot_g = g;
  comicdot_b = b;
}

int comicdot_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void comicdot_switchin(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

void comicdot_switchout(magic_api *api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
  api->stopsound();
}

int comicdot_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 comicdot_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 8;
  else
    return 0;
}

Uint8 comicdot_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

void comicdot_set_size(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas ATTRIBUTE_UNUSED,
                       SDL_Surface *last ATTRIBUTE_UNUSED,
                       Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  comicdot_radius = size * 4;
}
