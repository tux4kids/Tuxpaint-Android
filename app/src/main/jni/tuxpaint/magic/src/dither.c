/* dither.c

   A Magic tool for Tux Paint that turns (parts of) an image into
   a black-and-white dithered representation.
   (Using Atkinso dithering: https://en.wikipedia.org/wiki/Atkinson_dithering)
   by Bill Kendrick <bill@newbreedsoftware.com>

   Last updated: May 10, 2024
*/


/* Inclusion of header files */
/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

enum
{
  /* Use the chosen color for the non-white pixels */
  TOOL_DITHER_VIA_COLOR,
  /* Use the image's color (hue/saturation w/ low value) for the non-white pixels */
  TOOL_DITHER_KEEP_COLOR,
  NUM_TOOLS
};

char *dither_names[NUM_TOOLS] = {
  gettext_noop("Dither"),
  gettext_noop("Dither (Keep Color)"),
};

char *dither_descr[NUM_TOOLS][2] = {
  {
   gettext_noop("Click and drag to replace parts of your image with a dithered pattern of dots in your chosen color."),
   gettext_noop("Click to replace your entire image with a dithered pattern of dots in your chosen color."),
   },
  {
   gettext_noop
   ("Click and drag to replace parts of your image with a dithered pattern of dots using the picture's original colors."),
   gettext_noop
   ("Click to replace your entire image with a dithered pattern of dots using the picture's original colors."),
   },
};

char *dither_icon_filenames[NUM_TOOLS] = {
  "dither.png",
  "dither_keep_color.png",
};

char *dither_snd_filenames[NUM_TOOLS] = {
  "dither.ogg",
  "dither_keep_color.ogg",
};

Mix_Chunk *snd_effects[NUM_TOOLS];

#define DITHER_SIZE_COUNT 4
#define DITHER_SIZE_DEFAULT 2
#define DITHER_SIZE_SCALE 8

Uint8 dither_sizes[NUM_TOOLS];

Uint32 dither_white, dither_black, dither_color;
Uint8 *dither_touched;
float *dither_vals;
int dither_click_mode;


void dither_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * snapshot, int old_x, int old_y, int x, int y, SDL_Rect * update_rect);

void dither_line_callback(void *pointer, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);

Uint32 dither_api_version(void);
int dither_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int dither_get_tool_count(magic_api * api);
SDL_Surface *dither_get_icon(magic_api * api, int which);
char *dither_get_name(magic_api * api, int which);
int dither_get_group(magic_api * api, int which);
int dither_get_order(int which);
char *dither_get_description(magic_api * api, int which, int mode);
int dither_requires_colors(magic_api * api, int which);
void dither_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void dither_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void dither_set_size(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * snapshot, Uint8 size, SDL_Rect * update_rect);
void dither_set_color(magic_api * api, int which,
                      SDL_Surface * canvas, SDL_Surface * snapshot, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void dither_release(magic_api * api, int which,
                    SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void dither_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void dither_shutdown(magic_api * api);
Uint8 dither_default_size(magic_api * api, int which, int mode);
Uint8 dither_accepted_sizes(magic_api * api, int which, int mode);
int dither_modes(magic_api * api, int which);


Uint32 dither_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int dither_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  int i;
  char filename[1024];

  for (i = 0; i < NUM_TOOLS; i++)
  {
    snprintf(filename, sizeof(filename), "%ssounds/magic/%s", api->data_directory, dither_snd_filenames[i]);
    snd_effects[i] = Mix_LoadWAV(filename);
  }

  for (i = 0; i < NUM_TOOLS; i++)
  {
    dither_sizes[i] = DITHER_SIZE_DEFAULT * DITHER_SIZE_SCALE;
  }

  return (1);
}


int dither_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return NUM_TOOLS;
}

SDL_Surface *dither_get_icon(magic_api *api, int which)
{
  char filename[1024];

  snprintf(filename, sizeof(filename), "%simages/magic/%s", api->data_directory, dither_icon_filenames[which]);

  return (IMG_Load(filename));
}


char *dither_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  return strdup(gettext(dither_names[which]));
}


int dither_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}


int dither_get_order(int which)
{
  return 610 + which;
}


char *dither_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode)
{
  if (mode == MODE_PAINT)
  {
    return strdup(gettext(dither_descr[which][0]));
  }
  else                          /* if (mode == MODE_FULLSCREEN) */
  {
    return strdup(gettext(dither_descr[which][1]));
  }
}


int dither_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  if (which == TOOL_DITHER_VIA_COLOR)
    return 1;
  else
    return 0;
}


int dither_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT | MODE_FULLSCREEN;
}


Uint8 dither_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return DITHER_SIZE_COUNT;
}


Uint8 dither_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return DITHER_SIZE_DEFAULT;
}


void dither_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
  {
    if (snd_effects[i] != NULL)
    {
      Mix_FreeChunk(snd_effects[i]);
    }
  }

  if (dither_touched == NULL)
  {
    free(dither_touched);
  }

  if (dither_vals == NULL)
  {
    free(dither_vals);
  }
}

void
dither_click(magic_api *api, int which, int mode,
             SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  int xx, yy;
  Uint8 r, g, b;

  dither_click_mode = mode;

  for (yy = 0; yy < canvas->h; yy++)
  {
    for (xx = 0; xx < canvas->w; xx++)
    {
      if (mode == MODE_PAINT)
      {
        dither_touched[yy * canvas->w + xx] = 0;
      }
      else
      {
        dither_touched[yy * canvas->w + xx] = 1;

        SDL_GetRGB(api->getpixel(snapshot, xx, yy), snapshot->format, &r, &g, &b);
        dither_vals[yy * canvas->w + xx] =
          (api->sRGB_to_linear(r) + api->sRGB_to_linear(g) + api->sRGB_to_linear(b)) / 3.0;

        if (xx == 0)
        {
          api->update_progress_bar();
        }
      }
    }
  }

  if (mode == MODE_PAINT)
  {
    dither_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
  }
  else
  {
    api->playsound(snd_effects[which], 128, 255);
    dither_release(api, which, canvas, snapshot, x, y, update_rect);
  }
}


void
dither_drag(magic_api *api, int which,
            SDL_Surface *canvas, SDL_Surface *snapshot, int old_x, int old_y, int x, int y, SDL_Rect *update_rect)
{
  int dither_size;

  SDL_LockSurface(snapshot);
  SDL_LockSurface(canvas);

  api->line((void *)api, which, canvas, snapshot, old_x, old_y, x, y, 1, dither_line_callback);

  SDL_UnlockSurface(canvas);
  SDL_UnlockSurface(snapshot);

  if (old_x > x)
  {
    int temp = old_x;

    old_x = x;
    x = temp;
  }
  if (old_y > y)
  {
    int temp = old_y;

    old_y = y;
    y = temp;
  }

  dither_size = dither_sizes[which];

  update_rect->x = old_x - dither_size;
  update_rect->y = old_y - dither_size;
  update_rect->w = (x + dither_size) - update_rect->x + 1;
  update_rect->h = (y + dither_size) - update_rect->y + 1;

  api->playsound(snd_effects[which], (x * 255) / canvas->w, 255);
}

/*
 * [ . P 0 1 ]
 * [ 2 3 4 . ]
 * [ . 5 . . ]
*/
int dither_x_pos[6] = { 1, 2, -1, 0, 1, 0 };
int dither_y_pos[6] = { 0, 0, 1, 1, 1, 2 };

void dither_release(magic_api *api, int which,
                    SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  Uint8 r, g, b;
  float val, err, h, s, v;
  int i, nx, ny;

  for (y = 0; y < canvas->h; y++)
  {
    for (x = 0; x < canvas->w; x++)
    {
      if (dither_touched[y * canvas->w + x])
      {
        val = dither_vals[y * canvas->w + x];
        if (val >= 0.5)
        {
          api->putpixel(canvas, x, y, dither_white);
          err = val - 1.0;
        }
        else
        {
          if (which == TOOL_DITHER_VIA_COLOR)
          {
            api->putpixel(canvas, x, y, dither_color);
          }
          else if (which == TOOL_DITHER_KEEP_COLOR)
          {
            SDL_GetRGB(api->getpixel(snapshot, x, y), snapshot->format, &r, &g, &b);
            if (r <= 32 && g <= 32 && b <= 32)
            {
              api->putpixel(canvas, x, y, dither_black);
            }
            else
            {
              api->rgbtohsv(r, g, b, &h, &s, &v);
              api->hsvtorgb(floor(h / 2.0) * 2.0, min(s + 0.5, 1.0), v * 0.66, &r, &g, &b);
              api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, r, g, b));
            }
          }
          err = val;
        }

        /* Diffuse */
        for (i = 0; i < 6; i++)
        {
          nx = x + dither_x_pos[i];
          ny = y + dither_y_pos[i];

          if (nx >= 0 && nx < canvas->w && ny >= 0 && ny < canvas->h)
          {
            if (dither_touched[ny * canvas->w + nx])
            {
              dither_vals[ny * canvas->w + nx] += (err / 8.0);
            }
          }
        }
      }
    }
  }
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  if (dither_click_mode == MODE_PAINT)
  {
    api->stopsound();
  }
}

void dither_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                      Uint8 r, Uint8 g, Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  if (r <= 240 || g <= 240 || b <= 240)
  {
    dither_color = SDL_MapRGB(canvas->format, r, g, b);
  }
  else
  {
    /* If the chosen color is very bright or white, fall back to black */
    dither_color = SDL_MapRGB(canvas->format, 0, 0, 0);
  }
}

void dither_set_size(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                     Uint8 size, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  dither_sizes[which] = size * DITHER_SIZE_SCALE;
}


void dither_line_callback(void *pointer, int which, SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y)
{
  magic_api *api = (magic_api *) pointer;
  int xx, yy, dither_size;
  Uint8 r, g, b;
  float val;

  dither_size = dither_sizes[which];

  if (dither_touched == NULL)
    return;

  /* Just do a simple threshold effect while interacting */
  for (yy = -dither_size; yy < dither_size; yy++)
  {
    if (y + yy >= 0 && y + yy < canvas->h)
    {
      for (xx = -dither_size; xx < dither_size; xx++)
      {
        if (x + xx >= 0 && x + xx < canvas->w)
        {
          if (!dither_touched[(y + yy) * canvas->w + (x + xx)])
          {
            dither_touched[(y + yy) * canvas->w + (x + xx)] = 1;

            SDL_GetRGB(api->getpixel(snapshot, x + xx, y + yy), snapshot->format, &r, &g, &b);
            val = (api->sRGB_to_linear(r) + api->sRGB_to_linear(g) + api->sRGB_to_linear(b)) / 3.0;
            dither_vals[(y + yy) * canvas->w + (x + xx)] = val;

            api->putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format, val * 255, val * 255, val * 255));
          }
        }
      }
    }
  }
}


void dither_switchin(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas)
{
  if (dither_touched == NULL)
  {
    dither_touched = (Uint8 *) malloc(sizeof(Uint8) * canvas->h * canvas->w);
  }
  if (dither_vals == NULL)
  {
    dither_vals = (float *)malloc(sizeof(float) * canvas->h * canvas->w);
  }

  dither_white = SDL_MapRGB(canvas->format, 255, 255, 255);
  dither_black = SDL_MapRGB(canvas->format, 0, 0, 0);
}

void dither_switchout(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}
