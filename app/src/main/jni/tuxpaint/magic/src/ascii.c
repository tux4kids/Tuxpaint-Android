/*
  ascii.c

  Converts the image to ASCII art.

  Tux Paint - A simple drawing program for children.

  Copyright (c) 2024 by Bill Kendrick

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

  Last updated: October 7, 2024
*/

//#define DEBUG

#if defined(DEBUG)
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include <stdio.h>
#include <string.h>
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

enum
{
  TOOL_TYPEWRITER,
  TOOL_COMPUTER,
  NUM_TOOLS
};

#define TOOL_COMPUTER_COLOR NUM_TOOLS

char *ascii_tool_names[NUM_TOOLS + 1] = {
  gettext_noop("Typewriter"),
  gettext_noop("Computer"),
  gettext_noop("Color Computer"),       // special version of "computer"
};

char *ascii_tool_filenames[NUM_TOOLS + 1] = {
  "typewriter",
  "computer",
  "color_computer",
};

static Mix_Chunk *ascii_snd[NUM_TOOLS];

/* For each variation, we'll have a bitmap with an arbitrary number
 * of potentially-proportionally-spaced characters (which we'll treat
 * as fixed-width), so we need to keep track of each character's X
 * poistion, how many characters there are, and the maximum width.
 */
#define MAX_CHARS 256
SDL_Surface *ascii_bitmap[NUM_TOOLS];
int ascii_char_x[NUM_TOOLS][MAX_CHARS];
int ascii_num_chars[NUM_TOOLS];
int ascii_char_maxwidth[NUM_TOOLS];
int ascii_char_brightness[NUM_TOOLS][MAX_CHARS];
SDL_Surface *ascii_snapshot = NULL;
int ascii_size;
Uint8 ascii_r, ascii_g, ascii_b;
Uint8 ascii_clear_r[NUM_TOOLS], ascii_clear_g[NUM_TOOLS], ascii_clear_b[NUM_TOOLS];

/* Based on CGA color palette
   <https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette> */
const Uint8 ascii_computer_colors[16][3] = {
  {0x00, 0x00, 0x00},           // Black
  {0x55, 0x55, 0x55},           // Dark gray
  {0xAA, 0xAA, 0xAA},           // Light gray
  {0xFF, 0xFF, 0xFF},           // White
  {0x00, 0x00, 0xAA},           // Blue
  {0x55, 0x55, 0xFF},           // Light blue
  {0x00, 0xAA, 0x00},           // Green
  {0x55, 0xFF, 0x55},           // Light green
  {0x00, 0xAA, 0xAA},           // Cyan
  {0x55, 0xFF, 0xFF},           // Light cyan
  {0xAA, 0x00, 0x00},           // Red
  {0xFF, 0x55, 0x55},           // Light red
  {0xAA, 0x00, 0xAA},           // Magenta
  {0xFF, 0x55, 0xFF},           // Light magenta
  {0xAA, 0x55, 0x00},           // Brown
  {0xFF, 0xFF, 0x55},           // Yellow
};

Uint32 ascii_api_version(void);
int ascii_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int ascii_get_tool_count(magic_api * api);
SDL_Surface *ascii_get_icon(magic_api * api, int which);
char *ascii_get_name(magic_api * api, int which);
int ascii_get_group(magic_api * api, int which);
int ascii_get_order(int which);
char *ascii_get_description(magic_api * api, int which, int mode);

void ascii_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * last, int ox, int oy, int x, int y, SDL_Rect * update_rect);

void ascii_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void ascii_release(magic_api * api, int which,
                   SDL_Surface * canvas, SDL_Surface * last, int x, int y, SDL_Rect * update_rect);

void ascii_shutdown(magic_api * api);
void ascii_set_color(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int ascii_requires_colors(magic_api * api, int which);
void ascii_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void ascii_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
int ascii_modes(magic_api * api, int which);
Uint8 ascii_accepted_sizes(magic_api * api, int which, int mode);
Uint8 ascii_default_size(magic_api * api, int which, int mode);
void ascii_set_size(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * last, Uint8 size,
                    SDL_Rect * update_rect);
void do_ascii_effect(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * last, int x, int y);
int get_best_char(int which, int brightness);
int get_bright(magic_api * api, int r, int g, int b);


Uint32 ascii_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int ascii_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];
  int i, j, x, y, xx, w, num_chars, all_clear, area, bright, clear_brightness;
  int min_bright, max_bright;
  Uint32 clear_pixel, pixel;
  Uint8 r, g, b;
  Uint8 clear_r, clear_g, clear_b;

  for (i = 0; i < NUM_TOOLS; i++)
  {
    ascii_snd[i] = NULL;
    ascii_bitmap[i] = NULL;
  }

  /* (N.B. Computer & Color Computer share sound & bitmap) */
  for (i = 0; i < NUM_TOOLS; i++)
  {
    /* Load our sound */
    snprintf(fname, sizeof(fname), "%ssounds/magic/ascii-%s.ogg", api->data_directory, ascii_tool_filenames[i]);
    ascii_snd[i] = Mix_LoadWAV(fname);

    /* Load and process our bitmap "font" */
    snprintf(fname, sizeof(fname), "%simages/magic/ascii-%s.png", api->data_directory, ascii_tool_filenames[i]);
    ascii_bitmap[i] = IMG_Load(fname);

    if (ascii_bitmap[i] == NULL)
    {
      fprintf(stderr, "Cannot load %s\n", fname);
      return (0);
    }

    clear_pixel = api->getpixel(ascii_bitmap[i], 0, 0);
    SDL_GetRGB(clear_pixel, ascii_bitmap[i]->format, &clear_r, &clear_g, &clear_b);
    DEBUG_PRINTF("%s; clear pixel %d (%d,%d,%d)\n", fname, clear_pixel, clear_r, clear_g, clear_b);
    clear_brightness = (clear_r + clear_g + clear_b) / 3;
    ascii_clear_r[i] = clear_r;
    ascii_clear_g[i] = clear_g;
    ascii_clear_b[i] = clear_b;

    num_chars = 0;
    for (x = 0; x < ascii_bitmap[i]->w; x++)
    {
      /* Skip whitespace between characters */
      do
      {
        all_clear = 1;
        for (y = 0; y < ascii_bitmap[i]->h /* && all_clear */ ; y++)
        {
          pixel = api->getpixel(ascii_bitmap[i], x, y);
          if (pixel != clear_pixel)
            all_clear = 0;
        }
        if (all_clear)
          x++;
      }
      while (all_clear && x < ascii_bitmap[i]->w);

      ascii_char_x[i][num_chars] = x;

      /* Capture the extent of the character */
      all_clear = 0;
      for (xx = x; xx < ascii_bitmap[i]->w && !all_clear; xx++)
      {
        all_clear = 1;
        for (y = 0; y < ascii_bitmap[i]->h /* && all_clear */ ; y++)
        {
          pixel = api->getpixel(ascii_bitmap[i], xx, y);
          if (pixel != clear_pixel)
          {
            all_clear = 0;
            SDL_GetRGB(pixel, ascii_bitmap[i]->format, &r, &g, &b);
            if (r == 255 && g == 0 && b == 255)
            {
              /* Magenta counts as a connecting pixel, but we
               * want it to appear as the clear color */
              api->putpixel(ascii_bitmap[i], xx, y, clear_pixel);
              DEBUG_PRINTF("x");
            }
            else
            {
              DEBUG_PRINTF("#");
            }
          }
          else
          {
            DEBUG_PRINTF("-");
          }
        }
        DEBUG_PRINTF("\n");
      }
      x = xx - 1;
      num_chars++;
      DEBUG_PRINTF(".......................................\n");
    }
    ascii_num_chars[i] = num_chars;
    DEBUG_PRINTF("%s has %d characters\n", fname, num_chars);

    /* Determine the max. width of any character */
    ascii_char_x[i][num_chars] = x;
    ascii_char_maxwidth[i] = 0;
    for (j = 0; j < num_chars; j++)
    {
      w = ascii_char_x[i][j + 1] - ascii_char_x[i][j];
      DEBUG_PRINTF("%d->%d = %d\n", j, j + 1, w);
      if (w > ascii_char_maxwidth[i])
      {
        ascii_char_maxwidth[i] = w;
      }
    }

    /* Calculate the intensity of each character */
    area = ascii_char_maxwidth[i] * ascii_bitmap[i]->h;

    DEBUG_PRINTF("%s max char width is %d -- * %d = area %d\n", fname, ascii_char_maxwidth[i], ascii_bitmap[i]->h,
                 area);

    for (j = 0; j < num_chars; j++)
    {
      bright = 0;
      for (y = 0; y < ascii_bitmap[i]->h; y++)
      {
        for (x = ascii_char_x[i][j]; x < ascii_char_x[i][j + 1]; x++)
        {
          pixel = api->getpixel(ascii_bitmap[i], x, y);
          SDL_GetRGB(pixel, ascii_bitmap[i]->format, &r, &g, &b);

          DEBUG_PRINTF("%3d (%3d) ", (r + g + b) / 3, get_bright(api, r, g, b));
          bright += get_bright(api, r, g, b);
        }
        DEBUG_PRINTF("\n");
      }
      DEBUG_PRINTF("char %3d brightness = %3d before padding -- ", j, bright / area);
      w = ascii_char_maxwidth[i] - (ascii_char_x[i][j + 1] - ascii_char_x[i][j]) - 2;   /* don't let padding affect _too_ much */
      if (w >= 1)
        bright += (clear_brightness * ascii_bitmap[i]->h * w);
      DEBUG_PRINTF("%3d after padding %d width\n", bright / area, w);
      ascii_char_brightness[i][j] = bright / area;
    }

    /* Stretch the brightnesses, so we cover more of 0->255 */
    min_bright = 255;
    max_bright = 0;
    for (j = 0; j < num_chars; j++)
    {
      if (ascii_char_brightness[i][j] > max_bright)
        max_bright = ascii_char_brightness[i][j];
      if (ascii_char_brightness[i][j] < max_bright)
        min_bright = ascii_char_brightness[i][j];
    }
    DEBUG_PRINTF("brightnesses between %d and %d\n", min_bright, max_bright);

    /* https://rosettacode.org/wiki/Map_range#C */
#define map_range(a1,a2,b1,b2,s) (b1 + (s-a1)*(b2-b1)/(a2-a1))

    for (j = 0; j < num_chars; j++)
    {
      DEBUG_PRINTF("mapping %3d -> ", ascii_char_brightness[i][j]);
      ascii_char_brightness[i][j] = map_range(min_bright, max_bright, 0, 255, ascii_char_brightness[i][j]);
      DEBUG_PRINTF("%3d\n", ascii_char_brightness[i][j]);
    }
  }

  return (1);
}

int ascii_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS + 1);
}

SDL_Surface *ascii_get_icon(magic_api *api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/ascii_%s.png", api->data_directory, ascii_tool_filenames[which]);

  return (IMG_Load(fname));
}

char *ascii_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  char tmp[1024];

  snprintf(tmp, sizeof(tmp), gettext("ASCII %s"), gettext(ascii_tool_names[which]));
  return (strdup(tmp));
}

int ascii_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_DISTORTS;
}

int ascii_get_order(int which)
{
  return 2050 + which;
}

char *ascii_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode)
{
  char tmp[1024];

  if (mode == MODE_PAINT)
    snprintf(tmp, sizeof(tmp), gettext("Click and drag to transform parts of your drawing to ASCII art (%s style)."),
             gettext(ascii_tool_names[which]));
  else
    snprintf(tmp, sizeof(tmp), gettext("Click to transform your entire drawing to ASCII art (%s style)."),
             gettext(ascii_tool_names[which]));

  return (strdup(tmp));
}

void ascii_drag(magic_api *api, int which, SDL_Surface *canvas,
                SDL_Surface *last, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  api->line((void *)api, which, canvas, last, ox, oy, x, y, 1, do_ascii_effect);

  /* FIXME */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  if (which == TOOL_COMPUTER_COLOR)
    which = TOOL_COMPUTER;

  api->playsound(ascii_snd[which], (x * 255) / canvas->w, 255);
}

void ascii_click(magic_api *api, int which, int mode,
                 SDL_Surface *canvas, SDL_Surface *last, int x, int y, SDL_Rect *update_rect)
{
  if (mode == MODE_PAINT)
    ascii_drag(api, which, canvas, last, x, y, x, y, update_rect);
  else
  {
    int xx, yy;

    if (which == TOOL_COMPUTER_COLOR)
      api->playsound(ascii_snd[TOOL_COMPUTER], (x * 255) / canvas->w, 255);
    else
      api->playsound(ascii_snd[which], (x * 255) / canvas->w, 255);

    for (yy = 0; yy < canvas->h; yy++)
    {
      for (xx = 0; xx < canvas->w; xx++)
      {
        do_ascii_effect(api, which, canvas, last, xx, yy);
      }
      if (yy % 10 == 0)
        api->update_progress_bar();
    }

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}

void ascii_release(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                   SDL_Surface *canvas ATTRIBUTE_UNUSED,
                   SDL_Surface *last ATTRIBUTE_UNUSED, int x ATTRIBUTE_UNUSED,
                   int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  api->stopsound();
}

void ascii_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
  {
    if (ascii_snd[i] != NULL)
      Mix_FreeChunk(ascii_snd[i]);
    if (ascii_bitmap[i] != NULL)
      SDL_FreeSurface(ascii_bitmap[i]);
  }

  if (ascii_snapshot != NULL)
  {
    SDL_FreeSurface(ascii_snapshot);
    ascii_snapshot = NULL;
  }
}

void ascii_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED,
                     SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b,
                     SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  /* If the bitmap's "clear" color, choose the opposite! */
  if (abs(r - ascii_clear_r[which]) < 8 && abs(g - ascii_clear_g[which]) < 8 && abs(b - ascii_clear_b[which]) < 8)
  {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }

  ascii_r = r;
  ascii_g = g;
  ascii_b = b;
}

int ascii_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  if (which == TOOL_TYPEWRITER || which == TOOL_COMPUTER)
    return 1;

  return 0;
}

void ascii_switchin(magic_api *api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
  if (ascii_snapshot == NULL)
    ascii_snapshot = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                                          canvas->format->BitsPerPixel, canvas->format->Rmask,
                                          canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  if (ascii_snapshot != NULL)
  {
    /* FIXME: When switching from PAINT to FULLSCREEN mode,
     * we switch out & back in, which means we take a fresh
     * snapshot even though we didn't leave the overall tool,
     * which is less than ideal. -bjk 2024.09.27 */
    SDL_BlitSurface(canvas, NULL, ascii_snapshot, NULL);
  }
}

void ascii_switchout(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

int ascii_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}


Uint8 ascii_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  if (mode == MODE_PAINT)
    return 6;
  else
    return 0;
}

Uint8 ascii_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 3;
  else
    return 0;
}

void ascii_set_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED,
                    Uint8 size, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  ascii_size = size;
}

void do_ascii_effect(void *ptr, int which, SDL_Surface *canvas, SDL_Surface *last ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int w, h, n, sx, sy, xx, yy, ww, brightness, rr, gg, bb;
  Uint8 r, g, b;
  Uint32 clear_pixel;
  Uint8 clear_brightness;
  SDL_Rect src, dest;
  int computer_color = 0;

  if (which == TOOL_COMPUTER_COLOR)
  {
    which = TOOL_COMPUTER;
    computer_color = 1;
  }

  w = ascii_char_maxwidth[which];
  h = ascii_bitmap[which]->h;

  x = (x / w) * w;
  y = (y / h) * h;

  for (sy = y - (h * (ascii_size - 1)); sy <= y + (h * (ascii_size - 1)); sy += h)
  {
    for (sx = x - (w * (ascii_size - 1)); sx <= x + (w * (ascii_size - 1)); sx += w)
    {
      if (!api->touched(sx, sy))
      {
        clear_pixel = api->getpixel(ascii_bitmap[which], 0, 0);
        SDL_GetRGB(clear_pixel, ascii_bitmap[which]->format, &r, &g, &b);
        clear_brightness = ((r + g + b) / 3.0);

        brightness = 0;
        rr = 0;
        gg = 0;
        bb = 0;
        for (yy = sy; yy < sy + h; yy++)
        {
          for (xx = sx; xx < sx + w; xx++)
          {
            SDL_GetRGB(api->getpixel(ascii_snapshot, xx, yy), ascii_snapshot->format, &r, &g, &b);
            brightness += get_bright(api, r, g, b);

            if (computer_color)
            {
              rr += r;
              gg += g;
              bb += b;
            }
          }
        }
        brightness = brightness / (w * h);


        /* Background (and also, all we draw, if "Space") */
        dest.x = sx;
        dest.y = sy;
        dest.w = w;
        dest.h = h;

        SDL_FillRect(canvas, &dest, clear_pixel);


        if (brightness != clear_brightness)
        {
          /* A visible character */

          if (computer_color)
          {
            int i, best;

            /* Find the best color, based on the avg. of the
               pixels we're replacing */
            rr /= (w * h);
            gg /= (w * h);
            bb /= (w * h);

            DEBUG_PRINTF("avg is %02x%02x%02x; ", rr, gg, bb);

            /* Map each RGB component to _plausible_ values (0x00, 0x55, 0xAA, 0xFF) */
            if (rr < 0x40)
              rr = 0x00;
            else if (rr <= 0x80)
              rr = 0x55;
            else if (rr <= 0xC0)
              rr = 0xAA;
            else
              rr = 0xFF;

            if (gg < 0x40)
              gg = 0x00;
            else if (gg <= 0x80)
              gg = 0x55;
            else if (gg <= 0xC0)
              gg = 0xAA;
            else
              gg = 0xFF;


            if (bb < 0x40)
              bb = 0x00;
            else if (bb <= 0x80)
              bb = 0x55;
            else if (bb <= 0xC0)
              bb = 0xAA;
            else
              bb = 0xFF;

            best = -1;
            for (i = 0; i < 16; i++)
            {
              if (rr == ascii_computer_colors[i][0] &&
                  gg == ascii_computer_colors[i][1] && bb == ascii_computer_colors[i][2])
              {
                /* Exact match */
                best = i;
              }
            }

            if (best == -1)
            {
              for (i = 0; i < 16; i++)
              {
                if ((rr == ascii_computer_colors[i][0] &&
                     gg == ascii_computer_colors[i][1] &&
                     abs(bb - ascii_computer_colors[i][2]) <= 0x55) ||
                    (gg == ascii_computer_colors[i][1] &&
                     bb == ascii_computer_colors[i][2] &&
                     abs(rr - ascii_computer_colors[i][0]) <= 0x55) ||
                    (bb == ascii_computer_colors[i][2] &&
                     rr == ascii_computer_colors[i][0] && abs(gg - ascii_computer_colors[i][1]) <= 0x55))
                {
                  /* Very close match */
                  best = i;
                }
              }
            }

            if (best == -1)
            {
              for (i = 0; i < 16; i++)
              {
                if ((rr == ascii_computer_colors[i][0] &&
                     abs(gg - ascii_computer_colors[i][1]) <= 0x55 &&
                     abs(bb - ascii_computer_colors[i][2]) <= 0x55) ||
                    (gg == ascii_computer_colors[i][1] &&
                     abs(bb - ascii_computer_colors[i][2]) <= 0x55 &&
                     abs(rr - ascii_computer_colors[i][0]) <= 0x55) ||
                    (bb == ascii_computer_colors[i][2] &&
                     abs(rr - ascii_computer_colors[i][0]) <= 0x55 && abs(gg - ascii_computer_colors[i][1]) <= 0x55))
                {
                  /* Pretty close match */
                  best = i;
                }
              }
            }

            DEBUG_PRINTF("best for %02x%02x%02x = %d: ", rr, gg, bb, best);
            if (best == -1)
              best = 0;         // oops!
            rr = ascii_computer_colors[best][0];
            gg = ascii_computer_colors[best][1];
            bb = ascii_computer_colors[best][2];
            DEBUG_PRINTF("%02x%02x%02x\n", rr, gg, bb);
          }
          else
          {
            /* Use the user-chosen color */
            rr = ascii_r;
            gg = ascii_g;
            bb = ascii_b;
          }


          /* Blit the glyph */
          n = get_best_char(which, brightness);

          ww = ascii_char_x[which][n + 1] - ascii_char_x[which][n];

          dest.x = sx + (w - ww) / 2;
          dest.y = sy;
          dest.w = ww;
          dest.h = h;
          SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, rr, gg, bb));

          src.x = ascii_char_x[which][n];
          src.y = 0;
          src.w = ww;
          src.h = h;

          dest.x = sx + (w - src.w) / 2;
          dest.y = sy;

          SDL_BlitSurface(ascii_bitmap[which], &src, canvas, &dest);
        }
      }
    }
  }
}

int get_best_char(int which, int brightness)
{
  int i, diff, best_idx, best_diff;

  best_idx = -1;
  best_diff = 255;
  for (i = 0; i < ascii_num_chars[which]; i++)
  {
    diff = abs(ascii_char_brightness[which][i] - brightness);

    if (diff == best_diff)
    {
      if (rand() % 10 <= 3)
        best_idx = 1;
    }
    else if (diff < best_diff)
    {
      best_diff = diff;
      best_idx = i;
    }
  }

  if (best_idx == -1)
  {
    /* Shouldn't happen, but just in case */
    best_idx = rand() % ascii_num_chars[which];
    printf("!?\n");
  }

  DEBUG_PRINTF("best for brightness %d is %d (brightness %d)\n",
               brightness, best_idx, ascii_char_brightness[which][best_idx]);

  return best_idx;
}

int get_bright(magic_api *api, int r, int g, int b)
{
  float fr, fg, fb, y;

  fr = api->sRGB_to_linear(r);
  fg = api->sRGB_to_linear(g);
  fb = api->sRGB_to_linear(b);

  y = (0.2126 * fr) + (0.7152 * fg) + (0.0722 * fb);

  return (int)(y * 255);
}
