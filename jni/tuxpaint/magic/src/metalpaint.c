/*
  metalpaint.c

  Metal Paint Magic Tool Plugin
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
  $Id: metalpaint.c,v 1.10 2011/11/26 22:04:50 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

static Mix_Chunk * metalpaint_snd;
static Uint8 metalpaint_r, metalpaint_g, metalpaint_b;

Uint32 metalpaint_api_version(void);
int metalpaint_init(magic_api * api);
int metalpaint_get_tool_count(magic_api * api);
SDL_Surface * metalpaint_get_icon(magic_api * api, int which);
char * metalpaint_get_name(magic_api * api, int which);
char * metalpaint_get_description(magic_api * api, int which, int mode);
static void do_metalpaint(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y);
void metalpaint_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void metalpaint_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void metalpaint_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void metalpaint_shutdown(magic_api * api);
void metalpaint_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int metalpaint_requires_colors(magic_api * api, int which);
void metalpaint_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void metalpaint_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int metalpaint_modes(magic_api * api, int which);


Uint32 metalpaint_api_version(void) { return(TP_MAGIC_API_VERSION); }


// No setup required:
int metalpaint_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/metalpaint.wav",
	    api->data_directory);
  metalpaint_snd = Mix_LoadWAV(fname);

  return(1);
}

// We have multiple tools:
int metalpaint_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * metalpaint_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/metalpaint.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * metalpaint_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Metal Paint")));
}

// Return our descriptions, localized:
char * metalpaint_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click and drag the mouse to paint with a metallic color.")));
}

#define METALPAINT_CYCLE 32

/* Based on 'Golden' gradient in The GIMP: */

static int metalpaint_gradient[METALPAINT_CYCLE] = {
   56,  64,  73,  83,  93, 102, 113, 123,
  139, 154, 168, 180, 185, 189, 183, 174,
  164, 152, 142, 135, 129, 138, 149, 158,
  166, 163, 158, 149, 140, 122, 103, 82
};

// Do the effect:

static void do_metalpaint(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
                int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  int xx, yy;
  int n;
  Uint8 r, g, b;

  for (yy = -8; yy < 8; yy++)
  {
    for (xx = -8; xx < 8; xx++)
    {
      n = metalpaint_gradient[((x + xx + y + yy) / 4) % METALPAINT_CYCLE];

      r = (metalpaint_r * n) / 255;
      g = (metalpaint_g * n) / 255;
      b = (metalpaint_b * n) / 255;

      api->putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format, r, g, b));
    }
  }
}

// Affect the canvas on drag:
void metalpaint_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1, do_metalpaint);

  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }

  update_rect->x = ox - 8;
  update_rect->y = oy - 8;
  update_rect->w = (x + 8) - update_rect->x;
  update_rect->h = (y + 8) - update_rect->h;

  api->playsound(metalpaint_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void metalpaint_click(magic_api * api, int which, int mode  ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  metalpaint_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

// Affect the canvas on release:
void metalpaint_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void metalpaint_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (metalpaint_snd != NULL)
    Mix_FreeChunk(metalpaint_snd);
}

// Record the color from Tux Paint:
void metalpaint_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  metalpaint_r = min(255, r + 64);
  metalpaint_g = min(255, g + 64);
  metalpaint_b = min(255, b + 64);
}

// Use colors:
int metalpaint_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void metalpaint_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void metalpaint_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int metalpaint_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
