/* swirls.c

   Transforms parts of the image into brush strokes that
   swirl or shoot out from around where you click,
   or in the case of Fur, paint following the mouse.

   Inspired by "Night Sky Scene [Pen Parallax]" Scratch Project
   by -HexaScape- <https://scratch.mit.edu/users/-HexaScape->

   Last updated: April 22, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

enum
{
  SWIRL_TOOL_CIRCLES,
  SWIRL_TOOL_RAYS,
  SWIRL_TOOL_FUR,
  NUM_SWIRL_TOOLS
};

char *swirl_names[NUM_SWIRL_TOOLS] = {
  gettext_noop("Circles"),
  gettext_noop("Rays"),
  gettext_noop("Fur")
};

char *swirl_descriptions[NUM_SWIRL_TOOLS][2] = {
  {
   gettext_noop("Click and drag to transform parts of your picture to circular brushstrokes."),
   gettext_noop("Click to turn your entire picture into circular brushstrokes.")},
  {
   gettext_noop("Click and drag to transform parts of your picture to brushstroke rays."),
   gettext_noop("Click to turn your entire picture into brushstroke rays.")},
  {
   gettext_noop("Click and drag to add fur to your picture."),
   ""},
};

char *swirl_icon_filenames[NUM_SWIRL_TOOLS] = {
  "swirls_circles.png",
  "swirls_rays.png",
  "swirls_fur.png"
};

char *swirl_sfx_filenames[NUM_SWIRL_TOOLS] = {
  "swirls_circles.ogg",
  "swirls_rays.ogg",
  "swirls_fur.ogg"
};

int SWIRLS_NUM_STROKES_PER_DRAG_LINE[NUM_SWIRL_TOOLS] = {
  5,
  5,
  15
};

int SWIRLS_DRAG_LINE_STROKE_RADIUS[NUM_SWIRL_TOOLS] = {
  64,
  64,
  16
};

int SWIRLS_STROKE_LENGTH[NUM_SWIRL_TOOLS] = {
  10,
  10,
  5
};

Mix_Chunk *snd_effects[NUM_SWIRL_TOOLS];
SDL_Surface *swirls_snapshot = NULL;
int swirls_start_x, swirls_start_y;
Uint32 swirl_stroke_color;
Uint8 swirl_fur_color_r, swirl_fur_color_g, swirl_fur_color_b;

Uint32 swirls_api_version(void);
int swirls_init(magic_api * api, Uint32 disabled_features);
int swirls_get_tool_count(magic_api * api);
SDL_Surface *swirls_get_icon(magic_api * api, int which);
char *swirls_get_name(magic_api * api, int which);
int swirls_get_group(magic_api * api, int which);
char *swirls_get_description(magic_api * api, int which, int mode);
int swirls_requires_colors(magic_api * api, int which);
int swirls_modes(magic_api * api, int which);
void swirls_shutdown(magic_api * api);
void swirls_click(magic_api * api, int which, int mode,
                  SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void swirls_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void swirls_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void swirls_line_callback_drag(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void swirls_draw_stroke(magic_api * api, int which, SDL_Surface * canvas, int x, int y);
void swirls_line_callback_draw_stroke(void *ptr, int which ATTRIBUTE_UNUSED,
                                      SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y);
void swirls_release(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void swirls_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void swirls_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
double get_angle(int x, int y, int target_x, int target_y);
Uint8 swirls_accepted_sizes(magic_api * api, int which, int mode);
Uint8 swirls_default_size(magic_api * api, int which, int mode);
void swirls_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                     SDL_Rect * update_rect);


Uint32 swirls_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int swirls_init(magic_api * api, Uint32 disabled_features ATTRIBUTE_UNUSED)
{
  int i;
  char fname[1024];

  for (i = 0; i < NUM_SWIRL_TOOLS; i++)
  {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s", api->data_directory, swirl_sfx_filenames[i]);
    snd_effects[i] = Mix_LoadWAV(fname);
  }

  return (1);
}

int swirls_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_SWIRL_TOOLS);
}


SDL_Surface *swirls_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s", api->data_directory, swirl_icon_filenames[which]);

  return (IMG_Load(fname));
}

char *swirls_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  return strdup(gettext(swirl_names[which]));
}

int swirls_get_group(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  if (which == SWIRL_TOOL_FUR)
  {
    return MAGIC_TYPE_PAINTING;
  }
  else
  {
    return MAGIC_TYPE_DISTORTS;
  }
}

char *swirls_get_description(magic_api * api ATTRIBUTE_UNUSED, int which, int mode)
{
  return strdup(gettext(swirl_descriptions[which][mode - 1]));
}

int swirls_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == SWIRL_TOOL_FUR)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int swirls_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  if (which == SWIRL_TOOL_FUR)
  {
    return MODE_PAINT;
  }
  else
  {
    return (MODE_PAINT | MODE_FULLSCREEN);
  }
}

void swirls_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_SWIRL_TOOLS; i++)
  {
    if (snd_effects[i] != NULL)
      Mix_FreeChunk(snd_effects[i]);
  }
}

void
swirls_click(magic_api * api, int which, int mode,
             SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect)
{
  if (snd_effects[which] != NULL)
    api->stopsound();

  swirls_start_x = x;
  swirls_start_y = y;

  if (mode == MODE_PAINT)
  {
    swirls_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
  }
  else
  {
    if (snd_effects[which] != NULL)
    {
      api->playsound(snd_effects[which], (x * 255) / canvas->w, 255);
    }

    for (x = 0; x < canvas->w; x++)
    {
      for (y = 0; y < canvas->h; y++)
      {
        if (rand() % 100 == 0)
        {
          swirls_draw_stroke(api, which, canvas, x, y);
        }
      }
    }

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}


void
swirls_drag(magic_api * api ATTRIBUTE_UNUSED, int which, SDL_Surface * canvas,
            SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect)
{
  if (which == SWIRL_TOOL_FUR)
  {
    swirls_start_x = x;
    swirls_start_y = y;
  }

  api->line((void *)api, which, canvas, snapshot, ox, oy, x, y, 1 /* FIXME: Consider fewer iterations? */ ,
            swirls_line_callback_drag);

  /* FIXME: Would be good to only update the area around the line (ox,oy)->(x,y) (+/- the maxium radius of the effect) */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void swirls_release(magic_api * api, int which,
                    SDL_Surface * canvas ATTRIBUTE_UNUSED,
                    SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                    int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  if (snd_effects[which] != NULL && which != SWIRL_TOOL_FUR)
    api->stopsound();
}


void swirls_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED,
                      SDL_Surface * last ATTRIBUTE_UNUSED,
                      Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  swirl_fur_color_r = r;
  swirl_fur_color_g = g;
  swirl_fur_color_b = b;
}


void swirls_line_callback_drag(void *ptr, int which,
                               SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  int i, ang_deg, radius, nx, ny;
  double ang_rad;
  magic_api *api = (magic_api *) ptr;

  if (snd_effects[which] != NULL)
    api->playsound(snd_effects[which], (x * 255) / canvas->w, 255);

  for (i = 0; i < SWIRLS_NUM_STROKES_PER_DRAG_LINE[which]; i++)
  {
    ang_deg = (rand() % 360);
    ang_rad = (ang_deg * M_PI) / 180.0;
    radius = (rand() % (SWIRLS_DRAG_LINE_STROKE_RADIUS[which] * 2)) - SWIRLS_DRAG_LINE_STROKE_RADIUS[which];

    nx = x + (int)(cos(ang_rad) * radius);
    ny = y + (int)(sin(ang_rad) * radius);

    swirls_draw_stroke(api, which, canvas, nx, ny);
  }
}

void swirls_draw_stroke(magic_api * api, int which, SDL_Surface * canvas, int x, int y)
{
  int x1, y1, x2, y2, len;
  double a;
  Uint8 r, g, b;
  float h, s, v;

  len = SWIRLS_STROKE_LENGTH[which];

  a = get_angle(x, y, swirls_start_x, swirls_start_y);
  if (which == SWIRL_TOOL_CIRCLES)
  {
    a = a + (M_PI / 2.0);
  }

  x1 = x - cos(a) * len;
  y1 = y - sin(a) * len;

  x2 = x + cos(a) * len;
  y2 = y + sin(a) * len;

  if (which == SWIRL_TOOL_FUR)
  {
    r = swirl_fur_color_r;
    g = swirl_fur_color_g;
    b = swirl_fur_color_b;
  }
  else
  {
    swirl_stroke_color = api->getpixel(swirls_snapshot, x, y);
    SDL_GetRGB(swirl_stroke_color, canvas->format, &r, &g, &b);
  }

  api->rgbtohsv(r, g, b, &h, &s, &v);
  h = h + (((rand() % 7) - 3) / 10.0);
  if (s > 0.00)
  {
    s = s + (((rand() % 3) - 1) / 10.0);
  }
  v = v + (((rand() % 3) - 1) / 10.0);
  if (h < 0.0)
  {
    h = h - 360.0;
  }
  else if (h >= 360.0)
  {
    h = h - 360.0;
  }
  if (s < 0.0)
  {
    s = 0.0;
  }
  else if (s > 1.0)
  {
    s = 1.0;
  }
  if (v < 0.0)
  {
    v = 0.0;
  }
  else if (v > 1.0)
  {
    v = 1.0;
  }
  api->hsvtorgb(h, s, v, &r, &g, &b);
  swirl_stroke_color = SDL_MapRGB(canvas->format, r, g, b);

  api->line((void *)api, which, canvas, NULL /* N/A */ ,
            x1, y1, x2, y2, 1, swirls_line_callback_draw_stroke);
}


void swirls_line_callback_draw_stroke(void *ptr, int which,
                                      SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  if (which == SWIRL_TOOL_FUR)
  {
    api->putpixel(canvas, x, y, swirl_stroke_color);
  }
  else
  {
    int xx, yy;

    for (yy = -1; yy <= 1; yy++)
    {
      for (xx = -1; xx <= 1; xx++)
      {
        api->putpixel(canvas, x + xx, y + yy, swirl_stroke_color);
      }
    }
  }
}


void swirls_switchin(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas)
{
  if (swirls_snapshot == NULL)
    swirls_snapshot = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                                           canvas->format->BitsPerPixel, canvas->format->Rmask,
                                           canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  if (swirls_snapshot != NULL)
    SDL_BlitSurface(canvas, NULL, swirls_snapshot, NULL);
}

void swirls_switchout(magic_api * api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

double get_angle(int x, int y, int target_x, int target_y)
{
  return atan2((double)(y - target_y), (double)(x - target_x));
}


Uint8 swirls_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  /* TODO - We could offer different radiuses for "Circles and Rays",
   * and perhaps some different functionality for "Fur", based on size.
   * For now, not utilizing Magic tool size feature. -bjk 2023.04.22 */
  return 0;
}

Uint8 swirls_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

void swirls_set_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                     Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}
