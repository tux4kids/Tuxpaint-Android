/*
  waves.c
  Credits: Bill Kendrick <bill@newbreedsoftware.com> (idea & Waves tool), Adam Rakowski <foo-script@o2.pl> (Wavelets tool)
  Waves Magic Tool Plugin
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
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"


/* Our globals: */

static Mix_Chunk * waves_snd[2];

/* Local function prototypes: */

Uint32 waves_api_version(void);
int waves_init(magic_api * api);
int waves_get_tool_count(magic_api * api);
SDL_Surface * waves_get_icon(magic_api * api, int which);
char * waves_get_name(magic_api * api, int which);
char * waves_get_description(magic_api * api, int which, int mode);
void waves_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		      SDL_Rect * update_rect);
void waves_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void waves_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void waves_shutdown(magic_api * api);
void waves_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int waves_requires_colors(magic_api * api, int which);
void waves_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void waves_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int waves_modes(magic_api * api, int which);

Uint32 waves_api_version(void) { return(TP_MAGIC_API_VERSION); }


// No setup required:
int waves_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/waves.ogg",
	    api->data_directory);
  waves_snd[0] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/sounds/magic/wavelet.ogg",
	    api->data_directory);
  waves_snd[1] = Mix_LoadWAV(fname);


  return(1);
}

// We have multiple tools:
int waves_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 2;
}

// Load our icons:
SDL_Surface * waves_get_icon(magic_api * api, int which)
{
  char fname[1024];

  if (!which) snprintf(fname, sizeof(fname), "%s/images/magic/waves.png", api->data_directory);
  else snprintf(fname, sizeof(fname), "%s/images/magic/wavelet.png", api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * waves_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (!which) return(strdup(gettext_noop("Waves")));
  else return strdup(gettext_noop("Wavelets"));
}

// Return our descriptions, localized:
char * waves_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  if (!which)
	return(strdup(gettext_noop("Click to make the picture horizontally wavy. Click toward the top for shorter waves, the bottom for taller waves, the left for small waves, and the right for long waves.")));
  return strdup(gettext_noop("Click to make the picture vertically wavy. Click toward the top for shorter waves, the bottom for taller waves, the left for small waves, and the right for long waves."));
}	


void waves_drag(magic_api * api ATTRIBUTE_UNUSED, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y,
		  SDL_Rect * update_rect)
{
  int xx, yy;
  SDL_Rect src, dest;
  int width;
  int height;

  SDL_BlitSurface(last, NULL, canvas, NULL);
 
  if (which==0)
  {
	  //waves effect
	  width = ((x * 10) / canvas->w) + 10;
	  height = ((canvas->h - y) / 10) + 1;
	  
	  for (yy = 0; yy < canvas->h; yy++)
	  {
	    xx = sin((yy * height) * M_PI / 180.0) * width; 

	    src.x = 0;
	    src.y = yy;
	    src.w = canvas->w;
	    src.h = 1;

	    dest.x = xx;
	    dest.y = yy;

	    SDL_BlitSurface(last, &src, canvas, &dest);
	  }
  }
  else
  {
	  width = ((x * 10) / canvas->w) + 10;
	  height = ((canvas->h - y) / 10) + 1;
	  
	  for (xx = 0; xx < canvas->w; xx++)
	  {
	    yy = sin((xx * height) * M_PI / 180.0) * width; 

	    src.x = xx;
	    src.y = 0;
	    src.w = 1;
	    src.h = canvas->h;

	    dest.x = xx;
	    dest.y = yy;

	    SDL_BlitSurface(last, &src, canvas, &dest);
	  }
  }
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

// Affect the canvas on click:
void waves_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  waves_drag(api, which, canvas, last, x, y, x, y, update_rect);
  api->playsound(waves_snd[which], 128, 255);
}

// Affect the canvas on release:
void waves_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void waves_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (waves_snd[0] != NULL)
    Mix_FreeChunk(waves_snd[0]);
  if (waves_snd[1] != NULL)
    Mix_FreeChunk(waves_snd[1]);
}

// Record the color from Tux Paint:
void waves_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int waves_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void waves_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void waves_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int waves_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
