/*
  mirror_flip.c

  Mirror and Flip Magic Tools Plugin
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
  $Id: mirror_flip.c,v 1.13 2011/11/26 22:04:50 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* What tools we contain: */

enum {
  TOOL_MIRROR,
  TOOL_FLIP,
  NUM_TOOLS
};

static Mix_Chunk * snd_effects[NUM_TOOLS];

/* Prototypes */
int mirror_flip_init(magic_api *);
Uint32 mirror_flip_api_version(void);
int mirror_flip_get_tool_count(magic_api *);
SDL_Surface * mirror_flip_get_icon(magic_api *, int);
char * mirror_flip_get_name(magic_api *, int);
char * mirror_flip_get_description(magic_api *, int, int);
void mirror_flip_drag(magic_api *, int, SDL_Surface *,
	SDL_Surface *, int, int, int, int,
	SDL_Rect *);
void mirror_flip_release(magic_api *, int, SDL_Surface *,
	SDL_Surface *, int, int, int, int,
	SDL_Rect *);
void mirror_flip_click(magic_api *, int, int,
	SDL_Surface *, SDL_Surface *,
	int, int,
	SDL_Rect *);
void mirror_flip_shutdown(magic_api *);
void mirror_flip_set_color(magic_api *, Uint8, Uint8, Uint8);
int mirror_flip_requires_colors(magic_api *, int);
void mirror_flip_switchin(magic_api *, int, int, SDL_Surface *);
void mirror_flip_switchout(magic_api *, int, int, SDL_Surface *);
int mirror_flip_modes(magic_api *, int);

// No setup required:
int mirror_flip_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/mirror.wav",
           api->data_directory);
  snd_effects[TOOL_MIRROR] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/sounds/magic/flip.wav",
           api->data_directory);
  snd_effects[TOOL_FLIP] = Mix_LoadWAV(fname);

  return(1);
}

Uint32 mirror_flip_api_version(void) { return(TP_MAGIC_API_VERSION); }

// We have multiple tools:
int mirror_flip_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(NUM_TOOLS);
}

// Load our icons:
SDL_Surface * mirror_flip_get_icon(magic_api * api, int which)
{
  char fname[1024];

  if (which == TOOL_MIRROR)
  {
    snprintf(fname, sizeof(fname), "%s/images/magic/mirror.png",
	     api->data_directory);
  }
  else if (which == TOOL_FLIP)
  {
    snprintf(fname, sizeof(fname), "%s/images/magic/flip.png",
	     api->data_directory);
  }

  return(IMG_Load(fname));
}

// Return our names, localized:
char * mirror_flip_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == TOOL_MIRROR)
    return(strdup(gettext_noop("Mirror")));
  else if (which == TOOL_FLIP)
    return(strdup(gettext_noop("Flip")));

  return(NULL);
}

// Return our descriptions, localized:
char * mirror_flip_get_description(magic_api * api ATTRIBUTE_UNUSED,
	  int which, int mode ATTRIBUTE_UNUSED)
{
  if (which == TOOL_MIRROR)
    return(strdup(
           gettext_noop("Click to make a mirror image.")));
  else
    return(strdup(
           gettext_noop("Click to flip the picture upside-down.")));

  return(NULL);
}

// We affect the whole canvas, so only do things on click, not drag:
void mirror_flip_drag(magic_api * api ATTRIBUTE_UNUSED, 
	  int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
	  SDL_Surface * last ATTRIBUTE_UNUSED, 
	  int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED,
	  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
	  SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  // No-op
}

void mirror_flip_release(magic_api * api ATTRIBUTE_UNUSED,
	  int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
	  SDL_Surface * last ATTRIBUTE_UNUSED, 
	  int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED,
	  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
	  SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  // No-op
}

// Affect the canvas on click:
void mirror_flip_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	   SDL_Surface * canvas, SDL_Surface * last,
	   int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
	   SDL_Rect * update_rect)
{
  int xx, yy;
  SDL_Rect src, dest;

  if (which == TOOL_MIRROR)
  {
    for (xx = 0; xx < canvas->w; xx++)
    {
      src.x = xx;
      src.y = 0;
      src.w = 1;
      src.h = canvas->h;

      dest.x = canvas->w - xx - 1;
      dest.y = 0;

      SDL_BlitSurface(last, &src, canvas, &dest);
    }

    api->special_notify(SPECIAL_MIRROR);
  }
  else if (which == TOOL_FLIP)
  {
    for (yy = 0; yy < canvas->h; yy++)
    {
      src.x = 0;
      src.y = yy;
      src.w = canvas->w;
      src.h = 1;

      dest.x = 0;
      dest.y = canvas->h - yy - 1;

      SDL_BlitSurface(last, &src, canvas, &dest);
    }

    api->special_notify(SPECIAL_FLIP);
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(snd_effects[which], 128, 255);
}

// No setup happened:
void mirror_flip_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (snd_effects[0] != NULL)
    Mix_FreeChunk(snd_effects[0]);
  if (snd_effects[1] != NULL)
    Mix_FreeChunk(snd_effects[1]);
}

// We don't use colors:
void mirror_flip_set_color(magic_api * api ATTRIBUTE_UNUSED, 
	  Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, 
	  Uint8 b ATTRIBUTE_UNUSED)
{
}

// We don't use colors:
int mirror_flip_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
	  int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void mirror_flip_switchin(magic_api * api ATTRIBUTE_UNUSED, 
	  int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, 
	  SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void mirror_flip_switchout(magic_api * api ATTRIBUTE_UNUSED, 
	  int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, 
	  SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int mirror_flip_modes(magic_api * api ATTRIBUTE_UNUSED, 
	  int which ATTRIBUTE_UNUSED)
{
  return(MODE_FULLSCREEN);
}
