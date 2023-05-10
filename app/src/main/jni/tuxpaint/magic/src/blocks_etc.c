/*
  blocks_etc.c

  Blocks, Chalk and Drip Magic Tools Plugin
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

  Last updated: April 23, 2023
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"

/* What tools we contain: */

enum
{
  TOOL_BLOCKS,
  TOOL_CHALK,
  TOOL_DRIP,
  NUM_TOOLS
};

static int EFFECT_REZ = 4;


/* Our globals: */

static Mix_Chunk *snd_effect[NUM_TOOLS];


/* Our function prototypes: */

int blocks_etc_init(magic_api * api, Uint32 disabled_features);
Uint32 blocks_etc_api_version(void);
int blocks_etc_get_tool_count(magic_api * api);
SDL_Surface *blocks_etc_get_icon(magic_api * api, int which);
char *blocks_etc_get_name(magic_api * api, int which);
int blocks_etc_get_group(magic_api * api, int which);
char *blocks_etc_get_description(magic_api * api, int which, int mode);
static void blocks_etc_linecb(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void blocks_etc_drag(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void blocks_etc_click(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void blocks_etc_release(magic_api * api, int which,
                        SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void blocks_etc_shutdown(magic_api * api);
void blocks_etc_set_color(magic_api * api, int which, SDL_Surface * canvas,
                          SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int blocks_etc_requires_colors(magic_api * api, int which);
void blocks_etc_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void blocks_etc_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int blocks_etc_modes(magic_api * api, int which);
Uint8 blocks_etc_accepted_sizes(magic_api * api, int which, int mode);
Uint8 blocks_etc_default_size(magic_api * api, int which, int mode);
void blocks_etc_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                         SDL_Rect * update_rect);



int blocks_etc_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/blocks.wav", api->data_directory);
  snd_effect[0] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%ssounds/magic/chalk.wav", api->data_directory);
  snd_effect[1] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%ssounds/magic/drip.wav", api->data_directory);
  snd_effect[2] = Mix_LoadWAV(fname);

  return (1);
}

Uint32 blocks_etc_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


// We have multiple tools:
int blocks_etc_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

// Load our icons:
SDL_Surface *blocks_etc_get_icon(magic_api * api, int which)
{
  char fname[1024];

  if (which == TOOL_BLOCKS)
  {
    snprintf(fname, sizeof(fname), "%simages/magic/blocks.png", api->data_directory);
  }
  else if (which == TOOL_CHALK)
  {
    snprintf(fname, sizeof(fname), "%simages/magic/chalk.png", api->data_directory);
  }
  else if (which == TOOL_DRIP)
  {
    snprintf(fname, sizeof(fname), "%simages/magic/drip.png", api->data_directory);
  }

  return (IMG_Load(fname));
}

// Return our names, localized:
char *blocks_etc_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == TOOL_BLOCKS)
    return (strdup(gettext_noop("Blocks")));
  else if (which == TOOL_CHALK)
    return (strdup(gettext_noop("Chalk")));
  else if (which == TOOL_DRIP)
    return (strdup(gettext_noop("Drip")));

  return (NULL);
}

// Return our group (all the same):
int blocks_etc_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_DISTORTS;
}

// Return our descriptions, localized:
char *blocks_etc_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  if (which == TOOL_BLOCKS)
  {
    if (mode == MODE_PAINT)
    {
      return (strdup(gettext_noop("Click and drag the mouse around to make the picture blocky.")));
    }
    else
    {
      return (strdup(gettext_noop("Click to make the entire picture blocky.")));
    }
  }
  else if (which == TOOL_CHALK)
  {
    if (mode == MODE_PAINT)
    {
      return (strdup(gettext_noop("Click and drag the mouse around to turn the picture into a chalk drawing.")));
    }
    else
    {
      return (strdup(gettext_noop("Click to turn the entire picture into a chalk drawing.")));
    }
  }
  else if (which == TOOL_DRIP)
  {
    if (mode == MODE_PAINT)
    {
      return (strdup(gettext_noop("Click and drag the mouse around to make the picture drip.")));
    }
    else
    {
      return (strdup(gettext_noop("Click to make the entire picture drip.")));
    }
  }

  return (NULL);
}

// Do the effect:

static void blocks_etc_linecb(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy;
  int h;
  SDL_Rect src, dest;
  Uint8 r, g, b;
  Uint32 colr;

  if (which == TOOL_BLOCKS)
  {
    /* Put x/y on exact grid points: */

    x = (x / EFFECT_REZ) * EFFECT_REZ;
    y = (y / EFFECT_REZ) * EFFECT_REZ;

    if (!api->touched(x, y))
    {
      for (yy = y - (EFFECT_REZ * 2); yy < y + (EFFECT_REZ * 2); yy = yy + EFFECT_REZ)
      {
        for (xx = x - (EFFECT_REZ * 2); xx < x + (EFFECT_REZ * 2); xx = xx + EFFECT_REZ)
        {
          Uint32 pix[(EFFECT_REZ * EFFECT_REZ)];
          Uint32 p_or = 0;
          Uint32 p_and = ~0;
          unsigned i = (EFFECT_REZ * EFFECT_REZ);

          while (i--)
          {
            Uint32 p_tmp;

            p_tmp = api->getpixel(last, xx + (i / EFFECT_REZ), yy + (i % EFFECT_REZ));
            p_or |= p_tmp;
            p_and &= p_tmp;
            pix[i] = p_tmp;
          }
          if (p_or == p_and)    // if all pixels the same already
          {
            SDL_GetRGB(p_or, last->format, &r, &g, &b);
          }
          else                  // nope, must average them
          {
            double r_sum = 0.0;
            double g_sum = 0.0;
            double b_sum = 0.0;

            i = (EFFECT_REZ * EFFECT_REZ);
            while (i--)
            {
              SDL_GetRGB(pix[i], last->format, &r, &g, &b);
              r_sum += api->sRGB_to_linear(r);
              g_sum += api->sRGB_to_linear(g);
              b_sum += api->sRGB_to_linear(b);
            }
            r = api->linear_to_sRGB(r_sum / (float)(EFFECT_REZ * EFFECT_REZ));
            g = api->linear_to_sRGB(g_sum / (float)(EFFECT_REZ * EFFECT_REZ));
            b = api->linear_to_sRGB(b_sum / (float)(EFFECT_REZ * EFFECT_REZ));
          }

          /* Draw block: */

          dest.x = xx;
          dest.y = yy;
          dest.w = EFFECT_REZ;
          dest.h = EFFECT_REZ;

          SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, r, g, b));
        }
      }
    }
  }
  else if (which == TOOL_CHALK)
  {
    for (yy = y - (EFFECT_REZ * 2); yy <= y + (EFFECT_REZ * 2); yy = yy + EFFECT_REZ)
    {
      for (xx = x - (EFFECT_REZ * 2); xx <= x + (EFFECT_REZ * 2); xx = xx + EFFECT_REZ)
      {
        dest.x = xx + ((rand() % (EFFECT_REZ + 1)) - (EFFECT_REZ / 2));
        dest.y = yy + ((rand() % (EFFECT_REZ + 1)) - (EFFECT_REZ / 2));
        dest.w = (rand() % EFFECT_REZ) + (EFFECT_REZ / 2);
        dest.h = (rand() % EFFECT_REZ) + (EFFECT_REZ / 2);

        colr = api->getpixel(last, clamp(0, xx, canvas->w - 1), clamp(0, yy, canvas->h - 1));
        SDL_FillRect(canvas, &dest, colr);
      }
    }
  }
  else if (which == TOOL_DRIP)
  {
    for (xx = x - (EFFECT_REZ * 2); xx <= x + (EFFECT_REZ * 2); xx++)
    {
      h = (rand() % (EFFECT_REZ * 2)) + (EFFECT_REZ * 2);

      for (yy = y; yy <= y + h; yy++)
      {
        src.x = xx;
        src.y = y;
        src.w = 1;
        src.h = (EFFECT_REZ * 4);

        dest.x = xx;
        dest.y = yy;

        SDL_BlitSurface(last, &src, canvas, &dest);
      }
    }
  }
}

// Affect the canvas on drag:
void blocks_etc_drag(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, blocks_etc_linecb);

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

  update_rect->x = ox - (EFFECT_REZ * 4);
  update_rect->y = oy - (EFFECT_REZ * 4);
  update_rect->w = (x + (EFFECT_REZ * 4)) - update_rect->x;
  update_rect->h = (y + (EFFECT_REZ * 4)) - update_rect->y;

  api->playsound(snd_effect[which], (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void blocks_etc_click(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (mode == MODE_PAINT)
  {
    blocks_etc_drag(api, which, canvas, last, x, y, x, y, update_rect);
  }
  else                          /* MODE_FULLSCREEN */
  {
    if (which != TOOL_DRIP)
    {
      for (y = 0; y < canvas->h; y += EFFECT_REZ)
      {
        if (y % 10 == 0)
        {
          api->update_progress_bar();
        }
        for (x = 0; x < canvas->w; x += EFFECT_REZ)
        {
          blocks_etc_linecb(api, which, canvas, last, x, y);
        }
      }
    }
    else
    {
      /* Drip (works from bottom-to-top) */
      int p = (canvas->h - 1) % 10;

      for (y = canvas->h - 1; y >= 0; y -= EFFECT_REZ)
      {
        if ((y + p) % 10 == 0)
        {
          api->update_progress_bar();
        }
        for (x = 0; x < canvas->w; x += EFFECT_REZ)
        {
          blocks_etc_linecb(api, which, canvas, last, x, y);
        }
      }
    }
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;

    api->playsound(snd_effect[which], (x * 255) / canvas->w, 255);
  }
}

// Affect the canvas on release:
void blocks_etc_release(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas ATTRIBUTE_UNUSED,
                        SDL_Surface * last ATTRIBUTE_UNUSED,
                        int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void blocks_etc_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (snd_effect[0] != NULL)
    Mix_FreeChunk(snd_effect[0]);

  if (snd_effect[1] != NULL)
    Mix_FreeChunk(snd_effect[1]);
}

// Record the color from Tux Paint:
void blocks_etc_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                          Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                          SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// Use colors:
int blocks_etc_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void blocks_etc_switchin(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void blocks_etc_switchout(magic_api * api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int blocks_etc_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 blocks_etc_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 4;
}

Uint8 blocks_etc_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  if (which == TOOL_BLOCKS)
    return 1;
  else
    return 2;
}

void blocks_etc_set_size(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                         SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  if (which == TOOL_BLOCKS)
    EFFECT_REZ = size * 4;
  else
    EFFECT_REZ = size * 2;
}
