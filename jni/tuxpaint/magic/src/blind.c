/*
  blind.c
//
  BLIND Magic Tools Plugin
  Tux Paint - A simple drawing program for children.

  By Pere Pujal Carabantes

  Copyright (c) 2002-2009 by Bill Kendrick and others; see AUTHORS.txt
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
*/

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

int BLIND_RADIUS = 16;
int BLIND_OPAQUE = 20;
int BLIND_THICKNESS = 30;
int blind_side; /* 0 top, 1 left, 2 bottom, 3 right */

static Uint8 blind_r, blind_g, blind_b, blind_light;
enum blind_sides{
  BLIND_SIDE_TOP,
  BLIND_SIDE_LEFT,
  BLIND_SIDE_BOTTOM,
  BLIND_SIDE_RIGHT};

enum blind_tools{
  BLIND_TOOL_BLIND,
  BLIND_NUMTOOLS};

Mix_Chunk * blind_snd;

// Prototypes
Uint32 blind_api_version(void);
void blind_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int blind_init(magic_api * api);
int blind_get_tool_count(magic_api * api);
SDL_Surface * blind_get_icon(magic_api * api, int which);
char * blind_get_name(magic_api * api, int which);
char * blind_get_description(magic_api * api, int which, int mode);
int blind_requires_colors(magic_api * api, int which);
void blind_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);
void blind_shutdown(magic_api * api);
void blind_paint_blind(void * ptr_to_api, int which_tool,
               SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void blind_drag(magic_api * api, int which, SDL_Surface * canvas,
		SDL_Surface * snapshot, int ox, int oy, int x, int y,
		SDL_Rect * update_rect);
void blind_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void blind_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void blind_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int blind_modes(magic_api * api, int which);

//				Housekeeping functions

Uint32 blind_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}

void blind_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)	//get the colors from API and store it in structure
{
  blind_r = r;
  blind_g = g;
  blind_b = b;
}

int blind_init(magic_api * api)
{
  char fname[1024];

    snprintf(fname, sizeof(fname), "%s/sounds/magic/blind.ogg", api->data_directory);
    blind_snd = Mix_LoadWAV(fname);

  return(1);
}

int blind_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return BLIND_NUMTOOLS;
}

SDL_Surface * blind_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/blind.png",
	     api->data_directory);

  return(IMG_Load(fname));
}

char * blind_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Blind")); 
}

char * blind_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED) 
{ 
    return strdup(gettext_noop("Click towards the edge of your picture to pull window blinds over it. Move perpendicularly to open or close the blinds."));
}

int blind_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void blind_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void blind_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
      Mix_FreeChunk(blind_snd);
}

// Interactivity functions

void blind_paint_blind(void * ptr_to_api, int which_tool ATTRIBUTE_UNUSED,
               SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api * api = (magic_api *) ptr_to_api;

	api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, (blind_r + blind_light) / 2, (blind_g + blind_light) / 2, (blind_b + blind_light) / 2));
}

/* void blind_do_blind(void * ptr_to_api, int which_tool,
               SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y)
{
	magic_api * api = (magic_api *) ptr_to_api;
	
	api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, 128, 128, 165));
	//api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, 0, 0, 255));
}

*/
void blind_drag(magic_api * api, int which, SDL_Surface * canvas,
		SDL_Surface * snapshot, int ox, int oy, int x, int y,
		SDL_Rect * update_rect)
{
  int opaque;
 
  SDL_BlitSurface(snapshot, NULL, canvas, NULL);
  switch (blind_side)
    {
      int i, j;
    case BLIND_SIDE_TOP:
      opaque = max((x * BLIND_THICKNESS) / canvas->w + 2, 2);
      for (i = y;i >= 0; i -= BLIND_THICKNESS)
	{
	  blind_light = 255;
	  for (j=i; j > i - opaque/2; j--)
	    {
	      api->line(api, which, canvas, snapshot, 0, j, canvas->w, j, 1, blind_paint_blind);
	      blind_light -=20;
	    }
	  for (j = i - opaque/2; j > i - opaque; j--)
	    {
	      api->line(api, which, canvas, snapshot, 0, j, canvas->w, j, 1, blind_paint_blind);
	      blind_light +=20;
	    }
	}
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = max(oy, y);
      api->playsound(blind_snd, 128, 255);
      break;

    case BLIND_SIDE_BOTTOM:
      opaque = max((x * BLIND_THICKNESS) / canvas->w + 2, 2);
      for (i = y; i <= canvas->h; i += BLIND_THICKNESS)
	{
	  blind_light = 255;
	  for (j = i; j < i + opaque / 2; j++)
	    {
	      api->line(api, which, canvas, snapshot, 0, j, canvas->w, j, 1, blind_paint_blind);
	      blind_light -= 20;
	    }
	  for (j = i + opaque / 2; j < i + opaque; j ++)
	    {
	      api->line(api, which, canvas, snapshot, 0, j, canvas->w, j, 1, blind_paint_blind);
	      blind_light += 20;
	    }
	}

      update_rect->x = 0;
      update_rect->y = min(oy, y);
      update_rect->w = canvas->w;
      update_rect->h = canvas->h - update_rect->y;
      api->playsound(blind_snd, 128, 255);
      break;

    case BLIND_SIDE_RIGHT:
      opaque = max((y * BLIND_THICKNESS) / canvas->h + 2, 2);
      for (i = x; i <= canvas->w; i += BLIND_THICKNESS)
	{
	  blind_light = 255;
	  for (j = i; j < i + opaque / 2; j++)
	    {
	      api->line(api, which, canvas, snapshot, j, 0, j, canvas->h, 1, blind_paint_blind);
	      blind_light -= 20;
	    }
	  for (j = i + opaque / 2; j < i + opaque; j ++)
	    {
	      api->line(api, which, canvas, snapshot, j, 0, j, canvas->h, 1, blind_paint_blind);
	      blind_light += 20;
	    }
	}

      update_rect->x = min(ox, x);
      update_rect->y = 0;
      update_rect->w = canvas->w - update_rect->x;
      update_rect->h = canvas->h;
      api->playsound(blind_snd, (x * 255) / canvas->w, 255);
      break;

    case BLIND_SIDE_LEFT:
      opaque = max((y * BLIND_THICKNESS) / canvas->h + 2, 2);
      for (i = x; i >= 0; i -= BLIND_THICKNESS)
	{
	  blind_light = 255;
	  for (j=i; j > i - opaque/2; j--)
	    {
	      api->line(api, which, canvas, snapshot, j, 0, j, canvas->h, 1, blind_paint_blind);
	      blind_light -=20;
	    }
	  for (j = i - opaque/2; j > i - opaque; j--)
	    {
	      api->line(api, which, canvas, snapshot, j, 0, j, canvas->h, 1, blind_paint_blind);
	      blind_light +=20;
	    }
	}
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = max(ox, x);
      update_rect->h = canvas->h;
      api->playsound(blind_snd, (x * 255) / canvas->w, 255);
      break;

    }
}

void blind_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
      if (y < canvas->h / 2)

	{
	  if (x < y) blind_side = 1; /* left */
	  else if (canvas->w - x < y) blind_side = 3; /* right */
	  else blind_side = 0; /* top */
	}
      else
	{
	  if (x < canvas->h - y) blind_side = 1; /* left */
	  else if (canvas->w - x < canvas->h - y) blind_side = 3; /* right */
	  else blind_side = 2; /* bottom */
	}

	blind_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void blind_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{	
	
}

void blind_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
	
}

int blind_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_FULLSCREEN | MODE_PAINT);
}
