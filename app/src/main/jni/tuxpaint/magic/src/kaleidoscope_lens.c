/* kaleidoscope_lens.c

   Transform the canvas as though looking at it through a
   kaleidoscope.
   Bill Kendrick

   Last updated: March 8, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

enum {
  KAL_LENS_4,
  KAL_LENS_6,
  KAL_LENS_8,
  NUM_TOOLS
};

static char * kaleidoscope_lens_snd_filenames[NUM_TOOLS] = {
  "kaleido-4.ogg",
  "kaleido-6.ogg",
  "kaleido-8.ogg",
};

static char * kaleidoscope_lens_icon_filenames[NUM_TOOLS] = {
  "kaleido-4.png",
  "kaleido-6.png",
  "kaleido-8.png",
};

char * kaleidoscope_lens_names[NUM_TOOLS] = {
  gettext_noop("Kaleido-4"),
  gettext_noop("Kaleido-6"),
  gettext_noop("Kaleido-8"),
};

char * kaleidoscope_lens_descrs[NUM_TOOLS] = {
  gettext_noop("Click and drag around your picture to look through it with a kaleidoscope!"),
  gettext_noop("Click and drag around your picture to look through it with a kaleidoscope!"),
  gettext_noop("Click and drag around your picture to look through it with a kaleidoscope!"),
};

Mix_Chunk *snd_effects[NUM_TOOLS];

Uint32 kaleidoscope_lens_api_version(void);
int kaleidoscope_lens_init(magic_api * api);
int kaleidoscope_lens_get_tool_count(magic_api * api);
SDL_Surface *kaleidoscope_lens_get_icon(magic_api * api, int which);
char *kaleidoscope_lens_get_name(magic_api * api, int which);
int kaleidoscope_lens_get_group(magic_api * api, int which);
char *kaleidoscope_lens_get_description(magic_api * api, int which, int mode);
int kaleidoscope_lens_requires_colors(magic_api * api, int which);
int kaleidoscope_lens_modes(magic_api * api, int which);
void kaleidoscope_lens_shutdown(magic_api * api);
void kaleidoscope_lens_click(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x,
                     int y, SDL_Rect * update_rect);
void kaleidoscope_lens_set_color(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void kaleidoscope_lens_drag(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * snapshot, int ox, int oy, int x, int y,
                    SDL_Rect * update_rect);
void kaleidoscope_lens_render(magic_api *, int which, SDL_Surface * canvas,
                              SDL_Surface * snapshot, int x, int y, int preview);
void kaleidoscope_lens_release(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int x, int y,
                       SDL_Rect * update_rect);
void kaleidoscope_lens_switchin(magic_api * api, int which, int mode,
                        SDL_Surface * canvas);
void kaleidoscope_lens_switchout(magic_api * api, int which, int mode,
                         SDL_Surface * canvas);
int mirror(int n, int max, int flip);


Uint32 kaleidoscope_lens_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int kaleidoscope_lens_init(magic_api * api)
{
  int i;
  char fname[1024];

  for (i = 0; i < NUM_TOOLS; i++) {
    snprintf(fname, sizeof(fname), "%ssounds/magic/%s",
             api->data_directory, kaleidoscope_lens_snd_filenames[i]);
    snd_effects[i] = Mix_LoadWAV(fname);
  }

  return (1);
}

int kaleidoscope_lens_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_TOOLS);
}


SDL_Surface *kaleidoscope_lens_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s",
           api->data_directory, kaleidoscope_lens_icon_filenames[which]);

  return (IMG_Load(fname));
}

char *kaleidoscope_lens_get_name(magic_api * api ATTRIBUTE_UNUSED,
                         int which)
{
  return strdup(gettext(kaleidoscope_lens_names[which]));
}

int kaleidoscope_lens_get_group(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PICTURE_WARPS;
}

char *kaleidoscope_lens_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                int which, int mode ATTRIBUTE_UNUSED)
{
  return strdup(gettext(kaleidoscope_lens_descrs[which]));
}

int kaleidoscope_lens_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                              int which ATTRIBUTE_UNUSED)
{
  return 0;
}

int kaleidoscope_lens_modes(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT_WITH_PREVIEW;
}

void kaleidoscope_lens_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    if (snd_effects[i] != NULL)
      Mix_FreeChunk(snd_effects[i]);
  }
}

void
kaleidoscope_lens_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect)
{
  api->stopsound();

  kaleidoscope_lens_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}

#define linear(start, end, dist, full) (start + (((end - start) * dist) / full))
#define length(x1, y1, x2, y2) sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2))

int mirror(int n, int max, int flip) {
  int adjusted;

  if (flip)
    n = max - n;

  do {
    adjusted = 0;
    if (n < 0) {
      n = -n;
      adjusted = 1;
    }
    if (n >= max) {
      n = (max - 1) - (n - max);
      adjusted = 1;
    }
  } while (adjusted);

  return n;
}

void
kaleidoscope_lens_drag(magic_api * api, int which, SDL_Surface * canvas,
              SDL_Surface * snapshot, int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED,
              int x, int y, SDL_Rect * update_rect)
{
  if (snd_effects[which] != NULL) {
    api->playsound(snd_effects[which], 128, 255);
  }

  kaleidoscope_lens_render(api, which, canvas, snapshot, x, y, 1);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void kaleidoscope_lens_render(magic_api * api, int which, SDL_Surface * canvas,
                              SDL_Surface * snapshot, int x, int y, int preview)
{
  int off_x, off_y, sides, max_radius;
  float angle, angle_offset;
  int s, r;
  float a1, a2;
  int dx, dy, dx2, dy2, len, src_x, src_y, xx, xxm, push, done;
  SDL_Rect dest;
  Uint32 colr;

  off_x = (canvas->w / 2) - x * 2;
  off_y = (canvas->h / 2) - y * 2;
  max_radius = max(canvas->w, canvas->h); /* FIXME: Better calculation should be used! */

  if (which == KAL_LENS_4) {
    sides = 4;
  } else if (which == KAL_LENS_6) {
    sides = 6;
  } else if (which == KAL_LENS_8) {
    sides = 8;
  } else {
    return;
  }

  angle = (2 * M_PI) / sides;
  angle_offset = angle / 2;

  /* Go around each side */
  for (s = 0; s < sides; s++) {
    a1 = (angle * (float) s) + angle_offset;
    a2 = (angle * (float) (s + 1)) + angle_offset;

    /* From the center outward... */
    for (r = 0; r < max_radius; r = r + (preview ? 4 : 1)) {
      dx = (canvas->w / 2) + cos(a1) * r;
      dy = (canvas->h / 2) - sin(a1) * r;

      dx2 = (canvas->w / 2) + cos(a2) * r;
      dy2 = (canvas->h / 2) - sin(a2) * r;

      len = length(dx, dy, dx2, dy2);

      /* Scan rows of the source, and draw along each triangle */
      if (len != 0) {
        xxm = ((len > 0 ? 1 : -1 ) * (preview ? 4 : 1));

        push = (canvas->w - abs(len)) / 2;
        xx = 0;
        done = 0;
        do {
          src_x = (canvas->w / 2) + off_x + xx + push;
          src_x = mirror(src_x, canvas->w, (s % 2));

          src_y = r + off_y;
          src_y = mirror(src_y, canvas->h, 0);

          colr = api->getpixel(snapshot, src_x, src_y);
          dest.x = linear(dx, dx2, xx, len);
          dest.y = linear(dy, dy2, xx, len);
          if (preview) {
            dest.w = 6;
            dest.h = 6;
          } else {
            dest.w = 2;
            dest.h = 2;
          }
          SDL_FillRect(canvas, &dest, colr);

          xx = xx + xxm;
          if ((xxm > 0 && xx > len) || (xxm < 0 && xx < len)) {
            done = 1;
          }
        } while (!done);
      }
    }
  }
}


void kaleidoscope_lens_release(magic_api * api, int which,
                  SDL_Surface * canvas,
                  SDL_Surface * snapshot,
                  int x, int y,
                  SDL_Rect * update_rect)
{
  kaleidoscope_lens_render(api, which, canvas, snapshot, x, y, 0);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->stopsound();
}


void kaleidoscope_lens_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                      SDL_Surface * canvas ATTRIBUTE_UNUSED,
                      SDL_Surface * last ATTRIBUTE_UNUSED,
                      Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED,
                      SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void kaleidoscope_lens_switchin(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void kaleidoscope_lens_switchout(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED,
                         int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}
