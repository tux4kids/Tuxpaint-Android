/* polyfill.c

   A Magic tool for Tux Paint that creates a filled polygon.
   by Bill Kendrick <bill@newbreedsoftware.com>
   with help from Pere Pujal Carabantes

   Scanline polygon fill routine based on public-domain code
   by Darel Rex Finley, 2007 <https://alienryderflex.com/polygon_fill/>

   Last updated: June 1, 2024
*/


#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

// #define DEBUG


enum
{
  TOOL_POLYFILL,
  NUM_TOOLS
};

char *polyfill_names[NUM_TOOLS] = {
  gettext_noop("Filled Polygon"),
};

char *polyfill_descr[NUM_TOOLS] = {
  gettext_noop
    ("Click multiple times in your picture to create a filled polygon. You may drag points to alter the shape. Drag adjacent points together to merge them. Connect the first and last points to complete the shape."),
};

char *polyfill_icon_filenames[NUM_TOOLS] = {
  "polyfill.png",
};

enum
{
  SND_PLACE,
  SND_MOVE,
  SND_REMOVE,
  SND_NEARBY,
  SND_FINISH,
  NUM_SOUNDS
};

char *polyfill_snd_filenames[NUM_SOUNDS] = {
  "polyfill_place.ogg",
  "polyfill_move.ogg",
  "polyfill_remove.ogg",
  "polyfill_nearby.ogg",
  "polyfill_finish.ogg",
};

#define SNAP_SIZE 16

#define MAX_PTS 100

SDL_Surface *polyfill_snapshot = NULL;
int polyfill_pt_x[MAX_PTS + 1];
int polyfill_pt_y[MAX_PTS + 1];
int polyfill_num_pts = 0;
int polyfill_editing = MAX_PTS;
int polyfill_dragged = 0;
int polyfill_active = 0;

Mix_Chunk *snd_effects[NUM_SOUNDS];

Uint32 polyfill_color, polyfill_color_red, polyfill_color_green;


Uint32 polyfill_api_version(void);
int polyfill_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int polyfill_get_tool_count(magic_api * api);
SDL_Surface *polyfill_get_icon(magic_api * api, int which);
char *polyfill_get_name(magic_api * api, int which);
int polyfill_get_group(magic_api * api, int which);
int polyfill_get_order(int which);
char *polyfill_get_description(magic_api * api, int which, int mode);
int polyfill_requires_colors(magic_api * api, int which);
int polyfill_modes(magic_api * api, int which);
Uint8 polyfill_accepted_sizes(magic_api * api, int which, int mode);
Uint8 polyfill_default_size(magic_api * api, int which, int mode);
void polyfill_shutdown(magic_api * api);
void
polyfill_click(magic_api * api, int which, int mode,
               SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void polyfill_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * snapshot, int old_x, int old_y, int x, int y, SDL_Rect * update_rect);
void polyfill_release(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void polyfill_set_color(magic_api * api, int which, SDL_Surface * canvas,
                        SDL_Surface * snapshot, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void polyfill_set_size(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * snapshot, Uint8 size, SDL_Rect * update_rect);
void polyfill_line_callback(void *pointer, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void polyfill_release(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void polyfill_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void polyfill_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void polyfill_draw_preview(magic_api * api, SDL_Surface * canvas, int show_handles);
void polyfill_draw_final(SDL_Surface * canvas);





Uint32 polyfill_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int polyfill_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  int i;
  char filename[1024];

  for (i = 0; i < NUM_SOUNDS; i++)
  {
    snprintf(filename, sizeof(filename), "%ssounds/magic/%s", api->data_directory, polyfill_snd_filenames[i]);
    snd_effects[i] = Mix_LoadWAV(filename);
  }

  return (1);
}


int polyfill_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return NUM_TOOLS;
}

SDL_Surface *polyfill_get_icon(magic_api *api, int which ATTRIBUTE_UNUSED)
{
  char filename[1024];

  snprintf(filename, sizeof(filename), "%simages/magic/%s", api->data_directory, polyfill_icon_filenames[which]);

  return (IMG_Load(filename));
}


char *polyfill_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  return strdup(gettext(polyfill_names[which]));
}


int polyfill_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_ARTISTIC;
}


int polyfill_get_order(int which)
{
  return 610 + which;           // FIXME
}


char *polyfill_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  return strdup(gettext(polyfill_descr[which]));
}


int polyfill_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}


int polyfill_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}


Uint8 polyfill_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 1;
}


Uint8 polyfill_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 1;
}


void polyfill_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  for (i = 0; i < NUM_SOUNDS; i++)
  {
    if (snd_effects[i] != NULL)
    {
      Mix_FreeChunk(snd_effects[i]);
    }
  }

  if (polyfill_snapshot != NULL)
  {
    SDL_FreeSurface(polyfill_snapshot);
    polyfill_snapshot = NULL;
  }
}

void
polyfill_click(magic_api *api, int which ATTRIBUTE_UNUSED,
               int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas,
               SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  int i;

  polyfill_dragged = 0;

#ifdef DEBUG
  printf("\nClick.  num_pts = %d\n", polyfill_num_pts);
#endif

  /* See if we're clicking a pre-existing point, to edit it? */
  polyfill_editing = MAX_PTS;
  for (i = 0; i < polyfill_num_pts && polyfill_editing == MAX_PTS; i++)
  {
    if (abs(x - polyfill_pt_x[i]) <= SNAP_SIZE && abs(y - polyfill_pt_y[i]) <= SNAP_SIZE)
    {
      polyfill_editing = i;
    }
  }

  if (polyfill_editing != MAX_PTS)
  {
#ifdef DEBUG
    printf("Clicked %d to edit it\n", polyfill_editing);
#endif

    polyfill_draw_preview(api, canvas, 1);

    return;
  }

  /* Trying to add a new point? */
  if (polyfill_num_pts < MAX_PTS)
  {
#ifdef DEBUG
    printf("Adding new point %d\n", polyfill_num_pts);
#endif

    polyfill_pt_x[polyfill_num_pts] = x;
    polyfill_pt_y[polyfill_num_pts] = y;
    polyfill_editing = polyfill_num_pts;
    polyfill_num_pts++;

    /* Add the new point */
    polyfill_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
    api->playsound(snd_effects[SND_PLACE], (x * 255) / canvas->w, 255);
  }
  else
  {
    /* Out of points! Move the last one to the new position */
    polyfill_pt_x[polyfill_num_pts - 1] = x;
    polyfill_pt_y[polyfill_num_pts - 1] = y;
    polyfill_editing = polyfill_num_pts - 1;

    polyfill_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
    api->playsound(snd_effects[SND_PLACE], (x * 255) / canvas->w, 255);

#ifdef DEBUG
    printf("Out of space for new points!\n");
#endif
  }
}


void
polyfill_drag(magic_api *api, int which ATTRIBUTE_UNUSED,
              SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED,
              int old_x ATTRIBUTE_UNUSED, int old_y ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  polyfill_dragged = 1;

  if (polyfill_editing >= MAX_PTS)
    return;

  polyfill_pt_x[polyfill_editing] = x;
  polyfill_pt_y[polyfill_editing] = y;

  polyfill_draw_preview(api, canvas, 1);

  if (polyfill_editing == polyfill_num_pts - 1 &&
      polyfill_num_pts >= 3 &&
      x >= polyfill_pt_x[0] - SNAP_SIZE &&
      x <= polyfill_pt_x[0] + SNAP_SIZE && y >= polyfill_pt_y[0] - SNAP_SIZE && y <= polyfill_pt_y[0] + SNAP_SIZE)
  {
    /* If placing/moving the final (red) point, and it's
       near the initial (green) point, play an electrostatic sound */
    api->playsound(snd_effects[SND_NEARBY], (x * 255) / canvas->w, 255);
  }
  else if (polyfill_editing == 0 &&
           polyfill_num_pts >= 3 &&
           x >= polyfill_pt_x[polyfill_num_pts - 1] - SNAP_SIZE &&
           x <= polyfill_pt_x[polyfill_num_pts - 1] + SNAP_SIZE &&
           y >= polyfill_pt_y[polyfill_num_pts - 1] - SNAP_SIZE && y <= polyfill_pt_y[polyfill_num_pts - 1] + SNAP_SIZE)
  {
    /* If moving the initial (green) point, and it's
       near the final (red) point, also play an electrostatic sound */
    api->playsound(snd_effects[SND_NEARBY], (x * 255) / canvas->w, 255);
  }
  else
  {
    /* Otherwise, play the normal "moving a point" sound */
    api->playsound(snd_effects[SND_MOVE], (x * 255) / canvas->w, 255);
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void polyfill_draw_preview(magic_api *api, SDL_Surface *canvas, int show_handles)
{
  int i, xx, yy;
  SDL_Rect dest;

  if (polyfill_snapshot == NULL)
    return;

  SDL_BlitSurface(polyfill_snapshot, NULL, canvas, NULL);

  if (show_handles)
  {
    for (i = 1; i < polyfill_num_pts - 1; i++)
    {
      for (yy = -4; yy <= 4; yy++)
      {
        for (xx = -4; xx <= 4; xx++)
        {
          api->xorpixel(canvas, polyfill_pt_x[i] + xx, polyfill_pt_y[i] + yy);
        }
      }
    }

    if (polyfill_num_pts > 0)
    {
      dest.x = polyfill_pt_x[0] - SNAP_SIZE;
      dest.y = polyfill_pt_y[0] - SNAP_SIZE;
      dest.w = SNAP_SIZE * 2;
      dest.h = SNAP_SIZE * 2;
      SDL_FillRect(canvas, &dest, polyfill_color_green);
    }

    if (polyfill_num_pts > 1)
    {
      dest.x = polyfill_pt_x[polyfill_num_pts - 1] - SNAP_SIZE;
      dest.y = polyfill_pt_y[polyfill_num_pts - 1] - SNAP_SIZE;
      dest.w = SNAP_SIZE * 2;
      dest.h = SNAP_SIZE * 2;
      SDL_FillRect(canvas, &dest, polyfill_color_red);
    }
  }

  for (i = 0; i < polyfill_num_pts - 1; i++)
  {
    api->line((void *)api, 0 /* which */ , canvas, NULL /* snapshot */ ,
              polyfill_pt_x[i], polyfill_pt_y[i],
              polyfill_pt_x[i + 1], polyfill_pt_y[i + 1], 1, polyfill_line_callback);
  }

  //SDL_UpdateRect(canvas, 0, 0, canvas->w, canvas->h);
}

void
polyfill_release(magic_api *api, int which ATTRIBUTE_UNUSED,
                 SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                 int x, int y, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  int i;

  if (polyfill_snapshot == NULL)
    return;

  if (polyfill_editing >= MAX_PTS)
    return;

#ifdef DEBUG
  printf("Release while editing %d\n", polyfill_editing);
#endif

  /* If they simply clicked the first point (without
     drawing to move it), and there are enough points, consider
     it a final placement of a new point! */
  if (polyfill_editing == 0 && polyfill_dragged == 0 && polyfill_num_pts > 2 && polyfill_num_pts <= MAX_PTS)
  {
#ifdef DEBUG
    printf("Clicked first point to end polygon!\n");
#endif

    polyfill_pt_x[polyfill_num_pts] = polyfill_pt_x[0];
    polyfill_pt_y[polyfill_num_pts] = polyfill_pt_y[0];
    polyfill_editing = polyfill_num_pts;
    polyfill_num_pts++;
  }

  /* Moved (or placed) the final spot at the beginning? */
  if (polyfill_num_pts > 3 &&
      ((polyfill_editing == polyfill_num_pts - 1 &&
        abs(x - polyfill_pt_x[0]) <= SNAP_SIZE &&
        abs(y - polyfill_pt_y[0]) <= SNAP_SIZE) ||
       (polyfill_editing == 0 &&
        abs(x - polyfill_pt_x[polyfill_num_pts - 1]) <= SNAP_SIZE &&
        abs(y - polyfill_pt_y[polyfill_num_pts - 1]) <= SNAP_SIZE)))
  {
#ifdef DEBUG
    printf("Ending the polygon!\n");
#endif

    /* Snap the points */
    if (polyfill_editing == 0)
    {
      polyfill_pt_x[0] = polyfill_pt_x[polyfill_num_pts - 1];
      polyfill_pt_y[0] = polyfill_pt_y[polyfill_num_pts - 1];
    }
    else
    {
      polyfill_pt_x[polyfill_num_pts - 1] = polyfill_pt_x[0];
      polyfill_pt_y[polyfill_num_pts - 1] = polyfill_pt_y[0];
    }

    polyfill_draw_final(canvas);

    /* Reset points */
    polyfill_num_pts = 0;
    polyfill_editing = MAX_PTS;

    /* Update snapshot ahead of next polygon */
    SDL_BlitSurface(canvas, NULL, polyfill_snapshot, NULL);

    /* Play "finish" sound effect */
    api->playsound(snd_effects[SND_FINISH], 128
                   /* TODO could be clever and determine midpoint of polygon */
                   , 255);

#ifdef DEBUG
    printf("Retract the undo we just took (ahead of finishing polygon)!\n");
#endif
    api->retract_undo();
    SDL_BlitSurface(canvas, NULL, snapshot, NULL);
  }
  else
  {
    /* Did not move (or place) the final spot at the beginning */

    /* Did we stick to points together? We can merge them */
    if (polyfill_num_pts > 2)
    {
      int to_merge = MAX_PTS;

      for (i = polyfill_editing - 1; i < polyfill_editing + 1; i++)
      {
        if (i >= 0 && i < polyfill_num_pts - 1)
        {
          if (abs(polyfill_pt_x[i] - polyfill_pt_x[i + 1]) <= SNAP_SIZE &&
              abs(polyfill_pt_y[i] - polyfill_pt_y[i + 1]) <= SNAP_SIZE)
          {
#ifdef DEBUG
            printf("%d & %d can be merged\n", i, i + 1);
#endif

            to_merge = i;
          }
        }
      }

      if (to_merge != MAX_PTS)
      {
#ifdef DEBUG
        printf("Merging %d with %d\n", to_merge, to_merge + 1);
#endif

        for (i = to_merge; i < polyfill_num_pts - 1; i++)
        {
          polyfill_pt_x[i] = polyfill_pt_x[i + 1];
          polyfill_pt_y[i] = polyfill_pt_y[i + 1];
        }
        polyfill_num_pts--;

        /* Play "remove" sound effect */
        api->playsound(snd_effects[SND_REMOVE], (x * 255) / canvas->w, 255);
      }
    }

    polyfill_draw_preview(api, canvas, 1);
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  /* Unless we just started a new polygon, don't let
     Tux Paint take more undo snapshots */
  if (polyfill_num_pts > 1)
  {
#ifdef DEBUG
    printf("Retract the undo we just took!\n");
#endif
    api->retract_undo();
  }
}

void polyfill_set_color(magic_api *api, int which ATTRIBUTE_UNUSED,
                        SDL_Surface *canvas,
                        SDL_Surface *snapshot ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b, SDL_Rect *update_rect)
{
  polyfill_color = SDL_MapRGB(canvas->format, r, g, b);

  if (polyfill_active)
  {
    polyfill_draw_preview(api, canvas, 1);
    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}

void polyfill_set_size(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas ATTRIBUTE_UNUSED,
                       SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                       Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
}


void polyfill_line_callback(void *pointer ATTRIBUTE_UNUSED,
                            int which ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                            SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  SDL_Rect dest;

  dest.x = x - 1;
  dest.y = y - 1;
  dest.w = 3;
  dest.h = 3;

  SDL_FillRect(canvas, &dest, polyfill_color);
}


void polyfill_switchin(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
  polyfill_color_red = SDL_MapRGB(canvas->format, 255, 0, 0);
  polyfill_color_green = SDL_MapRGB(canvas->format, 0, 255, 0);

  if (polyfill_snapshot == NULL)
  {
    polyfill_snapshot =
      SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                           canvas->format->BitsPerPixel,
                           canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);
  }

  if (polyfill_snapshot != NULL)
  {
    SDL_BlitSurface(canvas, NULL, polyfill_snapshot, NULL);
  }

  polyfill_active = 1;
}

void polyfill_switchout(magic_api *api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
  if (polyfill_num_pts > 0)
  {
#ifdef DEBUG
    printf("Retract the undo we just took (on our way out)!\n");
#endif
    api->retract_undo();
  }

  polyfill_num_pts = 0;
  polyfill_editing = MAX_PTS;
  polyfill_active = 0;

  if (polyfill_snapshot != NULL)
  {
    SDL_BlitSurface(polyfill_snapshot, NULL, canvas, NULL);
  }
}

/* Based on public-domain code by Darel Rex Finley, 2007
   https://alienryderflex.com/polygon_fill/
*/
void polyfill_draw_final(SDL_Surface *canvas)
{
  int i, j, ymin, ymax, y, nodes, swap;
  int nodeX[256];
  SDL_Rect rect;

  SDL_BlitSurface(polyfill_snapshot, NULL, canvas, NULL);

  ymin = canvas->w;
  ymax = 0;
  for (i = 0; i < polyfill_num_pts; i++)
  {
    if (polyfill_pt_y[i] < ymin)
    {
      ymin = polyfill_pt_y[i];
    }
    if (polyfill_pt_y[i] > ymax)
    {
      ymax = polyfill_pt_y[i];
    }
  }

#ifdef DEBUG
  printf("ymin %d -> ymax %d\n", ymin, ymax);
#endif

  for (y = ymin; y <= ymax; y++)
  {
    nodes = 0;
    j = polyfill_num_pts - 2;

    for (i = 0; i < polyfill_num_pts - 1; i++)
    {
      if ((polyfill_pt_y[i] < y && polyfill_pt_y[j] >= y) || (polyfill_pt_y[j] < y && polyfill_pt_y[i] >= y))
      {
        nodeX[nodes++] = (int)
          ((double)polyfill_pt_x[i] +
           (double)(y - polyfill_pt_y[i]) /
           (double)(polyfill_pt_y[j] - polyfill_pt_y[i]) * (double)(polyfill_pt_x[j] - polyfill_pt_x[i]));
      }

      j = i;
    }

    // Sort the nodes, via a simple “Bubble” sort.
    i = 0;
    while (i < nodes - 1)
    {
      if (nodeX[i] > nodeX[i + 1])
      {
        swap = nodeX[i];
        nodeX[i] = nodeX[i + 1];
        nodeX[i + 1] = swap;
        if (i)
          i--;
      }
      else
      {
        i++;
      }
    }

    // Fill the pixels between node pairs.
    for (i = 0; i < nodes; i += 2)
    {
      if (nodeX[i] >= canvas->w)
        break;

      if (nodeX[i + 1] > 0)
      {
        if (nodeX[i] < 0)
          nodeX[i] = 0;
        if (nodeX[i + 1] > canvas->w - 1)
          nodeX[i + 1] = canvas->w - 1;

        rect.x = nodeX[i];
        rect.y = y;
        rect.w = nodeX[i + 1] - nodeX[i] + 1;
        rect.h = 1;
        SDL_FillRect(canvas, &rect, polyfill_color);
      }
    }
  }
}
