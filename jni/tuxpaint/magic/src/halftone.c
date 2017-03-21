/* halftone.c

   Last modified: 2011.07.17
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

enum {
  TOOL_HALFTONE,
  NUM_TOOLS
};


const char * snd_filenames[NUM_TOOLS] = {
  "halftone.wav",
};

const char * icon_filenames[NUM_TOOLS] = {
  "halftone.png",
};

const char * names[NUM_TOOLS] = {
  gettext_noop("Halftone"),
};

const char * descs[NUM_TOOLS] = {
  gettext_noop("Click and drag to turn your drawing into a newspaper."),
};

Mix_Chunk * snd_effect[NUM_TOOLS];

static SDL_Surface * canvas_backup, * square;

/* Function Prototypes: */

void halftone_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);
void halftone_line_callback(void * ptr, int which,
                           SDL_Surface * canvas, SDL_Surface * snapshot,
                           int x, int y);
Uint32 halftone_api_version(void);
int halftone_init(magic_api * api);
int halftone_get_tool_count(magic_api * api);
SDL_Surface * halftone_get_icon(magic_api * api, int which);
char * halftone_get_name(magic_api * api, int which);
char * halftone_get_description(magic_api * api, int which, int mode);
int halftone_requires_colors(magic_api * api, int which);
int halftone_modes(magic_api * api, int which);
void halftone_shutdown(magic_api * api);
void halftone_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);
void halftone_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect);
void halftone_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void halftone_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void halftone_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void halftone_rgb2cmyk(Uint8 r, Uint8 g, Uint8 b, float cmyk[]);

Uint32 halftone_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}

int halftone_init(magic_api * api)
{
  int i;
  char fname[1024];

  canvas_backup = NULL;
  square = NULL;

  for (i = 0; i < NUM_TOOLS; i++)
  {
    snprintf(fname, sizeof(fname),
             "%ssounds/magic/%s",
	     api->data_directory, snd_filenames[i]);

    snd_effect[i] = Mix_LoadWAV(fname);
    if (snd_effect[i] == NULL) {
      SDL_FreeSurface(canvas_backup);
      SDL_FreeSurface(square);
      return(0);
    }
  }


  return(1);
}

int halftone_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(NUM_TOOLS);
}

SDL_Surface * halftone_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s",
	     api->data_directory, icon_filenames[which]);

  return(IMG_Load(fname));
}

char * halftone_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  const char * our_name_english;
  const char * our_name_localized;

  our_name_english = names[which];
  our_name_localized = gettext_noop(our_name_english);

  return(strdup(our_name_localized));
}

char * halftone_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  const char * our_desc_english;
  const char * our_desc_localized;

  our_desc_english = descs[which];
  our_desc_localized = gettext_noop(our_desc_english);

  return(strdup(our_desc_localized));
}

int halftone_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

int halftone_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}

void halftone_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
    Mix_FreeChunk(snd_effect[i]);

  SDL_FreeSurface(canvas_backup);
  SDL_FreeSurface(square);
}

void halftone_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect)
{
  halftone_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}

void halftone_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  api->line((void *) api, which, canvas, snapshot,
            ox, oy, x, y, 4, halftone_line_callback);

  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }

  update_rect->x = ox - 16;
  update_rect->y = oy - 16;
  update_rect->w = (x + 16) - update_rect->x;
  update_rect->h = (y + 16) - update_rect->h;

  api->playsound(snd_effect[which],
                 (x * 255) / canvas->w, // pan
	         255); // distance
}

enum {
  CHAN_CYAN,
  CHAN_MAGENTA,
  CHAN_YELLOW,
  CHAN_BLACK,
  NUM_CHANS
};

Uint8 chan_colors[NUM_CHANS][3] = {
  {   0, 255, 255 },
  { 255,   0, 255 },
  { 255, 255,   0 },
  {   0,   0,   0 }
};

int chan_angles[NUM_CHANS] = {
  100,
  15,
  0,
  45
};

void halftone_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
	           SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
	           int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, 
                   SDL_Rect * update_rect ATTRIBUTE_UNUSED)

{
}

void halftone_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, 
                   Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

void halftone_line_callback(void * ptr, int which ATTRIBUTE_UNUSED,
                           SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                           int x, int y)
{
  Uint8 r, g, b, or, og, ob;
  Uint32 total_r, total_g, total_b;
  Uint32 pixel;
  int xx, yy, xxx, yyy, channel, ox, oy, sqx, sqy;
  SDL_Rect dest;
  magic_api * api = (magic_api *) ptr;
  float cmyk[4];

  pixel = SDL_MapRGB(square->format, 255, 255, 255);
  SDL_FillRect(square, NULL, pixel);

  /* Lock to grid, centered around mouse */
  x = ((x / 8) - 1) * 8;
  y = ((y / 8) - 1) * 8;

  if (api->touched(x, y)) { return; }

  for (xx = 0; xx < 16; xx = xx + 4) {
    for (yy = 0; yy < 16; yy = yy + 4) {
      /* Get avg color around the mouse */
      total_r = total_g = total_b = 0;
      for (xxx = 0; xxx < 4; xxx++) {
        for (yyy = 0; yyy < 4; yyy++) {
          SDL_GetRGB(api->getpixel(canvas_backup, x + xx + xxx, y + yy + yyy), canvas_backup->format, &r, &g, &b);
          total_r += r;
          total_g += g;
          total_b += b;
        }
      }
      total_r /= 16;
      total_g /= 16;
      total_b /= 16;

      /* Convert to CMYK values */
      halftone_rgb2cmyk(total_r, total_g, total_b, cmyk);

      /* Draw C, M, Y and K blobs into our 'square' surface */
      for (channel = 0; channel < NUM_CHANS; channel++) {
        r = chan_colors[channel][0];
        g = chan_colors[channel][1];
        b = chan_colors[channel][2];

        for (xxx = 0; xxx < 8; xxx++) {
          for (yyy = 0; yyy < 8; yyy++) {
            /* A circle blob, radius based upon channel (C, M, Y or K) strength for this color */

            /* FIXME: Base it upon this channel's angle! -bjk 2011.07.17 */
            ox = xxx;
            oy = yyy;

            sqx = (xx + ox) % 16;
            sqy = (yy + oy) % 16;

            if (api->in_circle(xxx - 4, yyy - 4, cmyk[channel] * 6.0)) {
              SDL_GetRGB(api->getpixel(square, sqx, sqy), square->format, &or, &og, &ob);

              if (or == 255 && og == 255 && ob == 255) {
                /* If it's just white, put full color down */
                pixel = SDL_MapRGB(square->format, r, g, b);
              } else {
                /* Otherwise, blend a little */
                pixel = SDL_MapRGB(square->format, (r + or) / 2, (g + og) / 2, (b + ob) / 2);
              }

              api->putpixel(square, sqx, sqy, pixel);
            }
          }
        }
      }
    }
  }

  dest.x = x;
  dest.y = y;

  SDL_BlitSurface(square, NULL, canvas, &dest);
}

void halftone_switchin(magic_api * api, int which ATTRIBUTE_UNUSED, 
                 int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  if (canvas_backup == NULL)
    canvas_backup = SDL_CreateRGBSurface(SDL_SWSURFACE, api->canvas_w, api->canvas_h, canvas->format->BitsPerPixel, canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  if (square == NULL)
    square = SDL_CreateRGBSurface(SDL_SWSURFACE, 16, 16, canvas->format->BitsPerPixel, canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  /* FIXME: What to do if they come back NULL!? :( */

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
    c = 1.0 - (((float) r) / 255.0);
    m = 1.0 - (((float) g) / 255.0);
    y = 1.0 - (((float) b) / 255.0);
   
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
