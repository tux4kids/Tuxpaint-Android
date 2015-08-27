/*
  snow.c

  snow, Add snow flakes or snow balls to the whole image.
  Tux Paint - A simple drawing program for children.

  Credits: Andrew Corcoran <akanewbie@gmail.com>

  Copyright (c) 2002-2007 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: June 6, 2008
  $Id: snow.c,v 1.9 2011/12/18 23:49:01 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <math.h>
#include <limits.h>
#include <time.h>

#ifndef gettext_noop
#define gettext_noop(String) String
#endif

static const int snow_AMOUNT = 400;
static const int snow_RADIUS = 5;
static SDL_Surface * snow_flake1;
static SDL_Surface * snow_flake2;

enum {
  TOOL_SNOWBALL,
  TOOL_SNOWFLAKE,
	snow_NUM_TOOLS
};

static Mix_Chunk * snow_snd_effect[snow_NUM_TOOLS];

const char * snow_snd_filenames[snow_NUM_TOOLS] = {
  "snowball.ogg",
  "snowflake.ogg",
};
const char * snow_icon_filenames[snow_NUM_TOOLS] = {
  "snowball.png",
  "snowflake.png",
};
const char * snow_names[snow_NUM_TOOLS] = {
  gettext_noop("Snow Ball"),
  gettext_noop("Snow Flake"),
};
const char * snow_descs[snow_NUM_TOOLS] = {
  gettext_noop("Click to add snow balls to your picture."),
  gettext_noop("Click to add snow flakes to your picture."),
};

Uint32 snow_api_version(void);
int snow_init(magic_api * api);
int snow_get_tool_count(magic_api * api);
SDL_Surface * snow_get_icon(magic_api * api, int which);
char * snow_get_name(magic_api * api, int which);
char * snow_get_description(magic_api * api, int which);
static void do_snow(void * ptr,SDL_Surface * canvas, SDL_Surface * last, int which, int snowAmount);
void snow_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void snow_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);

void snow_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void snow_shutdown(magic_api * api);
void snow_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int snow_requires_colors(magic_api * api, int which);
void snow_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void snow_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int snow_modes(magic_api * api, int which);
Uint32 snow_api_version(void) { return(TP_MAGIC_API_VERSION); }

//Load sounds
int snow_init(magic_api * api){

  int i;
  char fname[1024];
  srand(time(0));

  snprintf(fname, sizeof(fname), "%s/images/magic/Snow_flake4.png", api->data_directory);
  snow_flake1 = IMG_Load(fname);
  if (snow_flake1==NULL){
    return(0);
  }

  snprintf(fname, sizeof(fname), "%s/images/magic/Snow_flake5.png", api->data_directory);
  snow_flake2 = IMG_Load(fname);
  if (snow_flake2==NULL){
    return(0);
  }

  if (snow_flake2==NULL){printf("meh\n");}
  for (i = 0; i < snow_NUM_TOOLS; i++){
    snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, snow_snd_filenames[i]);
    snow_snd_effect[i] = Mix_LoadWAV(fname);
  }
  return(1);
}

int snow_get_tool_count(magic_api * api ATTRIBUTE_UNUSED){
  return(snow_NUM_TOOLS);
}

// Load our icons:
SDL_Surface * snow_get_icon(magic_api * api, int which){
  char fname[1024];
  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, snow_icon_filenames[which]);
  return(IMG_Load(fname));
}

// Return our names, localized:
char * snow_get_name(magic_api * api ATTRIBUTE_UNUSED, int which){
    return(strdup(gettext_noop(snow_names[which])));
}

// Return our descriptions, localized:
char * snow_get_description(magic_api * api ATTRIBUTE_UNUSED, int which){
  return(strdup(gettext_noop(snow_descs[which])));
}

// Do the effect:
static void do_snow(void * ptr,SDL_Surface * canvas, SDL_Surface * last, int which, int snowAmount){
	magic_api * api = (magic_api *) ptr;

  int i,x,y,centre_x,centre_y;
  Uint8 r,g,b;
  SDL_Rect dest;

  for(i=0; i<snowAmount; i++){
    centre_x = rand() % canvas->w;
    centre_y = rand() % canvas->h;
    if (which == TOOL_SNOWBALL){
      for (y = -snow_RADIUS; y < snow_RADIUS; y++){
		    for (x= -snow_RADIUS; x < snow_RADIUS; x++){
            if (api->in_circle(x ,y, snow_RADIUS)){
              SDL_GetRGB(api->getpixel(last, centre_x + x, centre_y + y), last->format, &r, &g, &b);
              api->putpixel(canvas, centre_x + x, centre_y + y, SDL_MapRGB(canvas->format, 255, 255, 255));
            }
          }  
        }
      }
    if(which == TOOL_SNOWFLAKE){
      dest.x = centre_x;
      dest.y = centre_y;
      if (rand()%2==0){
        SDL_BlitSurface(snow_flake1, NULL, canvas, &dest);
      }else {
        SDL_BlitSurface(snow_flake2, NULL, canvas, &dest);
      }
    }
  }
}

// Affect the canvas on drag:
void snow_drag(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
	          SDL_Surface * last ATTRIBUTE_UNUSED, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
		  SDL_Rect * update_rect ATTRIBUTE_UNUSED){
   // No-op
}

// Affect the canvas on click:
void snow_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect){
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
  
  do_snow(api, canvas,  last, which, snow_AMOUNT);
  api->playsound(snow_snd_effect[which], 128, 255);
}

// Affect the canvas on release:
void snow_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void snow_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
	//Clean up sounds
	int i;
	for(i=0; i<snow_NUM_TOOLS; i++){
		if(snow_snd_effect[i] != NULL){
			Mix_FreeChunk(snow_snd_effect[i]);
		}
	}
  if (snow_flake1 != NULL){
    SDL_FreeSurface(snow_flake1);
  }
  if (snow_flake2 != NULL){
    SDL_FreeSurface(snow_flake2);
  }
}

// Record the color from Tux Paint:
void snow_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int snow_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void snow_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED){
}

void snow_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int snow_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_FULLSCREEN);
}

