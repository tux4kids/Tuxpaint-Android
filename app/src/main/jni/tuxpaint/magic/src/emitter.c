/*
  emitter.c

  "Emitter" Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2024-2024 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: December 26, 2024
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL2_rotozoom.h"

#define EMITTER_QUEUE_SIZE 64
#define EMITTER_QUEUE_SIZE_SCALE 8

#define EMITTER_DRAW_THRESHOLD 16

enum
{
  EMITTER_HEARTS,
  EMITTER_SPARKLES,
  EMITTER_STARS,
  NUM_EMITTERS
};

/* Names of the tools (for button labels) */
char *emitter_names[NUM_EMITTERS] = {
  gettext_noop("Hearts"),
  gettext_noop("Sparkles"),
  gettext_noop("Stars"),
};

/* For the "Draw a trail of %s ..." descriptions */
char *emitter_descs[NUM_EMITTERS] = {
  gettext_noop("hearts"),
  gettext_noop("sparkles"),
  gettext_noop("stars"),
};

/* How many frames the image contains */
int emitter_frames[NUM_EMITTERS] = {
  1,
  4,
  1,
};

/* Max angle (clockwise or counter-clockwise) to rotate, if at all */
int emitter_rotate[NUM_EMITTERS] = {
  45,
  0,
  180,
};

/* Scale of emitter xm/ym speeds */
#define EMITTER_SPEED 64

/* Max random-direction speed for each emitter type */
int emitter_speed[NUM_EMITTERS] = {
  16,
  8,
  4,
};

/* Gravity (if any) of the emitter */
int emitter_gravity[NUM_EMITTERS] = {
  -2,
  2,
  0,
};

/* Whether emitter duplicates itself */
int emitter_duplicate[NUM_EMITTERS] = {
  0,
  1,
  0,
};

/* Our globals: */

static Mix_Chunk *emitter_snds[NUM_EMITTERS];
int last_x, last_y;
Uint8 emitter_r, emitter_g, emitter_b;
int emitter_max_trail_length;
int emitter_cur_trail_length;
int emitter_queue_x[EMITTER_QUEUE_SIZE], emitter_queue_y[EMITTER_QUEUE_SIZE];
int emitter_queue_xm[EMITTER_QUEUE_SIZE], emitter_queue_ym[EMITTER_QUEUE_SIZE];
SDL_Surface **emitter_images[NUM_EMITTERS][EMITTER_QUEUE_SIZE];

/* Function prototypes: */

Uint32 emitter_api_version(void);
int emitter_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int emitter_get_tool_count(magic_api * api);
SDL_Surface *emitter_get_icon(magic_api * api, int which);
char *emitter_get_name(magic_api * api, int which);
int emitter_get_group(magic_api * api, int which);
int emitter_get_order(int which);
char *emitter_get_description(magic_api * api, int which, int mode);
void emitter_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void emitter_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                   SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void emitter_release(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void emitter_shutdown(magic_api * api);
void emitter_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int emitter_requires_colors(magic_api * api, int which);
void emitter_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void emitter_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int emitter_modes(magic_api * api, int which);
Uint8 emitter_accepted_sizes(magic_api * api, int which, int mode);
Uint8 emitter_default_size(magic_api * api, int which, int mode);
void emitter_set_size(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);


Uint32 emitter_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


// No setup required:
int emitter_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  int i, j, k;
  char fname[1024];
  SDL_Surface *surf;
  SDL_Rect src;
  Uint32 amask;

  for (i = 0; i < NUM_EMITTERS; i++)
  {
    snprintf(fname, sizeof(fname), "%ssounds/magic/emitter%d.ogg", api->data_directory, i);
    emitter_snds[i] = Mix_LoadWAV(fname);
  }

  for (i = 0; i < NUM_EMITTERS; i++)
  {
    emitter_images[i][0] = (SDL_Surface * *)malloc(sizeof(SDL_Surface *) * emitter_frames[i]);
    if (emitter_images[i][0] == NULL)
    {
      fprintf(stderr, "Cannot allocate %s (%d) emitter's surface #0\n", emitter_names[i], i);
      return (0);
    }

    snprintf(fname, sizeof(fname), "%simages/magic/emitter%d.png", api->data_directory, i);
    surf = IMG_Load(fname);
    if (surf == NULL)
    {
      fprintf(stderr, "Cannot load %s (%d) emitter's image: '%s'\n", emitter_names[i], i, fname);
      return (0);
    }

    if (emitter_frames[i] == 1)
    {
      emitter_images[i][0][0] = surf;
    }
    else
    {
      for (j = 0; j < emitter_frames[i]; j++)
      {
        amask = ~(surf->format->Rmask | surf->format->Gmask | surf->format->Bmask);

        emitter_images[i][0][j] =
          SDL_CreateRGBSurface(SDL_SWSURFACE,
                               surf->w / emitter_frames[i],
                               surf->h,
                               surf->format->BitsPerPixel,
                               surf->format->Rmask, surf->format->Gmask, surf->format->Bmask, amask);

        src.x = (surf->w / emitter_frames[i]) * j;
        src.y = 0;
        src.w = (surf->w / emitter_frames[i]);
        src.h = surf->h;

        SDL_BlitSurface(surf, &src, emitter_images[i][0][j], NULL);
      }
      SDL_FreeSurface(surf);
    }

    for (j = 1; j < EMITTER_QUEUE_SIZE; j++)
    {
      int w, h;
      float w_scale, h_scale;

      emitter_images[i][j] = (SDL_Surface * *)malloc(sizeof(SDL_Surface *) * emitter_frames[i]);
      if (emitter_images[i][j] == NULL)
      {
        fprintf(stderr, "Cannot allocate %s (%d) emitter's surface #%d\n", emitter_names[i], i, j);
        return (0);
      }

      for (k = 0; k < emitter_frames[i]; k++)
      {
        w = emitter_images[i][0][k]->w - (emitter_images[i][0][k]->w * j / EMITTER_QUEUE_SIZE);
        h = emitter_images[i][0][k]->h - (emitter_images[i][0][k]->h * j / EMITTER_QUEUE_SIZE);
        w_scale = (float)w / (float)emitter_images[i][0][k]->w;
        h_scale = (float)h / (float)emitter_images[i][0][k]->h;

        emitter_images[i][j][k] = zoomSurface(emitter_images[i][0][k], w_scale, h_scale, 1 /* smooth */ );

        if (emitter_images[i][j][k] == NULL)
        {
          fprintf(stderr,
                  "Cannot scale %s (%d) emitter's image ('%s') (frame %d) to %d's size: %d x %d\n",
                  emitter_names[i], i, fname, k, j, w, h);
          return (0);
        }
      }
    }
  }

  return (1);
}

// We have multiple tools:
int emitter_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (NUM_EMITTERS);
}

// Load our icons:
SDL_Surface *emitter_get_icon(magic_api *api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/emitter%d_icon.png", api->data_directory, which);

  return (IMG_Load(fname));
}

// Return our names, localized:
char *emitter_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext(emitter_names[which])));
}

int emitter_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

int emitter_get_order(int which)
{
  return 2700 + which;
}

// Return our descriptions, localized:
char *emitter_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  char desc[1024];

  snprintf(desc, sizeof(desc),
           gettext("Click and drag to draw a trail of %s on your picture."), gettext(emitter_descs[which]));
  return (strdup(desc));
}

void emitter_drag(magic_api *api, int which, SDL_Surface *canvas,
                  SDL_Surface *last, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  int i, img;
  SDL_Surface *tmpSurf, *srcSurf;
  SDL_Rect dest;

  SDL_BlitSurface(last, NULL, canvas, NULL);

  /* Move all of the objects */
  for (i = 0; i < emitter_cur_trail_length; i++)
  {
    emitter_queue_x[i] += emitter_queue_xm[i] / EMITTER_SPEED;
    emitter_queue_y[i] += emitter_queue_ym[i] / EMITTER_SPEED;

    emitter_queue_ym[i] += emitter_gravity[which];

    if (emitter_duplicate[which] && (rand() % 16) == 0)
    {
      int d;

      d = (rand() % (i + 1));

      emitter_queue_x[d] = emitter_queue_x[i];
      emitter_queue_y[d] = emitter_queue_y[i];
      emitter_queue_xm[d] = emitter_queue_xm[i];
      emitter_queue_ym[d] = emitter_queue_ym[i] * 2;
    }
  }

  /* If we've moved a significant distance, emit another object */
  if (abs(x - last_x) > EMITTER_DRAW_THRESHOLD || abs(y - last_y) > EMITTER_DRAW_THRESHOLD)
  {
    if (emitter_cur_trail_length < emitter_max_trail_length - 1)
    {
      /* Trail not full, add another */
      emitter_cur_trail_length++;
    }
    else
    {
      /* Trail is full, rotate queue */
      for (i = 0; i < emitter_cur_trail_length; i++)
      {
        emitter_queue_x[i] = emitter_queue_x[i + 1];
        emitter_queue_y[i] = emitter_queue_y[i + 1];
        emitter_queue_xm[i] = emitter_queue_xm[i + 1];
        emitter_queue_ym[i] = emitter_queue_ym[i + 1];
      }
    }

    /* Add the new object */
    emitter_queue_x[emitter_cur_trail_length] = x;
    emitter_queue_y[emitter_cur_trail_length] = y;
    emitter_queue_xm[emitter_cur_trail_length] = (rand() % emitter_speed[which] * 2) - emitter_speed[which];
    emitter_queue_ym[emitter_cur_trail_length] = (rand() % emitter_speed[which] * 2) - emitter_speed[which];

    last_x = x;
    last_y = y;
  }

  /* Draw all of them */
  for (i = 0; i < emitter_cur_trail_length + 1; i++)
  {
    img = (emitter_cur_trail_length - i);

    img = img + (rand() % 4) - 2;
    if (img < 0)
      img = 0;
    else if (img >= EMITTER_QUEUE_SIZE)
      img = EMITTER_QUEUE_SIZE - 1;

    if (emitter_frames[which] == 1)
    {
      srcSurf = emitter_images[which][img][0];
    }
    else
    {
      srcSurf = emitter_images[which][img][rand() % emitter_frames[which]];
    }

    if (emitter_rotate[which] != 0)
    {
      tmpSurf =
        rotozoomSurface(srcSurf, (rand() % emitter_rotate[which] * 2) - emitter_rotate[which], 1.0 /* no scale */ ,
                        1 /* smoothing */ );
      if (tmpSurf == NULL)
        return;                 // Abort!
    }
    else
    {
      tmpSurf = srcSurf;
    }

    if (tmpSurf != NULL)
    {
      int xx, yy;
      Uint32 amask;
      SDL_Surface *tmpSurf2;
      Uint8 r, g, b, a;

      dest.x = emitter_queue_x[i] - tmpSurf->w / 2;
      dest.y = emitter_queue_y[i] - tmpSurf->h / 2;
      dest.w = tmpSurf->w;
      dest.h = tmpSurf->h;

      dest.x += (rand() % 4) - 2;
      dest.y += (rand() % 4) - 2;

      amask = ~(tmpSurf->format->Rmask | tmpSurf->format->Gmask | tmpSurf->format->Bmask);

      tmpSurf2 =
        SDL_CreateRGBSurface(SDL_SWSURFACE,
                             tmpSurf->w,
                             tmpSurf->h,
                             tmpSurf->format->BitsPerPixel,
                             tmpSurf->format->Rmask, tmpSurf->format->Gmask, tmpSurf->format->Bmask, amask);

      if (tmpSurf2 != NULL)
      {
        SDL_LockSurface(tmpSurf);
        SDL_LockSurface(tmpSurf2);

        for (y = 0; y < tmpSurf->h; y++)
        {
          for (x = 0; x < tmpSurf->w; x++)
          {
            SDL_GetRGBA(api->getpixel(tmpSurf, x, y), tmpSurf->format, &r, &g, &b, &a);
            api->putpixel(tmpSurf2, x, y,
                          SDL_MapRGBA(tmpSurf2->format, (r + emitter_r) >> 1,
                                      (g + emitter_g) >> 1, (b + emitter_b) >> 1, a));
          }
        }
        SDL_UnlockSurface(tmpSurf2);
        SDL_UnlockSurface(tmpSurf);

        SDL_BlitSurface(tmpSurf2, NULL, canvas, &dest);
        SDL_FreeSurface(tmpSurf2);
      }
    }

    if (emitter_rotate[which] != 0 && tmpSurf != NULL)
    {
      SDL_FreeSurface(tmpSurf);
    }
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(emitter_snds[which], (x * 255) / canvas->w, 255);
}

void emitter_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  emitter_queue_x[0] = x;
  emitter_queue_y[0] = y;
  emitter_queue_xm[0] = (rand() % emitter_speed[which] * 2) - emitter_speed[which];
  emitter_queue_ym[0] = (rand() % emitter_speed[which] * 2) - emitter_speed[which];
  emitter_cur_trail_length = 0;
  last_x = -100;
  last_y = -100;

  emitter_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void emitter_release(magic_api *api, int which,
                     SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED,
                     int x, int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
}

void emitter_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_EMITTERS; i++)
    Mix_FreeChunk(emitter_snds[i]);
}

void emitter_set_color(magic_api *api, int which ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas ATTRIBUTE_UNUSED,
                       SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                       Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  emitter_r = r;
  emitter_g = g;
  emitter_b = b;
}

int emitter_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void emitter_switchin(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

void emitter_switchout(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int emitter_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 emitter_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return EMITTER_QUEUE_SIZE / EMITTER_QUEUE_SIZE_SCALE;
}

Uint8 emitter_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (EMITTER_QUEUE_SIZE / EMITTER_QUEUE_SIZE_SCALE) / 2;
}

void emitter_set_size(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED,
                      SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 size, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  emitter_max_trail_length = size * EMITTER_QUEUE_SIZE_SCALE;
}
