/*
  rosette.c

  Rosette Magic Tools Plugin + Picasso Magic Tools Plugin 
  Tux Paint - A simple drawing program for children.

  Credits: Adam 'foo-script' Rakowski <foo-script@o2.pl>

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

// sound only plays on release
// also same sound for both tools

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <math.h>	//for sin, cos, ...
#define ROSETTE_R 8	//circle's diameter

static int xmid, ymid;

struct rosette_rgb
{
	Uint8 r, g, b;
};

struct rosette_rgb rosette_colors;

Mix_Chunk * rosette_snd;

//				Housekeeping functions

Uint32 rosette_api_version(void);
void rosette_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int rosette_init(magic_api * api);
int rosette_get_tool_count(magic_api * api);
SDL_Surface * rosette_get_icon(magic_api * api, int which);
char * rosette_get_name(magic_api * api, int which);
char * rosette_get_description(magic_api * api, int which, int mode);
int rosette_requires_colors(magic_api * api, int which);
void rosette_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);
void rosette_shutdown(magic_api * api);
void rosette_draw(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void rosette_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);

void rosette_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void rosette_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void rosette_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int rosette_modes(magic_api * api, int which);
void rosette_circle(void * ptr, int which,
                           SDL_Surface * canvas, SDL_Surface * snapshot,
                           int x, int y);

Uint32 rosette_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}

void rosette_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)	//get the colors from API and store it in structure
{
	rosette_colors.r=r;
	rosette_colors.g=g;
	rosette_colors.b=b;
}

int rosette_init(magic_api * api)
{
  char fname[1024];
	
    snprintf(fname, sizeof(fname), "%s/sounds/magic/picasso.ogg", api->data_directory);
    rosette_snd = Mix_LoadWAV(fname);

  return(1);
}

int rosette_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 2;
}

SDL_Surface * rosette_get_icon(magic_api * api, int which)
{
  char fname[1024];
	
	if (!which)
		snprintf(fname, sizeof(fname), "%s/images/magic/rosette.png", api->data_directory);
	else snprintf(fname, sizeof(fname), "%s/images/magic/picasso.png", api->data_directory);

  return(IMG_Load(fname));
}

char * rosette_get_name(magic_api * api ATTRIBUTE_UNUSED, int which) { if (!which) return strdup(gettext_noop("Rosette")); else return strdup(gettext_noop("Picasso"));}

char * rosette_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
	if (!which)
		return strdup(gettext_noop("Click and start drawing your rosette.")); //just k'scope with 3 bits?  
	else
		return strdup(gettext_noop("You can draw just like Picasso!")); //what is this actually doing?
}

int rosette_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED) { return 1; }

void rosette_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void rosette_shutdown(magic_api * api ATTRIBUTE_UNUSED) { Mix_FreeChunk(rosette_snd); }

// Interactivity functions

void rosette_circle(void * ptr, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                           int x, int y)
{
	magic_api * api = (magic_api *) ptr;
  
	int xx, yy;
	
	for (yy = y - ROSETTE_R; yy < y + ROSETTE_R; yy++)
		for (xx = x - ROSETTE_R; xx < x + ROSETTE_R; xx++)
			if (api->in_circle(xx - x , yy - y , ROSETTE_R/2))
				api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, rosette_colors.r, rosette_colors.g, rosette_colors.b));

}

void rosette_draw(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y)
{
	magic_api * api = (magic_api *) ptr;
	
	double angle;
	double xx, yy;		//distance to the center of the image
	int x1, y1, x2, y2;
	
	xx=(double)(xmid-x);
	yy=(double)(y-ymid);
	
	if (which==0)
	{
		angle=2*M_PI/3;	//an angle between brushes
		
		x1=(int)(xx*cos(angle)-yy*sin(angle));
		y1=(int)(xx*sin(angle)+yy*cos(angle));
	
		x2=(int)(xx*cos(2*angle)-yy*sin(2*angle));
		y2=(int)(xx*sin(2*angle)+yy*cos(2*angle));
	}
	else
	{
		angle=atan(yy/xx);
		
		if ((xx<0) && (yy>0)) angle+=M_PI;
		
		if ((xx<0) && (yy<0)) angle+=M_PI;
		
		if ((xx>0) && (yy<0)) angle+=2*M_PI;

		if ((y==ymid) && (xx<0)) angle=M_PI;
		
		x1=(int)(xx*cos(2*angle)-yy*sin(2*angle));
		y1=(int)(xx*sin(2*angle)-yy*cos(angle));
		
		x2=(int)(xx*cos(2*angle)-yy*sin(2*angle));
		y2=(int)(xx*sin(2*angle)+yy*cos(2*angle));
	}

	rosette_circle(api, which, canvas, snapshot, x, y);
	rosette_circle(api, which, canvas, snapshot, (-1)*(x1-xmid), y1+ymid);
	rosette_circle(api, which, canvas, snapshot, (-1)*(x2-xmid), y2+ymid);
}

void rosette_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
	api->line((void *) api, which, canvas, snapshot, ox, oy, x, y, 1, rosette_draw);
	api->playsound(rosette_snd, (x * 255) / canvas->w, 255);

	update_rect->x=update_rect->y=0;
	update_rect->w=canvas->w;
	update_rect->h=canvas->h;
}

void rosette_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
	rosette_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void rosette_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{	
	xmid=canvas->w/2;
	ymid=canvas->h/2;
}

void rosette_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
	
}

int rosette_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}

