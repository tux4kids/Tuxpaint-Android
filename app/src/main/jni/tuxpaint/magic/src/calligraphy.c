/*
  calligraphy.c

  Calligraphy Magic Tool Plugin
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
  $Id$
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include <math.h>

typedef struct
{
  float x, y;
} Point2D;

static Mix_Chunk *calligraphy_snd;
static Point2D calligraphy_control_points[4];
static int calligraphy_r, calligraphy_g, calligraphy_b;
static int calligraphy_old_thick;
static Uint32 calligraphy_last_time;
static SDL_Surface *calligraphy_brush, *calligraphy_colored_brush;

/* Local Function Prototypes */
static Point2D calligraphy_PointOnCubicBezier(Point2D * cp, float t);
static void calligraphy_ComputeBezier(Point2D * cp, int numberOfPoints, Point2D * curve);
static float calligraphy_dist(float x1, float y1, float x2, float y2);
int calligraphy_init(magic_api * api);
Uint32 calligraphy_api_version(void);
int calligraphy_get_tool_count(magic_api * api);
SDL_Surface *calligraphy_get_icon(magic_api * api, int which);
char *calligraphy_get_name(magic_api * api, int which);
char *calligraphy_get_description(magic_api * api, int which, int mode);
void calligraphy_drag(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void calligraphy_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void calligraphy_release(magic_api * api, int which,
                         SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);
void calligraphy_shutdown(magic_api * api);
void calligraphy_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int calligraphy_requires_colors(magic_api * api, int which);
void calligraphy_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void calligraphy_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int calligraphy_modes(magic_api * api, int which);

// No setup required:
int calligraphy_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/sounds/magic/calligraphy.ogg", api->data_directory);

  calligraphy_snd = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/images/magic/calligraphy_brush.png", api->data_directory);

  calligraphy_brush = IMG_Load(fname);
  calligraphy_colored_brush = NULL;

  if (calligraphy_brush == NULL)
    return (0);

  calligraphy_last_time = 0;

  /* (Force blit first time we get a color) */
  calligraphy_r = -1;
  calligraphy_g = -1;
  calligraphy_b = -1;

  return (1);
}

Uint32 calligraphy_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

// Only one tool:
int calligraphy_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}

// Load our icon:
SDL_Surface *calligraphy_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/calligraphy.png", api->data_directory);
  return (IMG_Load(fname));
}

// Return our name, localized:
char *calligraphy_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Calligraphy")));
}

// Return our description, localized:
char *calligraphy_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                                  int mode ATTRIBUTE_UNUSED)
{
  return (strdup(gettext_noop("Click and drag the mouse around to draw in calligraphy.")));
}


void calligraphy_drag(magic_api * api, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas,
                      SDL_Surface * last ATTRIBUTE_UNUSED, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  Point2D *curve;
  int i, n_points, thick, new_thick;
  Uint32 colr;
  SDL_Rect src, dest;

//  if (SDL_GetTicks() < calligraphy_last_time + 5)
//    return;

  calligraphy_control_points[0].x = calligraphy_control_points[1].x;
  calligraphy_control_points[0].y = calligraphy_control_points[1].y;
  calligraphy_control_points[1].x = calligraphy_control_points[2].x;
  calligraphy_control_points[1].y = calligraphy_control_points[2].y;
  calligraphy_control_points[2].x = calligraphy_control_points[3].x;
  calligraphy_control_points[2].y = calligraphy_control_points[3].y;
  calligraphy_control_points[3].x = x;
  calligraphy_control_points[3].y = y;

  calligraphy_last_time = SDL_GetTicks();


/*
  if ((calligraphy_control_points[0].x == calligraphy_control_points[1].x &&
       calligraphy_control_points[0].y == calligraphy_control_points[1].y) ||
      (calligraphy_control_points[1].x == calligraphy_control_points[2].x &&
       calligraphy_control_points[1].y == calligraphy_control_points[2].y) ||
      (calligraphy_control_points[2].x == calligraphy_control_points[3].x &&
       calligraphy_control_points[2].y == calligraphy_control_points[3].y))
    return; // No-op; not enough control points yet!
*/

  n_points = calligraphy_dist(calligraphy_control_points[0].x,
                              calligraphy_control_points[0].y,
                              calligraphy_control_points[1].x,
                              calligraphy_control_points[1].y) +
    calligraphy_dist(calligraphy_control_points[1].x,
                     calligraphy_control_points[1].y,
                     calligraphy_control_points[2].x,
                     calligraphy_control_points[2].y) +
    calligraphy_dist(calligraphy_control_points[2].x,
                     calligraphy_control_points[2].y, calligraphy_control_points[3].x, calligraphy_control_points[3].y);

  if (n_points == 0)
    return;                     // No-op; not any points to plot


  curve = (Point2D *) malloc(sizeof(Point2D) * n_points);

  calligraphy_ComputeBezier(calligraphy_control_points, n_points, curve);

  colr = SDL_MapRGB(canvas->format, calligraphy_r, calligraphy_g, calligraphy_b);

  new_thick = 40 - min((n_points /* / 2 */ ), 32);

  for (i = 0; i < n_points - 1; i++)
    {
      thick = ((new_thick * i) + (calligraphy_old_thick * (n_points - i))) / n_points;


      /* The new way, using an antialiased brush bitmap */

      x = curve[i].x;
      y = curve[i].y;

      src.x = calligraphy_brush->w - thick / 2 - thick / 4;
      src.w = thick / 2 + thick / 4;
      src.y = 0;
      src.h = thick / 4;

      dest.x = x - thick / 4;
      dest.y = y - thick / 4;

      SDL_BlitSurface(calligraphy_colored_brush, &src, canvas, &dest);


      src.x = 0;
      src.w = thick / 2 + thick / 4;
      src.y = calligraphy_brush->h - thick / 4;
      src.h = thick / 4;

      dest.x = x - thick / 2;
      dest.y = y;

      SDL_BlitSurface(calligraphy_colored_brush, &src, canvas, &dest);

      /* Old way; using putpixel:
         SDL_LockSurface(canvas);

         for (j = -(thick / 2); j < (thick / 2) + 1; j++)
         {
         x = curve[i].x + j;
         y = curve[i].y - (j / 2); // 30 degrees

         api->putpixel(canvas, x, y, colr);
         api->putpixel(canvas, x + 1, y, colr);
         api->putpixel(canvas, x, y + 1, colr);
         api->putpixel(canvas, x + 1, y + 1, colr);
         }

         SDL_UnlockSurface(canvas);
       */
    }

  calligraphy_old_thick = (calligraphy_old_thick + new_thick) / 2;

  free(curve);



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

  update_rect->x = ox - 16;
  update_rect->y = oy - 16;
  update_rect->w = (x + 16) - update_rect->x;
  update_rect->h = (y + 16) - update_rect->h;

  /* FIXME */

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(calligraphy_snd, (x * 255) / canvas->w, 255);
}

void calligraphy_click(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       int x, int y, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  calligraphy_old_thick = 8;
  calligraphy_last_time = 0;

  calligraphy_control_points[0].x = x;
  calligraphy_control_points[0].y = y;
  calligraphy_control_points[1].x = x;
  calligraphy_control_points[1].y = y;
  calligraphy_control_points[2].x = x;
  calligraphy_control_points[2].y = y;
  calligraphy_control_points[3].x = x;
  calligraphy_control_points[3].y = y;
}


void calligraphy_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                         int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void calligraphy_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (calligraphy_snd != NULL)
    Mix_FreeChunk(calligraphy_snd);
  if (calligraphy_brush != NULL)
    SDL_FreeSurface(calligraphy_brush);
  if (calligraphy_colored_brush != NULL)
    SDL_FreeSurface(calligraphy_colored_brush);
}

// We don't use colors
void calligraphy_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b)
{
  int x, y;
  Uint8 a;
  Uint32 amask;

  if (calligraphy_r == r && calligraphy_g == g && calligraphy_b == b)
    return;

  calligraphy_r = r;
  calligraphy_g = g;
  calligraphy_b = b;

  if (calligraphy_colored_brush != NULL)
    SDL_FreeSurface(calligraphy_colored_brush);

  amask = ~(calligraphy_brush->format->Rmask | calligraphy_brush->format->Gmask | calligraphy_brush->format->Bmask);

  calligraphy_colored_brush =
    SDL_CreateRGBSurface(SDL_SWSURFACE,
                         calligraphy_brush->w,
                         calligraphy_brush->h,
                         calligraphy_brush->format->BitsPerPixel,
                         calligraphy_brush->format->Rmask,
                         calligraphy_brush->format->Gmask, calligraphy_brush->format->Bmask, amask);

  if (calligraphy_colored_brush == NULL)
    return;                     // FIXME: Error!

  SDL_LockSurface(calligraphy_brush);
  SDL_LockSurface(calligraphy_colored_brush);


  for (y = 0; y < calligraphy_brush->h; y++)
    {
      for (x = 0; x < calligraphy_brush->w; x++)
        {
          SDL_GetRGBA(api->getpixel(calligraphy_brush, x, y), calligraphy_brush->format, &r, &g, &b, &a);

          api->putpixel(calligraphy_colored_brush, x, y,
                        SDL_MapRGBA(calligraphy_colored_brush->format, calligraphy_r, calligraphy_g, calligraphy_b, a));
        }
    }

  SDL_UnlockSurface(calligraphy_colored_brush);
  SDL_UnlockSurface(calligraphy_brush);
}

// We don't use colors
int calligraphy_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
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

static Point2D calligraphy_PointOnCubicBezier(Point2D * cp, float t)
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

static void calligraphy_ComputeBezier(Point2D * cp, int numberOfPoints, Point2D * curve)
{
  float dt;
  int i;

  dt = 1.0 / (numberOfPoints - 1);

  for (i = 0; i < numberOfPoints; i++)
    curve[i] = calligraphy_PointOnCubicBezier(cp, i * dt);
}

static float calligraphy_dist(float x1, float y1, float x2, float y2)
{
  float d;

  d = (sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));

  return d;
}

void calligraphy_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                          SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void calligraphy_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

int calligraphy_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT);
}
