/*
  warp.c

  Warp Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2023-2023 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: May 16, 2023
*/

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL2_gfxPrimitives.h"

enum {
  TOOL_WARP,
  NUM_TOOLS
};

char * warp_icons[NUM_TOOLS] = {
  "rainbow.png", // FIXME
};

char * warp_snd_fnames[NUM_TOOLS] = {
  "rainbow.ogg", // FIXME
};

char * warp_tool_names[NUM_TOOLS] = {
  gettext_noop("Warp"),
};

#define NUM_WARP_SIZES 4
#define MAX_WARP_RADIUS 64
#define WARP_MESH_RES 16

typedef struct warp_mesh_s {
  int scr_x;
  int scr_y;
  float pt_x;
  float pt_y;
} warp_mesh_t;

/* Our globals: */

static int warp_radius;
int warp_mesh_w, warp_mesh_h;
float warp_dx, warp_dy;
static warp_mesh_t * * warp_mesh = NULL;

static Mix_Chunk * warp_snd[NUM_TOOLS];

int warp_init(magic_api * api, Uint32 disabled_features);
Uint32 warp_api_version(void);
int warp_get_tool_count(magic_api * api);
SDL_Surface *warp_get_icon(magic_api * api, int which);
char *warp_get_name(magic_api * api, int which);
int warp_get_group(magic_api * api, int which);
char *warp_get_description(magic_api * api, int which, int mode);
static void warp_linecb(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);

void warp_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void warp_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void warp_release(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);


void warp_shutdown(magic_api * api);
void warp_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int warp_requires_colors(magic_api * api, int which);
void warp_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void warp_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int warp_modes(magic_api * api, int which);
Uint8 warp_accepted_sizes(magic_api * api, int which, int mode);
Uint8 warp_default_size(magic_api * api, int which, int mode);
void warp_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                      SDL_Rect * update_rect);


Uint32 warp_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// Load our sfx:
int warp_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  int i;
  char fname[1024];

  for (i = 0; i < NUM_TOOLS; i++) {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s", api->data_directory, warp_snd_fnames[i]);
    warp_snd[i] = Mix_LoadWAV(fname);
  }

  return (1);
}

// We have multiple tools:
int warp_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return NUM_TOOLS;
}

// Load our icons:
SDL_Surface *warp_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, warp_icons[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *warp_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext(warp_tool_names[which])));
}

// Return our group:
int warp_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

// Return our descriptions, localized:
char *warp_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Warp"))); // FIXME
}

// Affect the canvas on drag:
void warp_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  int xx, yy;
  float stroke_len;

  if (warp_mesh == NULL)
    return;

  if (ox == x && oy == y)
    return;

  stroke_len = sqrt((ox - x) * (ox - x) + (oy - y) * (oy - y));
  warp_dx = (float) (x - ox) / stroke_len;
  warp_dy = (float) (y - oy) / stroke_len;

  api->line(api, which, canvas, last, ox, oy, x, y, 1, warp_linecb);

  SDL_BlitSurface(last, NULL, canvas, NULL);

  for (yy = 0; yy < warp_mesh_h - 1; yy++) {
    for (xx = 0; xx < warp_mesh_w - 1; xx++) {
      /* FIXME: Just crib perspective_preview() from perspective.c? */
      /*
      Sint16 pts_x[4];
      Sint16 pts_y[4];

      pts_x[0] = (Sint16) warp_mesh[yy][xx].pt_x;
      pts_y[0] = (Sint16) warp_mesh[yy][xx].pt_y;
      pts_x[1] = (Sint16) warp_mesh[yy][xx + 1].pt_x;
      pts_y[1] = (Sint16) warp_mesh[yy][xx + 1].pt_y;
      pts_x[2] = (Sint16) warp_mesh[yy + 1][xx].pt_x;
      pts_y[2] = (Sint16) warp_mesh[yy + 1][xx].pt_y;
      pts_x[3] = (Sint16) warp_mesh[yy + 1][xx + 1].pt_x;
      pts_y[3] = (Sint16) warp_mesh[yy + 1][xx + 1].pt_y;

      filledPolygonColor(canvas, pts_x, pts_y, 4, api->getpixel(last, warp_mesh[yy][xx].scr_x, warp_mesh[yy][xx].scr_y));
      */
    }
  }

  /*
  if (ox > x) {
    int tmp;
    tmp = ox;
    ox = x;
    x = tmp;
  }
  if (oy > y) {
    int tmp;
    tmp = oy;
    oy = x;
    y = tmp;
  }

  update_rect->x = x - warp_radius;
  update_rect->y = y - warp_radius;
  update_rect->w = warp_radius * 2;
  update_rect->h = warp_radius * 2;
  */

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(warp_snd[which], (x * 255) / canvas->w, 255);
}

static void warp_linecb(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y) {
  float intensity;
  magic_api *api = (magic_api *) ptr;
  int mx, my;
  float dx, dy;

  for (my = 0; my < warp_mesh_h; my++) {
    for (mx = 0; mx < warp_mesh_w; mx++) {
      if (api->in_circle(warp_mesh[my][mx].pt_x - x, warp_mesh[my][mx].pt_y - y, warp_radius)) {
        dx = warp_mesh[my][mx].pt_x - (float) x;
        dy = warp_mesh[my][mx].pt_y - (float) y;
        intensity = ((float) warp_radius - sqrt(dx * dx + dy * dy)) / (float) warp_radius;

        warp_mesh[my][mx].pt_x += (warp_dx * intensity);
        warp_mesh[my][mx].pt_y += (warp_dy * intensity);
      }
    }
    api->update_progress_bar();
  }
}

// Affect the canvas on click:
void warp_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{
  if (warp_mesh == NULL)
    return;

  // FIXME

  warp_drag(api, which, canvas, last, x, y, x, y, update_rect);
}

void warp_release(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED,
                     SDL_Surface * last ATTRIBUTE_UNUSED,
                     int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// Clean up
void warp_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    if (warp_snd[i] != NULL)
      Mix_FreeChunk(warp_snd[i]);
  }
}

void warp_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

int warp_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

void warp_switchin(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  int x, y;

  warp_mesh_w = (canvas->w / WARP_MESH_RES);
  warp_mesh_h = (canvas->h / WARP_MESH_RES);
  warp_mesh = (warp_mesh_t * *) malloc(sizeof(warp_mesh_t *) * warp_mesh_h);
  memset(warp_mesh, (int) NULL, sizeof(warp_mesh_t *) * warp_mesh_h);

  if (warp_mesh == NULL) {
    fprintf(stderr, "warp cannot allocate warp_mesh!\n");
    return;
  }
  for (y = 0; y < warp_mesh_h; y++) {
    warp_mesh[y] = (warp_mesh_t *) malloc(sizeof(warp_mesh_t) * warp_mesh_w);
    if (warp_mesh[y] == NULL) {
      fprintf(stderr, "warp cannot allocate warp_mesh!\n");
      return;
    }
  }

  for (y = 0; y < warp_mesh_h; y++) {
    for (x = 0; x < warp_mesh_w; x++) {
      warp_mesh[y][x].scr_x = x * WARP_MESH_RES;
      warp_mesh[y][x].scr_y = y * WARP_MESH_RES;
      warp_mesh[y][x].pt_x = x * WARP_MESH_RES;
      warp_mesh[y][x].pt_y = y * WARP_MESH_RES;
    }
  }
}

void warp_switchout(magic_api * api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  int y;

  /* FIXME: Crashes! */
  return;

  if (warp_mesh != NULL) {
    for (y = 0; y < warp_mesh_h; y++) {
      if (warp_mesh[y] != NULL) {
        printf("freeing mesh row %d\n", y); fflush(stdout);
        free(warp_mesh);
        warp_mesh[y] = NULL;
      }
    }
    free(warp_mesh);
    warp_mesh = NULL;
  }
}

int warp_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 warp_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return NUM_WARP_SIZES;
}

Uint8 warp_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (NUM_WARP_SIZES / 2);
}

void warp_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 size,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  warp_radius = (size * MAX_WARP_RADIUS) / NUM_WARP_SIZES;
}
