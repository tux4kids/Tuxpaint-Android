/*
  fill.c

  Fill Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2008 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

  Flood fill code based on Wikipedia example:
  http://www.wikipedia.org/wiki/Flood_fill/C_example
  by Damian Yerrick - http://www.wikipedia.org/wiki/Damian_Yerrick

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
  $Id: fill.c,v 1.12 2011/11/26 22:04:50 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"


/* Our globals: */

static Mix_Chunk * fill_snd;
static Uint8 fill_r, fill_g, fill_b;

/* Local function prototypes: */

static int colors_close(magic_api * api, SDL_Surface * canvas,
			Uint32 c1, Uint32 c2);
static void do_flood_fill(magic_api * api, SDL_Surface * canvas, int x, int y,
                   Uint32 cur_colr, Uint32 old_colr);
int fill_modes(magic_api * api, int which);
void fill_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void fill_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
int fill_requires_colors(magic_api * api, int which);
void fill_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void fill_shutdown(magic_api * api);
void fill_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void fill_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void fill_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
                  SDL_Rect * update_rect);
char * fill_get_description(magic_api * api, int which, int mode);
char * fill_get_name(magic_api * api, int which);
int fill_get_tool_count(magic_api * api);
SDL_Surface * fill_get_icon(magic_api * api, int which);
Uint32 fill_api_version(void);
int fill_init(magic_api * api);


// No setup required:
int fill_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/fill.wav",
	    api->data_directory);
  fill_snd = Mix_LoadWAV(fname);

  return(1);
}

Uint32 fill_api_version(void) { return(TP_MAGIC_API_VERSION); }

// We have multiple tools:
int fill_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * fill_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/fill.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * fill_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Fill")));
}

// Return our descriptions, localized:
char * fill_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop(
"Click in the picture to fill that area with color.")));
}


// Affect the canvas on drag:
void fill_drag(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
	          SDL_Surface * last ATTRIBUTE_UNUSED, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
                  SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// Affect the canvas on click:
void fill_click(magic_api * api, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x, int y, SDL_Rect * update_rect)
{
  do_flood_fill(api, canvas, x, y, SDL_MapRGB(canvas->format,
                                              fill_r, fill_g, fill_b),
                api->getpixel(canvas, x, y));

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void fill_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void fill_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Mix_FreeChunk(fill_snd);
}

// Record the color from Tux Paint:
void fill_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
  fill_r = r;
  fill_g = g;
  fill_b = b;
}

// Use colors:
int fill_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}


static int colors_close(magic_api * api, SDL_Surface * canvas,
			Uint32 c1, Uint32 c2)
{
  Uint8 r1, g1, b1, r2, g2, b2;

  if (c1 == c2)
  {
    /* Get it over with quick, if possible! */

    return 1;
  }
  else
  {
    double r, g, b;
    SDL_GetRGB(c1, canvas->format, &r1, &g1, &b1);
    SDL_GetRGB(c2, canvas->format, &r2, &g2, &b2);

    // use distance in linear RGB space
    r = api->sRGB_to_linear(r1) - api->sRGB_to_linear(r2);
    r *= r;
    g = api->sRGB_to_linear(g1) - api->sRGB_to_linear(g2);
    g *= g;
    b = api->sRGB_to_linear(b1) - api->sRGB_to_linear(b2);
    b *= b;

    // easy to confuse:
    //   dark grey, brown, purple
    //   light grey, tan
    //   red, orange
    return r + g + b < 0.04;
  }
}


static void do_flood_fill(magic_api * api, SDL_Surface * canvas, int x, int y,
                   Uint32 cur_colr, Uint32 old_colr)
{
  int fillL, fillR, i, in_line;
  static unsigned char prog_anim;


  if (cur_colr == old_colr || colors_close(api, canvas, cur_colr, old_colr))
    return;


  fillL = x;
  fillR = x;

  prog_anim++;
  if ((prog_anim % 4) == 0)
  {
    api->update_progress_bar();
    api->playsound(fill_snd, (x * 255) / canvas->w, 255);
  }


  /* Find left side, filling along the way */

  in_line = 1;

  while (in_line)
  {
    api->putpixel(canvas, fillL, y, cur_colr);
    fillL--;

    in_line =
      (fillL < 0) ? 0 : colors_close(api, canvas,
				     api->getpixel(canvas, fillL, y),
                                     old_colr);
  }

  fillL++;

  /* Find right side, filling along the way */

  in_line = 1;
  while (in_line)
  {
    api->putpixel(canvas, fillR, y, cur_colr);
    fillR++;

    in_line = (fillR >= canvas->w) ? 0 : colors_close(api, canvas,
					   api->getpixel(canvas, fillR, y),
                                                      old_colr);
  }

  fillR--;


  /* Search top and bottom */

  for (i = fillL; i <= fillR; i++)
  {
    if (y > 0 && colors_close(api, canvas, api->getpixel(canvas, i, y - 1),
			      old_colr))
      do_flood_fill(api, canvas, i, y - 1, cur_colr, old_colr);

    if (y < canvas->h
        && colors_close(api, canvas, api->getpixel(canvas, i, y + 1), old_colr))
      do_flood_fill(api, canvas, i, y + 1, cur_colr, old_colr);
  }
}

void fill_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void fill_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int fill_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT);
}
