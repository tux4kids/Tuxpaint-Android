/*
  rotate.c

  Rotates the image on the canvas.

  Tux Paint - A simple drawing program for children.

  Copyright (c) 2024 by Bill Kendrick

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

  Last updated: October 7, 2024
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL2_rotozoom.h"

static Mix_Chunk *rotate_snd_drag, *rotate_snd_release;
SDL_Surface *rotate_snapshot = NULL;
Uint32 rotate_color;
float rotate_last_angle = 0.0;
int rotate_clicked_since_switchin = 0;

Uint32 rotate_api_version(void);
int rotate_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int rotate_get_tool_count(magic_api * api);
SDL_Surface *rotate_get_icon(magic_api * api, int which);
char *rotate_get_name(magic_api * api, int which);
int rotate_get_group(magic_api * api, int which);
int rotate_get_order(int which);
char *rotate_get_description(magic_api * api, int which, int mode);

void rotate_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void rotate_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void rotate_release(magic_api * api, int which,
                    SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void rotate_shutdown(magic_api * api);
void rotate_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int rotate_requires_colors(magic_api * api, int which);
void rotate_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void rotate_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int rotate_modes(magic_api * api, int which);
Uint8 rotate_accepted_sizes(magic_api * api, int which, int mode);
Uint8 rotate_default_size(magic_api * api, int which, int mode);
void rotate_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                     SDL_Rect * update_rect);
float do_rotate(SDL_Surface * canvas, int x, int y, int smoothing_flag);
void rotate_xorline_callback(void *pointer, int tool, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);


Uint32 rotate_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int rotate_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/rotate-drag.ogg", api->data_directory);
  rotate_snd_drag = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%ssounds/magic/rotate-release.ogg", api->data_directory);
  rotate_snd_release = Mix_LoadWAV(fname);

  return (1);
}

int rotate_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (1);
}

SDL_Surface *rotate_get_icon(magic_api *api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/rotate.png", api->data_directory);

  return (IMG_Load(fname));
}

char *rotate_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Rotate")));
}

int rotate_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PICTURE_WARPS;
}

int rotate_get_order(int which ATTRIBUTE_UNUSED)
{
  return 900;
}

char *rotate_get_description(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext("Click and drag to rotate your drawing.")));
}

float do_rotate(SDL_Surface *canvas, int x, int y, int smoothing_flag)
{
  SDL_Surface *new_surf;
  float angle_rad;
  SDL_Rect dest;

  if (rotate_snapshot == NULL)
    return 0.0;                 /* abort! */

  /* Render a rotated version of the snapshot */
  /* ---------------------------------------- */
  /* Calculate angle based on X/Y click vs. center of canvas */
  angle_rad = -atan2(y - (canvas->h / 2), x - (canvas->w / 2));
  /* Add previous angle, so they stack up
     (allows you to click a spot, and while rotating it remains
     under the pointer; versus always re-rotating) */
  angle_rad += rotate_last_angle;
  new_surf = rotozoomSurface(rotate_snapshot, (angle_rad * 180.0 / M_PI), 1.0 /* no zoom */ , smoothing_flag);

  /* Draw background color on canvas */
  /* ------------------------------- */
  SDL_FillRect(canvas, NULL, rotate_color);

  /* Place rotated version in the center of the live canvas */
  /* ------------------------------------------------------ */
  dest.x = (canvas->w - new_surf->w) / 2;
  dest.y = (canvas->h - new_surf->h) / 2;
  dest.w = new_surf->w;
  dest.h = new_surf->h;
  SDL_BlitSurface(new_surf, NULL, canvas, &dest);
  SDL_FreeSurface(new_surf);
  /* Return the angle we ended up at */
  return angle_rad;
}

void rotate_xorline_callback(void *pointer ATTRIBUTE_UNUSED, int tool ATTRIBUTE_UNUSED,
                             SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) pointer;

  api->xorpixel(canvas, x, y);
}

void rotate_drag(magic_api *api, int which ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                 SDL_Surface *last, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED,
                 int x, int y, SDL_Rect *update_rect)
{
  float ang;
  int xx, yy;

  /* Rotate interactively based on the X/Y position of the mouse */
  ang = do_rotate(canvas, x, y, SMOOTHING_OFF);

  /* Draw indicator lines */
  xx = (canvas->w / 2) + ((int)(cos(ang) * 100));
  yy = (canvas->h / 2) - ((int)(sin(ang) * 100));
  api->line((void *)api, which, canvas, last, canvas->w / 2, canvas->h / 2, xx, yy, 1, rotate_xorline_callback);

  xx = (canvas->w / 2) + ((int)(cos(ang + (M_PI / 2)) * 200));
  yy = (canvas->h / 2) - ((int)(sin(ang + (M_PI / 2)) * 200));
  api->line((void *)api, which, canvas, last, canvas->w / 2, canvas->h / 2, xx, yy, 1, rotate_xorline_callback);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(rotate_snd_drag, 128, 255);
}

void rotate_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  /* Calculate the starting angle as the OPPOSITE of
   * where you clicked (so that `rotate_drag()` ends up
   * rotating 0 radians), and stack it onto the current angle */
  rotate_last_angle += atan2(y - (canvas->h / 2), x - (canvas->w / 2));

  /* Record the fact that we've clicked at least once since
   * switching [back] to thsi tool */
  rotate_clicked_since_switchin = 1;

  /* Stop any sound (in case "release" version is playing),
     so we can play the main "drag" sound immediately */
  api->stopsound();

  /* Call the drag function to do the work
   * (it will add the click positions angle, making it a net
   * 0-radian rotation _this time_) */
  rotate_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void rotate_release(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                    SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  /* Final rotation work; and now, record the final angle
   * we landed at, so we can reuse it -- both for stacking up
   * the rotation as the user clicks/drags/releases repeatedly,
   * but also so the canvas can be re-rotated to the same angle
   * if the background color gets changed in the meantime. */
  rotate_last_angle = do_rotate(canvas, x, y, SMOOTHING_ON);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  /* Stop any "drag" sound, and play "release" immediately */
  api->stopsound();
  api->playsound(rotate_snd_release, 128, 255);
}

void rotate_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (rotate_snd_drag != NULL)
    Mix_FreeChunk(rotate_snd_drag);

  if (rotate_snd_release != NULL)
    Mix_FreeChunk(rotate_snd_release);

  if (rotate_snapshot != NULL)
  {
    SDL_FreeSurface(rotate_snapshot);
    rotate_snapshot = NULL;
  }
}

void rotate_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                      SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b, SDL_Rect *update_rect)
{
  /* Record the new color */
  rotate_color = SDL_MapRGB(canvas->format, r, g, b);

  /* If we've been rotating the canvas, go ahead and
   * re-rotate it at the same angle (using canvas center as
   * a way to make `do_rotate()` calculate a 0-radian rotation);
   * we'll render it with the new background color. */
  if (rotate_clicked_since_switchin)
  {
    do_rotate(canvas, canvas->w / 2, canvas->h / 2, SMOOTHING_ON);
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}

int rotate_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void rotate_switchin(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
  if (rotate_snapshot == NULL)
    rotate_snapshot = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                                           canvas->format->BitsPerPixel, canvas->format->Rmask,
                                           canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  if (rotate_snapshot != NULL)
  {
    SDL_BlitSurface(canvas, NULL, rotate_snapshot, NULL);
  }

  /* Our first time [back].  We haven't clicked yet,
   * and our current rotation is 0 radians */
  rotate_clicked_since_switchin = 0;
  rotate_last_angle = 0.0;
}

void rotate_switchout(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
  /* Since `set_color()` gets called _before_ `switchin()`
   * we need to clear this flag on our way out, so we don't
   * draw the rotated canvas again, THEN take a new snapshot,
   * and hence undo anything that was done while we were away
   * from this tool! */
  /* (See https://sourceforge.net/p/tuxpaint/bugs/286/) */
  rotate_clicked_since_switchin = 0;
}

int rotate_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 rotate_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

Uint8 rotate_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

void rotate_set_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED,
                     Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
}
