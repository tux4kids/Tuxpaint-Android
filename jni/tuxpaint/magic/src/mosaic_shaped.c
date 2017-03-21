/*
  mosaic_shaped.c

  mosaic_shaped, Add a mosaic_shaped effect to the image using a combination of other tools.
  Tux Paint - A simple drawing program for children.

  Credits:
  * Andrew Corcoran <akanewbie@gmail.com> for the edge step used in hexagon
  * Whoever who wrote the "Fill" magic tool
  * Bill Kendrick for the code derived from api->touched
  * Pere Pujal for joining all toghether
  * Caroline Ford for the text descriptions

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
  $Id: mosaic_shaped.c,v 1.11 2016/09/19 20:56:22 perepujal Exp $
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

static void mosaic_shaped_sharpen_pixel(void * ptr, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void reset_counter(SDL_Surface * canvas, Uint8 * counter);
static void fill_square(magic_api * api, SDL_Surface * canvas, int x, int y, int size, Uint32 color);
static void deform(magic_api * api, SDL_Surface * srfc);
static void do_mosaic_shaped_full(void * ptr, SDL_Surface * canvas, SDL_Surface * last, int which, SDL_Rect * update_rect);
static void mosaic_shaped_fill(void * ptr_to_api, int which, SDL_Surface * canvas,
                               SDL_Surface * last, int x, int y);
static void mosaic_shaped_paint(void * ptr, int which, SDL_Surface * canvas,
                               SDL_Surface * last, int x, int y);

Uint32 mosaic_shaped_api_version(void);
int mosaic_shaped_init(magic_api * api);
int mosaic_shaped_get_tool_count(magic_api * api);
SDL_Surface * mosaic_shaped_get_icon(magic_api * api, int which);
char * mosaic_shaped_get_name(magic_api * api, int which);

char * mosaic_shaped_get_description(magic_api * api, int which, int mode);

void mosaic_shaped_drag(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * last, int ox, int oy, int x, int y,
                        SDL_Rect * update_rect);

void mosaic_shaped_click(magic_api * api, int which, int mode,
                         SDL_Surface * canvas, SDL_Surface * last,
                         int x, int y, SDL_Rect * update_rect);

void mosaic_shaped_release(magic_api * api, int which,
                           SDL_Surface * canvas, SDL_Surface * last,
                           int x, int y, SDL_Rect * update_rect);

void mosaic_shaped_shutdown(magic_api * api);

void mosaic_shaped_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);

int mosaic_shaped_requires_colors(magic_api * api, int which);

void mosaic_shaped_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);

void mosaic_shaped_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);

int mosaic_shaped_modes(magic_api * api, int which);

int scan_fill(magic_api * api, SDL_Surface * canvas, SDL_Surface * srfc, int x, int y, int fill_edge, int fill_tile, int size, Uint32 color);

Uint8 * mosaic_shaped_counted;
Uint8 * mosaic_shaped_done;
Uint8 mosaic_shaped_r, mosaic_shaped_g, mosaic_shaped_b;

int mosaic_shaped_average_r, mosaic_shaped_average_g, mosaic_shaped_average_b, mosaic_shaped_average_count;
Uint32 pixel_average, black, white;

enum
{
    TOOL_SQUARE,
    TOOL_HEX,
    TOOL_IRREGULAR,
    mosaic_shaped_NUM_TOOLS
};

static Mix_Chunk * mosaic_shaped_snd_effect[mosaic_shaped_NUM_TOOLS];
static SDL_Surface * canvas_shaped;
static SDL_Surface * canvas_back;
static SDL_Surface * mosaic_shaped_pattern;

const char * mosaic_shaped_snd_filenames[mosaic_shaped_NUM_TOOLS] =
{
    "mosaic_shaped_square.ogg",
    "mosaic_shaped_hex.ogg",
    "mosaic_shaped_irregular.ogg", /* FIXME */ /*what's problem?*/
};

const char * mosaic_shaped_icon_filenames[mosaic_shaped_NUM_TOOLS] =
{
    "mosaic_shaped_square.png",
    "mosaic_shaped_hex.png",
    "mosaic_shaped_irregular.png",
};

const char * mosaic_shaped_pattern_filenames[mosaic_shaped_NUM_TOOLS] =
{
    "mosaic_shaped_square_pattern.png",
    "mosaic_shaped_hex_pattern.png",
    "mosaic_shaped_irregular_pattern.png",
};

const char * mosaic_shaped_names[mosaic_shaped_NUM_TOOLS] =
{
    gettext_noop("Square Mosaic"),
    gettext_noop("Hexagon Mosaic"),
    gettext_noop("Irregular Mosaic"),

};

const char * mosaic_shaped_descs[mosaic_shaped_NUM_TOOLS][2] =
{
    {
        gettext_noop("Click and drag the mouse to add a square mosaic to parts of your picture."),
        gettext_noop("Click to add a square mosaic to your entire picture."),
    },

    {
        gettext_noop("Click and drag the mouse to add a hexagonal mosaic to parts of your picture."),
        gettext_noop("Click to add a hexagonal mosaic to your entire picture."),
    },

    {
        gettext_noop("Click and drag the mouse to add an irregular mosaic to parts of your picture."),
        gettext_noop("Click to add an irregular mosaic to your entire picture."),
    },
};

Uint32 mosaic_shaped_api_version(void)
{
    return (TP_MAGIC_API_VERSION);
}

//Load sounds
int mosaic_shaped_init(magic_api * api)
{
    int i;
    char fname[1024];
    mosaic_shaped_pattern = NULL;

    for (i = 0; i < mosaic_shaped_NUM_TOOLS; i++)
    {
        snprintf(fname, sizeof(fname), "%ssounds/magic/%s", api->data_directory, mosaic_shaped_snd_filenames[i]);
        mosaic_shaped_snd_effect[i] = Mix_LoadWAV(fname);
    }

    return (1);
}

int mosaic_shaped_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
    return (mosaic_shaped_NUM_TOOLS);
}

// Load our icons:
SDL_Surface * mosaic_shaped_get_icon(magic_api * api, int which)
{
    char fname[1024];
    snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, mosaic_shaped_icon_filenames[which]);
    return (IMG_Load(fname));
}

// Return our names, localized:
char * mosaic_shaped_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
    return (strdup(gettext_noop(mosaic_shaped_names[which])));
}

// Return our descriptions, localized:
char * mosaic_shaped_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
    return (strdup(gettext_noop(mosaic_shaped_descs[which][mode - 1])));
}

//Calculates the grey scale value for a rgb pixel
static int mosaic_shaped_grey(Uint8 r1, Uint8 g1, Uint8 b1)
{
    return 0.3 * r1 + .59 * g1 + 0.11 * b1;
}

// Do the effect for the full image
static void do_mosaic_shaped_full(void * ptr, SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
    int i, j, size;
    Uint32 mosaic_shaped_color;
    magic_api * api = (magic_api *) ptr;
    mosaic_shaped_color = SDL_MapRGBA(canvas->format, mosaic_shaped_r, mosaic_shaped_g, mosaic_shaped_b, 0);

    for (i = 3; i < canvas->w - 3; i += 2)
    {
        api->playsound(mosaic_shaped_snd_effect[which], 128, 255);
        api->update_progress_bar();

        for (j = 3; j < canvas->h - 3; j += 2)
        {
            if (!mosaic_shaped_done[canvas->w * j + i])
                if (!mosaic_shaped_counted[canvas->w * j + i])
                    if (api->getpixel(canvas_shaped, i, j) != black)
                    {
                        mosaic_shaped_average_r = 0;
                        mosaic_shaped_average_g = 0;
                        mosaic_shaped_average_b = 0;
                        mosaic_shaped_average_count = 0;
                        scan_fill(api, canvas, canvas_shaped, i, j, 1, 0, 1, mosaic_shaped_color);

                        if (mosaic_shaped_average_count > 0)
                        {
                            reset_counter(canvas, mosaic_shaped_counted);
                            size = 0;
                            pixel_average = SDL_MapRGB(canvas->format, mosaic_shaped_average_r / mosaic_shaped_average_count, mosaic_shaped_average_g / mosaic_shaped_average_count, mosaic_shaped_average_b / mosaic_shaped_average_count);
                            scan_fill(api, canvas, canvas_shaped, i, j, 0, 1, size, pixel_average);
                        }
                    }
        }
    }
}

/* Fills a tesera */
static void mosaic_shaped_fill(void * ptr_to_api, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas,
                               SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
    Uint32 mosaic_shaped_color;
    int size;
    magic_api * api = (magic_api *) ptr_to_api;
    x = clamp(0, x, canvas->w - 1);
    y = clamp(0, y, canvas->h - 1);
    mosaic_shaped_color = SDL_MapRGBA(canvas->format, mosaic_shaped_r, mosaic_shaped_g, mosaic_shaped_b, 0);
    mosaic_shaped_average_r = 0;
    mosaic_shaped_average_g = 0;
    mosaic_shaped_average_b = 0;
    mosaic_shaped_average_count = 0;

    if (api->getpixel(canvas_shaped, x, y) == black)
    {
        return;
    }

    scan_fill(api, canvas, canvas_shaped, x, y, 1, 0, 1, mosaic_shaped_color);

    if (mosaic_shaped_average_count > 0)
    {
        size = 0;
        pixel_average = SDL_MapRGB(canvas->format, mosaic_shaped_average_r / mosaic_shaped_average_count, mosaic_shaped_average_g / mosaic_shaped_average_count, mosaic_shaped_average_b / mosaic_shaped_average_count);
        reset_counter(canvas, mosaic_shaped_counted);
        scan_fill(api, canvas, canvas_shaped, x, y, 0, 1, size, pixel_average);
    }
}

// Affect the canvas on drag:
void mosaic_shaped_drag(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * last, int ox, int oy, int x, int y,
                        SDL_Rect * update_rect)
{
    api->line(api, which, canvas, last, ox, oy, x, y, 1, mosaic_shaped_fill);
    update_rect->x = min(ox, x) - mosaic_shaped_pattern->w;
    update_rect->y = min(oy, y) - mosaic_shaped_pattern->h;
    update_rect->w = max(ox, x) + mosaic_shaped_pattern->w - update_rect->x;
    update_rect->h = max(oy, y) + mosaic_shaped_pattern->h - update_rect->y;
    api->playsound(mosaic_shaped_snd_effect[which], (x * 255) / canvas->w, 255);
}

// Affect the canvas on click:
void mosaic_shaped_click(magic_api * api, int which, int mode,
                         SDL_Surface * canvas, SDL_Surface * last,
                         int x, int y, SDL_Rect * update_rect)
{
    if (mode == MODE_FULLSCREEN)
    {
        update_rect->x = 0;
        update_rect->y = 0;
        update_rect->w = canvas->w;
        update_rect->h = canvas->h;
        do_mosaic_shaped_full(api, canvas,  last, which, update_rect);
        api->playsound(mosaic_shaped_snd_effect[which], 128, 255);
    }
    else
    {
        mosaic_shaped_drag(api, which, canvas, last, x, y, x, y, update_rect);
    }
}

// Affect the canvas on release:
void mosaic_shaped_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void mosaic_shaped_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
    //Clean up sounds
    int i;

    for (i = 0; i < mosaic_shaped_NUM_TOOLS; i++)
    {
        if (mosaic_shaped_snd_effect[i] != NULL)
        {
            Mix_FreeChunk(mosaic_shaped_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void mosaic_shaped_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b)
{
    mosaic_shaped_r = r;
    mosaic_shaped_g = g;
    mosaic_shaped_b = b;
}

// Use colors:
int mosaic_shaped_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
    return 1;
}

//Sharpen a pixel
static void mosaic_shaped_sharpen_pixel(void * ptr,
                                        SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last,
                                        int x, int y)
{
    magic_api * api = (magic_api *) ptr;
    Uint8 r1, g1, b1;
    int grey;
    int i, j;
    double sobel_1 = 0, sobel_2 = 0;
    double temp;
    //Sobel weighting masks
    const int sobel_weights_1[3][3] = {	{1, 2, 1},
        {0, 0, 0},
        { -1, -2, -1}
    };
    const int sobel_weights_2[3][3] = {	{ -1, 0, 1},
        { -2, 0, 2},
        { -1, 0, 1}
    };
    sobel_1 = 0;
    sobel_2 = 0;

    for (i = -1; i < 2; i++)
    {
        for (j = -1; j < 2; j++)
        {
            //No need to check if inside canvas, getpixel does it for us.
            SDL_GetRGB(api->getpixel(last, x + i, y + j), last->format, &r1, &g1, &b1);
            grey = mosaic_shaped_grey(r1, g1, b1);
            sobel_1 += grey * sobel_weights_1[i + 1][j + 1];
            sobel_2 += grey * sobel_weights_2[i + 1][j + 1];
        }
    }

    temp = sqrt(sobel_1 * sobel_1 + sobel_2 * sobel_2);
    temp = (temp / 1443) * 255.0;

    if (temp > 25)
    {
        api->putpixel(canvas_shaped, x, y, SDL_MapRGBA(canvas_shaped->format, 0, 0, 0, 0));
    }
}



void mosaic_shaped_switchin(magic_api * api, int which, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
    int y, x;
    int i, j;
    SDL_Rect  rect;
    SDL_Surface * surf_aux, *tmp, *tmp2;
    Uint32 amask;
    char fname[1024];
    mosaic_shaped_counted = (Uint8 *) malloc(sizeof(Uint8) * (canvas->w * canvas->h));

    if (mosaic_shaped_counted == NULL)
    {
        fprintf(stderr, "\nError: Can't build drawing touch mask!\n");
        exit(1);
    }

    mosaic_shaped_done = (Uint8 *) malloc(sizeof(Uint8) * (canvas->w * canvas->h));

    if (mosaic_shaped_done == NULL)
    {
        fprintf(stderr, "\nError: Can't build drawing touch mask!\n");
        exit(1);
    }

    amask = ~(canvas->format->Rmask |
              canvas->format->Gmask |
              canvas->format->Bmask);

    tmp = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                         canvas->w,
                                         canvas->h,
                                         canvas->format->BitsPerPixel,
                                         canvas->format->Rmask,
                                         canvas->format->Gmask,
                                         canvas->format->Bmask, amask);
    canvas_shaped = SDL_ConvertSurfaceFormat(tmp, SDL_PIXELFORMAT_RGB888, 0);
    SDL_FreeSurface(tmp);

    tmp2 = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    canvas->w + 10,
                                    canvas->h + 10,
                                    canvas->format->BitsPerPixel,
                                    canvas->format->Rmask,
                                    canvas->format->Gmask,
                                    canvas->format->Bmask, amask);
    surf_aux =  SDL_ConvertSurfaceFormat(tmp2, SDL_PIXELFORMAT_RGB888, 0);
    SDL_FreeSurface(tmp2);

    mosaic_shaped_pattern = IMG_Load(fname);
    SDL_SetSurfaceBlendMode (mosaic_shaped_pattern, SDL_BLENDMODE_NONE);
    /* Generation of patterns now in the program, solves #210 */
    if (which == TOOL_SQUARE)
    {
      mosaic_shaped_pattern = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    16,
                                    16,
                                    canvas->format->BitsPerPixel,
                                    canvas->format->Rmask,
                                    canvas->format->Gmask,
                                    canvas->format->Bmask, amask);
      SDL_FillRect(mosaic_shaped_pattern, NULL, SDL_MapRGBA(mosaic_shaped_pattern->format, 255, 255, 255, SDL_ALPHA_OPAQUE));
      /* Shape */
      for (i = 0;i < mosaic_shaped_pattern->w; i++)
      {
	api->putpixel(mosaic_shaped_pattern, 0, i, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
	api->putpixel(mosaic_shaped_pattern, mosaic_shaped_pattern->w - 1, i, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
	api->putpixel(mosaic_shaped_pattern, i, 0, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
	api->putpixel(mosaic_shaped_pattern, i, mosaic_shaped_pattern->w - 1, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
      }
      /* Shadow */
      for  (i = 1; i < mosaic_shaped_pattern->w - 1; i++)
      {
	api->putpixel(mosaic_shaped_pattern, 1, i, SDL_MapRGBA(mosaic_shaped_pattern->format, 128, 128, 128, SDL_ALPHA_OPAQUE));
	api->putpixel(mosaic_shaped_pattern, mosaic_shaped_pattern->w - 2, i, SDL_MapRGBA(mosaic_shaped_pattern->format, 128, 128, 128, SDL_ALPHA_OPAQUE));
	api->putpixel(mosaic_shaped_pattern, i, 1, SDL_MapRGBA(mosaic_shaped_pattern->format, 128, 128, 128, SDL_ALPHA_OPAQUE));
	api->putpixel(mosaic_shaped_pattern, i, mosaic_shaped_pattern->w - 2, SDL_MapRGBA(mosaic_shaped_pattern->format, 128, 128, 128, SDL_ALPHA_OPAQUE));

      }
      api->putpixel(mosaic_shaped_pattern, 2, 2, SDL_MapRGBA(mosaic_shaped_pattern->format, 152, 152, 152, SDL_ALPHA_OPAQUE));
      api->putpixel(mosaic_shaped_pattern, 2, mosaic_shaped_pattern->h - 3, SDL_MapRGBA(mosaic_shaped_pattern->format, 152, 152, 152, SDL_ALPHA_OPAQUE));
      api->putpixel(mosaic_shaped_pattern, mosaic_shaped_pattern->w - 3, 2, SDL_MapRGBA(mosaic_shaped_pattern->format, 152, 152, 152, SDL_ALPHA_OPAQUE));
      api->putpixel(mosaic_shaped_pattern, mosaic_shaped_pattern->w - 3, mosaic_shaped_pattern->h - 3, SDL_MapRGBA(mosaic_shaped_pattern->format, 152, 152, 152, SDL_ALPHA_OPAQUE));
    }
    else if (which == TOOL_IRREGULAR)
    {
      mosaic_shaped_pattern = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    64,
                                    64,
                                    canvas->format->BitsPerPixel,
                                    canvas->format->Rmask,
                                    canvas->format->Gmask,
                                    canvas->format->Bmask, amask);
      SDL_FillRect(mosaic_shaped_pattern, NULL, SDL_MapRGBA(mosaic_shaped_pattern->format, 255, 255, 255, SDL_ALPHA_OPAQUE));

      /* Start/end of lines taken from the original mosaic_shaped_irregular_pattern.png */
      api->line(api, which, mosaic_shaped_pattern, NULL, 0, 8, 36, 23, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 0, 43, 36, 23, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 0, 26, 28, 53, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 0, 54, 10, 63, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 55, 0, 36, 23, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 63, 43, 28, 53, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 24, 63, 28, 53, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 24, 0, 27, 19, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 63, 8, 50, 6, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 10, 0, 4, 10, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 10, 0, 25,7, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 41, 0, 26,12, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 41, 63, 28, 53, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 41, 63, 56, 58, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 63, 53, 55, 45, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 55, 63, 59, 49, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 10, 63, 20, 45, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 63, 26, 40, 18, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 4, 30, 14, 14, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 18, 33, 21, 17, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 23, 48, 29, 27, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 37, 50, 36, 23, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 44, 13, 37, 3, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 59, 24, 55, 7, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 49, 47, 54, 23, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 36, 35, 51, 37, 1, mosaic_shaped_paint);
      api->line(api, which, mosaic_shaped_pattern, NULL, 61, 44, 52, 31, 1, mosaic_shaped_paint);
    }

    else if(which == TOOL_HEX)
    {
        mosaic_shaped_pattern = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                48,
                                28,
                                canvas->format->BitsPerPixel,
                                canvas->format->Rmask,
                                canvas->format->Gmask,
                                canvas->format->Bmask, amask);
        SDL_FillRect(mosaic_shaped_pattern, NULL, SDL_MapRGBA(mosaic_shaped_pattern->format, 255, 255, 255, SDL_ALPHA_OPAQUE));

        api->line(api, which, mosaic_shaped_pattern, NULL, 0, 16, 8, 0, 1, mosaic_shaped_paint);
        api->putpixel(mosaic_shaped_pattern, 26 , 27, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
        api->putpixel(mosaic_shaped_pattern, 26 , 26, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
        api->putpixel(mosaic_shaped_pattern, 26 , 25, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
        api->line(api, which, mosaic_shaped_pattern, NULL, 8, 0, 26, 0, 1, mosaic_shaped_paint);
        api->line(api, which, mosaic_shaped_pattern, NULL, 26, 0, 32, 14, 1, mosaic_shaped_paint);
        api->line(api, which, mosaic_shaped_pattern, NULL, 32, 14, 26, 27, 1, mosaic_shaped_paint);
        api->line(api, which, mosaic_shaped_pattern, NULL, 32, 14, 47, 14, 1, mosaic_shaped_paint);
        api->line(api, which, mosaic_shaped_pattern, NULL, 0,13 ,8, 27, 1, mosaic_shaped_paint);

        //make pattern more accurate
        api->putpixel(mosaic_shaped_pattern, 9 , 27, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
        api->putpixel(mosaic_shaped_pattern, 9 , 26, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
        api->putpixel(mosaic_shaped_pattern, 25 , 27, SDL_MapRGBA(mosaic_shaped_pattern->format, 0, 0, 0, SDL_ALPHA_OPAQUE));

    }

    SDL_SetSurfaceBlendMode (mosaic_shaped_pattern, SDL_BLENDMODE_NONE);
    rect.w = mosaic_shaped_pattern->w;
    rect.h = mosaic_shaped_pattern->h;

    for (i = 0; i < surf_aux->w; i += mosaic_shaped_pattern->w)
        for (j = 0; j < surf_aux->h; j += mosaic_shaped_pattern->h)
        {
            rect.x = i;
            rect.y = j;
            SDL_BlitSurface(mosaic_shaped_pattern, NULL, surf_aux, &rect);
        }

    if (which == TOOL_IRREGULAR)
    {
        deform(api, surf_aux);
    }

    SDL_SetSurfaceBlendMode (surf_aux, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(surf_aux, NULL, canvas_shaped, NULL);
    SDL_FreeSurface(surf_aux);
    black = SDL_MapRGBA(canvas->format, 0, 0, 0, 0);
    white = SDL_MapRGBA(canvas->format, 255, 255, 255, 0);

    /* Two black lines at the edges */
    for (i = 0; i < canvas->w; i++)
    {
        api->putpixel(canvas_shaped, i, 0, black);
        api->putpixel(canvas_shaped, i, 1, black);
        api->putpixel(canvas_shaped, i, canvas->h - 1, black);
        api->putpixel(canvas_shaped, i, canvas->h - 2, black);
    }

    for (j = 0; j < canvas->h; j++)
    {
        api->putpixel(canvas_shaped, 0, j, black);
        api->putpixel(canvas_shaped, 1, j, black);
        api->putpixel(canvas_shaped, canvas->w - 1, j, black);
        api->putpixel(canvas_shaped, canvas->w - 2, j, black);
    }

    /* A copy of canvas at switchin, will be used to draw from it as snapshot changes at each click */
    canvas_back = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                       canvas->w,
                                       canvas->h,
                                       canvas->format->BitsPerPixel,
                                       canvas->format->Rmask,
                                       canvas->format->Gmask,
                                       canvas->format->Bmask, amask);
    SDL_BlitSurface(canvas, NULL, canvas_back, NULL);

    if (which != TOOL_SQUARE) /* The pattern for square is small enouth to not need an additional shape */
        for (y = 0; y < canvas->h; y++)
        {
            for (x = 0; x < canvas->w; x++)
            {
                mosaic_shaped_sharpen_pixel(api, canvas_shaped, canvas, x, y);
            }
        }

    reset_counter(canvas, mosaic_shaped_counted);
    reset_counter(canvas, mosaic_shaped_done);
}

void mosaic_shaped_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
    SDL_FreeSurface(canvas_shaped);
    SDL_FreeSurface(canvas_back);
    SDL_FreeSurface(mosaic_shaped_pattern);
    free(mosaic_shaped_counted);
}

int mosaic_shaped_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
    return (MODE_PAINT | MODE_FULLSCREEN);
}

void reset_counter(SDL_Surface * canvas, Uint8 * counter)
{
    int i, j;

    for (j = 0; j < canvas->h; j++)
        for (i = 0; i < canvas->w; i++)
        {
            counter[j * canvas->w + i] = 0;
        }
}


int scan_fill_count;

int scan_fill(magic_api * api, SDL_Surface * canvas, SDL_Surface * srfc, int x, int y, int fill_edge, int fill_tile, int size, Uint32 color)
{
    int leftx, rightx;
    Uint8 r, g, b, a;
    int i;

    leftx = x - 1;
    rightx = x + 1;

    /* Abort, if we recurse too deep! -bjk 2014.08.05 */
    scan_fill_count++;
    if (scan_fill_count > 50000 && 0)
    {
        scan_fill_count--;
        return (0);
    }

    if (mosaic_shaped_counted[y * canvas->w + x] == 1)
    {
        scan_fill_count--;
        return (0);
    }

    if (api->getpixel(srfc, x, y) == black)
    {
        if (fill_edge == 1)
        {
            fill_square(api, canvas, x, y, size, color);
        }

        scan_fill_count--;
        return (0); /* No need to check more */
    }

    if (fill_tile == 1)
    {
        Uint32 shadow;
        Uint8 shr, shg, shb, sha;
        Uint8 cnvsr, cnvsg, cnvsb, cnvsa;
        shadow = api->getpixel(srfc, x, y);
        SDL_GetRGBA(shadow, srfc->format, &shr, &shg, &shb, &sha);
        SDL_GetRGBA(pixel_average, srfc->format, &cnvsr, &cnvsg, &cnvsb, &cnvsa);
        shadow = SDL_MapRGBA(canvas->format, (shr * cnvsr) / 255, (shg * cnvsg) / 255, (shb * cnvsb) / 255, 0); //(shr + cnvsr) /2, ;
        api->putpixel(canvas, x, y, shadow);
        mosaic_shaped_counted[y * canvas->w + x] = 1;
        mosaic_shaped_done[y * canvas->w + x] = 1;
    }
    else
    {
        SDL_GetRGBA(api->getpixel(canvas_back, x, y), canvas_back->format, &r, &g, &b, &a);
        mosaic_shaped_average_r += r;
        mosaic_shaped_average_g += g;
        mosaic_shaped_average_b += b;
        mosaic_shaped_average_count += 1;
        mosaic_shaped_counted[y * canvas->w + x] = 1;
    }

    /* Search right */
    while (scan_fill(api, canvas, srfc, rightx, y, fill_edge, fill_tile, size, color) && (rightx < canvas->w))
    {
        rightx ++;
    }

    /* Search left */
    while (scan_fill(api, canvas, srfc, leftx, y, fill_edge, fill_tile, size, color) && (leftx >= 0))
    {
        leftx --;
    }

    /* Top / bottom */
    for (i = leftx; i <= rightx; i++)
    {
        if (y > 0)
        {
            scan_fill(api, canvas, srfc, i, y - 1, fill_edge, fill_tile, size, color);
        }

        if (y + 1 < canvas->w)
        {
            scan_fill(api, canvas, srfc, i, y + 1, fill_edge, fill_tile, size, color);
        }
    }

    scan_fill_count--;
    return (1);
}


void fill_square(magic_api * api, SDL_Surface * canvas, int x, int y, int size, Uint32 color)
{
    int i, j;

    for (i = (x - size); i < (x + 1 + size); i++)
        for (j = (y - size); j < (y + 1 + size); j++)
        {
            api->putpixel(canvas, i, j, color);
        }
}

void deform(magic_api * api, SDL_Surface * srfc)
{
    int i, j;

    for (j = 0; j < srfc->h; j++)
        for (i = 0; i < srfc->w; i++)
        {
            api->putpixel(srfc, i, j, api->getpixel(srfc, i + sin(j * M_PI / 90) * 10 + 10, j));
        }

    for (i = 0; i < srfc->w; i++)
        for (j = 0; j < srfc->h; j++)
        {
            api->putpixel(srfc, i, j, api->getpixel(srfc, i, j + sin(i * M_PI / 90) * 10 + 10));
        }
}

/* Paints a 2 pixel square with black and  shadows around 3 more pixels */
static void mosaic_shaped_paint(void * ptr, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas,
                               SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  int radius, shadow;
  int i, j, ii, jj;
  Uint8 r, g, b, a;
  Uint32 shadow_tone;

  black = SDL_MapRGBA(canvas->format, 0, 0, 0, SDL_ALPHA_OPAQUE);

  radius = 1;
  shadow = 3;

  for (i = -(radius + shadow); i < (radius + shadow); i++)
    for (j = -(radius + shadow); j < (radius + shadow); j++)
    {
      /* Ensure effects on the edges reaches the opposite side if necessary */
      ii = x + i;
      if (ii < 0) ii += canvas->w;
      if (ii >= canvas->w) ii -= canvas->w;
      jj = y + j;
      if (jj < 0) jj += canvas->h;
      if (jj >= canvas->h) ii -= canvas->h;

      /* Shadow_tone is also used as a marker, anything already painted on black must finally be black */
      shadow_tone = api->getpixel(canvas, ii, jj);

      //      if (abs(i) <= radius && abs(j) <= radius)
	if (0 <=i && i <= 1 && 0 <= j && j <= 1)
	api->putpixel(canvas, ii, jj, black);

      else if (api->in_circle(i, j, radius + shadow) && shadow_tone != black)
      {
	SDL_GetRGBA(shadow_tone, canvas->format, &r, &g, &b, &a);

	/* Shadows should be shadows, not black */
	if (r > 10) r -= 9;
	if (g > 10) g -= 9;
	if (b > 10) b -= 9;

      	api->putpixel(canvas, ii, jj, SDL_MapRGBA(canvas->format, r, g, b, SDL_ALPHA_OPAQUE));

      }

    }
}
