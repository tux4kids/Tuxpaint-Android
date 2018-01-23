/*
  mosaic.c

  mosaic, Add a mosaic effect to the image using a combination of other tools.
  Requires the mosaicAll sharpen and noise tools.
  Tux Paint - A simple drawing program for children.

  Credits: Andrew Corcoran <akanewbie@gmail.com>

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
  $Id$
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

static void mosaic_noise_pixel(void *ptr, SDL_Surface * canvas, int noise_AMOUNT, int x, int y);
static void mosaic_blur_pixel(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void mosaic_sharpen_pixel(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
static void reset_mosaic_blured(SDL_Surface * canvas);

/* Prototypes */
Uint32 mosaic_api_version(void);
int mosaic_init(magic_api *);
int mosaic_get_tool_count(magic_api *);
SDL_Surface *mosaic_get_icon(magic_api *, int);
char *mosaic_get_name(magic_api *, int);
char *mosaic_get_description(magic_api *, int, int);
void mosaic_paint(void *, int, SDL_Surface *, SDL_Surface *, int, int);
void mosaic_drag(magic_api *, int, SDL_Surface *, SDL_Surface *, int, int, int, int, SDL_Rect *);
void mosaic_click(magic_api *, int, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);
void mosaic_release(magic_api *, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);
void mosaic_shutdown(magic_api *);
void mosaic_set_color(magic_api *, Uint8, Uint8, Uint8);
int mosaic_requires_colors(magic_api *, int);
void mosaic_switchin(magic_api *, int, int, SDL_Surface *);
void mosaic_switchout(magic_api *, int, int, SDL_Surface *);
int mosaic_modes(magic_api *, int);

static const int mosaic_AMOUNT = 300;
static const int mosaic_RADIUS = 16;
static const double mosaic_SHARPEN = 1.0;
static int randnoise ATTRIBUTE_UNUSED;
Uint8 *mosaic_blured;
enum
{
  TOOL_MOSAIC,
  mosaic_NUM_TOOLS
};

static Mix_Chunk *mosaic_snd_effect[mosaic_NUM_TOOLS];
static SDL_Surface *canvas_noise;
static SDL_Surface *canvas_blur;
static SDL_Surface *canvas_sharp;

const char *mosaic_snd_filenames[mosaic_NUM_TOOLS] = {
  "mosaic.ogg",                 /* FIXME */
};

const char *mosaic_icon_filenames[mosaic_NUM_TOOLS] = {
  "mosaic.png",
};

const char *mosaic_names[mosaic_NUM_TOOLS] = {
  gettext_noop("Mosaic"),
};

const char *mosaic_descs[mosaic_NUM_TOOLS][2] = {
  {gettext_noop("Click and drag the mouse to add a mosaic effect to parts of your picture."),
   gettext_noop("Click to add a mosaic effect to your entire picture."),},
};

Uint32 mosaic_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

//Load sounds
int mosaic_init(magic_api * api)
{

  int i;
  char fname[1024];

  for (i = 0; i < mosaic_NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%s/sounds/magic/%s", api->data_directory, mosaic_snd_filenames[i]);
      mosaic_snd_effect[i] = Mix_LoadWAV(fname);
    }

  return (1);
}

int mosaic_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (mosaic_NUM_TOOLS);
}

// Load our icons:
SDL_Surface *mosaic_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, mosaic_icon_filenames[which]);
  return (IMG_Load(fname));
}

// Return our names, localized:
char *mosaic_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return (strdup(gettext_noop(mosaic_names[which])));
}

// Return our descriptions, localized:
char *mosaic_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return (strdup(gettext_noop(mosaic_descs[which][mode - 1])));
}

//Calculates the grey scale value for a rgb pixel
static int mosaic_grey(Uint8 r1, Uint8 g1, Uint8 b1)
{
  return 0.3 * r1 + .59 * g1 + 0.11 * b1;
}

// Do the effect for the full image
static void do_mosaic_full(void *ptr, SDL_Surface * canvas,
                           SDL_Surface * last ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{

  magic_api *api = (magic_api *) ptr;

  int x, y;

  Uint32 amask = ~(canvas->format->Rmask | canvas->format->Gmask | canvas->format->Bmask);
  SDL_Surface *mosaic_temp = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                                  canvas->w,
                                                  canvas->h,
                                                  canvas->format->BitsPerPixel,
                                                  canvas->format->Rmask,
                                                  canvas->format->Gmask,
                                                  canvas->format->Bmask, amask);



  api->update_progress_bar();

  for (y = 0; y < canvas->h; y++)
    {

      for (x = 0; x < canvas->w; x++)
        {
          mosaic_blur_pixel(api, mosaic_temp, canvas_noise, x, y);
        }
    }

  api->update_progress_bar();

  for (y = 0; y < canvas->h; y++)
    {
      for (x = 0; x < canvas->w; x++)
        {
          mosaic_sharpen_pixel(api, canvas, mosaic_temp, x, y);
        }
    }
  SDL_FreeSurface(mosaic_temp);
}

/* Paint the brush, noise is yet done at switchin, 
   blurs 2 pixels around the brush in order to get sharpen well done.*/
void mosaic_paint(void *ptr_to_api, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * last ATTRIBUTE_UNUSED, int x, int y)
{
  int i, j, pix_row_pos;

  magic_api *api = (magic_api *) ptr_to_api;

  for (j = max(0, y - mosaic_RADIUS - 2); j < min(canvas->h, y + mosaic_RADIUS + 2); j++)
    {
      pix_row_pos = j * canvas->w;
      for (i = max(0, x - mosaic_RADIUS - 2); i < min(canvas->w, x + mosaic_RADIUS + 2); i++)
        if (!mosaic_blured[pix_row_pos + i] && api->in_circle(i - x, j - y, mosaic_RADIUS + 2))
          {
            mosaic_blur_pixel(api, canvas_blur, canvas_noise, i, j);
            mosaic_blured[pix_row_pos + i] = 1; /* Track what are yet blured */
          }
    }

  for (i = x - mosaic_RADIUS; i < x + mosaic_RADIUS; i++)
    for (j = y - mosaic_RADIUS; j < y + mosaic_RADIUS; j++)
      if (api->in_circle(i - x, j - y, mosaic_RADIUS))
        if (!api->touched(i, j))
          {
            mosaic_sharpen_pixel(api, canvas_sharp, canvas_blur, i, j);
            api->putpixel(canvas, i, j, api->getpixel(canvas_sharp, i, j));
          }
}

// Affect the canvas on drag:
void mosaic_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line(api, which, canvas, last, ox, oy, x, y, 1, mosaic_paint);

  update_rect->x = min(ox, x) - mosaic_RADIUS;
  update_rect->y = min(oy, y) - mosaic_RADIUS;
  update_rect->w = max(ox, x) + mosaic_RADIUS - update_rect->x;
  update_rect->h = max(oy, y) + mosaic_RADIUS - update_rect->y;
  api->playsound(mosaic_snd_effect[which], (x * 255) / canvas->w, 255);

}

// Affect the canvas on click:
void mosaic_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect)
{

  if (mode == MODE_FULLSCREEN)
    {
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;

      do_mosaic_full(api, canvas_noise, last, which);
      SDL_BlitSurface(canvas_noise, NULL, canvas, NULL);
      api->playsound(mosaic_snd_effect[which], 128, 255);
    }
  else
    mosaic_drag(api, which, canvas, last, x, y, x, y, update_rect);

}

// Affect the canvas on release:
void mosaic_release(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED,
                    SDL_Surface * last ATTRIBUTE_UNUSED,
                    int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

// No setup happened:
void mosaic_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  //Clean up sounds
  int i;

  for (i = 0; i < mosaic_NUM_TOOLS; i++)
    {
      if (mosaic_snd_effect[i] != NULL)
        {
          Mix_FreeChunk(mosaic_snd_effect[i]);
        }
    }
}

// Record the color from Tux Paint:
void mosaic_set_color(magic_api * api ATTRIBUTE_UNUSED,
                      Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

// Use colors:
int mosaic_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

//Add noise to a pixel
static void mosaic_noise_pixel(void *ptr, SDL_Surface * canvas, int noise_AMOUNT, int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  Uint8 temp[3];
  double temp2[3];
  int k;

  SDL_GetRGB(api->getpixel(canvas, x, y), canvas->format, &temp[0], &temp[1], &temp[2]);
  for (k = 0; k < 3; k++)
    {
      temp2[k] = clamp(0.0, (int)temp[k] - (rand() % noise_AMOUNT) + noise_AMOUNT / 2.0, 255.0);
    }
  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, temp2[0], temp2[1], temp2[2]));
}

//Blur a pixel
static void mosaic_blur_pixel(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int i, j, k;
  Uint8 temp[3];
  double blurValue[3];

  //5x5 gaussiann weighting window
  const int weight[5][5] = { {1, 4, 7, 4, 1},
  {4, 16, 26, 16, 4},
  {7, 26, 41, 26, 7},
  {4, 16, 26, 16, 4},
  {1, 4, 7, 4, 1}
  };

  for (k = 0; k < 3; k++)
    {
      blurValue[k] = 0;
    }

  for (i = -2; i < 3; i++)
    {
      for (j = -2; j < 3; j++)
        {
          //Add the pixels around the current one wieghted 
          SDL_GetRGB(api->getpixel(last, x + i, y + j), last->format, &temp[0], &temp[1], &temp[2]);
          for (k = 0; k < 3; k++)
            {
              blurValue[k] += temp[k] * weight[i + 2][j + 2];
            }
        }
    }
  for (k = 0; k < 3; k++)
    {
      blurValue[k] /= 273;
    }
  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, blurValue[0], blurValue[1], blurValue[2]));
}

//Sharpen a pixel
static void mosaic_sharpen_pixel(void *ptr, SDL_Surface * canvas, SDL_Surface * last, int x, int y)
{

  magic_api *api = (magic_api *) ptr;

  Uint8 r1, g1, b1;
  int grey;
  int i, j;
  double sobel_1 = 0, sobel_2 = 0;
  double temp;

  //Sobel weighting masks
  const int sobel_weights_1[3][3] = { {1, 2, 1},
  {0, 0, 0},
  {-1, -2, -1}
  };
  const int sobel_weights_2[3][3] = { {-1, 0, 1},
  {-2, 0, 2},
  {-1, 0, 1}
  };

  sobel_1 = 0;
  sobel_2 = 0;
  for (i = -1; i < 2; i++)
    {
      for (j = -1; j < 2; j++)
        {
          //No need to check if inside canvas, getpixel does it for us.
          SDL_GetRGB(api->getpixel(last, x + i, y + j), last->format, &r1, &g1, &b1);
          grey = mosaic_grey(r1, g1, b1);
          sobel_1 += grey * sobel_weights_1[i + 1][j + 1];
          sobel_2 += grey * sobel_weights_2[i + 1][j + 1];
        }
    }

  temp = sqrt(sobel_1 * sobel_1 + sobel_2 * sobel_2);
  temp = (temp / 1443) * 255.0;

  SDL_GetRGB(api->getpixel(last, x, y), last->format, &r1, &g1, &b1);
  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, clamp(0.0, r1 + mosaic_SHARPEN * temp, 255.0),
                                         clamp(0.0, g1 + mosaic_SHARPEN * temp, 255.0),
                                         clamp(0.0, b1 + mosaic_SHARPEN * temp, 255.0)));

}

void mosaic_switchin(magic_api * api, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  int y, x;
  Uint32 amask;

  mosaic_blured = (Uint8 *) malloc(sizeof(Uint8) * (canvas->w * canvas->h));
  if (mosaic_blured == NULL)
    {
      fprintf(stderr, "\nError: Can't build drawing touch mask!\n");
      exit(1);
    }

  amask = ~(canvas->format->Rmask | canvas->format->Gmask | canvas->format->Bmask);

  canvas_noise = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                      canvas->w,
                                      canvas->h,
                                      canvas->format->BitsPerPixel,
                                      canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, amask);

  SDL_BlitSurface(canvas, NULL, canvas_noise, NULL);

  for (y = 0; y < canvas->h; y++)
    {
      for (x = 0; x < canvas->w; x++)
        {
          mosaic_noise_pixel(api, canvas_noise, mosaic_AMOUNT, x, y);
        }
    }

  canvas_blur = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                     canvas->w,
                                     canvas->h,
                                     canvas->format->BitsPerPixel,
                                     canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, amask);

  canvas_sharp = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                      canvas->w,
                                      canvas->h,
                                      canvas->format->BitsPerPixel,
                                      canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, amask);
  reset_mosaic_blured(canvas);
}

void mosaic_switchout(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  SDL_FreeSurface(canvas_noise);
  SDL_FreeSurface(canvas_blur);
  SDL_FreeSurface(canvas_sharp);
  free(mosaic_blured);
}

int mosaic_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}

void reset_mosaic_blured(SDL_Surface * canvas)
{
  int i, j;

  for (j = 0; j < canvas->h; j++)
    for (i = 0; i < canvas->w; i++)
      mosaic_blured[j * canvas->w + i] = 0;

}
