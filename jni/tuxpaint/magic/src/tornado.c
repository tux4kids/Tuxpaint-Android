/*
  tornado.c

  Tornado Magic Tool Plugin
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2008 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

  Some modifications to convert the flower plugin in to a tornado
  plugin by Pere Pujal i Carabantes
  pere@fornol.no-ip.org

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

  Last updated: May 29, 2009
  $Id: tornado.c,v 1.6 2011/05/13 10:05:57 perepujal Exp $
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our globals: */

enum { SIDE_LEFT, SIDE_RIGHT };
enum { LEAFSIDE_RIGHT_DOWN,
       LEAFSIDE_LEFT_DOWN,
       LEAFSIDE_RIGHT_UP,
       LEAFSIDE_LEFT_UP };

static Mix_Chunk /* * tornado_click_snd, */ * tornado_release_snd;
static Uint8 tornado_r, tornado_g, tornado_b;
static int tornado_min_x, tornado_max_x, tornado_bottom_x, tornado_bottom_y;
static int tornado_side_first;
static int tornado_side_decided;
static SDL_Surface * tornado_base, * tornado_cloud,
  * tornado_cloud_colorized;
  static int top_w;

/* Local function prototypes: */

typedef struct
{
  float x, y;
} Point2D;

static void tornado_predrag(magic_api * api, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y);
static void tornado_drawbase(magic_api * api, SDL_Surface * canvas);
static void tornado_drawstalk(magic_api * api, SDL_Surface * canvas, SDL_Surface * last,
		      int top_x, int top_y, int minx, int maxx,
		      int bottom_x, int bottom_y, int final);
static void tornado_drawtornado(magic_api * api, SDL_Surface * canvas, int x, int y);
static Point2D tornado_PointOnCubicBezier(Point2D* cp, float t);
static void tornado_ComputeBezier(Point2D* cp, int numberOfPoints, Point2D* curve);
static void tornado_colorize_cloud(magic_api * api);
static Uint32 tornado_mess(Uint32 pixel, SDL_Surface * canvas);
Uint32 tornado_api_version(void);
int tornado_init(magic_api * api);
int tornado_get_tool_count(magic_api * api);
SDL_Surface * tornado_get_icon(magic_api * api, int which);
char * tornado_get_name(magic_api * api, int which);

char * tornado_get_description(magic_api * api, int which, int mode);






void tornado_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void tornado_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect);
void tornado_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * last,
		     int x, int y, SDL_Rect * update_rect);

void tornado_shutdown(magic_api * api);
void tornado_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
int tornado_requires_colors(magic_api * api, int which);
void tornado_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void tornado_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int tornado_modes(magic_api * api, int which);



Uint32 tornado_api_version(void) { return(TP_MAGIC_API_VERSION); }


// No setup required:
int tornado_init(magic_api * api)
{
  char fname[1024];

/*
  snprintf(fname, sizeof(fname), "%s/sounds/magic/tornado_click.ogg",
	    api->data_directory);
  tornado_click_snd = Mix_LoadWAV(fname);
*/

  snprintf(fname, sizeof(fname), "%s/sounds/magic/tornado_release.ogg",
	    api->data_directory);
  tornado_release_snd = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%s/images/magic/tornado_base.png",
	    api->data_directory);
  tornado_base = IMG_Load(fname);

  snprintf(fname, sizeof(fname), "%s/images/magic/tornado_cloud.png",
	    api->data_directory);
  tornado_cloud = IMG_Load(fname);

  return(1);
}

// We have multiple tools:
int tornado_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(1);
}

// Load our icons:
SDL_Surface * tornado_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/tornado.png",
	   api->data_directory);

  return(IMG_Load(fname));
}

// Return our names, localized:
char * tornado_get_name(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Tornado")));
}

// Return our descriptions, localized:
char * tornado_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click and drag to draw a tornado funnel on your picture.")));
}

// Affect the canvas on drag:
static void tornado_predrag(magic_api * api ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
	          SDL_Surface * last ATTRIBUTE_UNUSED, int ox, int oy, int x, int y)
{
  if (x < tornado_min_x)
    tornado_min_x = x;
  if (ox < tornado_min_x)
    tornado_min_x = ox;
  if (x > tornado_max_x)
    tornado_max_x = x;
  if (ox > tornado_max_x)
    tornado_max_x = ox;

  if (y > tornado_bottom_y)
    y = tornado_bottom_y;
  if (oy > tornado_bottom_y)
    y = tornado_bottom_y;

  // Determine which way to bend first:
  //
  if (tornado_side_decided == 0)
  {
    if (x < tornado_bottom_x - 10)
    {
      tornado_side_first = SIDE_LEFT;
      tornado_side_decided = 1;
    }
    else if (x > tornado_bottom_x + 10)
    {
      tornado_side_first = SIDE_RIGHT;
      tornado_side_decided = 1;
    }
  }
}

void tornado_drag(magic_api * api, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas,
	          SDL_Surface * last, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  tornado_predrag(api, canvas, last, ox, oy, x, y);


  /* Erase any old stuff; this is a live-edited effect: */

  SDL_BlitSurface(last, NULL, canvas, NULL);


  /* Draw the base and the stalk (low-quality) for now: */

  tornado_drawstalk(api, canvas, last,
		   x, y, tornado_min_x, tornado_max_x,
		   tornado_bottom_x, tornado_bottom_y, !(api->button_down()));
  
  tornado_drawbase(api, canvas);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w; 
  update_rect->h = canvas->h;
}

// Affect the canvas on click:
void tornado_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  tornado_min_x = x;
  tornado_max_x = x;
  tornado_bottom_x = x;
  tornado_bottom_y = y;// - tornado_base->h;

  tornado_side_decided = 0;
  tornado_side_first = SIDE_LEFT;

  tornado_drag(api, which, canvas, last, x, y, x, y, update_rect);
 
/* 
  api->playsound(tornado_click_snd, (x * 255) / canvas->w, 255);
*/
}

// Affect the canvas on release:
void tornado_release(magic_api * api, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * last,
	           int x, int y, SDL_Rect * update_rect)
{
  /* Don't let tornado be too low compared to base: */

  if (y >= tornado_bottom_y - 128)
    y = tornado_bottom_y - 128;


  /* Do final calcs and draw base: */

  tornado_predrag(api, canvas, last, x, y, x, y);

  
  /* Erase any old stuff: */

  SDL_BlitSurface(last, NULL, canvas, NULL);


  /* Draw high-quality stalk, and tornado: */

  tornado_drawstalk(api, canvas, last,
		   x, y, tornado_min_x, tornado_max_x,
		   tornado_bottom_x, tornado_bottom_y, 1);

   tornado_drawtornado(api, canvas, x, y);

    tornado_drawbase(api, canvas);

  
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(tornado_release_snd, (x * 255) / canvas->w, 255);
}


static void tornado_drawtornado(magic_api * api, SDL_Surface * canvas, int x, int y)
{
  SDL_Surface * aux_surf;
  SDL_Rect dest;

  aux_surf = api->scale(tornado_cloud_colorized, top_w *2, top_w,0);
  dest.x = x - (aux_surf->w / 2);
  dest.y = y - (aux_surf->h / 2);

  SDL_BlitSurface(aux_surf, NULL, canvas, &dest);
  SDL_FreeSurface(aux_surf);
}

static void tornado_drawbase(magic_api * api ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  SDL_Rect dest;

  dest.x = tornado_bottom_x - (tornado_base->w / 2);
  dest.y = tornado_bottom_y - tornado_base->h / 2;

  SDL_BlitSurface(tornado_base, NULL, canvas, &dest);
}

static Uint32 tornado_mess(Uint32 pixel, SDL_Surface * canvas)
{
  Uint8 r, g, b, a;
  float f = (float)rand()*255/RAND_MAX;
  SDL_GetRGBA(pixel, canvas->format, &r, &g, &b, &a);
  return (SDL_MapRGBA(canvas->format,
		      (tornado_r + r + (Uint8)f * 2) / 4,
		      (tornado_g + g + (Uint8)f * 2) / 4,
		      (tornado_b + b + (Uint8)f * 2) / 4,
		      a));
}

static void tornado_drawstalk(magic_api * api, SDL_Surface * canvas, SDL_Surface * last,
		      int top_x, int top_y, int minx, int maxx,
		      int bottom_x, int bottom_y, int final)
{
  Point2D control_points[4];
  Point2D * curve;
  int i, n_points;
  int left, right;
  SDL_Rect dest;
  int rotation = 0;
  int p;
  int ii, ww;

  /* Compute a nice bezier curve for the stalk, based on the
     base (x,y), leftmost (x), rightmost (x), and top (x,y) */

  control_points[0].x = top_x;
  control_points[0].y = top_y;

  if (tornado_side_first == SIDE_LEFT)
  {
    control_points[1].x = minx;
    control_points[2].x = maxx;
  }
  else
  {
    control_points[1].x = maxx;
    control_points[2].x = minx;
  }

  control_points[1].y = ((bottom_y - top_y) / 3) + top_y;
  control_points[2].y = (((bottom_y - top_y) / 3) * 2) + top_y;
 
  control_points[3].x = bottom_x;
  control_points[3].y = bottom_y;
 
  if (final == 0)
    n_points = 8;
  else
    n_points = max(bottom_y - top_y, maxx - minx);

  curve = (Point2D *) malloc(sizeof(Point2D) * n_points);

  tornado_ComputeBezier(control_points, n_points, curve);
  if (n_points * n_points / 1000 > canvas->w / 2)
    top_w = canvas->w / 2;
  else
    top_w = max(32, n_points * n_points / 1000);

  /* Draw the curve: */
 
  for (i = 0; i < n_points - 1; i++)
  {
    if (final == 0)
    {
      dest.x = curve[i].x;
      dest.y = curve[i].y;
      dest.w = 2;
      dest.h = 2;
      SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, 0, 0, 0));
    }
    else
    {
      ii = n_points - i;
      /* min 10 pixels then ii^2 / 2000 or 4 * ii^2 / canvas->w,
	 don't let the top of funnel be wider than the half of canvas */
      if (n_points * n_points / 2000 > canvas->w / 4)
	ww = 4 * n_points * n_points / canvas->w;
      else
	ww = 2000;

      left = min(curve[i].x, curve[i + 1].x)-5-ii*ii/ww;
      right = max(curve[i].x, curve[i + 1].x)+5+ii*ii/ww;

      dest.x = left;
      dest.y = curve[i].y;
      dest.w = right - left + 1;
      dest.h = 2;
    }

    rotation +=3;
    /* The body of the tornado: 3x 1y rotation + some random particles */
    for (p = dest.x; p < dest.x + dest.w; p++)
      {
	if ((float)rand() * 100 / RAND_MAX > 10 )
	  {
	    api->putpixel(canvas, p, dest.y, api->getpixel(last, dest.x + (p - dest.x + rotation) % dest.w , dest.y));
	  }
	else
	  {
	    api->putpixel(canvas, p, dest.y, tornado_mess(api->getpixel(last, dest.x + (p - dest.x + rotation) % dest.w , dest.y), canvas));
	  }
      }
 
   /* Some random particles flying around the tornado */
    for (p = dest.x - dest.w * 20 / 100; p < dest.x + dest.w + dest.w * 20 / 100; p++)
      {
	if ((float)rand() * 100 / RAND_MAX < 5 && ((p < dest.x) || (p > dest.w)))
	  api->putpixel(canvas, p, dest.y, tornado_mess(api->getpixel(last, dest.x + (p - dest.x + rotation) % dest.w , dest.y), canvas));
      }
  } 

  free(curve);
}

void tornado_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
/*
  if (tornado_click_snd != NULL)
    Mix_FreeChunk(tornado_click_snd);
*/

  if (tornado_release_snd != NULL)
    Mix_FreeChunk(tornado_release_snd);

  if (tornado_base != NULL)
    SDL_FreeSurface(tornado_base);
  if (tornado_cloud != NULL)
    SDL_FreeSurface(tornado_cloud);
  if (tornado_cloud_colorized != NULL)
    SDL_FreeSurface(tornado_cloud_colorized);
}

// Record the color from Tux Paint:
void tornado_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b)
{
  tornado_r = r;
  tornado_g = g;
  tornado_b = b;

  tornado_colorize_cloud(api);
}

// Use colors:
int tornado_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
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

static Point2D tornado_PointOnCubicBezier( Point2D* cp, float t )
{
    float   ax, bx, cx;
    float   ay, by, cy;
    float   tSquared, tCubed;
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

static void tornado_ComputeBezier( Point2D* cp, int numberOfPoints, Point2D* curve )
{
    float   dt;
    int   i;

    dt = 1.0 / ( numberOfPoints - 1 );

    for( i = 0; i < numberOfPoints; i++)
        curve[i] = tornado_PointOnCubicBezier( cp, i*dt );
}


static void tornado_colorize_cloud(magic_api * api)
{
  Uint32 amask;
  int x, y;
  Uint8 r, g, b, a;

  if (tornado_cloud_colorized != NULL)
    SDL_FreeSurface(tornado_cloud_colorized);

  /* Create a surface to render into: */

  amask = ~(tornado_cloud->format->Rmask |
            tornado_cloud->format->Gmask |
            tornado_cloud->format->Bmask);

  tornado_cloud_colorized =
    SDL_CreateRGBSurface(SDL_SWSURFACE,
                         tornado_cloud->w,
                         tornado_cloud->h,
                         tornado_cloud->format->BitsPerPixel,
                         tornado_cloud->format->Rmask,
                         tornado_cloud->format->Gmask,
                         tornado_cloud->format->Bmask, amask);

  /* Render the new cloud: */

  SDL_LockSurface(tornado_cloud);
  SDL_LockSurface(tornado_cloud_colorized);

  for (y = 0; y < tornado_cloud->h; y++)
  {
    for (x = 0; x < tornado_cloud->w; x++)
    {
      SDL_GetRGBA(api->getpixel(tornado_cloud, x, y),
                  tornado_cloud->format, &r, &g, &b, &a);

      api->putpixel(tornado_cloud_colorized, x, y,
                    SDL_MapRGBA(tornado_cloud_colorized->format,
				(tornado_r + r * 2) / 3, (tornado_g + g * 2) / 3, (tornado_b + b * 2) / 3, a));
    }
  }

  SDL_UnlockSurface(tornado_cloud_colorized);
  SDL_UnlockSurface(tornado_cloud);
}

void tornado_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void tornado_switchout(magic_api * api, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  api->stopsound();
}

int tornado_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT_WITH_PREVIEW);
}
