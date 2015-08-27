/*
  light.c

  Light Magic Tool Plugin
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
  $Id: light.c,v 1.9 2011/11/26 22:04:50 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include "math.h"

/* Our globals: */

static Mix_Chunk * light1_snd, * light2_snd;
static float light_h, light_s, light_v;
Uint32 light_api_version(void);
int light_init(magic_api * api);
int light_get_tool_count(magic_api * api);
SDL_Surface * light_get_icon(magic_api * api, int which);
char * light_get_name(magic_api * api, int which);
char * light_get_description(magic_api * api, int which, int mode);
static void do_light(void * ptr, int which, SDL_Surface * canvas, SDL_Surface * last,
                int x, int y);
void light_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void light_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void light_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void light_shutdown(magic_api * api);
void light_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int light_requires_colors(magic_api * api, int which);
void light_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void light_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int light_modes(magic_api * api, int which);


Uint32 light_api_version(void) { return(TP_MAGIC_API_VERSION); }


// No setup required:
int light_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/light1.ogg",
	    api->data_directory);
  light1_snd = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/sounds/magic/light2.ogg",
	    api->data_directory);
  light2_snd = Mix_LoadWAV(fname);

  return(1);
}

// We have multiple tools:
int light_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * light_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/light.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * light_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Light")));
}

// Return our descriptions, localized:
char * light_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click and drag to draw a beam of light on your picture.")));
}

// Do the effect:

static void do_light(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
                int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  int xx, yy;
  Uint32 pix;
  Uint8 r, g, b;
  float h, s, v, new_h, new_s, new_v;
  float adj;

  for (yy = -8; yy < 8; yy++)
  {
    for (xx = -8; xx < 8; xx++)
    {
      if (api->in_circle(xx, yy, 8))
      {
        pix = api->getpixel(canvas, x + xx, y + yy);

        SDL_GetRGB(pix, canvas->format, &r, &g, &b);

        adj = (7.99 - sqrt(abs(xx * yy))) / 128.0;

        api->rgbtohsv(r, g, b, &h, &s, &v);

        v = min((float) 1.0, v + adj);

        if (light_h == -1 && h == -1)
        {
          new_h = -1;
          new_s = 0;
          new_v = v;
        }
        else if (light_h == -1)
        {
          new_h = h;
          new_s = max(0.0, s - adj / 2.0);
          new_v = v;
        }
        else if (h == -1)
        {
          new_h = light_h;
          new_s = max(0.0, light_s - adj / 2.0);
          new_v = v;
        }
        else
        {
          new_h = (light_h + h) / 2;
          new_s = max(0.0, s - adj / 2.0);
          new_v = v;
        }

        api->hsvtorgb(new_h, new_s, new_v, &r, &g, &b);

        api->putpixel(canvas, x + xx, y + yy,
                      SDL_MapRGB(canvas->format, r, g, b));
      }
    }
  }
}

// Affect the canvas on drag:
void light_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  api->line((void *) api, which, canvas, last, ox, oy, x, y, 1, do_light);

  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }

  update_rect->x = ox - 8;
  update_rect->y = oy - 8;
  update_rect->w = (x + 8) - update_rect->x;
  update_rect->h = (y + 8) - update_rect->h;

  api->playsound(light1_snd, (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void light_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  light_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

// Affect the canvas on release:
void light_release(magic_api * api, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  api->playsound(light2_snd, (x * 255) / canvas->w, 255);
}

// No setup happened:
void light_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (light1_snd != NULL)
    Mix_FreeChunk(light1_snd);
  if (light2_snd != NULL)
    Mix_FreeChunk(light2_snd);
}

// Record the color from Tux Paint:
void light_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b)
{
  api->rgbtohsv(r, g, b, &light_h, &light_s, &light_v);
}

// Use colors:
int light_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void light_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{ 
}

void light_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int light_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
