/*
  smooth.c

  FIXME: WORK IN PROGRESS

  Smoothed-Line Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Idea: Pere Pujal i Carabantes (https://sourceforge.net/p/tuxpaint/feature-requests/238/)
  Based on: calligraphy.c by Bill Kendrick

  Copyright (c) 2023 by Bill Kendrick and others; see AUTHORS.txt
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

  Last updated: May 22, 2023
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <math.h>

enum {
  TOOL_SMOOTH,
  TOOL_SQUIGGLES,
  TOOL_LOOPS,
  NUM_TOOLS
};

char * smooth_icon_fnames[NUM_TOOLS] = {
  "smooth.png",
  "squiggles.png",
  "loops.png",
};

char * smooth_snd_fnames[NUM_TOOLS] = {
  "smooth.ogg",
  "squiggles.ogg",
  "loops.ogg",
};

char * smooth_names[NUM_TOOLS] = {
  gettext_noop("Smooth"),
  gettext_noop("Squiggles"),
  gettext_noop("Loops"),
};

char * smooth_descrs[NUM_TOOLS] = {
  gettext_noop("Click and drag the mouse around to draw in freehand; it will be smoothed when you let go."),
  gettext_noop("Click and drag the mouse around to draw squiggles."),
  gettext_noop("Click and drag the mouse around to draw loop-the-loops."),
};

int smooth_sizes[NUM_TOOLS] = {
  8,
  4,
  8,
};

static Mix_Chunk *smooth_snds[NUM_TOOLS];
static Uint32 smooth_color;
static int smooth_size;

typedef struct
{
  float x, y;
} Point2D;

#define MAX_CTRL_POINTS 4096
static Point2D smooth_control_points[MAX_CTRL_POINTS];
static int num_input_points = 0, smooth_capture = 0;
int smooth_squiggle_angle = 0, smooth_squiggle_rad = 0;

/* Local Function Prototypes */
static Point2D smooth_PointOnCubicBezier(Point2D * cp, float t);
static void smooth_ComputeBezier(Point2D * cp, int numberOfPoints, Point2D * curve);
static float smooth_dist(float x1, float y1, float x2, float y2);

int smooth_init(magic_api * api, Uint32 disabled_features);
Uint32 smooth_api_version(void);
int smooth_get_tool_count(magic_api * api);
SDL_Surface *smooth_get_icon(magic_api * api, int which);
char *smooth_get_name(magic_api * api, int which);
int smooth_get_group(magic_api * api, int which);
char *smooth_get_description(magic_api * api, int which, int mode);
static void smooth_linecb(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void smooth_squiggle_linecb(void *ptr, int which,
                                   SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y);
void smooth_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void smooth_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void smooth_release(magic_api * api, int which, SDL_Surface * canvas,
                         SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void smooth_shutdown(magic_api * api);
void smooth_set_color(magic_api * api, int which, SDL_Surface * canvas,
                           SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int smooth_requires_colors(magic_api * api, int which);
void smooth_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void smooth_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int smooth_modes(magic_api * api, int which);
Uint8 smooth_accepted_sizes(magic_api * api, int which, int mode);
Uint8 smooth_default_size(magic_api * api, int which, int mode);
void smooth_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                          SDL_Rect * update_rect);



int smooth_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  char fname[1024];
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s", api->data_directory, smooth_snd_fnames[i]);
    smooth_snds[i] = Mix_LoadWAV(fname);
  }

  return (1);
}

Uint32 smooth_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// Only one tool:
int smooth_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

// Load our icon:
SDL_Surface *smooth_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, smooth_icon_fnames[which]);
  return (IMG_Load(fname));
}

// Return our name, localized:
char *smooth_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext(smooth_names[which])));
}

// Return our group
int smooth_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

// Return our description, localized:
char *smooth_get_description(magic_api * api ATTRIBUTE_UNUSED,
                             int which, int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext(smooth_descrs[which])));
}


static void smooth_linecb(void *ptr, int which,
                          SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  SDL_Rect dest;

  if (which == TOOL_SMOOTH) {
    /* FIXME */
    dest.x = x;
    dest.y = y;
    dest.w = smooth_size;
    dest.h = smooth_size;

    SDL_FillRect(canvas, &dest, smooth_color);
  } else if (which == TOOL_LOOPS) {
    /* FIXME */
    dest.x = x;
    dest.y = y;
    dest.w = 2;
    dest.h = 2;

    SDL_FillRect(canvas, &dest, smooth_color);
  } else if (which == TOOL_SQUIGGLES) {
    api->putpixel(canvas, x, y, smooth_color);
  }
}

#define LOOP_RAD_CALC (smooth_size * 10)

static void smooth_squiggle_linecb(void *ptr, int which,
                                   SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx1, yy1, xx2, yy2, i;

  xx2 = x + (cos(smooth_squiggle_angle * M_PI / 180.0) * smooth_squiggle_rad);
  yy2 = y - (sin(smooth_squiggle_angle * M_PI / 180.0) * smooth_squiggle_rad);

  for (i = 0; i < 3; i++) {
    xx1 = xx2;
    yy1 = yy2;

    if (which == TOOL_LOOPS) {
      smooth_squiggle_angle += 5;
      smooth_squiggle_rad = LOOP_RAD_CALC;
    } else if (which == TOOL_SQUIGGLES) {
      smooth_squiggle_angle += (rand() % 5) + 5;
      smooth_squiggle_rad += ((rand() % 3 * smooth_size) - smooth_size);
      if (smooth_squiggle_rad < 5) {
        smooth_squiggle_rad += 5;
      } else if (smooth_squiggle_rad >= 15 * smooth_size) {
        smooth_squiggle_rad -= ((rand() % 10) + 10);
      }
    }

    xx2 = x + (cos(smooth_squiggle_angle * M_PI / 180.0) * smooth_squiggle_rad);
    yy2 = y - (sin(smooth_squiggle_angle * M_PI / 180.0) * smooth_squiggle_rad);

    api->line((void *)api, which, canvas, last, xx1, yy1, xx2, yy2, 1, smooth_linecb);
  }

  if (smooth_squiggle_angle >= 360) {
    smooth_squiggle_angle -= 360;
  }
}

void smooth_drag(magic_api * api, int which,
                      SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  if (which == TOOL_SMOOTH) {
    if (num_input_points >= MAX_CTRL_POINTS) {
      return;
    }
    smooth_capture = (smooth_capture + 1) % 4;

    if (smooth_capture == 1) {
      num_input_points++;
      smooth_control_points[num_input_points].x = x;
      smooth_control_points[num_input_points].y = y;
    }
  }

  if (which == TOOL_SMOOTH) {
    api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, smooth_linecb);
  } else if (which == TOOL_SQUIGGLES || which == TOOL_LOOPS) {
    api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, smooth_squiggle_linecb);
  }

  if (which == TOOL_SMOOTH) {
    if (ox > x)
    {
      int tmp = ox;

      ox = x;
      x = tmp;
    }
    if (oy > y)
    {
      int tmp = oy;

      oy = y;
      y = tmp;
    }

    // FIXME
    update_rect->x = ox - 16;
    update_rect->y = oy - 16;
    update_rect->w = (x + 16) - update_rect->x;
    update_rect->h = (y + 16) - update_rect->y;
  } else if (which == TOOL_LOOPS) {
    // FIXME
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  } else if (which == TOOL_SQUIGGLES) {
    // FIXME
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }

  api->playsound(smooth_snds[which], (x * 255) / canvas->w, 255);
}

void smooth_click(magic_api * api,
                  int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * last,
                  int x, int y, SDL_Rect * update_rect)
{
  num_input_points = 0;
  smooth_control_points[num_input_points].x = x;
  smooth_control_points[num_input_points].y = y;

  smooth_capture = 0;
  if (which == TOOL_SQUIGGLES) {
    smooth_squiggle_angle = 0;
    smooth_squiggle_rad = 0;
    smooth_drag(api, which, canvas, last, x, y, x, y, update_rect);
  } else if (which == TOOL_LOOPS) {
    smooth_squiggle_angle = 0;
    smooth_squiggle_rad = LOOP_RAD_CALC;
    smooth_drag(api, which, canvas, last, x, y, x, y, update_rect);
  }
}


void smooth_release(magic_api * api,
                         int which ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas,
                         SDL_Surface * last,
                         int x, int y, SDL_Rect * update_rect)
{
  if (which == TOOL_SMOOTH) {
    Point2D *curve;
    int p, i, n_points;

    for (p = 0; p < 4; p++) {
      num_input_points++;
      smooth_control_points[num_input_points].x = x;
      smooth_control_points[num_input_points].y = y;
    }

    SDL_BlitSurface(last, NULL, canvas, NULL);

    for (p = 0; p < num_input_points - 3; p += 3) {
      n_points = smooth_dist(smooth_control_points[p + 0].x,
                             smooth_control_points[p + 0].y,
                             smooth_control_points[p + 1].x,
                             smooth_control_points[p + 1].y) +
                 smooth_dist(smooth_control_points[p + 1].x,
                             smooth_control_points[p + 1].y,
                             smooth_control_points[p + 2].x,
                             smooth_control_points[p + 2].y) +
                 smooth_dist(smooth_control_points[p + 2].x,
                             smooth_control_points[p + 2].y,
                             smooth_control_points[p + 3].x,
                             smooth_control_points[p + 3].y);

      if (n_points == 0)
        continue;                     // No-op; not any points to plot


      curve = (Point2D *) malloc(sizeof(Point2D) * n_points);

      smooth_ComputeBezier(smooth_control_points + p, n_points, curve);

      for (i = 0; i < n_points - 1; i++)
      {
        api->line((void *)api, which, canvas, last, curve[i].x, curve[i].y, curve[i + 1].x, curve[i + 1].y, 1, smooth_linecb);
      }

      free(curve);
      api->update_progress_bar();
    }
  }

  api->stopsound();

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void smooth_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    if (smooth_snds[i] != NULL)
      Mix_FreeChunk(smooth_snds[i]);
  }
}

void smooth_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas,
                           SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b,
                           SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  smooth_color = SDL_MapRGB(canvas->format, r, g, b);
}

int smooth_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}


/*
Code to generate a cubic Bezier curve
*/

/*
cp is a 4 element array where:
cp[0] is the starting point, or P0 in the above diagram
cp[1] is the first control point, or P1 in the above diagram
cp[2] is the second control point, or P2 in the above diagram
cp[3] is the end point, or P3 in the above diagram
t is the parameter value, 0 <= t <= 1
*/

static Point2D smooth_PointOnCubicBezier(Point2D * cp, float t)
{
  float ax, bx, cx;
  float ay, by, cy;
  float tSquared, tCubed;
  Point2D result;

  /* calculate the polynomial coefficients */

  cx = 3.0 * (cp[1].x - cp[0].x);
  bx = 3.0 * (cp[2].x - cp[1].x) - cx;
  ax = cp[3].x - cp[0].x - cx - bx;

  cy = 3.0 * (cp[1].y - cp[0].y);
  by = 3.0 * (cp[2].y - cp[1].y) - cy;
  ay = cp[3].y - cp[0].y - cy - by;

  /* calculate the curve point at parameter value t */

  tSquared = t * t;
  tCubed = tSquared * t;

  result.x = (ax * tCubed) + (bx * tSquared) + (cx * t) + cp[0].x;
  result.y = (ay * tCubed) + (by * tSquared) + (cy * t) + cp[0].y;

  return result;
}


/*
 ComputeBezier fills an array of Point2D structs with the curve
 points generated from the control points cp. Caller must
 allocate sufficient memory for the result, which is
 <sizeof(Point2D) numberOfPoints>
*/

static void smooth_ComputeBezier(Point2D * cp, int numberOfPoints, Point2D * curve)
{
  float dt;
  int i;

  dt = 1.0 / (numberOfPoints - 1);

  for (i = 0; i < numberOfPoints; i++)
    curve[i] = smooth_PointOnCubicBezier(cp, i * dt);
}

static float smooth_dist(float x1, float y1, float x2, float y2)
{
  float d;

  d = (sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));

  return d;
}

void smooth_switchin(magic_api * api ATTRIBUTE_UNUSED,
                          int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void smooth_switchout(magic_api * api ATTRIBUTE_UNUSED,
                           int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int smooth_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}


Uint8 smooth_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  return smooth_sizes[which];
}

Uint8 smooth_default_size(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  return smooth_sizes[which] / 2;
}

void smooth_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                          Uint8 size, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  smooth_size = size;
}
