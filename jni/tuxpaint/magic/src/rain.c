/*
  rain.c

  rain, Add a rain effect to the image
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

  Last updated: October 4, 2008
  $Id: rain.c,v 1.9 2011/12/19 09:32:26 perepujal Exp $
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

void rain_click(magic_api *, int, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);

static const int rain_SIZE = 30;
static const int rain_AMOUNT = 200;

enum {
	TOOL_rain,
	rain_NUM_TOOLS
};

static Mix_Chunk * rain_snd_effect[rain_NUM_TOOLS];

const char * rain_snd_filenames[rain_NUM_TOOLS] = {
  "rain.ogg",
};
const char * rain_icon_filenames[rain_NUM_TOOLS] = {
  "rain.png",
};
const char * rain_names[rain_NUM_TOOLS] = {
  gettext_noop("Rain"),
};
const char * rain_descs[rain_NUM_TOOLS][2] = {
  {gettext_noop("Click to place a rain drop onto your picture."),
    gettext_noop("Click to cover your picture with rain drops."),},
};

Uint32 rain_api_version(void);
int rain_init(magic_api * api);
int rain_get_tool_count(magic_api * api);
SDL_Surface * rain_get_icon(magic_api * api, int which);
char * rain_get_name(magic_api * api, int which);
char * rain_get_description(magic_api * api, int which, int mode);
static void do_rain_drop(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y);
static void rain_linecb(void * ptr, int which,
                        SDL_Surface * canvas, SDL_Surface * last,
                        int x, int y);
void rain_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void rain_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void rain_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void rain_shutdown(magic_api * api);
void rain_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int rain_requires_colors(magic_api * api, int which);
void rain_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void rain_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int rain_modes(magic_api * api, int which);

Uint32 rain_api_version(void) { return(TP_MAGIC_API_VERSION); }

//Checks if a a pixel is inside a raindrop shape centered on the origin
static int rain_inRainShape(double x, double y, double r){
  if ( sqrt( x*x + y*y ) < ( r * pow( cos( atan2(x,y) ), 10.0) ) ){
    return 1;
  }
  return 0;
}

int rain_init(magic_api * api){

  int i;
  char fname[1024];
  //Load sounds
  for (i = 0; i < rain_NUM_TOOLS; i++){
    snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, rain_snd_filenames[i]);
    rain_snd_effect[i] = Mix_LoadWAV(fname);
  }

  return(1);
}

int rain_get_tool_count(magic_api * api ATTRIBUTE_UNUSED){
  return(rain_NUM_TOOLS);
}

// Load our icons:
SDL_Surface * rain_get_icon(magic_api * api, int which){
  char fname[1024];
  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, rain_icon_filenames[which]);
  return(IMG_Load(fname));
}

// Return our names, localized:
char * rain_get_name(magic_api * api ATTRIBUTE_UNUSED, int which){
    return(strdup(gettext_noop(rain_names[which])));
}

// Return our descriptions, localized:
char * rain_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode){
  return(strdup(gettext_noop(rain_descs[which][mode-1])));
}

// Do the effect:
static void do_rain_drop(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
                int x, int y){
  magic_api * api = (magic_api *) ptr;

  int xx, yy;
  Uint8 r,g,b;

  for (yy = y - rain_SIZE/2; yy < y + rain_SIZE/2; yy++){
    for (xx = x - rain_SIZE; xx < x + rain_SIZE; xx++){
      if (rain_inRainShape(xx - x, yy - y + rain_SIZE/2, rain_SIZE)){
        //api->rgbtohsv(rain_r, rain_g, rain_b, &h, &s, &v);
        //api->hsvtorgb(h, s, rain_weights[(yy-y)*((rain_SIZE*2) -1)+(xx-x)], &r, &g, &b);
        SDL_GetRGB(api->getpixel(canvas, xx , yy), canvas->format, &r, &g, &b);
        api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format,  clamp(0, r - 50, 255), 
                                                                  clamp(0, g - 50, 255), 
                                                                  clamp(0, b + 200, 255)));
      }
    }
  }

}

static void rain_linecb(void * ptr, int which,
                        SDL_Surface * canvas, SDL_Surface * last,
                        int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  SDL_Rect rect;

  if (rand() % 10 == 0) {
    rain_click(api, which, MODE_PAINT, canvas, last,
               x + (rand() % rain_SIZE * 2) - rain_SIZE,
               y + (rand() % rain_SIZE * 2) - rain_SIZE,
               &rect);
  }
}

// Affect the canvas on drag:
void rain_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect){
  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1, rain_linecb);

  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }

  update_rect->x = ox - rain_SIZE * 2;
  update_rect->y = oy - rain_SIZE * 2;
  update_rect->w = rain_SIZE * 4;
  update_rect->h = rain_SIZE * 4;
}

// Affect the canvas on click:
void rain_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect){

  if (mode == MODE_PAINT){
    do_rain_drop(api, which, canvas, last, x, y);

    update_rect->x = x - rain_SIZE;
    update_rect->y = y - rain_SIZE;
    update_rect->w = rain_SIZE * 2;
    update_rect->h = rain_SIZE * 2;

    api->playsound(rain_snd_effect[which], (x * 255) / canvas->w, 255);
  }else{

    int i;
    for(i=0; i<rain_AMOUNT; i++){
      do_rain_drop(api, which, canvas, last, rand() % canvas->w, rand() % canvas->h);
    }

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;

    api->playsound(rain_snd_effect[which], 128, 255);
  }
}

// Affect the canvas on release:
void rain_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void rain_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
	//Clean up sounds
	int i;
	for(i=0; i<rain_NUM_TOOLS; i++){
		if(rain_snd_effect[i] != NULL){
			Mix_FreeChunk(rain_snd_effect[i]);
		}
	}
}

// Record the color from Tux Paint:
void rain_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int rain_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}


void rain_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void rain_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int rain_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_FULLSCREEN|MODE_PAINT);
}


