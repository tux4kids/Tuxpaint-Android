/* googlyeyes.c

   Draws a googly eye at the click position, and looks
   towards where you drag+release.

  Last updated: February 12, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#define NUM_SIZES 2
int sizes[NUM_SIZES] = { 100, 50 };
char * size_names[NUM_SIZES] = {
  gettext_noop("large googly eye"),
  gettext_noop("small googly eye")
};
char * img_filenames[NUM_SIZES] = {
  "googlyeyes.png",
  "googlyeyes-sm.png"
};

Mix_Chunk *snd_effect = NULL;
SDL_Surface * googlyeyes_img_bkgd[NUM_SIZES];
SDL_Surface * googlyeyes_img_pupil[NUM_SIZES];
SDL_Surface * googlyeyes_img_reflection[NUM_SIZES];
int eye_x, eye_y;

Uint32 googlyeyes_api_version(void);
int googlyeyes_init(magic_api * api);
int googlyeyes_get_tool_count(magic_api * api);
SDL_Surface *googlyeyes_get_icon(magic_api * api, int which);
char *googlyeyes_get_name(magic_api * api, int which);
int googlyeyes_get_group(magic_api * api, int which);
char *googlyeyes_get_description(magic_api * api, int which, int mode);
int googlyeyes_requires_colors(magic_api * api, int which);
int googlyeyes_modes(magic_api * api, int which);
void googlyeyes_shutdown(magic_api * api);
void googlyeyes_click(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x,
                     int y, SDL_Rect * update_rect);
void googlyeyes_set_color(magic_api * api, int which, SDL_Surface * canvas,
                          SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void googlyeyes_drag(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * snapshot, int ox, int oy, int x, int y,
                    SDL_Rect * update_rect);
void googlyeyes_release(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int x, int y,
                       SDL_Rect * update_rect);
void googlyeyes_switchin(magic_api * api, int which, int mode,
                        SDL_Surface * canvas);
void googlyeyes_switchout(magic_api * api, int which, int mode,
                         SDL_Surface * canvas);


Uint32 googlyeyes_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int googlyeyes_init(magic_api * api)
{
  char fname[1024];
  int i;

  snprintf(fname, sizeof(fname), "%ssounds/magic/googlyeyes.ogg",
           api->data_directory);
  snd_effect = Mix_LoadWAV(fname);

  for (i = 0; i < NUM_SIZES; i++) {
    googlyeyes_img_bkgd[i] = NULL;
    googlyeyes_img_pupil[i] = NULL;
    googlyeyes_img_reflection[i] = NULL;
  }

  /* Load the base images (100% scale) */
  snprintf(fname, sizeof(fname), "%simages/magic/googly-eyes-bkgd.png",
           api->data_directory);
  googlyeyes_img_bkgd[0] = IMG_Load(fname);
  if (googlyeyes_img_bkgd[0] == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  snprintf(fname, sizeof(fname), "%simages/magic/googly-eyes-pupil.png",
           api->data_directory);
  googlyeyes_img_pupil[0] = IMG_Load(fname);
  if (googlyeyes_img_pupil[0] == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  snprintf(fname, sizeof(fname), "%simages/magic/googly-eyes-reflection.png",
           api->data_directory);
  googlyeyes_img_reflection[0] = IMG_Load(fname);
  if (googlyeyes_img_reflection[0] == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  /* Create the scaled versions */
  for (i = 1; i < NUM_SIZES; i++) {
    googlyeyes_img_bkgd[i] = api->scale(googlyeyes_img_bkgd[0],
                                        (googlyeyes_img_bkgd[0]->w * sizes[i]) / 100,
                                        (googlyeyes_img_bkgd[0]->h * sizes[i]) / 100,
                                        1);

    if (googlyeyes_img_bkgd[i] == NULL) {
      fprintf(stderr, "Cannot scale bkgd to %d%%", sizes[i]);
      return(1);
    }

    googlyeyes_img_pupil[i] = api->scale(googlyeyes_img_pupil[0],
                                        (googlyeyes_img_pupil[0]->w * sizes[i]) / 100,
                                        (googlyeyes_img_pupil[0]->h * sizes[i]) / 100,
                                        1);

    if (googlyeyes_img_pupil[i] == NULL) {
      fprintf(stderr, "Cannot scale pupil to %d%%", sizes[i]);
      return(1);
    }

    googlyeyes_img_reflection[i] = api->scale(googlyeyes_img_reflection[0],
                                        (googlyeyes_img_reflection[0]->w * sizes[i]) / 100,
                                        (googlyeyes_img_reflection[0]->h * sizes[i]) / 100,
                                        1);

    if (googlyeyes_img_reflection[i] == NULL) {
      fprintf(stderr, "Cannot scale reflection to %d%%", sizes[i]);
      return(1);
    }
  }

  return (1);
}

int googlyeyes_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (NUM_SIZES);
}


SDL_Surface *googlyeyes_get_icon(magic_api * api, int which)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/%s",
           api->data_directory, img_filenames[which]);

  return (IMG_Load(fname));
}

char *googlyeyes_get_name(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext("Google Eyes"));
}

int googlyeyes_get_group(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_ARTISTIC;
}

char *googlyeyes_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                int which ATTRIBUTE_UNUSED,
                                int mode ATTRIBUTE_UNUSED)
{
  char tmp[512];
  sprintf(tmp, gettext("Click to place a %s, then drag and release to make it look that direction."), gettext(size_names[which]));

  return strdup(tmp);
}

int googlyeyes_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                              int which ATTRIBUTE_UNUSED)
{
  return 0;
}

int googlyeyes_modes(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}

void googlyeyes_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  int i;

  if (snd_effect != NULL)
    Mix_FreeChunk(snd_effect);

  for (i = 0; i < NUM_SIZES; i++) {
    if (googlyeyes_img_bkgd[i] != NULL)
      SDL_FreeSurface(googlyeyes_img_bkgd[i]);

    if (googlyeyes_img_pupil[i] != NULL)
      SDL_FreeSurface(googlyeyes_img_pupil[i]);

    if (googlyeyes_img_reflection[i] != NULL)
      SDL_FreeSurface(googlyeyes_img_reflection[i]);
  }
}


void
googlyeyes_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect)
{
  eye_x = x;
  eye_y = y;

  if (eye_x < googlyeyes_img_bkgd[which]->w / 2)
    eye_x = googlyeyes_img_bkgd[which]->w / 2;
  if (eye_y < googlyeyes_img_bkgd[which]->h / 2)
    eye_y = googlyeyes_img_bkgd[which]->h / 2;

  api->stopsound();
  api->playsound(snd_effect, (x * 255) / canvas->w, 255);

  googlyeyes_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


void
googlyeyes_drag(magic_api * api ATTRIBUTE_UNUSED, int which, SDL_Surface * canvas,
               SDL_Surface * snapshot, int ox ATTRIBUTE_UNUSED,
               int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  SDL_Rect dest;
  int max_radius;

  /* Set the destination for the main background */
  update_rect->x = eye_x - googlyeyes_img_bkgd[which]->w / 2;
  update_rect->y = eye_y - googlyeyes_img_bkgd[which]->h / 2;
  update_rect->w = googlyeyes_img_bkgd[which]->w;
  update_rect->h = googlyeyes_img_bkgd[which]->h;

  /* Erase the eye (drop snapshot pixels back into canvas) */
  SDL_BlitSurface(snapshot, update_rect, canvas, update_rect);

  /* 1. Draw the background */
  SDL_BlitSurface(googlyeyes_img_bkgd[which], NULL, canvas, update_rect);

  /* 2. Draw the pupil */
  max_radius = ((googlyeyes_img_bkgd[which]->w - googlyeyes_img_pupil[which]->w) / 2);
  if (sqrt(((x - eye_x) * (x - eye_x)) + ((y - eye_y) * (y - eye_y))) > max_radius) {
    /* If drag position would place pupil outside the circular bounds
     * of the background, put it on the edge, "looking towards" (pointing at)
     * the mouse. */
    /* (Calculations borrowed from Xeyes' "computePupil()",
     * https://github.com/freedesktop/xorg-xeyes/blob/master/Eyes.c) */
    double dx, dy, angle;

    dx = (double) (x - eye_x);
    dy = (double) (y - eye_y);
    angle = atan2(dy, dx);

    x = eye_x + (cos(angle) * max_radius);
    y = eye_y + (sin(angle) * max_radius);
  }

  dest.x = x - googlyeyes_img_pupil[which]->w / 2;
  dest.y = y - googlyeyes_img_pupil[which]->h / 2;
  dest.w = googlyeyes_img_pupil[which]->w;
  dest.h = googlyeyes_img_pupil[which]->h;
  SDL_BlitSurface(googlyeyes_img_pupil[which], NULL, canvas, &dest);

  /* 3. Draw the reflection */
  dest.x = eye_x - googlyeyes_img_reflection[which]->w / 2;
  dest.y = eye_y - googlyeyes_img_reflection[which]->h / 2;
  dest.w = googlyeyes_img_reflection[which]->w;
  dest.h = googlyeyes_img_reflection[which]->h;
  SDL_BlitSurface(googlyeyes_img_reflection[which], NULL, canvas, &dest);
}


void
googlyeyes_release(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
                  SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void googlyeyes_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
                          SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
}


void googlyeyes_switchin(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void googlyeyes_switchout(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED,
                         int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}
