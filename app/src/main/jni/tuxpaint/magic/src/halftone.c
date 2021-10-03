/* halftone.c

   Last modified: 2021.09.21
*/


/* Inclusion of header files: */
/* -------------------------- */

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#define deg_cos(x) cos((x) * M_PI / 180.0)
#define deg_sin(x) sin((x) * M_PI / 180.0)

#define GRID_SIZE 16 /* Size of the grid, and hence max size of the circle (it may fill more, into a square shape) */
#define OFFSET_RADIUS 2 /* Radius for when offsetting C, M, Y, and K colors by their angles (see `chan_angles[]`) */

enum
{
  TOOL_HALFTONE,
  NUM_TOOLS
};


const char *snd_filenames[NUM_TOOLS] = {
  "halftone.ogg",
};

const char *icon_filenames[NUM_TOOLS] = {
  "halftone.png",
};

const char *names[NUM_TOOLS] = {
  gettext_noop("Halftone"),
};

const int groups[NUM_TOOLS] = {
  MAGIC_TYPE_DISTORTS,
};

const char *descs[NUM_TOOLS] = {
  gettext_noop("Click and drag to turn your drawing into a newspaper."),
};

Mix_Chunk *snd_effect[NUM_TOOLS];

static SDL_Surface *canvas_backup, *square;

/* Function Prototypes: */

void halftone_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void halftone_line_callback(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
Uint32 halftone_api_version(void);
int halftone_init(magic_api * api);
int halftone_get_tool_count(magic_api * api);
SDL_Surface *halftone_get_icon(magic_api * api, int which);
char *halftone_get_name(magic_api * api, int which);
int halftone_get_group(magic_api * api, int which);
char *halftone_get_description(magic_api * api, int which, int mode);
int halftone_requires_colors(magic_api * api, int which);
int halftone_modes(magic_api * api, int which);
void halftone_shutdown(magic_api * api);
void halftone_click(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void halftone_release(magic_api * api, int which,
                      SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void halftone_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void halftone_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void halftone_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void halftone_rgb2cmyk(Uint8 r, Uint8 g, Uint8 b, float cmyk[]);

Uint32 halftone_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int halftone_init(magic_api * api)
{
  int i;
  char fname[1024];

  canvas_backup = NULL;
  square = NULL;

  for (i = 0; i < NUM_TOOLS; i++)
    {
      snprintf(fname, sizeof(fname), "%ssounds/magic/%s", api->data_directory, snd_filenames[i]);

      snd_effect[i] = Mix_LoadWAV(fname);
    }


  return (1);
}

int halftone_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}

SDL_Surface *halftone_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, icon_filenames[which]);

  return (IMG_Load(fname));
}

char *halftone_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  const char *our_name_english;
  const char *our_name_localized;

  our_name_english = names[which];
  our_name_localized = gettext_noop(our_name_english);

  return (strdup(our_name_localized));
}

int halftone_get_group(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return groups[which];
}

char *halftone_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  const char *our_desc_english;
  const char *our_desc_localized;

  our_desc_english = descs[which];
  our_desc_localized = gettext_noop(our_desc_english);

  return (strdup(our_desc_localized));
}

int halftone_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

int halftone_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}

void halftone_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    if (snd_effect[i] != NULL) {
      Mix_FreeChunk(snd_effect[i]);
    }
  }

  SDL_FreeSurface(canvas_backup);
  SDL_FreeSurface(square);
}

void halftone_click(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect)
{
  int full_x, full_y;

  if (mode == MODE_PAINT)
    {
      halftone_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
    }
  else
    {
      for (full_y = 0; full_y < canvas->h; full_y += GRID_SIZE)
        {
          for (full_x = 0; full_x < canvas->w; full_x += GRID_SIZE)
            {
              halftone_line_callback(api, which, canvas, snapshot, full_x, full_y);
            }
        }
      api->playsound(snd_effect[which], 128, 255);
      update_rect->x = 0;
      update_rect->y = 0;
      update_rect->w = canvas->w;
      update_rect->h = canvas->h;
    }
}

void halftone_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  api->line((void *)api, which, canvas, snapshot, ox, oy, x, y, 4, halftone_line_callback);

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

  ox = (ox / GRID_SIZE) * GRID_SIZE + (GRID_SIZE / 2);
  oy = (oy / GRID_SIZE) * GRID_SIZE + (GRID_SIZE / 2);
  x = (x / GRID_SIZE) * GRID_SIZE + (GRID_SIZE / 2);
  y = (y / GRID_SIZE) * GRID_SIZE + (GRID_SIZE / 2);

  update_rect->x = ox - GRID_SIZE / 2;
  update_rect->y = oy - GRID_SIZE / 2;
  update_rect->w = (x + GRID_SIZE / 2) - update_rect->x;
  update_rect->h = (y + GRID_SIZE / 2) - update_rect->y;

  api->playsound(snd_effect[which], (x * 255) / canvas->w,      // pan
                 255);          // distance
}

enum
{
  CHAN_CYAN,
  CHAN_MAGENTA,
  CHAN_YELLOW,
  CHAN_BLACK,
  NUM_CHANS
};

Uint8 chan_colors[NUM_CHANS][3] = {
  {0, 255, 255}, /* Cyan */
  {255, 0, 255}, /* Magenta */
  {255, 255, 0}, /* Yellow */
  {0, 0, 0}      /* Black */
};

int chan_angles[NUM_CHANS] = {
  75, /* Cyan */
  15, /* Magenta */
  90, /* Yellow */
  45  /* Black */
};

void halftone_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                      int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}

void halftone_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED,
                        Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}


void halftone_line_callback(void *ptr, int which ATTRIBUTE_UNUSED,
                            SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  Uint8 r, g, b, or, og, ob;
  Uint32 total_r, total_g, total_b;
  int px_cnt;
  Uint32 pixel;
  int xxx, yyy, channel, ox, oy, sqx, sqy;
  SDL_Rect dest;
  magic_api *api = (magic_api *) ptr;
  float cmyk[4];

  /* Start the pixel with white */
  pixel = SDL_MapRGB(square->format, 255, 255, 255);
  SDL_FillRect(square, NULL, pixel);

  /* Lock to a grid, centered around mouse */
  x = (x / GRID_SIZE) * GRID_SIZE + (GRID_SIZE / 2);
  y = (y / GRID_SIZE) * GRID_SIZE + (GRID_SIZE / 2);

  if (api->touched(x, y))
    {
      return;
    }

  /* Get the average color around the mouse */
  total_r = total_g = total_b = 0;
  px_cnt = 0;
  for (xxx = -(GRID_SIZE / 2); xxx < (GRID_SIZE / 2); xxx++)
    {
      for (yyy = -(GRID_SIZE / 2); yyy < (GRID_SIZE / 2); yyy++)
        {
          SDL_GetRGB(api->getpixel(canvas_backup, x + xxx, y + yyy), canvas_backup->format, &r, &g, &b);
          total_r += r;
          total_g += g;
          total_b += b;
          px_cnt++;
        }
    }

  total_r /= (GRID_SIZE * GRID_SIZE);
  total_g /= (GRID_SIZE * GRID_SIZE);
  total_b /= (GRID_SIZE * GRID_SIZE);


  /* Convert the average color from RGB to CMYK values, for 'painting' later */
  halftone_rgb2cmyk(total_r, total_g, total_b, cmyk);

  /* Draw C, M, Y and K blobs into our 'square' surface */
  for (channel = 0; channel < NUM_CHANS; channel++)
    {
      for (xxx = -(GRID_SIZE / 2) - 1; xxx < (GRID_SIZE / 2) + 1; xxx++)
        {
          for (yyy = -(GRID_SIZE / 2) - 1; yyy < (GRID_SIZE / 2) + 1; yyy++)
            {
              /* A circle blob, radius based upon channel (C, M, Y or K) strength for this color */

              ox = xxx + deg_cos(chan_angles[channel]) * OFFSET_RADIUS;
              oy = yyy + deg_sin(chan_angles[channel]) * OFFSET_RADIUS;

              sqx = ((GRID_SIZE / 2) + ox) % GRID_SIZE;
              sqy = ((GRID_SIZE / 2) + oy) % GRID_SIZE;

              /* Use intensity of the CMKY channel in question to decide
                 how big of a circle to paint */
              if (api->in_circle(xxx, yyy, cmyk[channel] * GRID_SIZE))
                {
                  /* Use the pure C, Y, M, or K color to paint with */
                  r = chan_colors[channel][0];
                  g = chan_colors[channel][1];
                  b = chan_colors[channel][2];

                  /* Additively blend with whatever we have in the
                     'square' buffer (which starts as white)
                     (since the target is RGB, we use `min()`) */
                  SDL_GetRGB(api->getpixel(square, sqx, sqy), square->format, &or, &og, &ob);
                  pixel = SDL_MapRGB(square->format, min(r * 1.2, or), min(g * 1.2, og), min(b * 1.2, ob));
                  api->putpixel(square, sqx, sqy, pixel);
                }
            }
        }
    }

  /* Copy the results to the canvas */
  dest.x = x - GRID_SIZE / 2;
  dest.y = y - GRID_SIZE / 2;
  dest.w = GRID_SIZE;
  dest.h = GRID_SIZE;

  SDL_BlitSurface(square, NULL, canvas, &dest);
}

void halftone_switchin(magic_api * api, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  if (canvas_backup == NULL)
    {
      canvas_backup =
        SDL_CreateRGBSurface(SDL_SWSURFACE, api->canvas_w, api->canvas_h, canvas->format->BitsPerPixel,
                             canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);
    }

  if (square == NULL)
    {
      square =
        SDL_CreateRGBSurface(SDL_SWSURFACE, GRID_SIZE, GRID_SIZE, canvas->format->BitsPerPixel, canvas->format->Rmask,
                             canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);
    }

  SDL_BlitSurface(canvas, NULL, canvas_backup, NULL);
}

void halftone_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                        int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void halftone_rgb2cmyk(Uint8 r, Uint8 g, Uint8 b, float cmyk[])
{
  float mincmy, c, m, y, k;

  /* Simple RGB to CMYK math (not worrying about color profiles, etc.),
     based on math found at http://www.javascripter.net/faq/rgb2cmyk.htm
     by Alexei Kourbatov <alexei@kourbatov.com> */

  if (r == 0 && g == 0 && b == 0)
    {
      /* Black */
      c = 0.0;
      m = 0.0;
      y = 0.0;
      k = 1.0;
    }
  else
    {
      c = 1.0 - (((float)r) / 255.0);
      m = 1.0 - (((float)g) / 255.0);
      y = 1.0 - (((float)b) / 255.0);

      mincmy = min(c, min(m, y));
      c = (c - mincmy) / (1.0 - mincmy);
      m = (m - mincmy) / (1.0 - mincmy);
      y = (y - mincmy) / (1.0 - mincmy);
      k = mincmy;
    }

  cmyk[0] = c;
  cmyk[1] = m;
  cmyk[2] = y;
  cmyk[3] = k;
}
