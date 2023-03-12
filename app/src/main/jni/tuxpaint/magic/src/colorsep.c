/* colorsep.c

   Color separation effect (a la red/cyan aka red/blue 3D glasses).
   Bill Kendrick

   Last updated: February 22, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

enum {
  COLORSEP_TOOL_3DGLASSES,
  COLORSEP_TOOL_COLORSEP,
  NUM_TOOLS
};

static char * colorsep_snd_filenames[NUM_TOOLS] = {
  "3dglasses.ogg",
  "colorsep.ogg"
};

static char * colorsep_icon_filenames[NUM_TOOLS] = {
  "3dglasses.png",
  "colorsep.png"
};

char * colorsep_names[NUM_TOOLS] = {
  gettext_noop("3D Glasses"),
  gettext_noop("Color Sep."),
};
char * colorsep_descrs[NUM_TOOLS] = {
  gettext_noop("Click and drag left and right to separate your picture's red and cyan, to make anaglyphs you can view with 3D glasses!"),
  gettext_noop("Click and drag to separate your picture's colors.")
};

Mix_Chunk *snd_effects[NUM_TOOLS];
int colorsep_click_x, colorsep_click_y;
float colorsep_r_pct, colorsep_g_pct, colorsep_b_pct;

Uint32 colorsep_api_version(void);
int colorsep_init(magic_api * api);
int colorsep_get_tool_count(magic_api * api);
SDL_Surface *colorsep_get_icon(magic_api * api, int which);
char *colorsep_get_name(magic_api * api, int which);
int colorsep_get_group(magic_api * api, int which);
char *colorsep_get_description(magic_api * api, int which, int mode);
int colorsep_requires_colors(magic_api * api, int which);
int colorsep_modes(magic_api * api, int which);
void colorsep_shutdown(magic_api * api);
void colorsep_click(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x,
                     int y, SDL_Rect * update_rect);
void colorsep_set_color(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void colorsep_drag(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * snapshot, int ox, int oy, int x, int y,
                    SDL_Rect * update_rect);
void colorsep_release(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int x, int y,
                       SDL_Rect * update_rect);
void colorsep_switchin(magic_api * api, int which, int mode,
                        SDL_Surface * canvas);
void colorsep_switchout(magic_api * api, int which, int mode,
                         SDL_Surface * canvas);


Uint32 colorsep_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int colorsep_init(magic_api * api)
{
  int i;
  char fname[1024];

  for (i = 0; i < NUM_TOOLS; i++) {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s",
             api->data_directory, colorsep_snd_filenames[i]);
    snd_effects[i] = Mix_LoadWAV(fname);
  }

  return (1);
}

int colorsep_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}


SDL_Surface *colorsep_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s",
           api->data_directory, colorsep_icon_filenames[which]);

  return (IMG_Load(fname));
}

char *colorsep_get_name(magic_api * api ATTRIBUTE_UNUSED,
                         int which)
{
  return strdup(gettext(colorsep_names[which]));
}

int colorsep_get_group(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}

char *colorsep_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                int which, int mode ATTRIBUTE_UNUSED)
{
  return strdup(gettext(colorsep_descrs[which]));
}

int colorsep_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                              int which ATTRIBUTE_UNUSED)
{
  if (which == COLORSEP_TOOL_COLORSEP)
    return 1;
  else
    return 0;
}

int colorsep_modes(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}

void colorsep_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    if (snd_effects[i] != NULL)
      Mix_FreeChunk(snd_effects[i]);
  }
}

void
colorsep_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect)
{
  api->stopsound();

  /* (Start off as if they clicked off to the side a little, so that
   * quick click+release will split the image bit by bit */
  colorsep_click_x = x;
  colorsep_click_y = y;
  colorsep_drag(api, which, canvas, snapshot, x, y, x - 10, y, update_rect);
}


void
colorsep_drag(magic_api * api ATTRIBUTE_UNUSED, int which, SDL_Surface * canvas,
              SDL_Surface * snapshot, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED,
              int x, int y, SDL_Rect * update_rect)
{
  int xx, yy, offset_x, offset_y;
  Uint8 r1, g1, b1, r2, g2, b2, r, g, b;

  offset_x = colorsep_click_x - x;
  if (which == COLORSEP_TOOL_3DGLASSES) {
    offset_y = 0;
  } else {
    offset_y = colorsep_click_y - y;
  }

  api->playsound(snd_effects[which], (((-offset_x * 255) / canvas->w) + 128), 255);

  for (yy = 0; yy < canvas->h; yy++) {
    for (xx = 0; xx < canvas->w; xx++) {
      SDL_GetRGB(api->getpixel(snapshot, xx + offset_x / 2, yy + offset_y / 2), snapshot->format, &r1, &g1, &b1);
      SDL_GetRGB(api->getpixel(snapshot, xx - offset_x / 2, yy - offset_y / 2), snapshot->format, &r2, &g2, &b2);

      if (which == COLORSEP_TOOL_3DGLASSES) {
        r = r1;
        g = g2;
        b = b2;
      } else {
        r = (Uint8) ((float) r1 * colorsep_r_pct) + ((float) r2 * (1.0 - colorsep_r_pct));
        g = (Uint8) ((float) g1 * colorsep_g_pct) + ((float) g2 * (1.0 - colorsep_g_pct));
        b = (Uint8) ((float) b1 * colorsep_b_pct) + ((float) b2 * (1.0 - colorsep_b_pct));
      }

      api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, r, g, b));
    }
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void colorsep_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas ATTRIBUTE_UNUSED,
                  SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
                  SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void colorsep_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED,
                      SDL_Surface * last ATTRIBUTE_UNUSED,
                      Uint8 r, Uint8 g, Uint8 b,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  colorsep_r_pct = (float) r / 255.0;
  colorsep_g_pct = (float) g / 255.0;
  colorsep_b_pct = (float) b / 255.0;

  if (colorsep_r_pct == colorsep_g_pct && colorsep_r_pct == colorsep_b_pct) {
    colorsep_r_pct = 1.0;
    colorsep_g_pct = 0.0;
    colorsep_b_pct = 0.0;
  }
}


void colorsep_switchin(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void colorsep_switchout(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED,
                         int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

