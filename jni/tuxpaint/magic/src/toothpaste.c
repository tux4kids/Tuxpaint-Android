/*
  toothpaste.c

  toothpaste, Add a toothpaste effect to the image
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
  $Id: toothpaste.c,v 1.7 2011/12/17 22:43:56 perepujal Exp $
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

double pi;
static Uint8 toothpaste_r, toothpaste_g, toothpaste_b;
static const int toothpaste_RADIUS = 10;
double* toothpaste_weights = NULL;

enum {
	TOOL_toothpaste,
	toothpaste_NUM_TOOLS
};

static Mix_Chunk * toothpaste_snd_effect[toothpaste_NUM_TOOLS];

const char * toothpaste_snd_filenames[toothpaste_NUM_TOOLS] = {
  "toothpaste.ogg",
};
const char * toothpaste_icon_filenames[toothpaste_NUM_TOOLS] = {
  "toothpaste.png",
};
const char * toothpaste_names[toothpaste_NUM_TOOLS] = {
  gettext_noop("Toothpaste"),
};
const char * toothpaste_descs[toothpaste_NUM_TOOLS] = {
  gettext_noop("Click and drag to squirt toothpaste onto your picture."),
};


Uint32 toothpaste_api_version(void);
int toothpaste_init(magic_api * api);
int toothpaste_get_tool_count(magic_api * api);
SDL_Surface * toothpaste_get_icon(magic_api * api, int which);
char * toothpaste_get_name(magic_api * api, int which);
char * toothpaste_get_description(magic_api * api, int which, int mode);
static void do_toothpaste(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y);
void toothpaste_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void toothpaste_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void toothpaste_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void toothpaste_shutdown(magic_api * api);
void toothpaste_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int toothpaste_requires_colors(magic_api * api, int which);
void toothpaste_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void toothpaste_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int toothpaste_modes(magic_api * api, int which);

Uint32 toothpaste_api_version(void) { return(TP_MAGIC_API_VERSION); }


int toothpaste_init(magic_api * api){

  int i;
  char fname[1024];
  int k,j;
  //Load sounds
  for (i = 0; i < toothpaste_NUM_TOOLS; i++){
    snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, toothpaste_snd_filenames[i]);
    toothpaste_snd_effect[i] = Mix_LoadWAV(fname);
  }
  
  //Set up weights
  pi = acos(0.0) * 2;
  toothpaste_weights = (double*)malloc(toothpaste_RADIUS*2 * toothpaste_RADIUS*2 * sizeof(double));
  if (toothpaste_weights == NULL){
    return(0);
  }

  for (k =  - toothpaste_RADIUS; k <  + toothpaste_RADIUS; k++){
    for (j =  - toothpaste_RADIUS; j <  + toothpaste_RADIUS; j++){
      if (api->in_circle(j , k, toothpaste_RADIUS)){
        toothpaste_weights[(k+toothpaste_RADIUS)*((toothpaste_RADIUS*2) -1)+(j+toothpaste_RADIUS)] = ((fabs(atan2((double)(j),(double)(k))))/pi);
      }
    }
  }

  return(1);
}

int toothpaste_get_tool_count(magic_api * api ATTRIBUTE_UNUSED){
  return(toothpaste_NUM_TOOLS);
}

// Load our icons:
SDL_Surface * toothpaste_get_icon(magic_api * api, int which  ATTRIBUTE_UNUSED){
  char fname[1024];
  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, toothpaste_icon_filenames[which]);
  return(IMG_Load(fname));
}

// Return our names, localized:
char * toothpaste_get_name(magic_api * api  ATTRIBUTE_UNUSED, int which){
    return(strdup(gettext_noop(toothpaste_names[which])));
}

// Return our descriptions, localized:
char * toothpaste_get_description(magic_api * api  ATTRIBUTE_UNUSED, int which, int mode  ATTRIBUTE_UNUSED){
  return(strdup(gettext_noop(toothpaste_descs[which])));
}

// Do the effect:
static void do_toothpaste(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last  ATTRIBUTE_UNUSED, int x, int y){
  magic_api * api = (magic_api *) ptr;

  int xx, yy;
  // double colr;
  float h,s,v;
  Uint8 r,g,b;

  for (yy = y - toothpaste_RADIUS; yy < y + toothpaste_RADIUS; yy++){
    for (xx = x - toothpaste_RADIUS; xx < x + toothpaste_RADIUS; xx++){
      if (api->in_circle(xx - x, yy - y, toothpaste_RADIUS) &&
	  !api->touched(xx, yy)){

        api->rgbtohsv(toothpaste_r, toothpaste_g, toothpaste_b, &h, &s, &v);
        api->hsvtorgb(h, s, toothpaste_weights[(yy-y+toothpaste_RADIUS)*((toothpaste_RADIUS*2) -1)+(xx-x+toothpaste_RADIUS)], &r, &g, &b);
        api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, r, g, b));

      }
    }
  }

}

// Affect the canvas on drag:
void toothpaste_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect){

  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1, do_toothpaste);

  api->playsound(toothpaste_snd_effect[which], (x * 255) / canvas->w, 255);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

}

// Affect the canvas on click:
void toothpaste_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect){

  toothpaste_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

// Affect the canvas on release:
void toothpaste_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void toothpaste_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
	//Clean up sounds
	int i;
	for(i=0; i<toothpaste_NUM_TOOLS; i++){
		if(toothpaste_snd_effect[i] != NULL){
			Mix_FreeChunk(toothpaste_snd_effect[i]);
		}
	}
  if (toothpaste_weights != NULL){
    free(toothpaste_weights);
    toothpaste_weights = NULL;
  }
}

// Record the color from Tux Paint:
void toothpaste_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  toothpaste_r = r;
  toothpaste_g = g;
  toothpaste_b = b;
}

// Use colors:
int toothpaste_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}


void toothpaste_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void toothpaste_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int toothpaste_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}


