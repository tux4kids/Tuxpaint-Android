/*
  fractal.c

  Freehand paint tree-like, fractal-like patterns.

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

#define NUM_TOOLS 4

typedef struct fract_opt_s
{
  int angle;
  float scale;
} fract_opt_t;

fract_opt_t fract_opt[NUM_TOOLS] = {
  {0, 0.5},
  {0, 1.5},
  {15, 1.1},
  {90, 0.75},
};


typedef struct pt_s
{
  int x, y;
} pt_t;

#define MAX_PTS 512
pt_t pts[MAX_PTS];
int num_pts = 0;

float fractal_click_x, fractal_click_y;

static Mix_Chunk *fractal_snd;
static int fractal_radius = 16;
Uint8 fractal_r, fractal_g, fractal_b;

/* These change during each recursive iteration */
int fractal_radius_cur;
float fractal_opacity_cur;

Uint32 fractal_api_version(void);
int fractal_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int fractal_get_tool_count(magic_api * api);
SDL_Surface *fractal_get_icon(magic_api * api, int which);
char *fractal_get_name(magic_api * api, int which);
int fractal_get_group(magic_api * api, int which);
int fractal_get_order(int which);
char *fractal_get_description(magic_api * api, int which, int mode);

void fractal_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void fractal_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void fractal_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void fractal_shutdown(magic_api * api);
void fractal_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int fractal_requires_colors(magic_api * api, int which);
void fractal_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void fractal_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int fractal_modes(magic_api * api, int which);
Uint8 fractal_accepted_sizes(magic_api * api, int which, int mode);
Uint8 fractal_default_size(magic_api * api, int which, int mode);
void fractal_set_size(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);
void do_fractal(magic_api * api, int which, SDL_Surface * canvas, int iter,
                float cx, float cy, float angle, float scale, float opacity, int final);

Uint32 fractal_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int fractal_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/fractals.ogg", api->data_directory);
  fractal_snd = Mix_LoadWAV(fname);

  return (1);
}

int fractal_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

SDL_Surface *fractal_get_icon(magic_api *api, int ATTRIBUTE_UNUSED which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/fractals.png", api->data_directory);

  return (IMG_Load(fname));
}

char *fractal_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  char tmp[128];

  snprintf(tmp, sizeof(tmp), gettext("Fractal #%d"), which + 1);
  return strdup(tmp);
}

int fractal_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

int fractal_get_order(int which)
{
  return 10001 + which;
}

char *fractal_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  char tmp[512];

  if (fract_opt[which].scale != 1.0)
  {
    if (fract_opt[which].angle != 0)
    {
      snprintf(tmp, sizeof(tmp),
               gettext
               ("Click and drag to sketch a shape. It will repeat, %1$s %2$d%% and rotating %3$d degrees."),
               (fract_opt[which].scale >
                1.0 ? gettext("scaling up") : gettext("scaling down")),
               (int)(fract_opt[which].scale * 100), fract_opt[which].angle);
    }
    else
    {
      snprintf(tmp, sizeof(tmp),
               gettext
               ("Click and drag to sketch a shape. It will repeat, %1$s %2$d%%."),
               (fract_opt[which].scale >
                1.0 ? gettext("scaling up") : gettext("scaling down")), (int)(fract_opt[which].scale * 100));
    }
  }
  else
  {
    snprintf(tmp, sizeof(tmp),
             gettext("Click and drag to sketch a shape. It will repeat, rotating %d degrees."), fract_opt[which].angle);
  }

  return (strdup(tmp));
}

static void do_fractal_circle(void *ptr, int which ATTRIBUTE_UNUSED,
                              SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;
  Uint8 r, g, b;
  Uint32 pix;

  for (yy = -fractal_radius_cur; yy < fractal_radius_cur; yy++)
  {
    for (xx = -fractal_radius_cur; xx < fractal_radius_cur; xx++)
    {
      if (fractal_opacity_cur < 1.0)
      {
        SDL_GetRGB(api->getpixel(canvas, xx + x, yy + y), canvas->format, &r, &g, &b);
        r = (Uint8) (((float)r * (1.0 - fractal_opacity_cur)) + ((float)fractal_r * fractal_opacity_cur));
        g = (Uint8) (((float)g * (1.0 - fractal_opacity_cur)) + ((float)fractal_g * fractal_opacity_cur));
        b = (Uint8) (((float)b * (1.0 - fractal_opacity_cur)) + ((float)fractal_b * fractal_opacity_cur));
      }
      else
      {
        r = fractal_r;
        g = fractal_g;
        b = fractal_b;
      }

      pix = SDL_MapRGB(canvas->format, r, g, b);
      api->putpixel(canvas, xx + x, yy + y, pix);
    }
  }
}

void do_fractal(magic_api *api, int which, SDL_Surface *canvas, int iter,
                float cx, float cy, float angle, float scale, float opacity, int final)
{
  int i;
  float x1, y1, x2, y2, nx, ny;
  float co, si;

  co = cosf(angle);
  si = sinf(angle);

  for (i = 0; i < num_pts - 1; i++)
  {
    x1 = (float)(pts[i].x);
    y1 = (float)(pts[i].y);
    x2 = (float)(pts[i + 1].x);
    y2 = (float)(pts[i + 1].y);

    /* Translate point relative to (0,0) origin */
    x1 -= cx;
    y1 -= cy;
    x2 -= cx;
    y2 -= cy;

    /* Rotate point */
    nx = (co * x1) - (si * y1);
    ny = (si * x1) + (co * y1);
    x1 = nx;
    y1 = ny;

    nx = (co * x2) - (si * y2);
    ny = (si * x2) + (co * y2);
    x2 = nx;
    y2 = ny;

    /* Scale point */
    x1 *= scale;
    y1 *= scale;
    x2 *= scale;
    y2 *= scale;

    /* Translate point back to starting position */
    x1 += cx;
    y1 += cy;
    x2 += cx;
    y2 += cy;

    fractal_radius_cur = (iter / 2) + 1;
    fractal_opacity_cur = opacity;
    api->line((void *)api, which, canvas, NULL, (int)x1, (int)y1, (int)x2,
              (int)y2, (final ? 1 : 10), do_fractal_circle);

    if (final && ((i % ((num_pts / 3) + 1)) == 1) && (iter > 1))
    {
      do_fractal(api, which, canvas, iter - 1, x2, y2,
                 angle + ((float)fract_opt[which].angle / 180.0 * M_PI),
                 scale * fract_opt[which].scale, opacity * 0.5, final);
    }
  }
}

void fractal_drag(magic_api *api, int which, SDL_Surface *canvas,
                  SDL_Surface *last, int ox ATTRIBUTE_UNUSED,
                  int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  if (num_pts < MAX_PTS)
  {
    pts[num_pts].x = x;
    pts[num_pts].y = y;
    num_pts++;
  }

  SDL_BlitSurface(last, NULL, canvas, NULL);

  do_fractal(api, which, canvas, fractal_radius, fractal_click_x, fractal_click_y, 0.0, 1.0, 1.0, 0);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(fractal_snd, (x * 255) / canvas->w, 255);
}

void fractal_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  pts[0].x = x;
  pts[0].y = y;
  num_pts = 1;

  fractal_click_x = (float)x;
  fractal_click_y = (float)y;
  fractal_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void fractal_release(magic_api *api, int which,
                     SDL_Surface *canvas,
                     SDL_Surface *last, int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect)
{
  SDL_BlitSurface(last, NULL, canvas, NULL);

  do_fractal(api, which, canvas, fractal_radius, fractal_click_x, fractal_click_y, 0.0, 1.0, 1.0, 1);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->stopsound();
}

void fractal_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (fractal_snd != NULL)
    Mix_FreeChunk(fractal_snd);
}

void fractal_set_color(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas ATTRIBUTE_UNUSED,
                       SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                       Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  fractal_r = r;
  fractal_g = g;
  fractal_b = b;
}

int fractal_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void fractal_switchin(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

void fractal_switchout(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int fractal_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 fractal_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

Uint8 fractal_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 2;
}

void fractal_set_size(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED,
                      SDL_Surface *last ATTRIBUTE_UNUSED,
                      Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  fractal_radius = size + 1;
}
