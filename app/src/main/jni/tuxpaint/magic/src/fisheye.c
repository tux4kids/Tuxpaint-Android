/*
  fisheye.c

  fisheye, Fisheye tool
  Tux Paint - A simple drawing program for children.

  Credits: Adam 'foo-script' Rakowski ; foo-script@o2.pl

  Copyright (c) 2002-2023 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  https://tuxpaint.org/

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

  Last updated: April 22, 2023
*/

#include <math.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

Mix_Chunk *fisheye_snd;
int last_x, last_y;
int fisheye_radius = 80;

/* Local function prototypes */
Uint32 fisheye_api_version(void);
void fisheye_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int fisheye_init(magic_api * api, Uint32 disabled_features);
int fisheye_get_tool_count(magic_api * api);
SDL_Surface *fisheye_get_icon(magic_api * api, int which);
char *fisheye_get_name(magic_api * api, int which);
int fisheye_get_group(magic_api * api, int which);
char *fisheye_get_description(magic_api * api, int which, int mode);
int fisheye_requires_colors(magic_api * api, int which);
void fisheye_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void fisheye_shutdown(magic_api * api);
void fisheye_draw(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
void fisheye_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void fisheye_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                   SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void fisheye_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void fisheye_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int fisheye_modes(magic_api * api, int which);
Uint8 fisheye_accepted_sizes(magic_api * api, int which, int mode);
Uint8 fisheye_default_size(magic_api * api, int which, int mode);
void fisheye_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                      SDL_Rect * update_rect);


//                              Housekeeping functions

void fisheye_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);

Uint32 fisheye_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

void fisheye_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

int fisheye_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/fisheye.ogg", api->data_directory);
  fisheye_snd = Mix_LoadWAV(fname);

  return (1);
}

int fisheye_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return 1;
}

SDL_Surface *fisheye_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/fisheye.png", api->data_directory);

  return (IMG_Load(fname));
}

char *fisheye_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Fisheye"));
}

int fisheye_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_DISTORTS;
}

char *fisheye_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return strdup(gettext_noop("Click on part of your picture to create a fisheye effect."));
}

int fisheye_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void fisheye_release(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED,
                     SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                     int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{

}

void fisheye_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  Mix_FreeChunk(fisheye_snd);
}

// do-fisheye

void fisheye_draw(void *ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas,
                  SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  SDL_Surface *oryg, *temp_src = NULL, *temp_dest = NULL, *output;
  SDL_Rect rect, temp_rect;
  int xx, yy;
  unsigned short int i;

  if (api->in_circle(last_x - x, last_y - y, fisheye_radius))
    return;

  last_x = x;
  last_y = y;

  oryg =
    SDL_CreateRGBSurface(SDL_SWSURFACE, fisheye_radius, fisheye_radius, canvas->format->BitsPerPixel,
                         canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  output =
    SDL_CreateRGBSurface(SDL_SWSURFACE, fisheye_radius, fisheye_radius, canvas->format->BitsPerPixel,
                         canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  rect.x = x - (fisheye_radius / 2);
  rect.y = y - (fisheye_radius / 2);
  rect.w = rect.h = fisheye_radius;

  SDL_BlitSurface(canvas, &rect, oryg, NULL);   //here we have a piece of source image. Now we've to scale it (keeping aspect ratio)

  //do vertical fisheye
  for (i = 0; i < fisheye_radius / 2; i++)
  {
    temp_src =
      SDL_CreateRGBSurface(SDL_SWSURFACE, 1, fisheye_radius, canvas->format->BitsPerPixel,
                           canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

    //let's take a smooth bar of scaled bitmap and copy it to temp
    //left side first
    rect.x = i;
    rect.y = 0;
    rect.w = 1;

    SDL_BlitSurface(oryg, &rect, temp_src, NULL);       //this bar is copied to temp_src

    temp_dest =
      SDL_CreateRGBSurface(SDL_SWSURFACE, 1, fisheye_radius + 2 * i,
                           canvas->format->BitsPerPixel,
                           canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

    temp_dest = api->scale(temp_src, 1, fisheye_radius + 2 * i, 0);     //temp_dest stores scaled temp_src

    temp_rect.x = 0;
    temp_rect.y = i;
    temp_rect.w = 1;
    temp_rect.h = fisheye_radius;

    SDL_BlitSurface(temp_dest, &temp_rect, output, &rect);      //let's copy it to output

    //right side then

    rect.x = (fisheye_radius - 1) - i;

    SDL_BlitSurface(oryg, &rect, temp_src, NULL);       //this bar is copied to temp_src //OK

    temp_dest = api->scale(temp_src, 1, fisheye_radius + 2 * i, 0);     //temp_dest stores scaled temp_src

    SDL_BlitSurface(temp_dest, &temp_rect, output, &rect);      //let's copy it to output
  }

  //do horizontal fisheye
  for (i = 0; i < fisheye_radius / 2; i++)
  {
    temp_src =
      SDL_CreateRGBSurface(SDL_SWSURFACE, fisheye_radius, 1, canvas->format->BitsPerPixel,
                           canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

    temp_dest =
      SDL_CreateRGBSurface(SDL_SWSURFACE, fisheye_radius + 2 * i, 1,
                           canvas->format->BitsPerPixel,
                           canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

    //upper side first
    rect.x = 0;
    rect.y = i;
    rect.w = fisheye_radius;
    rect.h = 1;

    temp_rect.x = i;
    temp_rect.y = 0;
    temp_rect.w = fisheye_radius;
    temp_rect.h = 1;

    SDL_BlitSurface(output, &rect, temp_src, NULL);

    temp_dest = api->scale(temp_src, fisheye_radius + 2 * i, 1, 0);

    SDL_BlitSurface(temp_dest, &temp_rect, output, &rect);

    //lower side then

    rect.y = (fisheye_radius - 1) - i;
    SDL_BlitSurface(output, &rect, temp_src, NULL);

    temp_dest = api->scale(temp_src, fisheye_radius + 2 * i, 1, 0);
    SDL_BlitSurface(temp_dest, &temp_rect, output, &rect);
  }

  rect.x = x - (fisheye_radius / 2);
  rect.y = y - (fisheye_radius / 2);
  rect.w = rect.h = fisheye_radius;

  //let's blit an area surrounded by a circle

  for (yy = y - (fisheye_radius / 2); yy < y + (fisheye_radius / 2); yy++)
  {
    for (xx = x - (fisheye_radius / 2); xx < x + (fisheye_radius / 2); xx++)
    {
      if (api->in_circle(xx - x, yy - y, (fisheye_radius / 2)))
      {
        api->putpixel(canvas, xx, yy,
                      api->getpixel(output, xx + (fisheye_radius / 2) - x, yy + (fisheye_radius / 2) - y));
      }
    }
  }

  SDL_FreeSurface(oryg);
  SDL_FreeSurface(output);
  if (temp_dest != NULL)
    SDL_FreeSurface(temp_dest);
  if (temp_src != NULL)
    SDL_FreeSurface(temp_src);

  api->playsound(fisheye_snd, (x * 255) / canvas->w, 255);
}

void fisheye_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{

  api->line(api, which, canvas, snapshot, ox, oy, x, y, 1, fisheye_draw);
  update_rect->x = min(ox, x) - (fisheye_radius / 2);
  update_rect->y = min(oy, y) - (fisheye_radius / 2);
  update_rect->w = max(ox, x) - update_rect->x + (fisheye_radius / 2);
  update_rect->h = max(oy, y) - update_rect->y + (fisheye_radius / 2);
}

void fisheye_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  last_x = -fisheye_radius;     /* A value that will be beyond any clicked position */
  last_y = -fisheye_radius;
  fisheye_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void fisheye_switchin(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{

}

void fisheye_switchout(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{

}

int fisheye_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 fisheye_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 5;                     // FIXME
}

Uint8 fisheye_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 2;                     // FIXME
}

void fisheye_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  fisheye_radius = size * 40;
}
