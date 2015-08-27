/*
  foam.c

  Foam Magic Tool Plugin
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

  Last updated: July 8, 2008
  $Id: foam.c,v 1.11 2011/11/26 22:04:50 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk * foam_snd;
static Uint8 foam_r, foam_g, foam_b;
static int foam_mask_w, foam_mask_h;
static int * foam_mask, * foam_mask_tmp;
static SDL_Surface * foam_7, * foam_5, * foam_3, * foam_1;

Uint32 foam_api_version(void);
int foam_init(magic_api * api);
char * foam_get_description(magic_api * api, int which, int mode);
void foam_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void foam_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void foam_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
SDL_Surface * foam_get_icon(magic_api * api, int which);
char * foam_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED);
void foam_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void foam_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void foam_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void foam_shutdown(magic_api * api);
int foam_get_tool_count(magic_api * api);
int foam_modes(magic_api * api, int which);
int foam_requires_colors(magic_api * api, int which);

#define FOAM_PROP 8
#define FOAM_RADIUS 3

Uint32 foam_api_version(void) { return(TP_MAGIC_API_VERSION); }


// No setup required:
int foam_init(magic_api * api)
{
  char fname[1024];
  SDL_Surface * foam_data;

  snprintf(fname, sizeof(fname), "%s/sounds/magic/foam.ogg",
	    api->data_directory);
  foam_snd = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/images/magic/foam_data.png",
	    api->data_directory);
  foam_data = IMG_Load(fname);

  foam_7 = api->scale(foam_data, ((api->canvas_w / FOAM_PROP) * 4) / 4,
				 ((api->canvas_h / FOAM_PROP) * 4) / 4, 0);
  foam_5 = api->scale(foam_data, ((api->canvas_w / FOAM_PROP) * 3) / 4,
				 ((api->canvas_h / FOAM_PROP) * 3) / 4, 0);
  foam_3 = api->scale(foam_data, ((api->canvas_w / FOAM_PROP) * 2) / 4,
				 ((api->canvas_h / FOAM_PROP) * 2) / 4, 0);
  foam_1 = api->scale(foam_data, ((api->canvas_w / FOAM_PROP) * 1) / 4,
				 ((api->canvas_h / FOAM_PROP) * 1) / 4, 0);

  SDL_FreeSurface(foam_data);
  
  return(1);
}

// We have multiple tools:
int foam_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * foam_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/foam.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * foam_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Foam")));
}

// Return our descriptions, localized:
char * foam_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click and drag the mouse to cover an area with foamy bubbles.")));
}

// Do the effect:

static void do_foam(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  int xx, yy, nx, ny;
  /* SDL_Rect dest; */

  for (yy = -FOAM_RADIUS; yy < FOAM_RADIUS; yy++)
  {
    for (xx = -FOAM_RADIUS; xx < FOAM_RADIUS; xx++)
    {
      if (api->in_circle(xx, yy, FOAM_RADIUS))
      {
        nx = (x / FOAM_PROP) + xx;
        ny = (y / FOAM_PROP) + yy;

        if (nx >= 0 && ny >= 0 &&
	    nx < foam_mask_w &&
	    ny < foam_mask_h)
        {
          foam_mask[ny * foam_mask_w + nx] = 1;
        }
      }
    }
  }
}

// Affect the canvas on drag:
void foam_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1, do_foam);

  foam_release(api, which, canvas, last, x, y, update_rect);

/* FIXME */
  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(foam_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void foam_click(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect)
{
  int i;

  if (foam_mask == NULL)
  {
    foam_mask_w = canvas->w / FOAM_PROP;
    foam_mask_h = canvas->h / FOAM_PROP;

    foam_mask = (int *) malloc(sizeof(int) * (foam_mask_w * foam_mask_h));
    foam_mask_tmp = (int *) malloc(sizeof(int) * (foam_mask_w * foam_mask_h));
  }

  for (i = 0; i < foam_mask_w * foam_mask_h; i++)
    foam_mask[i] = 0;

  foam_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

static int foam_mask_test(int r, int x, int y)
{
  int xx, yy;
  int tot, bub_r;

  tot = 0;

  for (yy = 0; yy < r; yy++)
  {
    for (xx = 0; xx < r; xx++)
    {
      if (x + xx < foam_mask_w && y + yy < foam_mask_h)
      {
        bub_r = foam_mask[((y + yy) * foam_mask_w) + (x + xx)];
        tot = tot + bub_r;
      }
    }
  } 

  return(tot);
}

// Affect the canvas on release:
void foam_release(magic_api * api ATTRIBUTE_UNUSED ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect)
{
  int xx, yy;
  int changes, max_iters;
  SDL_Rect dest;
  int n;
  SDL_Surface * img;

  SDL_BlitSurface(last, NULL, canvas, NULL);

  memcpy(foam_mask_tmp, foam_mask, (sizeof(int) * (foam_mask_w * foam_mask_h)));


  max_iters = 2;

  do
  {
    changes = 0;
    max_iters--;

    for (yy = 0; yy < foam_mask_h - 7; yy++)
    {
      for (xx = 0; xx < foam_mask_w - 7; xx++)
      {
        if (foam_mask_test(7, xx, yy) >= 40)
        {
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 0)] = 7;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 2)] = 1;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 4)] = 1;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 5)] = 2;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 6)] = 0;

          foam_mask[((yy + 1) * foam_mask_w) + (xx + 0)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 1)] = 1;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 4)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 5)] = 2;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 6)] = 0;

          foam_mask[((yy + 2) * foam_mask_w) + (xx + 0)] = 1;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 4)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 5)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 6)] = 1;

          foam_mask[((yy + 3) * foam_mask_w) + (xx + 0)] = 0;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 1)] = 1;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 4)] = 0;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 5)] = 0;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 6)] = 0;

          foam_mask[((yy + 4) * foam_mask_w) + (xx + 0)] = 1;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 4)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 5)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 6)] = 1;

          foam_mask[((yy + 5) * foam_mask_w) + (xx + 0)] = 2;
          foam_mask[((yy + 5) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 5) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 5) * foam_mask_w) + (xx + 3)] = 7;
          foam_mask[((yy + 5) * foam_mask_w) + (xx + 4)] = 0;
          foam_mask[((yy + 5) * foam_mask_w) + (xx + 5)] = 3;
          foam_mask[((yy + 5) * foam_mask_w) + (xx + 6)] = 0;

          foam_mask[((yy + 6) * foam_mask_w) + (xx + 0)] = 0;
          foam_mask[((yy + 6) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 6) * foam_mask_w) + (xx + 2)] = 1;
          foam_mask[((yy + 6) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 6) * foam_mask_w) + (xx + 4)] = 1;
          foam_mask[((yy + 6) * foam_mask_w) + (xx + 5)] = 0;
          foam_mask[((yy + 6) * foam_mask_w) + (xx + 6)] = 2;

          changes = 1;
        }
        else if (foam_mask_test(5, xx, yy) >= 30)
        {
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 0)] = 2;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 1)] = 1;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 3)] = 1;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 4)] = 2;

          foam_mask[((yy + 1) * foam_mask_w) + (xx + 0)] = 1;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 4)] = 1;

          foam_mask[((yy + 2) * foam_mask_w) + (xx + 0)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 2)] = 5;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 3)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 4)] = 0;

          foam_mask[((yy + 3) * foam_mask_w) + (xx + 0)] = 2;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 2)] = 1;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 3)] = 2;
          foam_mask[((yy + 3) * foam_mask_w) + (xx + 4)] = 0;

          foam_mask[((yy + 4) * foam_mask_w) + (xx + 0)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 1)] = 1;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 2)] = 0;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 3)] = 1;
          foam_mask[((yy + 4) * foam_mask_w) + (xx + 4)] = 0;

          changes = 1;
        }
        else if (foam_mask_test(3, xx, yy) >= 8)
        {
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 0)] = 1;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 0) * foam_mask_w) + (xx + 2)] = 1;

          foam_mask[((yy + 1) * foam_mask_w) + (xx + 0)] = 0;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 1)] = 3;
          foam_mask[((yy + 1) * foam_mask_w) + (xx + 2)] = 0;

          foam_mask[((yy + 2) * foam_mask_w) + (xx + 0)] = 1;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 1)] = 0;
          foam_mask[((yy + 2) * foam_mask_w) + (xx + 2)] = 1;

          changes = 1;
        }
      }
    }
  }
  while (changes && max_iters > 0);


  for (yy = 0; yy < foam_mask_h; yy++)
  {
    for (xx = 0; xx < foam_mask_w; xx++)
    {
      n = foam_mask[yy * foam_mask_w + xx];

      img = NULL;

      
      if (n == 1)
        img = foam_1;
      else if (n == 3)
        img = foam_3;
      else if (n == 5)
        img = foam_5;
      else if (n == 7)
        img = foam_7;

      if (img != NULL)
      {
        dest.x = (xx * FOAM_PROP) - (img->w / 2) + ((rand() % 15) - 7);
        dest.y = (yy * FOAM_PROP) - (img->h / 2) + ((rand() % 15) - 7);

        SDL_BlitSurface(img, NULL, canvas, &dest);
      }
    }
  }
  
  memcpy(foam_mask, foam_mask_tmp, (sizeof(int) * (foam_mask_w * foam_mask_h)));

  
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

// No setup happened:
void foam_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (foam_snd != NULL)
    Mix_FreeChunk(foam_snd);

  if (foam_mask != NULL)
    free(foam_mask);
 
  if (foam_1 != NULL)
    SDL_FreeSurface(foam_1);
  if (foam_3 != NULL)
    SDL_FreeSurface(foam_3);
  if (foam_5 != NULL)
    SDL_FreeSurface(foam_5);
  if (foam_7 != NULL)
    SDL_FreeSurface(foam_7);
}

// Record the color from Tux Paint:
void foam_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  foam_r = r;
  foam_g = g;
  foam_b = b;
}

// Use colors:
int foam_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;  /* FIXME: Would be nice to tint the bubbles */
}

void foam_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,  SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void foam_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int foam_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
