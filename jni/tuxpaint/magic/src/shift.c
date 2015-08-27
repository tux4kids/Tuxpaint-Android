/*
  shift.c

  Shift Magic Tool Plugin
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
  $Id: shift.c,v 1.12 2011/05/10 12:50:44 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include "math.h"

/* Our globals: */

static int shift_x, shift_y;
static Mix_Chunk * shift_snd;


/* Local function prototypes: */

static void shift_doit(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect, int crosshairs);
Uint32 shift_api_version(void);
int shift_init(magic_api * api);
int shift_get_tool_count(magic_api * api);
SDL_Surface * shift_get_icon(magic_api * api, int which);
char * shift_get_name(magic_api * api, int which);
char * shift_get_description(magic_api * api, int which, int mode);
void shift_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void shift_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void shift_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void shift_shutdown(magic_api * api);
void shift_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int shift_requires_colors(magic_api * api, int which);

void shift_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void shift_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int shift_modes(magic_api * api, int which);



Uint32 shift_api_version(void) { return(TP_MAGIC_API_VERSION); }


// No setup required:
int shift_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/shift.ogg",
	    api->data_directory);
  shift_snd = Mix_LoadWAV(fname);

  return(1);
}

// We have multiple tools:
int shift_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * shift_get_icon(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/shift.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * shift_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Shift")));
}

// Return our descriptions, localized:
char * shift_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click and drag to shift your picture around on the canvas.")));
}


// Affect the canvas on drag:
void shift_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  if (ox == x && oy == y)
    return; /* No-op */

  shift_doit(api, which, canvas, last, ox, oy, x, y, update_rect, 1);
}

static void shift_doit(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas,
	          SDL_Surface * last, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y,
		  SDL_Rect * update_rect, int crosshairs)
{
  SDL_Rect dest;
  int dx, dy;



  dx = x - shift_x;
  dy = y - shift_y;

  while (dx < -canvas->w)
    dx += canvas->w;
  while (dx > canvas->w)
    dx -= canvas->w;

  while (dy < -canvas->h)
    dy += canvas->h;
  while (dy > canvas->h)
    dy -= canvas->h;


  /* Center */

  dest.x = dx;
  dest.y = dy;

  SDL_BlitSurface(last, NULL, canvas, &dest);


  if (dy > 0)
  {
    if (dx > 0)
    {
      /* Top Left */

      dest.x = dx - canvas->w;
      dest.y = dy - canvas->h;

      SDL_BlitSurface(last, NULL, canvas, &dest);
    }


    /* Top */

    dest.x = dx;
    dest.y = dy - canvas->h;

    SDL_BlitSurface(last, NULL, canvas, &dest);


    if (dx < 0)
    {
      /* Top Right */

      dest.x = dx + canvas->w;
      dest.y = dy - canvas->h;

      SDL_BlitSurface(last, NULL, canvas, &dest);
    }
  }


  if (dx > 0)
  {
    /* Left */

    dest.x = dx - canvas->w;
    dest.y = dy;

    SDL_BlitSurface(last, NULL, canvas, &dest);
  }

  if (dx < 0)
  {
    /* Right */

    dest.x = dx + canvas->w;
    dest.y = dy;

    SDL_BlitSurface(last, NULL, canvas, &dest);
  }


  if (dy < 0)
  {
    if (dx > 0)
    {
      /* Bottom Left */

      dest.x = dx - canvas->w;
      dest.y = dy + canvas->h;

      SDL_BlitSurface(last, NULL, canvas, &dest);
    }


    /* Bottom */

    dest.x = dx;
    dest.y = dy + canvas->h;

    SDL_BlitSurface(last, NULL, canvas, &dest);


    if (dx < 0)
    {
      /* Bottom Right */

      dest.x = dx + canvas->w;
      dest.y = dy + canvas->h;

      SDL_BlitSurface(last, NULL, canvas, &dest);
    }
  }


  if (crosshairs)
  {
    dest.x = (canvas->w / 2) - 1;
    dest.y = 0;
    dest.w = 3;
    dest.h = canvas->h;

    SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, 255, 255, 255));

    dest.x = 0;
    dest.y = (canvas->h / 2) - 1;
    dest.w = canvas->w;
    dest.h = 3;

    SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, 255, 255, 255));


    dest.x = canvas->w / 2;
    dest.y = 0;
    dest.w = 1;
    dest.h = canvas->h;

    SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, 0, 0, 0));

    dest.x = 0;
    dest.y = canvas->h / 2;
    dest.w = canvas->w;
    dest.h = 1;

    SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, 0, 0, 0));
  }


  /* Update everything! */

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(shift_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void shift_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  shift_x = x;
  shift_y = y;
  
  shift_doit(api, which, canvas, last, x, y, x, y, update_rect, 1);
}

// Affect the canvas on release:
void shift_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  shift_doit(api, which, canvas, last, x, y, x, y, update_rect, 0);
  api->stopsound();
}


// No setup happened:
void shift_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (shift_snd != NULL)
    Mix_FreeChunk(shift_snd);
}

// Record the color from Tux Paint:
void shift_set_color(magic_api * api ATTRIBUTE_UNUSED,
		     Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int shift_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void shift_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void shift_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int shift_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT_WITH_PREVIEW);
}
