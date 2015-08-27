/*
  perspective.c

  perspective, stretches the plane of the image.
  zoom, zooms in and out the image.

  Tux Paint - A simple drawing program for children.

  Credits: Andrew Corcoran <akanewbie@gmail.com>

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

  Last updated: May 6, 2009
  $Id: perspective.c,v 1.4 2011/05/09 23:38:53 perepujal Exp $
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

static void perspective_preview(magic_api * api, int which,
				SDL_Surface * canvas, SDL_Surface * last,
				int x, int y, SDL_Rect * update_rect, float step);
Uint32 perspective_api_version(void);
int perspective_init(magic_api * api);
int perspective_get_tool_count(magic_api * api);
SDL_Surface * perspective_get_icon(magic_api * api, int which);
char * perspective_get_name(magic_api * api, int which);

char * perspective_get_description(magic_api * api, int which, int mode);

void perspective_drag(magic_api * api, int which, SDL_Surface * canvas,
		      SDL_Surface * last, int ox, int oy, int x, int y,
		      SDL_Rect * update_rect);

void perspective_click(magic_api * api, int which, int mode,
		       SDL_Surface * canvas, SDL_Surface * last,
		       int x, int y, SDL_Rect * update_rect);

void perspective_release(magic_api * api, int which,
			 SDL_Surface * canvas, SDL_Surface * last,
			 int x, int y, SDL_Rect * update_rect);

void perspective_shutdown(magic_api * api);

void perspective_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);

int perspective_requires_colors(magic_api * api, int which);

void perspective_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);

void perspective_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);

int perspective_modes(magic_api * api, int which);

int scan_fill(magic_api * api, SDL_Surface * canvas, SDL_Surface * srfc,int x,int y, int fill_edge, int fill_tile, int size, Uint32 color);

void perspective_line(void * ptr_to_api, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);




static const int perspective_AMOUNT= 300;
static const int perspective_RADIUS = 16;
static const double perspective_SHARPEN = 1.0;
Uint8 perspective_r, perspective_g, perspective_b;
int corner;
int dash;

int click_x, click_y;
int new_w, new_h, old_h, sound_h;

int perspective_average_r, perspective_average_g, perspective_average_b, perspective_average_count;
Uint32 pixel_average, black, white;

int otop_left_x, otop_left_y, otop_right_x, otop_right_y;
int obottom_right_x, obottom_right_y, obottom_left_x, obottom_left_y;

int top_left_x, top_left_y, top_right_x, top_right_y;
int bottom_right_x, bottom_right_y, bottom_left_x, bottom_left_y;

float top_advc_x, right_advc_x, bottom_advc_x, left_advc_x;
float top_advc_y, right_advc_y, bottom_advc_y, left_advc_y;


enum {
  TOOL_PERSPECTIVE,
  TOOL_ZOOM,
  perspective_NUM_TOOLS
};

enum {
  TOP_LEFT,
  TOP_RIGHT,
  BOTTOM_RIGHT,
  BOTTOM_LEFT
};


/* A copy of canvas at switchin, will be used to draw from it as snapshot changes at each click */
static SDL_Surface * canvas_back;

static Mix_Chunk * perspective_snd_effect[perspective_NUM_TOOLS + 1];

const char * perspective_snd_filenames[perspective_NUM_TOOLS + 1] = {
  "perspective.ogg",
  "zoom_up.ogg",
  "zoom_down.ogg",
};

const char * perspective_icon_filenames[perspective_NUM_TOOLS] = {
  "perspective.png",
  "zoom.png",
};

const char * perspective_names[perspective_NUM_TOOLS] = {
  gettext_noop("Perspective"),
  gettext_noop("Zoom"),

};

const char * perspective_descs[perspective_NUM_TOOLS] = {
  gettext_noop("Click on the corners and drag where you want to stretch the picture."),
   

  gettext_noop("Click and drag up to zoom in or drag down to zoom out the picture."),
  

};

Uint32 perspective_api_version(void) { return(TP_MAGIC_API_VERSION); }

//Load sounds
int perspective_init(magic_api * api){
  int i;
  char fname[1024];
  for (i = 0; i <= perspective_NUM_TOOLS; i++){
    snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, perspective_snd_filenames[i]);
    perspective_snd_effect[i] = Mix_LoadWAV(fname);
  }
  return(1);
}

int perspective_get_tool_count(magic_api * api ATTRIBUTE_UNUSED){
  return(perspective_NUM_TOOLS);
}

// Load our icons:
SDL_Surface * perspective_get_icon(magic_api * api, int which){
  char fname[1024];
  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, perspective_icon_filenames[which]);
  return(IMG_Load(fname));
}

// Return our names, localized:
char * perspective_get_name(magic_api * api ATTRIBUTE_UNUSED, int which){
  return(strdup(gettext_noop(perspective_names[which])));
}

// Return our descriptions, localized:
char * perspective_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED){
  return(strdup(gettext_noop(perspective_descs[which])));
}
 

// Affect the canvas on drag:
void perspective_drag(magic_api * api, int which, SDL_Surface * canvas,
		      SDL_Surface * last, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y,
		      SDL_Rect * update_rect){
  switch (which)
    {
    case TOOL_PERSPECTIVE:
      {
	switch (corner)
	  {
	  case TOP_LEFT:
	    {
	      top_left_x = x;
	      top_left_y = y;
	    }
	    break;

	  case TOP_RIGHT:
	    {
	      top_right_x = x;
	      top_right_y = y;
	    }
	    break;

	  case BOTTOM_LEFT:
	    {
	      bottom_left_x = x;
	      bottom_left_y = y;
	    }
	    break;

	  case BOTTOM_RIGHT:
	    {
	      bottom_right_x = x;
	      bottom_right_y = y;
	    }
	    break;
	  }

	SDL_BlitSurface(canvas_back, NULL, canvas, NULL);

	perspective_preview( api, which,
			     canvas , last,
			     x, y , update_rect , 2.0);

	/* Draw a square and the current shape relative to it as a visual reference */
	/* square */
	api->line(api, which, canvas, last, otop_left_x, otop_left_y, otop_right_x, otop_right_y, 1,  perspective_line);
	api->line(api, which, canvas, last, otop_left_x, otop_left_y, obottom_left_x, obottom_left_y, 1, perspective_line);
	api->line(api, which, canvas, last, obottom_left_x, obottom_left_y, obottom_right_x, obottom_right_y, 1, perspective_line);
	api->line(api, which, canvas, last, obottom_right_x, obottom_right_y, otop_right_x, otop_right_y, 1, perspective_line);

	/* shape */
	api->line(api, which, canvas, last, top_left_x, top_left_y, top_right_x, top_right_y, 1, perspective_line);
	api->line(api, which, canvas, last, top_left_x, top_left_y, bottom_left_x, bottom_left_y, 1, perspective_line);
	api->line(api, which, canvas, last, bottom_left_x, bottom_left_y, bottom_right_x, bottom_right_y, 1, perspective_line);
	api->line(api, which, canvas, last, bottom_right_x, bottom_right_y, top_right_x, top_right_y, 1, perspective_line);




	api->playsound(perspective_snd_effect[which], (x * 255) / canvas->w, 255);
      }
      break;
    case TOOL_ZOOM:
      {
	int x_distance, y_distance;

	update_rect->x = update_rect->y = 0;
	update_rect->w = canvas->w;
	update_rect->h = canvas->h;

	SDL_FillRect(canvas, update_rect, SDL_MapRGB(canvas->format, perspective_r, perspective_g, perspective_b));

	new_h = max(1, old_h + click_y - y);
	new_w = canvas->w * new_h / canvas->h;
	if (new_h >= sound_h)
	  api->playsound(perspective_snd_effect[which], 127, 255);
	else
	  api->playsound(perspective_snd_effect[which + 1], 127, 255);
	sound_h = new_h;

	x_distance = (otop_right_x - otop_left_x) *  new_w / canvas->w;
	top_left_x = bottom_left_x = canvas->w / 2 - x_distance / 2;
	top_right_x = bottom_right_x = canvas->w / 2 + x_distance / 2;

	y_distance = (obottom_left_y - otop_left_y) * new_w / canvas->w;
	top_left_y = top_right_y = canvas->h / 2 - y_distance / 2;
	bottom_left_y = bottom_right_y = canvas->h / 2 + y_distance / 2;

	perspective_preview( api, which,
			     canvas , last,
			     x, y , update_rect , 2.0);

	update_rect->x = update_rect->y =0;
	update_rect->w = canvas->w;
	update_rect->h = canvas->h;
      }
      break;



    }
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

}

// Affect the canvas on click:
void perspective_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
		       SDL_Surface * canvas, SDL_Surface * last,
		       int x, int y, SDL_Rect * update_rect){
  switch(which)
    {case TOOL_PERSPECTIVE:
	{

	  if (x < canvas->w / 2)
	    {
	      if (y < canvas->h / 2)
		{
		  corner = TOP_LEFT;
		}
	      else
		{
		  corner = BOTTOM_LEFT;
		}
	    }
	  else
	    {
	      if (y < canvas->h / 2)
		{
		  corner = TOP_RIGHT;
		}
	      else
		{
		  corner = BOTTOM_RIGHT;
		}
	    }


	}
	break;
    case TOOL_ZOOM:
      {
	click_x = x;
	click_y = y;
	old_h = new_h;
      }
      break;
    }
  perspective_drag(api, which, canvas, last, x, y, x, y, update_rect);

}

// Affect the canvas on release:
void perspective_release(magic_api * api, int which,
			 SDL_Surface * canvas, SDL_Surface * last,
			 int x, int y, SDL_Rect * update_rect)
{
  switch (which)
    {
    case TOOL_PERSPECTIVE:{
      perspective_preview( api, which,
			   canvas , last,
			   x, y , update_rect , 0.5);
    }
      break;

    case TOOL_ZOOM:
      { 
	SDL_Surface * aux_surf;
	SDL_Surface * scaled_surf;

	update_rect->x = update_rect->y = 0;
	update_rect->w = canvas->w;
	update_rect->h = canvas->h;

	SDL_FillRect(canvas, update_rect, SDL_MapRGB(canvas->format, perspective_r, perspective_g, perspective_b));


	if(new_h < canvas->h)
	  {
	    scaled_surf = api->scale(canvas_back, new_w, new_h, 0);
	    update_rect->x = (canvas->w - new_w) / 2;
	    update_rect->y = (canvas->h - new_h) / 2;
	    update_rect->w = new_w;
	    update_rect->h = new_h;
	    SDL_BlitSurface(scaled_surf, NULL, canvas, update_rect);
	  }
	else
	  {
	    int aux_x, aux_y, aux_h, aux_w;
	    aux_h = canvas->h * canvas->h / new_h;
	    aux_w =  canvas->w * aux_h / canvas->h;
	    aux_x = canvas->w / 2 - aux_w / 2;
	    aux_y = canvas->h / 2 - aux_h / 2;

	    update_rect->x = canvas->w / 2 - aux_w / 2;
	    update_rect->y = canvas->h / 2 - aux_h / 2;
	    update_rect->w = aux_w;
	    update_rect->h = aux_h;

	    aux_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
					    aux_w,
					    aux_h,
					    canvas->format->BitsPerPixel,
					    canvas->format->Rmask,
					    canvas->format->Gmask,
					    canvas->format->Bmask, 0);

	    SDL_BlitSurface(canvas_back, update_rect, aux_surf, NULL);
	    scaled_surf = api->scale(aux_surf, canvas->w, canvas->h, 0);
	    SDL_BlitSurface(scaled_surf, NULL, canvas, NULL);
	    SDL_FreeSurface(aux_surf);
	  }
	SDL_FreeSurface(scaled_surf);

	update_rect->x = update_rect->y = 0;
	update_rect->w = canvas->w;
	update_rect->h = canvas->h;

      }
      break;
    }
}

void perspective_preview(magic_api * api, int which ATTRIBUTE_UNUSED,
			 SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
			 int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect, float step)
{
  float i, j;
  float ax, ay, bx, by, dx, dy;
  int ox_distance, oy_distance;
  int center_ofset_x, center_ofset_y;


  update_rect->x = update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  SDL_FillRect(canvas, update_rect, SDL_MapRGB(canvas->format, perspective_r, perspective_g, perspective_b));

  ox_distance = otop_right_x - otop_left_x;
  oy_distance = obottom_left_y - otop_left_y;
 
  top_advc_x = (float)(top_right_x - top_left_x) / ox_distance;
  top_advc_y = (float)(top_right_y - top_left_y) / ox_distance;

  left_advc_x = (float)(bottom_left_x - top_left_x) / oy_distance;
  left_advc_y = (float)(bottom_left_y - top_left_y) / oy_distance;


  right_advc_x = (float)(bottom_right_x - top_right_x) / oy_distance;
  right_advc_y = (float)(bottom_right_y - top_right_y) / oy_distance;

  bottom_advc_x = (float)(bottom_right_x - bottom_left_x) / ox_distance;
  bottom_advc_y = (float)(bottom_right_y - bottom_left_y) / ox_distance;

  center_ofset_x = (otop_left_x - top_left_x) * 2;
  center_ofset_y = (otop_left_y - top_left_y) * 2;

  for(i = 0; i < canvas->w; i += step)
    {
      ax = (float)top_advc_x * i;
      ay = (float)top_advc_y * i;
      bx = (float)bottom_advc_x * i + (bottom_left_x - top_left_x) * 2 ;
      by =  (float)bottom_advc_y * i + (bottom_left_y - top_left_y) * 2;

      for(j = 0; j < canvas->h; j += step)
	{
	  dx = (float)(bx - ax) / canvas->h * j;
	  dy = (float)(by - ay)/ canvas->h * j;

	  api->putpixel(canvas, ax + dx - center_ofset_x, ay + dy - center_ofset_y, api->getpixel(canvas_back, i, j));
	}
    }  
}

// No setup happened:
void perspective_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;
  for(i=0; i<perspective_NUM_TOOLS + 1; i++){
    if(perspective_snd_effect[i] != NULL){
      Mix_FreeChunk(perspective_snd_effect[i]);
    }
  }
}

// Record the color from Tux Paint:
void perspective_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  perspective_r = r;
  perspective_g = g;
  perspective_b = b;
}

// Use colors:
int perspective_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void perspective_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  Uint32 amask;
  new_w = canvas->w;
  new_h = canvas->h;

  top_left_x = otop_left_x = bottom_left_x = obottom_left_x = canvas->w / 4;
  top_left_y = otop_left_y = top_right_y = otop_right_y = canvas->h / 4;

  top_right_x = otop_right_x = bottom_right_x = obottom_right_x = canvas->w - otop_left_x;

  bottom_left_y = obottom_left_y = bottom_right_y = obottom_right_y = canvas->h - otop_left_y;

  black = SDL_MapRGBA(canvas->format, 0, 0, 0, 0);
  white = SDL_MapRGBA(canvas->format, 255, 255, 255, 0);

  amask = ~(canvas->format->Rmask |
		   canvas->format->Gmask |
		   canvas->format->Bmask);

  canvas_back = SDL_CreateRGBSurface(SDL_SWSURFACE,
				     canvas->w,
				     canvas->h,
				     canvas->format->BitsPerPixel,
				     canvas->format->Rmask,
				     canvas->format->Gmask,
				     canvas->format->Bmask, amask);

  SDL_BlitSurface(canvas, NULL, canvas_back, NULL);
}

void perspective_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  SDL_FreeSurface(canvas_back);
}

int perspective_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT_WITH_PREVIEW);
}

void perspective_line(void * ptr_to_api, int which ATTRIBUTE_UNUSED,  SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api * api = (magic_api *) ptr_to_api;
  dash += 1;
  if (dash > 8) dash = 0;
  if (dash > 3)
    api->putpixel(canvas, x, y, black);
  else
    api->putpixel(canvas, x, y, white);
}
