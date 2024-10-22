/* n_pt_persp.c

   1-, 2-, and 3-point perspective, axonometric (isometric, dimetric,
   and trimetric), and oblique line-drawing tools,

   Different complexity (expertise) levels offer different
   tools.  In Advanced mode, there are "Draw" and "Select"
   (vanishing point editing) tools.  In Beginner mode,
   there are only "Draw" tools; the user is forced to use our
   default vanishing points (a second "3-point" draw tool is
   provided with an alternative vanishing point).  And in
   Novice mode, this plugin offers NO tools.

   TODO - See if we can get `snap_to()` working for
   axon. & oblique drawing modes.

   by Bill Kendrick <bill@newbreedsoftware.com>
   with help from Pere Pujal Carabantes

   December 12, 2023 - January 20, 2024
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

// #define DEBUG
// #define PERF

#define SNAP 10

/* All _possible_ tools */
enum
{
  /* 1-point perspective */
  TOOL_1PT_SELECT,              /* advanced only */
  TOOL_1PT_DRAW,                /* advanced & beginner */

  /* 2-point perspective */
  TOOL_2PT_SELECT,              /* advanced only */
  TOOL_2PT_DRAW,                /* advanced & beginner */

  /* 3-point perspective */
  TOOL_3PT_SELECT,              /* advanced only */
  TOOL_3PT_DRAW,                /* advanced & beginner */
  TOOL_3PT_SELECT_ALT,          /* beginner only (not directly accessible; used for drawing guides) */
  TOOL_3PT_DRAW_ALT,            /* beginner only */

  /* Isometric */
  TOOL_ISO_SELECT,              /* (not directly accessible; used for drawing guides) */
  TOOL_ISO_DRAW,                /* advanced & beginner (N.B. isometric defined by exact angles; no "SELECT" tool) */

  /* Dimetric */
  TOOL_DIM_SELECT,              /* advanced only */
  TOOL_DIM_DRAW,                /* advanced & beginner */

  /* Trimetric */
  TOOL_TRI_SELECT,              /* advanced only */
  TOOL_TRI_DRAW,                /* advanced & beginner */

  /* Oblique */
  TOOL_OBLQ_SELECT,             /* advanced only */
  TOOL_OBLQ_DRAW,               /* advanced & beginner */
  TOOL_OBLQ_SELECT_ALT,         /* beginner only (not directly accessible; used for drawing guides) */
  TOOL_OBLQ_DRAW_ALT,           /* beginner only */

  NUM_TOOLS
};


#ifdef DEBUG
char *tool_debug_names[NUM_TOOLS] = {
  /* 1-point perspective */
  "1pt select",
  "1pt draw",

  /* 2-point perspective */
  "2pt select",
  "2pt draw",

  /* 3-point perspective */
  "3pt select",
  "3pt draw",
  "3pt select alt",
  "3pt draw alt",

  /* Isometric */
  "iso select",
  "iso draw",

  /* Dimetric */
  "dim select",
  "dim draw",

  /* Trimetric */
  "tri select",
  "tri draw",

  /* Oblique */
  "oblq select alt",
  "oblq draw alt",
  "oblq select",
  "oblq draw",
};
#endif

Uint8 complexity;

int num_tools[NUM_MAGIC_COMPLEXITY_LEVELS] = {
  0,                            /* Novice */
  9,                            /* Beginner */
  13,                           /* Advanced */
};

int *which_to_tool;

int which_to_tool_per_complexity[NUM_MAGIC_COMPLEXITY_LEVELS][NUM_TOOLS] = {
  /* Novice */
  {
   -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1,
   },
  /* Beginner */
  {
   TOOL_1PT_DRAW,
   TOOL_2PT_DRAW,
   TOOL_3PT_DRAW,
   TOOL_3PT_DRAW_ALT,
   TOOL_ISO_DRAW,
   TOOL_DIM_DRAW,
   TOOL_TRI_DRAW,
   TOOL_OBLQ_DRAW,
   TOOL_OBLQ_DRAW_ALT,
   -1, -1, -1, -1, -1, -1, -1, -1, -1,
   },
  /* Advanced */
  {
   TOOL_1PT_SELECT,
   TOOL_1PT_DRAW,
   TOOL_2PT_SELECT,
   TOOL_2PT_DRAW,
   TOOL_3PT_SELECT,
   TOOL_3PT_DRAW,
   TOOL_ISO_DRAW,
   TOOL_DIM_SELECT,
   TOOL_DIM_DRAW,
   TOOL_TRI_SELECT,
   TOOL_TRI_DRAW,
   TOOL_OBLQ_SELECT,
   TOOL_OBLQ_DRAW,
   -1, -1, -1, -1, -1,
   },
};

const char *icon_filenames[NUM_TOOLS] = {
  /* 1-point perspective */
  "1pt_persp_select.png",
  "1pt_persp_draw.png",

  /* 2-point perspective */
  "2pt_persp_select.png",
  "2pt_persp_draw.png",

  /* 3-point perspective */
  "3pt_persp_select.png",
  "3pt_persp_draw.png",
  "",
  "3pt_persp_draw_alt.png",

  /* Isometric */
  "",
  "isometric_draw.png",

  /* Dimetric */
  "dimetric_select.png",
  "dimetric_draw.png",

  /* Trimetric */
  "trimetric_select.png",
  "trimetric_draw.png",

  /* Oblique */
  "oblique_select.png",
  "oblique_draw.png",
  "",
  "oblique_draw_alt.png",
};


const char *tool_names[NUM_TOOLS] = {
  /* 1-point perspective */
  gettext_noop("1-Point Select"),
  gettext_noop("1-Point Draw"),

  /* 2-point perspective */
  gettext_noop("2-Point Select"),
  gettext_noop("2-Point Draw"),

  /* 3-point perspective */
  gettext_noop("3-Point Select"),
  gettext_noop("3-Point Draw"),
  "",
  gettext_noop("3-Point Draw Down"),

  /* Isometric */
  "",
  gettext_noop("Isometric Lines"),

  /* Dimetric */
  gettext_noop("Dimetric Select"),
  gettext_noop("Dimetric Draw"),

  /* Trimetric */
  gettext_noop("Trimetric Select"),
  gettext_noop("Trimetric Draw"),

  /* Oblique */
  gettext_noop("Oblique Select"),
  gettext_noop("Oblique Draw"),
  "",
  gettext_noop("Oblique Draw Left"),
};


const char *tool_descriptions[NUM_TOOLS] = {
  /* 1-point perspective */
  gettext_noop("Click in your drawing to pick a vanishing point for the 1-point perspective painting tool."),
  gettext_noop("Click and drag to draw lines with your 1-point perspective vanishing point."),

  /* 2-point perspective */
  gettext_noop("Click two places in your drawing to pick vanishing points for the 2-point perspective painting tool."),
  gettext_noop("Click and drag to draw lines with your 2-point perspective vanishing points."),

  /* 3-point perspective */
  gettext_noop
    ("Click three places in your drawing to pick vanishing points for the 3-point perspective painting tool."),
  gettext_noop("Click and drag to draw lines with your 3-point perspective vanishing points."),
  "",
  gettext_noop("Click and drag to draw lines with your 3-point perspective vanishing points (downward perspective)."),

  /* Isometric */
  "",
  gettext_noop("Click and drag to draw lines with an isometric projection."),

  /* Dimetric */
  gettext_noop("Click in your drawing to adjust the angle used by the dimetric projection painting tool."),
  gettext_noop("Click and drag to draw lines with dimetric projection."),

  /* Trimetric */
  gettext_noop("Click in your drawing to adjust the angles used by the trimetric projection painting tool."),
  gettext_noop("Click and drag to draw lines with trimetric projection."),

  /* Oblique */
  gettext_noop("Click in your drawing to adjust the angle used by the oblique projection painting tool."),
  gettext_noop("Click and drag to draw lines with oblique projection."),
  "",
  gettext_noop("Click and drag to draw lines with oblique projection (right-facing)."),
};


/* Sound effects (same for everyone) */
enum
{
  SND_SELECT,
  SND_DRAW_CLICK,
  SND_DRAW_RELEASE,
  NUM_SNDS
};

Mix_Chunk *sound_effects[NUM_SNDS];

const char *sound_filenames[NUM_SNDS] = {
  "n_pt_persp_select.ogg",
  "n_pt_persp_click.ogg",
  "n_pt_persp_release.ogg",
};

Uint8 n_pt_persp_r, n_pt_persp_g, n_pt_persp_b;
Uint8 n_pt_persp_size = 1;
SDL_Surface *n_pt_persp_snapshot = NULL;

int a1_pt_x, a1_pt_y;
int a2_pt_x[2], a2_pt_y[2], a2_pt_cur;
int a3_pt_x[3], a3_pt_y[3], a3_pt_cur;
int a3b_pt_x[3], a3b_pt_y[3];
float dim_ang;
int tri_ang_chosen;
float tri_ang[2];
float oblq_ang;
float oblqb_ang;

#define MIN_AXONOMETRIC_ANGLE (15.0 * M_PI / 180.0)
#define MAX_AXONOMETRIC_ANGLE (75.0 * M_PI / 180.0)

#define MIN_OBLIQUE_ANGLE (30.0 * M_PI / 180.0)
#define MAX_OBLIQUE_ANGLE (60.0 * M_PI / 180.0)

int line_start_x, line_start_y;
float a2_valid_angle[8];
float a3_valid_angle[8];

/* Function prototypes: */
Uint32 n_pt_persp_api_version(void);
int n_pt_persp_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int n_pt_persp_get_tool_count(magic_api * api);
SDL_Surface *n_pt_persp_get_icon(magic_api * api, int which);
char *n_pt_persp_get_name(magic_api * api, int which);
int n_pt_persp_get_group(magic_api * api, int which);
int n_pt_persp_get_order(int which);
char *n_pt_persp_get_description(magic_api * api, int which, int mode);
int n_pt_persp_requires_colors(magic_api * api, int which);
int n_pt_persp_modes(magic_api * api, int which);
Uint8 n_pt_persp_accepted_sizes(magic_api * api, int which, int mode);
Uint8 n_pt_persp_default_size(magic_api * api, int which, int mode);
void n_pt_persp_shutdown(magic_api * api);
void n_pt_persp_click(magic_api * api, int which, int mode,
                      SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void n_pt_persp_vanish_pt_moved(magic_api * api, int which, SDL_Surface * canvas, SDL_Rect * update_rect);
void n_pt_persp_drag(magic_api * api, int which,
                     SDL_Surface * canvas, SDL_Surface * snapshot,
                     int old_x, int old_y, int x, int y, SDL_Rect * update_rect);
void n_pt_persp_work(magic_api * api, int tool, SDL_Surface * canvas, int x, int y, SDL_Rect * update_rect, int xor);
void n_pt_persp_release(magic_api * api, int which,
                        SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void n_pt_persp_set_color(magic_api * api, int which, SDL_Surface * canvas,
                          SDL_Surface * snapshot, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void n_pt_persp_set_size(magic_api * api, int which, int mode,
                         SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);
void n_pt_persp_line_xor_callback(void *pointer, int tool, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void n_pt_persp_line_xor_thick_callback(void *pointer, int tool, SDL_Surface * canvas,
                                        SDL_Surface * snapshot, int x, int y);
void n_pt_persp_line_callback(void *pointer, int tool, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void n_pt_persp_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void n_pt_persp_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void n_pt_persp_draw_points(magic_api * api, int tool, SDL_Surface * canvas);
void n_pt_persp_draw_one_point(magic_api * api, SDL_Surface * canvas, int x, int y, int i);

#ifdef SNAP_TO
void snap_to(int which, int *x, int *y);
#endif


Uint32 n_pt_persp_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


int n_pt_persp_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level)
{
  int i;
  char filename[1024];

  for (i = 0; i < NUM_SNDS; i++)
  {
    sound_effects[i] = NULL;
  }

  complexity = complexity_level;
  which_to_tool = which_to_tool_per_complexity[complexity_level];

#ifdef DEBUG
  printf("complexity level %d; tool mapping:\n", complexity);
  fflush(stdout);
  for (i = 0; i < NUM_TOOLS; i++)
  {
    printf("%d -> %d ", i, which_to_tool[i]);
    fflush(stdout);
    if (which_to_tool[i] != -1)
    {
      printf("%s", tool_debug_names[which_to_tool[i]]);
    }
    printf("\n");
    fflush(stdout);
  }
#endif


  if (complexity_level == MAGIC_COMPLEXITY_NOVICE)
  {
    /* No N-point perspective tools _at all_, if in Novice mode */
#ifdef DEBUG
    printf("n_pt_persp_init -- MAGIC_COMPLEXITY_NOVICE so no tools for you!\n");
    fflush(stdout);
#endif
    return 0;
  }


  for (i = 0; i < NUM_SNDS; i++)
  {
    snprintf(filename, sizeof(filename), "%ssounds/magic/%s", api->data_directory, sound_filenames[i]);
    sound_effects[i] = Mix_LoadWAV(filename);
  }

  /* Set default vanishing point positions: */

  /* 1-pt perspective initial vanishing point: Center of canvas */
  a1_pt_x = api->canvas_w / 2;
  a1_pt_y = api->canvas_w / 2;

  /* 2-pt perspective initial vanishing points: Left and right, midway up/down the canvas */
  a2_pt_x[0] = 0;
  a2_pt_y[0] = api->canvas_h / 2;

  a2_pt_x[1] = api->canvas_w - 1;
  a2_pt_y[1] = api->canvas_h / 2;

  a2_pt_cur = 0;

  /* 3-pt perspective initial vanishing points: top center, and left and right near bottom of canvas */
  a3_pt_x[0] = api->canvas_w * 1 / 20;
  a3_pt_y[0] = api->canvas_h * 19 / 20;

  a3_pt_x[1] = api->canvas_w * 19 / 20;
  a3_pt_y[1] = api->canvas_h * 19 / 20;

  a3_pt_x[2] = api->canvas_w / 2;
  a3_pt_y[2] = api->canvas_h * 1 / 20;

  a3_pt_cur = 0;

  /* 3-pt perspective alternative initial vanishing points: bottom center, and left and right near top of canvas */
  a3b_pt_x[0] = api->canvas_w * 1 / 20;
  a3b_pt_y[0] = api->canvas_h * 1 / 20;

  a3b_pt_x[1] = api->canvas_w * 19 / 20;
  a3b_pt_y[1] = api->canvas_h * 1 / 20;

  a3b_pt_x[2] = api->canvas_w / 2;
  a3b_pt_y[2] = api->canvas_h * 19 / 20;


  /* Set default angles: */
  dim_ang = 45.0 * M_PI / 180.0;

  tri_ang[0] = 30 * M_PI / 180.0;
  tri_ang[1] = 165 * M_PI / 180.0;
  tri_ang_chosen = 0;

  oblq_ang = 45 * M_PI / 180.0;
  oblqb_ang = -45 * M_PI / 180.0;


  /* Generate our own snapshot surface */

  n_pt_persp_snapshot = SDL_CreateRGBSurface(SDL_SWSURFACE, api->canvas_w, api->canvas_h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);  // FIXME: Safe?
  if (n_pt_persp_snapshot == NULL)
  {
    fprintf(stderr, "n_pt_persp -- Could not create a 32-bit surface of size %d x %d!\n", api->canvas_w, api->canvas_h);
    return (0);
  }

  return (1);
}


int n_pt_persp_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (num_tools[complexity]);
}


SDL_Surface *n_pt_persp_get_icon(magic_api *api, int which)
{
  char filename[1024];

#ifdef DEBUG
  printf("\nn_pt_persp_get_icon\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

  snprintf(filename, sizeof(filename), "%simages/magic/%s", api->data_directory, icon_filenames[which_to_tool[which]]);

  return (IMG_Load(filename));
}


char *n_pt_persp_get_name(magic_api *api ATTRIBUTE_UNUSED, int which)
{
#ifdef DEBUG
  printf("\nn_pt_persp_get_name\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif
  return (strdup(gettext(tool_names[which_to_tool[which]])));
}


int n_pt_persp_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return (MAGIC_TYPE_PROJECTIONS);
}

int n_pt_persp_get_order(int which)
{
  /* Use the order they appear in the TOOL... enum */
  return which;
}


char *n_pt_persp_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
#ifdef DEBUG
  printf("\nn_pt_persp_get_description\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif
  return (strdup(gettext(tool_descriptions[which_to_tool[which]])));
}


int n_pt_persp_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which)
{
#ifdef DEBUG
  printf("\nn_pt_persp_requires_colors\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

  which = which_to_tool[which];

  if (which == TOOL_1PT_DRAW || which == TOOL_2PT_DRAW || which == TOOL_3PT_DRAW || which == TOOL_3PT_DRAW_ALT ||
      which == TOOL_ISO_DRAW || which == TOOL_DIM_DRAW || which == TOOL_TRI_DRAW ||
      which == TOOL_OBLQ_DRAW || which == TOOL_OBLQ_DRAW_ALT)
    return 1;
  else
    return 0;
}


int n_pt_persp_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}

Uint8 n_pt_persp_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
#ifdef DEBUG
  printf("\nn_pt_persp_accepted_sizes\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

  which = which_to_tool[which];

  if (which == TOOL_1PT_DRAW || which == TOOL_2PT_DRAW || which == TOOL_3PT_DRAW || which == TOOL_3PT_DRAW_ALT ||
      which == TOOL_ISO_DRAW || which == TOOL_DIM_DRAW || which == TOOL_TRI_DRAW ||
      which == TOOL_OBLQ_DRAW || which == TOOL_OBLQ_DRAW_ALT)
  {
    return 4;
  }
  else
  {
    return 0;
  }
}


Uint8 n_pt_persp_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 1;
}


void n_pt_persp_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i;

  if (n_pt_persp_snapshot != NULL)
  {
    SDL_FreeSurface(n_pt_persp_snapshot);
  }

  for (i = 0; i < NUM_SNDS; i++)
  {
    if (sound_effects[i] != NULL)
    {
      Mix_FreeChunk(sound_effects[i]);
    }
  }
}


void n_pt_persp_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  int pick, i, tool;
  float dist, min_dist;

#ifdef DEBUG
  printf("\nn_pt_persp_click\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

#ifdef SNAP_TO
  snap_to(which, &x, &y);
#endif

  tool = which_to_tool[which];

  pick = 0;
  min_dist = FLT_MAX;

  if (tool == TOOL_1PT_SELECT)
  {
    /* Set position of 1-point perspective */
    a1_pt_x = x;
    a1_pt_y = y;

    n_pt_persp_vanish_pt_moved(api, tool, canvas, update_rect);
  }
  else if (tool == TOOL_2PT_SELECT)
  {
    /* Pick closest 2-point perspective & move it */

    for (i = 0; i < 2; i++)
    {
      dist = sqrt(pow(a2_pt_x[i] - x, 2) + pow(a2_pt_y[i] - y, 2));
      if (dist < min_dist)
      {
        pick = i;
        min_dist = dist;
      }
    }

    a2_pt_cur = pick;

    a2_pt_x[a2_pt_cur] = x;
    a2_pt_y[a2_pt_cur] = y;

    n_pt_persp_vanish_pt_moved(api, tool, canvas, update_rect);
  }
  else if (tool == TOOL_3PT_SELECT)
  {
    /* Pick closest 3-point perspective & move it */
    for (i = 0; i < 3; i++)
    {
      dist = sqrt(pow(a3_pt_x[i] - x, 2) + pow(a3_pt_y[i] - y, 2));
      if (dist < min_dist)
      {
        pick = i;
        min_dist = dist;
      }
    }

    a3_pt_cur = pick;

    a3_pt_x[a3_pt_cur] = x;
    a3_pt_y[a3_pt_cur] = y;

    n_pt_persp_vanish_pt_moved(api, tool, canvas, update_rect);
  }
  else if (tool == TOOL_DIM_SELECT || tool == TOOL_TRI_SELECT || tool == TOOL_OBLQ_SELECT)
  {
    /* The call to _drag() below will set angle(s) for Dimetric, Trimetric, and Oblique */
    if (tool == TOOL_TRI_SELECT)
    {
      if (x < canvas->w / 2)
      {
        if (y < canvas->h / 2)
        {
          /* top left */
          tri_ang_chosen = 1;
        }
        else
        {
          /* bottom left */
          tri_ang_chosen = 0;
        }
      }
      else
      {
        if (y < canvas->h / 2)
        {
          /* top right */
          tri_ang_chosen = 0;
        }
        else
        {
          /* bottom right */
          tri_ang_chosen = 1;
        }
      }
    }
    n_pt_persp_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
  }
  else
  {
    /* Not a SELECT; must be a DRAW! */
    int i;

    api->playsound(sound_effects[SND_DRAW_CLICK], (x * 255) / canvas->w, 255);

    /* Start drawing a line */
    SDL_BlitSurface(canvas, NULL, n_pt_persp_snapshot, NULL);

    if (tool == TOOL_2PT_DRAW)
    {
      /* Horizon between vanishing points, and perpendicular (rise above/below) */
      a2_valid_angle[0] = atan2f(a2_pt_y[1] - a2_pt_y[0], a2_pt_x[1] - a2_pt_x[0]);
      a2_valid_angle[1] = a2_valid_angle[0] + M_PI;
      a2_valid_angle[2] = a2_valid_angle[0] + (M_PI / 2.0);
      a2_valid_angle[3] = a2_valid_angle[0] + (M_PI / 2.0) + M_PI;

      /* Angles that point toward the two vanishing points */
      if (x == a2_pt_x[0])
      {
        x++;
      }
      if (x == a2_pt_x[1])
      {
        x++;
      }
      a2_valid_angle[4] = atan2f(a2_pt_y[0] - y, a2_pt_x[0] - x);
      a2_valid_angle[5] = a2_valid_angle[4] + M_PI;
      a2_valid_angle[6] = atan2f(a2_pt_y[1] - y, a2_pt_x[1] - x);
      a2_valid_angle[7] = a2_valid_angle[6] + M_PI;

      for (i = 0; i < 8; i++)
      {
        if (a2_valid_angle[i] > M_PI)
        {
          a2_valid_angle[i] -= (M_PI * 2);
        }
      }
    }
    else if (tool == TOOL_3PT_DRAW)
    {
      /* Horizon between vanishing points, and perpendicular (rise above/below) */
      a3_valid_angle[0] = atan2f(a3_pt_y[1] - a3_pt_y[0], a3_pt_x[1] - a3_pt_x[0]);
      a3_valid_angle[1] = a3_valid_angle[0] + M_PI;

      /* Angles that point toward the three vanishing points */
      a3_valid_angle[2] = atan2f(a3_pt_y[0] - y, a3_pt_x[0] - x);
      a3_valid_angle[3] = a3_valid_angle[2] + M_PI;
      a3_valid_angle[4] = atan2f(a3_pt_y[1] - y, a3_pt_x[1] - x);
      a3_valid_angle[5] = a3_valid_angle[4] + M_PI;
      a3_valid_angle[6] = atan2f(a3_pt_y[2] - y, a3_pt_x[2] - x);
      a3_valid_angle[7] = a3_valid_angle[6] + M_PI;

      for (i = 0; i < 8; i++)
      {
        if (a3_valid_angle[i] > M_PI)
        {
          a3_valid_angle[i] -= (M_PI * 2);
        }
      }
    }
    else if (tool == TOOL_3PT_DRAW_ALT)
    {
      /* Horizon between vanishing points, and perpendicular (rise above/below) */
      a3_valid_angle[0] = atan2f(a3b_pt_y[1] - a3b_pt_y[0], a3b_pt_x[1] - a3b_pt_x[0]);
      a3_valid_angle[1] = a3_valid_angle[0] + M_PI;

      /* Angles that point toward the three vanishing points */
      a3_valid_angle[2] = atan2f(a3b_pt_y[0] - y, a3b_pt_x[0] - x);
      a3_valid_angle[3] = a3_valid_angle[2] + M_PI;
      a3_valid_angle[4] = atan2f(a3b_pt_y[1] - y, a3b_pt_x[1] - x);
      a3_valid_angle[5] = a3_valid_angle[4] + M_PI;
      a3_valid_angle[6] = atan2f(a3b_pt_y[2] - y, a3b_pt_x[2] - x);
      a3_valid_angle[7] = a3_valid_angle[6] + M_PI;

      for (i = 0; i < 8; i++)
      {
        if (a3_valid_angle[i] > M_PI)
        {
          a3_valid_angle[i] -= (M_PI * 2);
        }
      }
    }
    /* N.B. For Isometric, Dimetric, Trimetric, and Oblique,
     * angles are always the same, regardless of the line's
     * position (unlike the perspective tools, where the angles
     * are related to the drawing position & the vanishing point(s)
     */

    line_start_x = x;
    line_start_y = y;
    n_pt_persp_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
  }
}


void n_pt_persp_vanish_pt_moved(magic_api *api, int which, SDL_Surface *canvas, SDL_Rect *update_rect)
{
  SDL_BlitSurface(n_pt_persp_snapshot, NULL, canvas, NULL);
  n_pt_persp_draw_points(api, which, canvas);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  api->playsound(sound_effects[SND_SELECT], 128, 255);
}


/* Affect the canvas on drag: */
void n_pt_persp_drag(magic_api *api, int which,
                     SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                     int old_x ATTRIBUTE_UNUSED, int old_y ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  int i, x1, y1, x2, y2;
  float slope;

#ifdef PERF
  Uint64 tick;

  tick = SDL_GetPerformanceCounter();
#endif

#ifdef SNAP_TO
  snap_to(which, &old_x, &old_y);
  snap_to(which, &x, &y);
#endif

#ifdef DEBUG
  printf("\nn_pt_persp_drag\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

  which = which_to_tool[which];

  /* Draw the line (preview) */
  n_pt_persp_work(api, which /* the tool */ , canvas, x, y, update_rect, 1);

  /* Show some guides */
  if (which == TOOL_1PT_DRAW)
  {
    /* 1-point perspective - draw */

    /* Horizontal line (horizon) */
    if (y != a1_pt_y)
    {
      api->line((void *)api, which, canvas, NULL, 0, a1_pt_y, canvas->w - 1, a1_pt_y, 12, n_pt_persp_line_xor_callback);
    }

    /* Horizontal line (from cursor) */
    api->line((void *)api, which, canvas, NULL, 0, y, canvas->w - 1, y, 6, n_pt_persp_line_xor_callback);

    /* Vertical line */
    api->line((void *)api, which, canvas, NULL, x, 0, x, canvas->h - 1, 6, n_pt_persp_line_xor_callback);

    /* Diagonal line to the vanishing point */
    api->line((void *)api, which, canvas, NULL, x, y, a1_pt_x, a1_pt_y, 12, n_pt_persp_line_xor_callback);
  }
  else if (which == TOOL_2PT_DRAW)
  {
    /* 2-point perspective - draw */
    n_pt_persp_draw_points(api, TOOL_2PT_SELECT, canvas);

    slope = ((float)a2_pt_y[0] - (float)a2_pt_y[1]) / ((float)a2_pt_x[0] - (float)a2_pt_x[1]);

    /* Horizon line (from the cursor) */
    x1 = 0;
    y1 = y - (x * slope);
    x2 = canvas->w;
    y2 = y + ((canvas->w - x) * slope);
    api->line((void *)api, which, canvas, NULL, x1, y1, x2, y2, 5, n_pt_persp_line_xor_callback);

    /* Perpendicular-to-horizon line (from the cursor) */
    if (slope == 0.0 || slope == M_PI)
    {
      x1 = x;
      y1 = 0;
      x2 = x;
      y2 = canvas->h;
    }
    else
    {
      float perp_slope = -(slope);

      x1 = x - (y * perp_slope);
      y1 = 0;
      x2 = x + ((canvas->h - y) * perp_slope);
      y2 = canvas->h;
    }

    api->line((void *)api, which, canvas, NULL, x1, y1, x2, y2, 5, n_pt_persp_line_xor_callback);

    /* Diagonal lines from cursor to the vanishing points */
    for (i = 0; i < 2; i++)
    {
      if (x != a2_pt_x[i])
      {
        slope = ((float)y - (float)a2_pt_y[i]) / ((float)x - (float)a2_pt_x[i]);

        x1 = 0;
        y1 = a2_pt_y[i] - (a2_pt_x[i] * slope);
        x2 = canvas->w;
        y2 = a2_pt_y[i] + ((canvas->w - a2_pt_x[i]) * slope);

        api->line((void *)api, which, canvas, NULL, x1, y1, x2, y2, 2, n_pt_persp_line_xor_callback);
      }
    }
  }
  else if (which == TOOL_3PT_DRAW || which == TOOL_3PT_DRAW_ALT)
  {
    /* 3-point perspective - draw */

    int a3_x[3], a3_y[3];

    if (which == TOOL_3PT_DRAW)
    {
      a3_x[0] = a3_pt_x[0];
      a3_y[0] = a3_pt_y[0];
      a3_x[1] = a3_pt_x[1];
      a3_y[1] = a3_pt_y[1];
      a3_x[2] = a3_pt_x[2];
      a3_y[2] = a3_pt_y[2];
      n_pt_persp_draw_points(api, TOOL_3PT_SELECT, canvas);
    }
    else
    {
      a3_x[0] = a3b_pt_x[0];
      a3_y[0] = a3b_pt_y[0];
      a3_x[1] = a3b_pt_x[1];
      a3_y[1] = a3b_pt_y[1];
      a3_x[2] = a3b_pt_x[2];
      a3_y[2] = a3b_pt_y[2];
      n_pt_persp_draw_points(api, TOOL_3PT_SELECT_ALT, canvas);
    }

    /* Horizon line (from the cursor) */
    slope = ((float)a3_y[0] - (float)a3_y[1]) / ((float)a3_x[0] - (float)a3_x[1]);
    x1 = 0;
    y1 = y - (x * slope);
    x2 = canvas->w;
    y2 = y + ((canvas->w - x) * slope);
    api->line((void *)api, which, canvas, NULL, x1, y1, x2, y2, 5, n_pt_persp_line_xor_callback);

    /* N.B. No "vertical" line; the 3rd vanishing point defines "up" and "down" */

    /* Diagonal lines from cursor to the vanishing points */
    for (i = 0; i < 3; i++)
    {
      if (x != a3_x[i])
      {
        slope = ((float)y - (float)a3_y[i]) / ((float)x - (float)a3_x[i]);

        x1 = 0;
        y1 = a3_y[i] - (a3_x[i] * slope);
        x2 = canvas->w;
        y2 = a3_y[i] + ((canvas->w - a3_x[i]) * slope);

        api->line((void *)api, which, canvas, NULL, x1, y1, x2, y2, 2, n_pt_persp_line_xor_callback);
      }
    }
  }
  else if (which == TOOL_ISO_DRAW || which == TOOL_DIM_DRAW || which == TOOL_TRI_DRAW ||
           which == TOOL_OBLQ_DRAW || which == TOOL_OBLQ_DRAW_ALT)
  {
    int guide_len;

    guide_len = max(canvas->w, canvas->h);

    /* All of these tools have a vertical guide */
    api->line((void *)api, which, canvas, NULL, x, 0, x, canvas->h - 1, 6, n_pt_persp_line_xor_callback);

    /* Isometric, Dimetric, Trimetric, or Oblique draw */
    if (which == TOOL_ISO_DRAW)
    {
      /* Isometric */
      float ang;

      ang = 30.0 * M_PI / 180.0;
      api->line((void *)api, which, canvas, NULL,
                x - cosf(ang) * guide_len, y + sinf(ang) * guide_len,
                x + cosf(ang) * guide_len, y - sinf(ang) * guide_len, 6, n_pt_persp_line_xor_callback);
      ang = 150.0 * M_PI / 180.0;
      api->line((void *)api, which, canvas, NULL,
                x - cosf(ang) * guide_len, y + sinf(ang) * guide_len,
                x + cosf(ang) * guide_len, y - sinf(ang) * guide_len, 6, n_pt_persp_line_xor_callback);
    }
    else if (which == TOOL_DIM_DRAW)
    {
      /* Dimetric */
      api->line((void *)api, which, canvas, NULL,
                x - cosf(dim_ang) * guide_len, y + sinf(dim_ang) * guide_len,
                x + cosf(dim_ang) * guide_len, y - sinf(dim_ang) * guide_len, 6, n_pt_persp_line_xor_callback);
      api->line((void *)api, which, canvas, NULL,
                x - cosf(M_PI - dim_ang) * guide_len, y + sinf(M_PI - dim_ang) * guide_len,
                x + cosf(M_PI - dim_ang) * guide_len, y - sinf(M_PI - dim_ang) * guide_len,
                6, n_pt_persp_line_xor_callback);
    }
    else if (which == TOOL_TRI_DRAW)
    {
      /* Trimetric */
      api->line((void *)api, which, canvas, NULL,
                x - cosf(tri_ang[0]) * guide_len, y + sinf(tri_ang[0]) * guide_len,
                x + cosf(tri_ang[0]) * guide_len, y - sinf(tri_ang[0]) * guide_len, 6, n_pt_persp_line_xor_callback);
      api->line((void *)api, which, canvas, NULL,
                x - cosf(tri_ang[1]) * guide_len, y + sinf(tri_ang[1]) * guide_len,
                x + cosf(tri_ang[1]) * guide_len, y - sinf(tri_ang[1]) * guide_len, 6, n_pt_persp_line_xor_callback);
    }
    else if (which == TOOL_OBLQ_DRAW || which == TOOL_OBLQ_DRAW_ALT)
    {
      /* Oblique */
      float ang;

      api->line((void *)api, which, canvas, NULL, 0, y, canvas->w - 1, y, 6, n_pt_persp_line_xor_callback);
      if (which == TOOL_OBLQ_DRAW)
      {
        ang = oblq_ang;
      }
      else
      {
        ang = oblqb_ang;
      }
      api->line((void *)api, which, canvas, NULL,
                x - cosf(ang) * guide_len, y + sinf(ang) * guide_len,
                x + cosf(ang) * guide_len, y - sinf(ang) * guide_len, 6, n_pt_persp_line_xor_callback);
    }
  }
  else if (which == TOOL_1PT_SELECT)
  {
    /* 1-point perspective - select */
    a1_pt_x = x;
    a1_pt_y = y;

    n_pt_persp_vanish_pt_moved(api, which, canvas, update_rect);
  }
  else if (which == TOOL_2PT_SELECT)
  {
    /* 2-point perspective - select */
    a2_pt_x[a2_pt_cur] = x;
    a2_pt_y[a2_pt_cur] = y;

    n_pt_persp_vanish_pt_moved(api, which, canvas, update_rect);
  }
  else if (which == TOOL_3PT_SELECT)
  {
    /* 3-point perspective - select */
    a3_pt_x[a3_pt_cur] = x;
    a3_pt_y[a3_pt_cur] = y;

    n_pt_persp_vanish_pt_moved(api, which, canvas, update_rect);
  }
  else if (which == TOOL_DIM_SELECT)
  {
    /* Dimetric - select */

    if (y > canvas->h / 2)
    {
      y = canvas->h - y;
    }
    if (x > canvas->w / 2)
    {
      x = canvas->w - x;
    }
    dim_ang = atan2f(canvas->h / 2 - y, canvas->w / 2 - x);

    if (dim_ang < MIN_AXONOMETRIC_ANGLE)
    {
      dim_ang = MIN_AXONOMETRIC_ANGLE;
    }
    else if (dim_ang > MAX_AXONOMETRIC_ANGLE)
    {
      dim_ang = MAX_AXONOMETRIC_ANGLE;
    }

#ifdef DEBUG
    printf("Dimetric select %.2f\n", dim_ang * 180.0 / M_PI);
#endif

    n_pt_persp_vanish_pt_moved(api, which, canvas, update_rect);
  }
  else if (which == TOOL_TRI_SELECT)
  {
    /* Trimetric - select */
    float ang, offset;

    ang = atan2f(canvas->h / 2 - y, x - canvas->w / 2);

#ifdef DEBUG
    printf("cursor ang = %.2f -> ", ang * 180.0 / M_PI);
#endif
    if (ang > M_PI)
    {
      ang -= M_PI;
    }
    else if (ang < 0)
    {
      ang += M_PI;
    }
#ifdef DEBUG
    printf("%.2f -> clipping for %d -> ", ang * 180.0 / M_PI, tri_ang_chosen);
#endif

    offset = ((M_PI / 2) * tri_ang_chosen);
    if (ang < MIN_AXONOMETRIC_ANGLE + offset)
    {
      ang = MIN_AXONOMETRIC_ANGLE + offset;
    }
    else if (ang > MAX_AXONOMETRIC_ANGLE + offset)
    {
      ang = MAX_AXONOMETRIC_ANGLE + offset;
    }
#ifdef DEBUG
    printf("%.2f -> \n", ang * 180.0 / M_PI);
#endif

    tri_ang[tri_ang_chosen] = ang;

#ifdef DEBUG
    printf("Trimetric select %.2f, %.2f\n", tri_ang[0] * 180.0 / M_PI, tri_ang[1] * 180.0 / M_PI);
#endif

    n_pt_persp_vanish_pt_moved(api, which, canvas, update_rect);
  }
  else if (which == TOOL_OBLQ_SELECT)
  {
    /* Oblique - select */

    if (y > canvas->h / 2)
    {
      y = canvas->h - y;
      x = canvas->w - x;
    }
    oblq_ang = atan2f(canvas->h / 2 - y, x - canvas->w / 2);

    if (oblq_ang < MIN_OBLIQUE_ANGLE)
    {
      oblq_ang = MIN_OBLIQUE_ANGLE;
    }
    else if (oblq_ang > MAX_OBLIQUE_ANGLE && oblq_ang < M_PI - MAX_OBLIQUE_ANGLE)
    {
      oblq_ang = MAX_OBLIQUE_ANGLE;
    }
    else if (oblq_ang > M_PI - MIN_OBLIQUE_ANGLE)
    {
      oblq_ang = M_PI - MIN_OBLIQUE_ANGLE;
    }

#ifdef DEBUG
    printf("Oblique select %.2f\n", oblq_ang * 180.0 / M_PI);
#endif
    n_pt_persp_vanish_pt_moved(api, which, canvas, update_rect);
  }

#ifdef PERF
  printf("%0.5f\n", (float)(SDL_GetPerformanceCounter() - tick) / (float)SDL_GetPerformanceFrequency());
#endif
}

void n_pt_persp_work(magic_api *api, int tool, SDL_Surface *canvas, int x, int y, SDL_Rect *update_rect, int xor)
{
  int x1, y1, x2, y2;
  float slope, slope2;
  int i, best_angle_idx;
  float cursor_angle, diff, best_diff;

  /* SDL_Rect area_rect; */

  if (n_pt_persp_snapshot == NULL)
    return;

  /* N.B. "which" is already set to the appropriate tool by the calling function! */

#ifdef DEBUG
  printf("\nn_pt_persp_work\n");
  printf("%d (%s)\n", tool, tool_debug_names[tool]);
  fflush(stdout);
#endif

  /* Adhere x & y to perspective! */

  x1 = y1 = x2 = y2 = 0;

  if (tool == TOOL_1PT_DRAW)
  {
    /* 1-point perspective */

    x1 = line_start_x;
    y1 = line_start_y;

    if (abs(line_start_x - x) <= SNAP)
    {
      /* Vertical */
      x2 = x1;
      y2 = y;
    }
    else if (abs(line_start_y - y) <= SNAP)
    {
      /* Horizontal */
      x2 = x;
      y2 = y1;
    }
    else
    {
      /* Diagonal */

      slope = ((float)y1 - (float)a1_pt_y) / ((float)x1 - (float)a1_pt_x);
      x2 = x;
      y2 = line_start_y + (slope * (x - line_start_x));

      /* Don't go past our cursor's Y */
      if ((y < line_start_y && y2 < y) || (y > line_start_y && y2 > y))
      {
        if (slope != 0.0)
        {
          y2 = y;
          x2 = ((y - line_start_y) / slope) + line_start_x;
        }
      }

      /* Snap to horizontal if cursor is on the wrong side */
      slope2 = ((float)line_start_y - (float)y) / ((float)line_start_x - (float)x);
      if ((slope2 > 0.00 && slope < 0.00) || (slope2 < 0.00 && slope > 0.00))
      {
        x2 = x;
        y2 = y1;
      }
    }
  }
  else if (tool == TOOL_2PT_DRAW || tool == TOOL_3PT_DRAW || tool == TOOL_3PT_DRAW_ALT)
  {
    float *valid_angle;

    /* 2- & 3-point perspective */

    if (tool == TOOL_2PT_DRAW)
    {
      valid_angle = a2_valid_angle;
    }
    else
    {                           /* TOOL_3PT_DRAW || TOOL_3PT_DRAW_ALT */
      valid_angle = a3_valid_angle;
    }

    /* Find the valid angle that the drawn angle fits best to */
    cursor_angle = atan2f(y - line_start_y, x - line_start_x);

    best_angle_idx = -1;
    best_diff = M_PI * 2;

    for (i = 0; i < 8; i++)
    {
      diff = fabs(cursor_angle - valid_angle[i]);

      if (diff < best_diff)
      {
        best_angle_idx = i;
        best_diff = diff;
      }
    }

    if (best_angle_idx == -1)
    {
      printf("???\n");
      return;
    }

    /* Calculate a line segment, so we can determine the slope */
    x1 = line_start_x;
    y1 = line_start_y;
    x2 = line_start_x + cosf(valid_angle[best_angle_idx]) * 1000;
    y2 = line_start_y + sinf(valid_angle[best_angle_idx]) * 1000;

    if (abs(x2 - x1) >= 2)
    {
      slope = ((float)y2 - (float)y1) / ((float)x2 - (float)x1);
      x2 = x;
      y2 = line_start_y + (slope * (x - line_start_x));

      /* Don't go past our cursor's Y */
      if ((y < line_start_y && y2 < y) || (y > line_start_y && y2 > y))
      {
        if (slope != 0.0)
        {
          y2 = y;
          x2 = ((y - line_start_y) / slope) + line_start_x;
        }
      }
    }
    else
    {
      x2 = x1;
      y2 = y;
    }
  }
  else if (tool == TOOL_ISO_DRAW || tool == TOOL_DIM_DRAW || tool == TOOL_TRI_DRAW ||
           tool == TOOL_OBLQ_DRAW || tool == TOOL_OBLQ_DRAW_ALT)
  {
    float valid_angles[6];
    float ang;

    /* Orthographic projections (isometric, dimetric, trimetric, oblique) */

    if (line_start_x == x)
    {
      /* Vertical */
      x1 = line_start_x;
      y1 = line_start_y;
      x2 = x1;
      y2 = y;
    }
    else
    {
      /* all tools include vertical */
      valid_angles[0] = -M_PI / 2;

      if (tool == TOOL_ISO_DRAW)
      {
        /* isometric diagonals */
        valid_angles[1] = -(30.0 * M_PI / 180.0);
        valid_angles[2] = -(150.0 * M_PI / 180.0);
      }
      else if (tool == TOOL_DIM_DRAW)
      {
        /* dimetric diagonals */
        valid_angles[1] = dim_ang;
        valid_angles[2] = M_PI - dim_ang;
      }
      else if (tool == TOOL_TRI_DRAW)
      {
        /* trimetric diagonals */
        valid_angles[1] = M_PI - tri_ang[0];
        valid_angles[2] = M_PI - tri_ang[1];
      }
      else if (tool == TOOL_OBLQ_DRAW)
      {
        /* horizontal */
        valid_angles[1] = 0.0;
        /* oblique diagonal */
        valid_angles[2] = M_PI - oblq_ang;
      }
      else if (tool == TOOL_OBLQ_DRAW_ALT)
      {
        /* horizontal */
        valid_angles[1] = 0.0;
        /* oblique diagonal */
        valid_angles[2] = M_PI - oblqb_ang;
      }

      /* And the opposite directions */
      for (i = 0; i < 3; i++)
      {
        valid_angles[i + 3] = valid_angles[i] + M_PI;
      }

#ifdef DEBUG
      printf("\n");
#endif
      for (i = 0; i < 6; i++)
      {
#ifdef DEBUG
        printf("  %.2f -> ", valid_angles[i] * 180.0 / M_PI);
#endif
        if (valid_angles[i] > M_PI)
        {
          valid_angles[i] -= (M_PI * 2);
        }
#ifdef DEBUG
        printf("%.2f\n", valid_angles[i] * 180.0 / M_PI);
#endif
      }

      cursor_angle = atan2f(line_start_y - y, line_start_x - x);
#ifdef DEBUG
      printf("cursor ang = %.2f\n", cursor_angle * 180.0 / M_PI);
#endif

      best_angle_idx = -1;
      best_diff = M_PI * 2;

      for (i = 0; i < 6; i++)
      {
#ifdef DEBUG
        printf("  #%d %.2f vs %.2f\n", i, cursor_angle * 180.0 / M_PI, valid_angles[i] * 180.0 / M_PI);
#endif
        diff = fabs(cursor_angle - valid_angles[i]);
        if (diff < best_diff)
        {
          best_angle_idx = i;
          best_diff = diff;
        }
        diff = fabs((cursor_angle - (M_PI * 2)) - valid_angles[i]);
        if (diff < best_diff)
        {
          best_angle_idx = i;
          best_diff = diff;
        }
      }

      if (best_angle_idx == -1)
      {
        printf("???\n");
        return;
      }

      ang = valid_angles[best_angle_idx];
#ifdef DEBUG
      printf("best ang = [%d] %.2f\n", best_angle_idx, ang * 180.0 / M_PI);
#endif

      x1 = line_start_x;
      y1 = line_start_y;
      x2 = line_start_x + cosf(ang) * 1000;
      y2 = line_start_y + sinf(ang) * 1000;

      if (abs(x2 - x1) >= 2)
      {
        slope = ((float)y2 - (float)y1) / ((float)x2 - (float)x1);
        x2 = x;
        y2 = line_start_y + (slope * (x - line_start_x));

        /* Don't go past our cursor's Y */
        if ((y < line_start_y && y2 < y) || (y > line_start_y && y2 > y))
        {
          if (slope != 0.0)
          {
            y2 = y;
            x2 = ((y - line_start_y) / slope) + line_start_x;
          }
        }
      }
      else
      {
        x2 = x1;
        y2 = y;
      }
    }
  }

  SDL_BlitSurface(n_pt_persp_snapshot, NULL, canvas, NULL);

  /* Draw the line */

  if (xor)
  {
    /* Still moving; use XOR */

    api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 3, n_pt_persp_line_xor_callback);
  }
  else
  {
    /* Released; draw the line for real */

    api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 1, n_pt_persp_line_callback);
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void n_pt_persp_release(magic_api *api, int which,
                        SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                        int x, int y, SDL_Rect *update_rect)
{
#ifdef DEBUG
  printf("\nn_pt_persp_release\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

#ifdef SNAP_TO
  snap_to(which, &x, &y);
#endif

  which = which_to_tool[which];

  if (which == TOOL_1PT_SELECT)
  {
    /* 1-point perspective - vanishing point drag released */
    api->stopsound();
  }
  else if (which == TOOL_2PT_SELECT)
  {
    /* 2-point perspective - vanishing point drag released */
    if (abs(a2_pt_x[0] - a2_pt_x[1]) < SNAP)
    {
      if (a2_pt_x[0] <= a2_pt_x[1])
      {
        a2_pt_x[0] -= (SNAP / 2);
        a2_pt_x[1] += (SNAP / 2);
      }
      else
      {
        a2_pt_x[0] += (SNAP / 2);
        a2_pt_x[1] -= (SNAP / 2);
      }
    }
    api->stopsound();
  }
  else if (which == TOOL_3PT_SELECT)
  {
    /* 3-point perspective - vanishing point drag released */
    api->stopsound();
  }
  else if (which == TOOL_DIM_SELECT || which == TOOL_TRI_SELECT || which == TOOL_OBLQ_SELECT)
  {
    /* Dimetric, Trimetric, Oblique - angle adjustment drag released */
    api->stopsound();
  }
  else
  {
    /* Draw the line (for real) */
    n_pt_persp_work(api, which, canvas, x, y, update_rect, 0);

    api->playsound(sound_effects[SND_DRAW_RELEASE], (x * 255) / canvas->w, 255);
  }
}


void n_pt_persp_set_color(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
                          SDL_Surface *canvas ATTRIBUTE_UNUSED,
                          SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                          Uint8 r, Uint8 g, Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  n_pt_persp_r = r;
  n_pt_persp_g = g;
  n_pt_persp_b = b;
}


void n_pt_persp_set_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                         SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *last ATTRIBUTE_UNUSED,
                         Uint8 size, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  n_pt_persp_size = ((size - 1) * 2) + 1;
}


void n_pt_persp_line_xor_callback(void *pointer, int tool ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                                  SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) pointer;

  api->xorpixel(canvas, x, y);
  api->xorpixel(canvas, x + 1, y + 1);
}

void n_pt_persp_line_xor_thick_callback(void *pointer, int tool ATTRIBUTE_UNUSED, SDL_Surface *canvas,
                                        SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  int i, j;

  magic_api *api = (magic_api *) pointer;

  for (i = -3; i <= 3; i++)
  {
    for (j = -3; j <= 3; j++)
    {
      if (abs(i) == abs(j))
      {
        api->xorpixel(canvas, x + i, y + j);
      }
    }
  }
}

void n_pt_persp_line_callback(void *pointer ATTRIBUTE_UNUSED, int tool ATTRIBUTE_UNUSED,
                              SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  SDL_Rect r;

  r.x = x - n_pt_persp_size / 2;
  r.y = y - n_pt_persp_size / 2;
  r.w = n_pt_persp_size;
  r.h = n_pt_persp_size;

  SDL_FillRect(canvas, &r, SDL_MapRGB(canvas->format, n_pt_persp_r, n_pt_persp_g, n_pt_persp_b));
}

void n_pt_persp_switchin(magic_api *api, int which, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
#ifdef DEBUG
  printf("\nn_pt_persp_switchin\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

  which = which_to_tool[which];

  if (which == TOOL_1PT_SELECT || which == TOOL_2PT_SELECT || which == TOOL_3PT_SELECT ||
      which == TOOL_DIM_SELECT || which == TOOL_TRI_SELECT || which == TOOL_OBLQ_SELECT)
  {
    SDL_BlitSurface(canvas, NULL, n_pt_persp_snapshot, NULL);

    n_pt_persp_draw_points(api, which, canvas);
  }
}

void n_pt_persp_switchout(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
#ifdef DEBUG
  printf("\nn_pt_persp_switchout\n");
  printf("%d becomes %d (%s)\n", which, which_to_tool[which], tool_debug_names[which_to_tool[which]]);
  fflush(stdout);
#endif

  which = which_to_tool[which];

  if (which == TOOL_1PT_SELECT || which == TOOL_2PT_SELECT || which == TOOL_3PT_SELECT ||
      which == TOOL_DIM_SELECT || which == TOOL_TRI_SELECT || which == TOOL_OBLQ_SELECT)
  {
    SDL_BlitSurface(n_pt_persp_snapshot, NULL, canvas, NULL);
  }
}

void n_pt_persp_draw_points(magic_api *api, int tool, SDL_Surface *canvas)
{
  int i, l, m, x1, y1, x2, y2, x;
  float slope;

#ifdef DEBUG
  printf("\nn_pt_persp_draw_points\n");
  printf("%d (%s)\n", tool, tool_debug_names[tool]);
  fflush(stdout);
#endif

  if (tool == TOOL_1PT_SELECT)
  {
    /* 1-point perspective */

    n_pt_persp_draw_one_point(api, canvas, a1_pt_x, a1_pt_y, 0);

    for (l = 0; l < 5; l++)
    {
      /* Diagonal from left to right sides of canvas */

      y1 = (a1_pt_y - (canvas->h / 2)) + (canvas->h / 5) * l + (canvas->h / 10);

      if (a1_pt_x > canvas->w / 2)
      {
        x1 = 0;
        x2 = canvas->w - 1;
      }
      else
      {
        x1 = canvas->w - 1;
        x2 = 0;
      }

      if (a1_pt_x != x1)
      {
        slope = ((float)a1_pt_y - (float)y1) / ((float)a1_pt_x - (float)x1);
        y2 = y1 + (x2 - x1) * slope;

        api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 6, n_pt_persp_line_xor_callback);

        /* Some vertical lines between the diagonals */
        if (l == 0)
        {
          for (m = 0; m < 8; m++)
          {
            int xx, yy1, yy2;
            int m_scale[8] = { -8, -4, -2, -1, 1, 2, 4, 8 };

            xx = a1_pt_x + ((float)(canvas->w / 10) * (float)m_scale[m]);
            yy1 = a1_pt_y + (a1_pt_x - xx) * slope;
            yy2 = a1_pt_y + (xx - a1_pt_x) * slope;

            api->line((void *)api, tool, canvas, NULL, xx, yy1, xx, yy2, 3, n_pt_persp_line_xor_callback);
          }
        }
      }
    }
  }
  else if (tool == TOOL_2PT_SELECT)
  {
    /* 2-point perspective */

    if (abs(a2_pt_x[0] - a2_pt_x[1]) < SNAP)
    {
      a2_pt_x[1] = a2_pt_x[0] + SNAP;
    }

    for (i = 0; i < 2; i++)
    {
      n_pt_persp_draw_one_point(api, canvas, a2_pt_x[i], a2_pt_y[i], i);
    }

    /* Horizon line (vanishing point) */
    slope = ((float)a2_pt_y[0] - (float)a2_pt_y[1]) / ((float)a2_pt_x[0] - (float)a2_pt_x[1]);
    x1 = 0;
    y1 = a2_pt_y[0] - (a2_pt_x[0] * slope);
    x2 = canvas->w;
    y2 = a2_pt_y[0] + ((canvas->w - a2_pt_x[0]) * slope);

    api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 12, n_pt_persp_line_xor_callback);

    x = (a2_pt_x[0] + a2_pt_x[1]) / 2;

    /* Perpendicular-to-horizon line */
    if (slope == 0.0 || slope == M_PI)
    {                           // FIXME: M_PI, really? -bjk 2024.01.20
      x1 = x;
      y1 = 0;
      x2 = x;
      y2 = canvas->h;
    }
    else
    {
      float perp_slope = -(slope);
      int y;

      x = (a2_pt_x[0] + a2_pt_x[1]) / 2;
      y = (a2_pt_y[0] + a2_pt_y[1]) / 2;

      x1 = x - (y * perp_slope);
      y1 = 0;
      x2 = x + ((canvas->h - y) * perp_slope);
      y2 = canvas->h;
    }

    api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 12, n_pt_persp_line_xor_callback);

    api->line((void *)api, tool, canvas, NULL, a2_pt_x[0], a2_pt_y[0], x2, y2, 12, n_pt_persp_line_xor_callback);

    api->line((void *)api, tool, canvas, NULL, a2_pt_x[1], a2_pt_y[1], x2, y2, 12, n_pt_persp_line_xor_callback);

    api->line((void *)api, tool, canvas, NULL, x1, y1, a2_pt_x[0], a2_pt_y[0], 12, n_pt_persp_line_xor_callback);

    api->line((void *)api, tool, canvas, NULL, x1, y1, a2_pt_x[1], a2_pt_y[1], 12, n_pt_persp_line_xor_callback);
  }
  else if (tool == TOOL_3PT_SELECT || tool == TOOL_3PT_SELECT_ALT)
  {
    /* 3-point perspective */
    int a3_x[3], a3_y[3];

    if (tool == TOOL_3PT_SELECT)
    {
      a3_x[0] = a3_pt_x[0];
      a3_y[0] = a3_pt_y[0];
      a3_x[1] = a3_pt_x[1];
      a3_y[1] = a3_pt_y[1];
      a3_x[2] = a3_pt_x[2];
      a3_y[2] = a3_pt_y[2];
    }
    else
    {
      a3_x[0] = a3b_pt_x[0];
      a3_y[0] = a3b_pt_y[0];
      a3_x[1] = a3b_pt_x[1];
      a3_y[1] = a3b_pt_y[1];
      a3_x[2] = a3b_pt_x[2];
      a3_y[2] = a3b_pt_y[2];
    }

    for (i = 0; i < 3; i++)
    {
      n_pt_persp_draw_one_point(api, canvas, a3_x[i], a3_y[i], i);
    }

    /* Horizon line (vanishing point) */
    slope = ((float)a3_y[0] - (float)a3_y[1]) / ((float)a3_x[0] - (float)a3_x[1]);
    x1 = 0;
    y1 = a3_y[0] - (a3_x[0] * slope);
    x2 = canvas->w;
    y2 = a3_y[0] + ((canvas->w - a3_x[0]) * slope);

    api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 12, n_pt_persp_line_xor_callback);

    for (i = 0; i < 6; i++)
    {
      x1 = a3_x[0] + (((a3_x[1] - a3_x[0]) / 5) * i);
      y1 = a3_y[0] + ((x1 - a3_x[0]) * slope);
      x2 = a3_x[2];
      y2 = a3_y[2];

      api->line((void *)api, tool, canvas, NULL, x1, y1, x2, y2, 12, n_pt_persp_line_xor_callback);
    }
  }
  else if (tool == TOOL_ISO_SELECT)
  {
    /* Isometric */

    /* vertical */
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2, 0, canvas->w / 2, canvas->h - 1, 12, n_pt_persp_line_xor_callback);

    /* the angle (always 30) */
    x1 = cosf(30.0 * M_PI / 180.0) * canvas->w;
    y1 = sinf(30.0 * M_PI / 180.0) * canvas->h;
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2 - x1, canvas->h / 2 - y1,
              canvas->w / 2 + x1, canvas->h / 2 + y1, 12, n_pt_persp_line_xor_callback);
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2 - x1, canvas->h / 2 + y1,
              canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_callback);
  }
  else if (tool == TOOL_DIM_SELECT)
  {
    /* Dimetric */

    /* vertical */
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2, 0, canvas->w / 2, canvas->h - 1, 12, n_pt_persp_line_xor_callback);

    /* the angle */
    x1 = cosf(dim_ang) * canvas->w;
    y1 = sinf(dim_ang) * canvas->h;
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2 - x1, canvas->h / 2 - y1,
              canvas->w / 2 + x1, canvas->h / 2 + y1, 12, n_pt_persp_line_xor_thick_callback);
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2 - x1, canvas->h / 2 + y1,
              canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_thick_callback);
  }
  else if (tool == TOOL_TRI_SELECT)
  {
    /* Trimetric */

    /* vertical */
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2, 0, canvas->w / 2, canvas->h - 1, 12, n_pt_persp_line_xor_callback);

    /* angle 1 */
    x1 = cosf(tri_ang[0]) * canvas->w;
    y1 = sinf(tri_ang[0]) * canvas->w;
    if (tri_ang_chosen == 0)
    {
      api->line((void *)api, tool, canvas, NULL,
                canvas->w / 2 - x1, canvas->h / 2 + y1,
                canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_thick_callback);
    }
    else
    {
      api->line((void *)api, tool, canvas, NULL,
                canvas->w / 2 - x1, canvas->h / 2 + y1,
                canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_callback);
    }

    /* angle 2 */
    x1 = cosf(tri_ang[1]) * canvas->w;
    y1 = sinf(tri_ang[1]) * canvas->w;
    if (tri_ang_chosen == 1)
    {
      api->line((void *)api, tool, canvas, NULL,
                canvas->w / 2 - x1, canvas->h / 2 + y1,
                canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_thick_callback);
    }
    else
    {
      api->line((void *)api, tool, canvas, NULL,
                canvas->w / 2 - x1, canvas->h / 2 + y1,
                canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_callback);
    }
  }
  else if (tool == TOOL_OBLQ_SELECT || tool == TOOL_OBLQ_SELECT_ALT)
  {
    /* Oblique */

    /* vertical */
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2, 0, canvas->w / 2, canvas->h - 1, 12, n_pt_persp_line_xor_callback);

    /* horizontal */
    api->line((void *)api, tool, canvas, NULL,
              0, canvas->h / 2, canvas->w - 1, canvas->h / 2, 12, n_pt_persp_line_xor_callback);

    /* diagonal (receding) */
    x1 = cosf(oblq_ang) * canvas->w;
    y1 = sinf(oblq_ang) * canvas->h;
    if (tool == TOOL_OBLQ_SELECT_ALT)
    {
      y1 = -y1;
    }
    api->line((void *)api, tool, canvas, NULL,
              canvas->w / 2 - x1, canvas->h / 2 + y1,
              canvas->w / 2 + x1, canvas->h / 2 - y1, 12, n_pt_persp_line_xor_thick_callback);
  }
}

#define DOT_WIDTH 12

void n_pt_persp_draw_one_point(magic_api *api, SDL_Surface *canvas, int x, int y, int i)
{
  int xx, yy;
  Uint8 r, g, b;

  for (yy = -(DOT_WIDTH + i); yy <= DOT_WIDTH + i; yy++)
  {
    for (xx = -(DOT_WIDTH + i) + (yy % 2); xx <= DOT_WIDTH + i; xx += 2)
    {
      if (api->in_circle(xx, yy, DOT_WIDTH) && abs(xx) >= i && abs(yy) >= i)
      {
        SDL_GetRGB(api->getpixel(canvas, x + xx, y + yy), canvas->format, &r, &g, &b);
        r ^= 255;
        g ^= 255;
        b ^= 255;
        api->putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format, r, g, b));
      }
    }
  }
}


#ifdef SNAP_TO

/* Snap to grids based on the tool */
void snap_to(int which, int *x, int *y)
{
  if (which == TOOL_ISO_DRAW)
  {
    /* FIXME */
  }
  else if (which == TOOL_DIM_DRAW)
  {
    /* FIXME */
  }
  else if (which == TOOL_TRI_DRAW)
  {
    /* FIXME */
  }
  else if (which == TOOL_OBLQ_DRAW || which == TOOL_OBLQ_DRAW_ALT)
  {
    /* FIXME */
  }
}

#endif
