/*
  distortion.c

  Distortion Magic Tool Plugin
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
  $Id: distortion.c,v 1.9 2011/11/26 22:04:50 perepujal Exp $
*/

/* Inclusion of header files: */
/* -------------------------- */

#include <stdio.h>
#include <string.h>  // For "strdup()"

#include "tp_magic_api.h"  // Tux Paint "Magic" tool API header
#include "SDL_image.h"  // For IMG_Load(), to load our PNG icon
#include "SDL_mixer.h"  // For Mix_LoadWAV(), to load our sound effects



/* Our global variables: */
/* --------------------- */

/* Sound effects: */
static Mix_Chunk * snd_effect;


/* Our local function prototypes: */
/* ------------------------------ */

// These functions are called by other functions within our plugin,
// so we provide a 'prototype' of them, so the compiler knows what
// they accept and return.  This lets us use them in other functions
// that are declared _before_ them.

Uint32 distortion_api_version(void);
int distortion_init(magic_api * api);
int distortion_get_tool_count(magic_api * api);
SDL_Surface * distortion_get_icon(magic_api * api, int which);
char * distortion_get_name(magic_api * api, int which);
char * distortion_get_description(magic_api * api, int which, int mode);
int distortion_requires_colors(magic_api * api, int which);
void distortion_shutdown(magic_api * api);

void distortion_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);
void distortion_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);

void distortion_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void distortion_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void distortion_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int distortion_modes(magic_api * api, int which);

void distortion_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);

static void distortion_line_callback(void * ptr, int which,
                           SDL_Surface * canvas, SDL_Surface * snapshot,
                           int x, int y);


/* Setup Functions: */
/* ---------------- */

Uint32 distortion_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}


// Initialization

int distortion_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname),
           "%s/sounds/magic/distortion.ogg",
	   api->data_directory);

  // Try to load the file!

  snd_effect = Mix_LoadWAV(fname);

  return(1);
}


// Report our tool count

int distortion_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}


// Load icons

SDL_Surface * distortion_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/distortion.png",
	     api->data_directory);


  // Try to load the image, and return the results to Tux Paint:

  return(IMG_Load(fname));
}


// Report our "Magic" tool names

char * distortion_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Distortion")));
}


// Report our "Magic" tool descriptions

char * distortion_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click and drag the mouse to cause distortion in your picture.")));
}

// Report whether we accept colors

int distortion_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}


// Shut down

void distortion_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Mix_FreeChunk(snd_effect);
}


/* Functions that respond to events in Tux Paint: */
/* ---------------------------------------------- */

// Affect the canvas on click:

void distortion_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect)
{
  distortion_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


// Affect the canvas on drag:

void distortion_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  api->line((void *) api, which, canvas, snapshot,
            ox, oy, x, y, 1, distortion_line_callback);


  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }


  update_rect->x = ox - 8;
  update_rect->y = oy - 8;
  update_rect->w = (x + 8) - update_rect->x;
  update_rect->h = (y + 8) - update_rect->h;


  api->playsound(snd_effect,
                 (x * 255) / canvas->w, // pan
	         255); // distance
}


// Affect the canvas on release:

void distortion_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void distortion_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}


// Our "callback" function

static void distortion_line_callback(void * ptr, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas, SDL_Surface * snapshot,
                           int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  int xx, yy;


  // This function handles both of our tools, so we need to check which
  // is being used right now.  We compare the 'which' argument that
  // Tux Paint sends to us with the values we enumerated above.

  for (yy = -8; yy < 8; yy++)
  {
    for (xx = -8; xx < 8; xx++)
    {
      if (api->in_circle(xx, yy, 8))
      {
        api->putpixel(canvas, x + xx, y + yy,
	        api->getpixel(snapshot,
			      x + xx / 2, y + yy));
      }
    }
  }
}

void distortion_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void distortion_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int distortion_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
