/*
  glasstile.c

  Glass Tile Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2008 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

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

  Last updated: July 9, 2008
  $Id: glasstile.c,v 1.12 2011/11/26 22:04:50 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk * glasstile_snd;

// Prototypes
Uint32 glasstile_api_version(void);
int glasstile_init(magic_api * api);
int glasstile_get_tool_count(magic_api * api);
SDL_Surface * glasstile_get_icon(magic_api * api, int which);
char * glasstile_get_name(magic_api * api, int which);
char * glasstile_get_description(magic_api * api, int which, int mode);
static void do_glasstile(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y);
void glasstile_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void glasstile_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void glasstile_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void glasstile_shutdown(magic_api * api);
void glasstile_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int glasstile_requires_colors(magic_api * api, int which);
void glasstile_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void glasstile_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int glasstile_modes(magic_api * api, int which);

Uint32 glasstile_api_version(void) { return(TP_MAGIC_API_VERSION); }

static int * * glasstile_hit;
static int glasstile_hit_xsize;
static int glasstile_hit_ysize;

// No setup required:
int glasstile_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/glasstile.ogg",
	    api->data_directory);
  glasstile_snd = Mix_LoadWAV(fname);
  
  glasstile_hit = NULL;
  glasstile_hit_ysize = 0;

  return(1);
}

// We have multiple tools:
int glasstile_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * glasstile_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/glasstile.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * glasstile_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Glass Tile")));
}

// Return our descriptions, localized:
char * glasstile_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return(strdup(gettext_noop("Click and drag the mouse to put glass tile over your picture.")));
  else
    return(strdup(gettext_noop("Click to cover your entire picture in glass tiles.")));
}

// Do the effect:

static void do_glasstile(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  int xx, yy, xl, xr, yt, yb;
  Uint8 r1, g1, b1,
        r2, g2, b2,
        r3, g3, b3,
        r4, g4, b4,
        r, g, b;
  Uint32 rgb;


#define GT_SIZE 20

  if (x < 0 || y < 0 || x >= canvas->w || y >= canvas->h)
    return;

  if (glasstile_hit[y / GT_SIZE][x / GT_SIZE])
    return;

  glasstile_hit[y / GT_SIZE][x / GT_SIZE] = 1;


  /* Align mouse (x,y) to the tile shape (to avoid smearing) */

  x = ((x / (GT_SIZE * 2)) * (GT_SIZE * 2)) + (GT_SIZE / 2);
  y = ((y / (GT_SIZE * 2)) * (GT_SIZE * 2)) + (GT_SIZE / 2);

  if (api->touched(x, y))
    return;


  /* Apply the effect: */

  for (yy = -GT_SIZE; yy < GT_SIZE; yy = yy + 2)
  {
    for (xx = -GT_SIZE; xx < GT_SIZE; xx = xx + 2)
    {
      SDL_GetRGB(api->getpixel(last, x + xx, y + yy), last->format,
		 &r1, &g1, &b1);
      SDL_GetRGB(api->getpixel(last, x + xx + 1, y + yy), last->format,
		 &r2, &g2, &b2);
      SDL_GetRGB(api->getpixel(last, x + xx, y + yy + 1), last->format,
		 &r3, &g3, &b3);
      SDL_GetRGB(api->getpixel(last, x + xx + 1, y + yy + 1), last->format,
		 &r4, &g4, &b4);

      r = (r1 + r2 + r3 + r4) >> 2;
      g = (g1 + g2 + g3 + g4) >> 2;
      b = (b1 + b2 + b3 + b4) >> 2;

      if (xx <= -GT_SIZE + 2 || yy == -GT_SIZE + 2)
      {
        r = min(255, r + 64);
        g = min(255, g + 64);
        b = min(255, b + 64);
      }
      else if (xx >= GT_SIZE - 3|| yy >= GT_SIZE - 3)
      {
        r = max(0, r - 64);
        g = max(0, g - 64);
        b = max(0, b - 64);
      }

      rgb = SDL_MapRGB(canvas->format, r, g, b);

      xl = (xx / 3) - GT_SIZE + (GT_SIZE / 3);
      xr = (xx / 3) + (GT_SIZE * 2) / 3;
      yt = (yy / 3) - GT_SIZE + (GT_SIZE / 3);
      yb = (yy / 3) + (GT_SIZE * 2) / 3;

      api->putpixel(canvas, x + xl, y + yt, rgb);
      api->putpixel(canvas, x + xx / 2, y + yt, rgb);
      api->putpixel(canvas, x + xr, y + yt, rgb);

      api->putpixel(canvas, x + xl, y + yy / 2, rgb);
      api->putpixel(canvas, x + xr, y + yy / 2, rgb);

      api->putpixel(canvas, x + xl, y + yb, rgb);
      api->putpixel(canvas, x + xx / 2, y + yb, rgb);
      api->putpixel(canvas, x + xr, y + yb, rgb);

      /* Center */

      api->putpixel(canvas, x + xx / 2, y + yy / 2, rgb);
    }
  }
}

// Affect the canvas on drag:
void glasstile_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1, do_glasstile);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

/* FIXME */
/*
  x = ((x / (GT_SIZE * 2)) * (GT_SIZE * 2));
  y = ((y / (GT_SIZE * 2)) * (GT_SIZE * 2));
  ox = ((ox / (GT_SIZE * 2)) * (GT_SIZE * 2));
  oy = ((oy / (GT_SIZE * 2)) * (GT_SIZE * 2));

  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }

  x -= GT_SIZE * 2;
  y -= GT_SIZE * 2;
  ox += GT_SIZE * 2;
  oy += GT_SIZE * 2;

  update_rect->x = x - 1;
  update_rect->y = y - 1;
  update_rect->w = ox - update_rect->x + 1;
  update_rect->h = oy - update_rect->h + 1;
*/

  api->playsound(glasstile_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void glasstile_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  int xx, yy;

  if (glasstile_hit == NULL)
  {
    glasstile_hit_ysize = (canvas->h / GT_SIZE) + 1;
    glasstile_hit_xsize = (canvas->w / GT_SIZE) + 1;

    glasstile_hit = (int * *) malloc(sizeof(int *) * glasstile_hit_ysize);

    for (yy = 0; yy < glasstile_hit_ysize; yy++)
      glasstile_hit[yy] = (int *) malloc(sizeof(int) * glasstile_hit_xsize);
  }

  for (yy = 0; yy < glasstile_hit_ysize; yy++)
    for (xx = 0; xx < glasstile_hit_xsize; xx++)
      glasstile_hit[yy][xx] = 0;

  if (mode == MODE_PAINT)
    glasstile_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else if (mode == MODE_FULLSCREEN)
  {
    for (y = 0; y < canvas->h; y = y + GT_SIZE)
      for (x = 0; x < canvas->w; x = x + GT_SIZE)
        do_glasstile(api, which, canvas, last, x, y);

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;

    /* FIXME: Play sfx */
  }
}

// Affect the canvas on release:
void glasstile_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void glasstile_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int y;

  if (glasstile_snd != NULL)
    Mix_FreeChunk(glasstile_snd);
  
  if (glasstile_hit != NULL)
  {
    for (y = 0; y < glasstile_hit_ysize; y++)
    {
      if (glasstile_hit[y] != NULL)
        free(glasstile_hit[y]);
    }
    free(glasstile_hit);
  }
}

// Record the color from Tux Paint:
void glasstile_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int glasstile_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void glasstile_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void glasstile_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int glasstile_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT | MODE_FULLSCREEN);
}
