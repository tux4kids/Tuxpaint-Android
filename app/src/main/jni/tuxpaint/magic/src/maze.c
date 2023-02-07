/* maze.c

  Allows painting generated maze puzzles on your picture.

  Last updated: February 5, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#define MAZE_BLOCK_SIZE 8
#define MAZE_PAINT_RADIUS ((MAZE_BLOCK_SIZE * 3) + 2)

#define MAZE_HALL 0
#define MAZE_UP 1
#define MAZE_DOWN 2
#define MAZE_LEFT 3
#define MAZE_RIGHT 4
#define MAZE_START 5
#define MAZE_WALL 255

int xm[4] = { 0, 0, -1, 1 };
int ym[4] = { -1, 1, 0, 0 };

typedef struct {
  int x, y;
} maze_start_t;

Mix_Chunk *snd_effect = NULL;
Uint8 * maze_mask = NULL, * new_maze_mask = NULL, * maze_array = NULL;
Uint32 * maze_color;
maze_start_t * maze_starts;
SDL_Surface * maze_snapshot = NULL;
Uint8 maze_r, maze_g, maze_b;
int maze_start_x, maze_start_y;
int num_maze_starts = 0, maze_starts_size = 0;

Uint32 maze_api_version(void);
int maze_init(magic_api * api);
int maze_get_tool_count(magic_api * api);
SDL_Surface *maze_get_icon(magic_api * api, int which);
char *maze_get_name(magic_api * api, int which);
int maze_get_group(magic_api * api, int which);
char *maze_get_description(magic_api * api, int which, int mode);
int maze_requires_colors(magic_api * api, int which);
int maze_modes(magic_api * api, int which);
void maze_shutdown(magic_api * api);
void maze_click(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x,
                     int y, SDL_Rect * update_rect);
void maze_set_color(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void maze_drag(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * snapshot, int ox, int oy, int x, int y,
                    SDL_Rect * update_rect);
void maze_line_callback_drag(void *ptr, int which, SDL_Surface * canvas,
                                  SDL_Surface * snapshot, int x, int y);
void maze_release(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int x, int y,
                       SDL_Rect * update_rect);
void maze_render(magic_api * api, SDL_Surface * canvas);
void maze_switchin(magic_api * api, int which, int mode,
                        SDL_Surface * canvas);
void maze_switchout(magic_api * api, int which, int mode,
                         SDL_Surface * canvas);
void zero_maze_arrays(SDL_Surface * canvas);
void draw_hall(SDL_Surface * canvas, int x, int y);
void maze_add_start(void);
int check_arrays(void);
void maze_collapse_contiguous(SDL_Surface * canvas);


Uint32 maze_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int maze_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/maze.ogg",
           api->data_directory);
  snd_effect = Mix_LoadWAV(fname);

  return (1);
}

int maze_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}


SDL_Surface *maze_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/maze.png",
           api->data_directory);

  return (IMG_Load(fname));
}

char *maze_get_name(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext("Maze"));
}

int maze_get_group(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_PAINTING;
}

char *maze_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                int which ATTRIBUTE_UNUSED,
                                int mode)
{
  if (mode == MODE_PAINT)
    return(gettext("Click and drag to paint a maze on your picture."));
  else
    return(gettext("Click to turn your entire picture into a maze."));
}

int maze_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                              int which ATTRIBUTE_UNUSED)
{
  return 1;
}

int maze_modes(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT | MODE_FULLSCREEN;
}

void maze_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (snd_effect != NULL)
    Mix_FreeChunk(snd_effect);

  if (maze_mask != NULL)
    free(maze_mask);

  if (new_maze_mask != NULL)
    free(new_maze_mask);

  if (maze_array != NULL)
    free(maze_array);

  if (maze_color != NULL)
    free(maze_color);
}

/* FIXME */
int check_arrays(void) {
  if (maze_array == NULL || maze_mask == NULL || new_maze_mask == NULL ||
      maze_color == NULL || maze_snapshot == NULL || maze_starts == NULL) {
    return(0);
  }
  return(1);
}

void
maze_click(magic_api * api, int which, int mode,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect)
{
  if (!check_arrays())
    return;

  if (snd_effect != NULL)
    api->stopsound();

  maze_start_x = floor(x / MAZE_BLOCK_SIZE) * MAZE_BLOCK_SIZE + (MAZE_BLOCK_SIZE / 2);
  maze_start_y = floor(y / MAZE_BLOCK_SIZE) * MAZE_BLOCK_SIZE + (MAZE_BLOCK_SIZE / 2);

  if (mode == MODE_PAINT) {
    maze_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
  } else {
    Uint32 color;

    maze_add_start();
    memset(maze_mask, 1, (canvas->w * canvas->h));

    color = SDL_MapRGB(canvas->format, maze_r, maze_g, maze_b);
//  memset(maze_color, color, (sizeof(Uint32) * (canvas->w * canvas->h))); // FIXME: Why doesn't this work? -bjk 2023.01.21
    for (y = 0; y < canvas->h; y++)
      for (x = 0; x < canvas->w; x++)
        maze_color[y * canvas->w + x] = color;

    api->playsound(snd_effect, 128, 255);

    maze_render(api, canvas);

    num_maze_starts = 0;

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}


void
maze_drag(magic_api * api ATTRIBUTE_UNUSED, int which, SDL_Surface * canvas,
               SDL_Surface * snapshot, int ox ATTRIBUTE_UNUSED,
               int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  if (!check_arrays())
    return;

  api->line((void *) api, which, canvas, snapshot, ox, oy, x, y, 1,
            maze_line_callback_drag);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void maze_release(magic_api * api, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                  int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED,
                  SDL_Rect * update_rect) {
  if (snd_effect != NULL)
    api->stopsound();

  maze_collapse_contiguous(canvas);

  maze_add_start();

  maze_render(api, canvas);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


#define STATE_DONE 0
#define STATE_KEEPGOING 1
#define STATE_PICKDIR 2

void maze_render(magic_api * api, SDL_Surface * canvas)
{
  int x, y, nx, ny, btwn_nx, btwn_ny, state, dir, old_dir, s;
  Uint32 iter;

  if (!check_arrays())
    return;

  /* Reset everything we've drawn [back to] wall: */
  memset(maze_array, MAZE_HALL, (canvas->w * canvas->h));
  for (y = 0; y < canvas->h; y++) {
    for (x = 0; x < canvas->w; x++) {
      if (maze_mask[y * canvas->w + x]) {
        maze_array[y * canvas->w + x] = MAZE_WALL;
      }
    }
  }

  /* Render a maze! */
  for (s = 0; s < num_maze_starts; s++) {
    x = maze_starts[s].x;
    y = maze_starts[s].y;

    /* Render from here, ONLY if it's still non-contiguous to anything: */
    if (maze_mask[y * canvas->w + x] == s + 1) {
      maze_array[y * canvas->w + x] = MAZE_START;

      state = STATE_PICKDIR;
      iter = 0;
      do {
        if (state == STATE_PICKDIR) {
          dir = (rand() % 4);
          old_dir = dir;
        }

        state = STATE_DONE;

        nx = x + (xm[dir] * MAZE_BLOCK_SIZE * 2);
        ny = y + (ym[dir] * MAZE_BLOCK_SIZE * 2);

        if (nx >= 0 && nx < canvas->w && ny >= 0 && ny < canvas->h && maze_array[ny * canvas->w + nx] == MAZE_WALL) {
          draw_hall(canvas, nx, ny);
          maze_array[ny * canvas->w + nx] = dir + 1;

          btwn_nx = x + (xm[dir] * MAZE_BLOCK_SIZE);
          btwn_ny = y + (ym[dir] * MAZE_BLOCK_SIZE);
          draw_hall(canvas, btwn_nx, btwn_ny);

          x = nx;
          y = ny;

          state = STATE_PICKDIR;
        }

        if (state == STATE_DONE) {
          dir = (dir + 1) % 4;

          if (dir != old_dir)
            state = STATE_KEEPGOING;
        }

        if (state == STATE_DONE) {
          dir = maze_array[y * canvas->w + x];
          draw_hall(canvas, x, y);

          if (dir != MAZE_START) {
            x = x - (xm[dir - 1] * MAZE_BLOCK_SIZE * 2);
            y = y - (ym[dir - 1] * MAZE_BLOCK_SIZE * 2);

            state = STATE_PICKDIR;
          }
        }
        iter++;
      }
      while (state != STATE_DONE && iter < 100000);
    }
  }

  /* Draw the maze onto the canvas */
  for (y = 0; y < canvas->h; y++) {
    for (x = 0; x < canvas->w; x++) {
      if (maze_array[y * canvas->w + x] == MAZE_WALL) {
        api->putpixel(canvas, x, y, maze_color[y * canvas->w + x]);
      } else {
        api->putpixel(canvas, x, y, api->getpixel(maze_snapshot, x, y));
      }
    }
  }
}


void maze_set_color(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED,
                    SDL_Surface * last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  maze_r = r;
  maze_g = g;
  maze_b = b;
}


void maze_line_callback_drag(void *ptr, int which ATTRIBUTE_UNUSED,
                                  SDL_Surface * canvas,
                                  SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                                  int x, int y)
{
  magic_api *api = (magic_api *) ptr;
  int xx, yy, idx;
  Uint32 color;

  if (snd_effect != NULL)
    api->playsound(snd_effect, (x * 255) / canvas->w, 255);

  color = SDL_MapRGB(canvas->format, maze_r, maze_g, maze_b);

  x = floor(x / (MAZE_BLOCK_SIZE * 3)) * (MAZE_BLOCK_SIZE * 3);
  y = floor(y / (MAZE_BLOCK_SIZE * 3)) * (MAZE_BLOCK_SIZE * 3);

  for (yy = -MAZE_PAINT_RADIUS; yy < MAZE_PAINT_RADIUS; yy++) {
    if (y + yy > 0 && y + yy < canvas->h) {
      for (xx = -MAZE_PAINT_RADIUS; xx < MAZE_PAINT_RADIUS; xx++) {
        if (x + xx > 0 && x + xx < canvas->w) {
          idx = (y + yy) * canvas->w + (x + xx);
          if (maze_mask[idx] == 0) {
            maze_mask[idx] = num_maze_starts + 1;
          }
          maze_color[idx] = color;
          api->putpixel(canvas, x + xx, y + yy, color);
        }
      }
    }
  }
}

void maze_switchin(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas)
{
  if (maze_array == NULL)
    {
      maze_array = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);
      if (maze_array == NULL)
        {
          fprintf(stderr, "maze: Cannot malloc() maze_array!\n");
          return;
        }
    }

  if (maze_mask == NULL)
    {
      maze_mask = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);
      if (maze_mask == NULL)
        {
          fprintf(stderr, "maze: Cannot malloc() maze_mask!\n");
          return;
        }
    }

  if (new_maze_mask == NULL)
    {
      new_maze_mask = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);
      if (new_maze_mask == NULL)
        {
          fprintf(stderr, "new_maze: Cannot malloc() maze_mask!\n");
          return;
        }
    }

  if (maze_color == NULL)
    {
      maze_color = (Uint32 *) malloc(sizeof(Uint32) * canvas->w * canvas->h);
      if (maze_color == NULL)
        {
          fprintf(stderr, "maze: Cannot malloc() maze_color!\n");
          return;
        }
    }

  if (maze_starts == NULL)
    {
      int w, h;
      w = ceil(canvas->w / MAZE_BLOCK_SIZE);
      h = ceil(canvas->h / MAZE_BLOCK_SIZE);
      maze_starts_size = (w * h);

      maze_starts = (maze_start_t *) malloc(sizeof(maze_start_t) * maze_starts_size);
      if (maze_starts == NULL)
        {
          fprintf(stderr, "maze: Cannot malloc() maze_starts!\n");
          return;
        }
    }

  zero_maze_arrays(canvas);

  if (maze_snapshot == NULL)
    maze_snapshot = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                         canvas->format->BitsPerPixel, canvas->format->Rmask,
                         canvas->format->Gmask, canvas->format->Bmask,
                         canvas->format->Amask);

  if (maze_snapshot != NULL)
    SDL_BlitSurface(canvas, NULL, maze_snapshot, NULL);
}

void maze_switchout(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED,
                         int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
  zero_maze_arrays(canvas);
}

void zero_maze_arrays(SDL_Surface * canvas)
{
  num_maze_starts = 0;

  if (maze_mask != NULL)
    memset(maze_mask, 0, (canvas->w * canvas->h));

  if (new_maze_mask != NULL)
    memset(new_maze_mask, 0, (canvas->w * canvas->h));

  if (maze_array != NULL)
    memset(maze_array, MAZE_HALL, (canvas->w * canvas->h));

  if (maze_color != NULL)
    memset(maze_color, 0, (sizeof(Uint32) * (canvas->w * canvas->h)));
}

void draw_hall(SDL_Surface * canvas, int x, int y) {
  int xx, yy;

  for (yy = -MAZE_BLOCK_SIZE / 2; yy < MAZE_BLOCK_SIZE / 2; yy++) {
    if (y + yy >= 0 && y + yy < canvas->h) {
      for (xx = -MAZE_BLOCK_SIZE / 2; xx < MAZE_BLOCK_SIZE / 2; xx++) {
        if (x + xx >= 0 && x + xx < canvas->w) {
          maze_array[(y + yy) * canvas->w + (x + xx)] = MAZE_HALL;
        }
      }
    }
  }
}

void maze_add_start(void) {
  maze_starts[num_maze_starts].x = maze_start_x;
  maze_starts[num_maze_starts].y = maze_start_y;

  num_maze_starts++;
}

void maze_collapse_contiguous(SDL_Surface * canvas) {
  int i, x, y, touching, idx;

  for (i = num_maze_starts; i > 0; i--) {
    touching = -1;
    for (y = 1; y < canvas->h - 1 && touching == -1; y++) {
      for (x = 1; x < canvas->w - 1 && touching == -1; x++) {
        idx = (y * canvas->w + x);
        if (maze_mask[idx] == i + 1) {
          if (maze_mask[idx - 1] != 0 && maze_mask[idx - 1] != i + 1) {
            touching = maze_mask[idx - 1];
          } else if (maze_mask[idx + 1] != 0 && maze_mask[idx + 1] != i + 1) {
            touching = maze_mask[idx + 1];
          } else if (maze_mask[idx - canvas->w] != 0 && maze_mask[idx - canvas->w] != i + 1) {
            touching = maze_mask[idx - canvas->w];
          } else if (maze_mask[idx + canvas->w] != 0 && maze_mask[idx + canvas->w] != i + 1) {
            touching = maze_mask[idx + canvas->w];
          }
        }
      }
    }

    if (touching != -1) {
      /* Touching someone else; turn all of our area
         into that area */
      for (y = 1; y < canvas->h; y++) {
        for (x = 1; x < canvas->w; x++) {
          idx = (y * canvas->w + x);
          if (maze_mask[idx] == i + 1) {
            maze_mask[idx] = touching;
          }
        }
      }
    }
  }
}

