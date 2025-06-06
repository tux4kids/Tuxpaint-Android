/*
  Strings -- draws string art.

  Last updated: October 7, 2024
*/
#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

unsigned int img_w, img_h;
static Uint8 string_r, string_g, string_b;
static int string_ox, string_oy;
static int string_vertex_x, string_vertex_y, string_vertex_done, string_vertex_distance;
static SDL_Surface *canvas_backup;
enum string_tools
{
  STRING_TOOL_FULL_BY_OFFSET,
  STRING_TOOL_TRIANGLE,
  STRING_TOOL_ANGLE,
  STRING_NUMTOOLS
};

Mix_Chunk *string_snd[STRING_NUMTOOLS];

// Custom function declarations

void string_callback(void *ptr, int which_tool, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void string_callback_xor(void *ptr, int which_tool, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void string_draw_triangle(magic_api * api, int which, SDL_Surface * canvas,
                          SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void string_draw_angle(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void string_draw_triangle_preview(magic_api * api, int which,
                                  SDL_Surface * canvas,
                                  SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void string_draw_angle_preview(magic_api * api, int which,
                               SDL_Surface * canvas, SDL_Surface * snapshot,
                               int ox, int oy, int x, int y, SDL_Rect * update_rect);
void scale_xcoord(int *xcoord);
void scale_ycoord(int *ycoord);
void scale_coords(int *ox, int *oy, int *x, int *y);
void string_draw_wrapper(magic_api * api, int which,
                         SDL_Surface * canvas, SDL_Surface * snapshot, int ox,
                         int oy, int x, int y, SDL_Rect * update_rect);
void string_set_vertex(int x, int y);
void compute_middle(int start_point, int end_point, int vertex, int *middle);


// Prototypes for required functions

void string_drag(magic_api * api, int which, SDL_Surface * canvas,
                 SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);


Uint32 string_api_version(void);
int string_modes(magic_api * api, int which);
void string_set_color(magic_api * api, int which, SDL_Surface * canvas,
                      SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
int string_get_tool_count(magic_api * api);
SDL_Surface *string_get_icon(magic_api * api, int which);
char *string_get_name(magic_api * api, int which);
int string_get_group(magic_api * api, int which);
int string_get_order(int which);
char *string_get_description(magic_api * api, int which, int mode);
int string_requires_colors(magic_api * api, int which);
void string_release(magic_api * api, int which,
                    SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
int string_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
void string_shutdown(magic_api * api);
void string_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * snapshot);
void string_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas, SDL_Surface * snapshot);
void string_click(magic_api * api, int which, int mode, SDL_Surface * canvas,
                  SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
Uint8 string_accepted_sizes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED);
Uint8 string_default_size(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED);
void string_set_size(magic_api * api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface * canvas ATTRIBUTE_UNUSED,
                     SDL_Surface * last ATTRIBUTE_UNUSED,
                     Uint8 size ATTRIBUTE_UNUSED, SDL_Rect * update_rect ATTRIBUTE_UNUSED);

// Required functions

Uint32 string_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int string_modes(magic_api *api ATTRIBUTE_UNUSED, int which)
{
  if (which == STRING_TOOL_FULL_BY_OFFSET)
    return (MODE_PAINT);
  else
    return (MODE_PAINT_WITH_PREVIEW);
}

void string_set_color(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED,
                      SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 r, Uint8 g,
                      Uint8 b, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  string_r = r;
  string_g = g;
  string_b = b;
}



int string_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return STRING_NUMTOOLS;
}

SDL_Surface *string_get_icon(magic_api *api, int which)
{
  char fname[1024];

  switch (which)
  {
  case STRING_TOOL_FULL_BY_OFFSET:
    snprintf(fname, sizeof(fname), "%simages/magic/string_art_full_by_offset.png", api->data_directory);
    break;
  case STRING_TOOL_TRIANGLE:
    snprintf(fname, sizeof(fname), "%simages/magic/string_art_triangles.png", api->data_directory);
    break;
  case STRING_TOOL_ANGLE:
    snprintf(fname, sizeof(fname), "%simages/magic/string_art_angles.png", api->data_directory);
    break;
  }

  return (IMG_Load(fname));
}


char *string_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  switch (which)
  {
  case STRING_TOOL_FULL_BY_OFFSET:
    return strdup(gettext("String edges"));
    break;
  case STRING_TOOL_TRIANGLE:
    return strdup(gettext("String corner"));
    break;
  default:
    return strdup(gettext("String 'V'"));
  }
}

int string_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_ARTISTIC;
}

int string_get_order(int which)
{
  switch (which)
  {
  case STRING_TOOL_FULL_BY_OFFSET:
    return 9000;
    break;
  case STRING_TOOL_TRIANGLE:
    return 9001;
    break;
  default:
    return 9002;
  }
}


char *string_get_description(magic_api *api ATTRIBUTE_UNUSED, int which, int mode ATTRIBUTE_UNUSED)
{
  switch (which)
  {
  case STRING_TOOL_FULL_BY_OFFSET:
    return
      strdup(gettext
             ("Click and drag to draw string art. Drag top-bottom to draw less or more lines, left or right to make a bigger hole."));
    break;
  case STRING_TOOL_TRIANGLE:
    return strdup(gettext("Click and drag to draw arrows made of string art."));
    break;
  default:
    return strdup(gettext("Draw string art arrows with free angles."));
  }
}

int string_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 1;
}

void string_release(magic_api *api, int which,
                    SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  int dx, dy;

  if (which == STRING_TOOL_TRIANGLE)
    string_draw_triangle((void *)api, which, canvas, snapshot, string_ox, string_oy, x, y, update_rect);
  if (which == STRING_TOOL_ANGLE)
  {
    if (!string_vertex_done)    // maybe we face small children, draw square angles aligned to the drag
    {
      dx = string_ox - x;
      dy = string_oy - y;
      y = y + dx;
      x = x - dy;
    }
    string_draw_angle((void *)api, which, canvas, snapshot, string_ox, string_oy, x, y, update_rect);
  }
}

int string_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/string.ogg", api->data_directory);
  string_snd[STRING_TOOL_FULL_BY_OFFSET] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%ssounds/magic/string2.ogg", api->data_directory);
  string_snd[STRING_TOOL_TRIANGLE] = Mix_LoadWAV(fname);

  snprintf(fname, sizeof(fname), "%ssounds/magic/string3.ogg", api->data_directory);
  string_snd[STRING_TOOL_ANGLE] = Mix_LoadWAV(fname);

  return (1);
}

void string_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  int i = 0;

  if (canvas_backup)
    SDL_FreeSurface(canvas_backup);

  while (i < STRING_NUMTOOLS)
  {
    if (string_snd[i] != NULL)
      Mix_FreeChunk(string_snd[i]);
    i++;
  }
}

void string_switchin(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED)
{
  canvas_backup =
    SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                         canvas->format->BitsPerPixel, canvas->format->Rmask,
                         canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);
}

void string_switchout(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                      SDL_Surface *canvas ATTRIBUTE_UNUSED, SDL_Surface *snapshot ATTRIBUTE_UNUSED)
{
  SDL_FreeSurface(canvas_backup);
  canvas_backup = NULL;
}

// Interactivity functions


void string_callback(void *ptr, int which ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  api->putpixel(canvas, x, y, SDL_MapRGBA(canvas->format, string_r, string_g, string_b, 255));
}

void string_callback_xor(void *ptr, int which ATTRIBUTE_UNUSED,
                         SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  api->xorpixel(canvas, x, y);
}


void string_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
                  SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  SDL_BlitSurface(canvas, NULL, canvas_backup, NULL);

  string_ox = x;
  string_oy = y;
  string_vertex_distance = 0;
  string_vertex_done = 0;
  string_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}

static void string_draw_full_by_offset(void *ptr, int which ATTRIBUTE_UNUSED,
                                       SDL_Surface *canvas,
                                       SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  magic_api *api = (magic_api *) ptr;
  int u;
  int i;
  int o;                        //offset

  // int n=y/5;
  int **a;
  float step_w, step_h, aux;
  int side = (int)(y / 3);

  SDL_BlitSurface(snapshot, 0, canvas, 0);

  if (side < 3)
    side = 3;

  o = (int)(side * 4 * x / canvas->w);
  step_w = canvas->w / (float)side;
  step_h = canvas->h / (float)side;

  a = malloc(sizeof(int *) * side * 4 * 2);

  for (i = 0; i < side * 4; i++)
  {
    a[i] = malloc(sizeof(int *) * 2);
    if (i < side)
    {
      a[i][0] = 0;
      aux = step_h * (float)i;
      a[i][1] = (int)aux;
    }
    else if (i < (side * 2))
    {
      a[i][0] = (int)((float)(i % side) * step_w);
      a[i][1] = canvas->h;
    }
    else if (i < (int)(side * 3))
    {
      a[i][0] = canvas->w;
      a[i][1] = (int)(canvas->h - (float)((i % side) * step_h));
    }
    else if (i < (int)(side * 4))
    {
      a[i][0] = (int)(canvas->w - ((float)((i % side) * step_w)));
      a[i][1] = 0;
    }
  }


  for (i = 0; i < side * 4; i++)
  {
    u = (i + o) % (side * 4);
    api->line((void *)api, which, canvas, snapshot, a[i][0], a[i][1], a[u][0], a[u][1], 1, string_callback);
  }

  for (i = 0; i < side * 4; i++)
  {
    free(a[i]);
  }
  free(a);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void scale_xcoord(int *xcoord)
{
  if (*xcoord < string_ox)
    *xcoord = string_ox - (string_ox - *xcoord) * 4;
  else
    *xcoord = string_ox + (*xcoord - string_ox) * 4;
}

void scale_ycoord(int *ycoord)
{
  if (*ycoord < string_oy)
    *ycoord = string_oy - (string_oy - *ycoord) * 4;
  else
    *ycoord = string_oy + (*ycoord - string_oy) * 4;
}

void scale_coords(int *ox, int *oy, int *x, int *y)
{
  scale_xcoord(ox);
  scale_xcoord(x);
  scale_ycoord(oy);
  scale_ycoord(y);
}

void compute_middle(int start_point, int end_point, int vertex, int *middle)
{
  *middle = min(start_point, end_point) + (max(start_point, end_point) - min(start_point, end_point)) / 2;
  *middle = min(*middle, vertex) + (max(*middle, vertex) - min(*middle, vertex)) / 2;
}

void string_draw_triangle_preview(magic_api *api, int which,
                                  SDL_Surface *canvas,
                                  SDL_Surface *snapshot, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  int middle_x, middle_y;

  scale_coords(&ox, &oy, &x, &y);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
  SDL_BlitSurface(canvas_backup, update_rect, canvas, update_rect);

  compute_middle(x, string_ox, string_ox, &middle_x);
  compute_middle(y, string_oy, string_oy, &middle_y);

  api->line((void *)api, which, canvas, snapshot, string_ox, string_oy, string_ox, y, 1, string_callback_xor);
  api->line((void *)api, which, canvas, snapshot, string_ox, string_oy, x, string_oy, 1, string_callback_xor);
  api->line((void *)api, which, canvas, snapshot, middle_x, middle_y, x, string_oy, 1, string_callback_xor);
  api->line((void *)api, which, canvas, snapshot, string_ox, y, middle_x, middle_y, 1, string_callback_xor);
}

void string_draw_angle_preview(magic_api *api, int which,
                               SDL_Surface *canvas, SDL_Surface *snapshot,
                               int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  int middle_x, middle_y;
  int dx, dy;

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
  SDL_BlitSurface(canvas_backup, update_rect, canvas, update_rect);

  api->line((void *)api, which, canvas, snapshot, string_ox, string_oy,
            string_vertex_x, string_vertex_y, 1, string_callback_xor);
  if (!string_vertex_done)
  {
    //    if(!string_vertex_done) // maybe we face small children, draw square angles aligned to the drag
    //{
    dx = string_ox - x;
    dy = string_oy - y;
    y = y + dx;
    x = x - dy;
  }

  compute_middle(string_ox, x, string_vertex_x, &middle_x);
  compute_middle(string_oy, y, string_vertex_y, &middle_y);

  api->line((void *)api, which, canvas, snapshot, string_vertex_x, string_vertex_y, x, y, 1, string_callback_xor);
  api->line((void *)api, which, canvas, snapshot, string_ox, string_oy, middle_x, middle_y, 1, string_callback_xor);
  api->line((void *)api, which, canvas, snapshot, x, y, middle_x, middle_y, 1, string_callback_xor);
}

void string_draw_angle(magic_api *api, int which ATTRIBUTE_UNUSED,
                       SDL_Surface *canvas,
                       SDL_Surface *snapshot ATTRIBUTE_UNUSED,
                       int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  float first_arm_step_x, first_arm_step_y, second_arm_step_x, second_arm_step_y;
  int i;
  int max_wh, steps;
  int max_separation = 10;

  update_rect->x = min(min(string_ox, string_vertex_x), x);
  update_rect->y = min(min(string_oy, string_vertex_y), y);
  update_rect->w = max(max(string_ox, string_vertex_x), x) - update_rect->x;
  update_rect->h = max(max(string_oy, string_vertex_y), y) - update_rect->y;
  SDL_BlitSurface(canvas_backup, update_rect, canvas, update_rect);

  max_wh =
    max(max(max(string_ox, string_vertex_x), x) -
        min(min(string_vertex_x, x), string_ox),
        max(max(string_oy, string_vertex_y), y) - min(min(string_vertex_y, y), string_oy));

  steps = max_wh / max_separation;
  first_arm_step_x = (float)(string_ox - string_vertex_x) / (float)steps;
  first_arm_step_y = (float)(string_oy - string_vertex_y) / (float)steps;
  second_arm_step_x = (float)(string_vertex_x - x) / (float)steps;
  second_arm_step_y = (float)(string_vertex_y - y) / (float)steps;

  for (i = 0; i <= steps; i++)
  {
    api->line((void *)api, 0, canvas, snapshot,
              string_ox - first_arm_step_x * i,
              string_oy - first_arm_step_y * i,
              string_vertex_x - second_arm_step_x * i, string_vertex_y - second_arm_step_y * i, 1, string_callback);
  }
}

void string_draw_triangle(magic_api *api, int which ATTRIBUTE_UNUSED,
                          SDL_Surface *canvas, SDL_Surface *snapshot,
                          int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  SDL_BlitSurface(canvas_backup, 0, canvas, 0);
  scale_coords(&ox, &oy, &x, &y);

  string_vertex_x = string_ox;
  string_vertex_y = string_oy;
  string_ox = string_vertex_x;
  string_oy = y;
  y = string_vertex_y;

  string_draw_angle((void *)api, which, canvas, snapshot, string_ox, string_oy, x, y, update_rect);
}

void string_draw_wrapper(magic_api *api, int which,
                         SDL_Surface *canvas, SDL_Surface *snapshot, int ox,
                         int oy, int x, int y, SDL_Rect *update_rect)
{
  if (which == STRING_TOOL_FULL_BY_OFFSET)
    string_draw_full_by_offset((void *)api, which, canvas, snapshot, x, y, update_rect);
  else if (which == STRING_TOOL_TRIANGLE)
    string_draw_triangle_preview((void *)api, which, canvas, snapshot, ox, oy, x, y, update_rect);
  else if (which == STRING_TOOL_ANGLE)
    string_draw_angle_preview((void *)api, which, canvas, snapshot, ox, oy, x, y, update_rect);
}

void string_set_vertex(int x, int y)
{
  int dx, dy;

  if (string_vertex_done)
    return;
  dx = max(string_ox, x) - min(string_ox, x);
  dy = max(string_oy, y) - min(string_oy, y);
  if (dx + dy > string_vertex_distance)
  {
    string_vertex_distance = dx + dy;
    string_vertex_x = x;
    string_vertex_y = y;
  }
  if (dx + dy + 30 < string_vertex_distance)
    string_vertex_done = 1;
}

void string_drag(magic_api *api, int which,
                 SDL_Surface *canvas, SDL_Surface *snapshot, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  if ((x < canvas->w) && (y < canvas->h) && (ox < canvas->w)
      && (oy < canvas->h) && ((signed)x > 0) && ((signed)y > 0) && ((signed)ox > 0) && ((signed)oy > 0))
  {
    string_set_vertex(x, y);
    string_draw_wrapper((void *)api, which, canvas, snapshot, ox, oy, x, y, update_rect);
    api->playsound(string_snd[which], (x * 255) / canvas->w, 255);

  }
}



Uint8 string_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

Uint8 string_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 0;
}

void string_set_size(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED,
                     SDL_Surface *last ATTRIBUTE_UNUSED,
                     Uint8 size ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
}
