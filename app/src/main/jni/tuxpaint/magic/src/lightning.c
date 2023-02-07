/* lightning.c

  Draws a lightning strike between the click
  and drag+release positions.

  Last updated: January 25, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

Mix_Chunk *snd_effect;
float lightning_h, lightning_s, lightning_v;
int sx, sy;


Uint32 lightning_api_version(void);
int lightning_init(magic_api * api);
int lightning_get_tool_count(magic_api * api);
SDL_Surface *lightning_get_icon(magic_api * api, int which);
char *lightning_get_name(magic_api * api, int which);
int lightning_get_group(magic_api * api, int which);
char *lightning_get_description(magic_api * api, int which, int mode);
int lightning_requires_colors(magic_api * api, int which);
int lightning_modes(magic_api * api, int which);
void lightning_shutdown(magic_api * api);
void lightning_click(magic_api * api, int which, int mode,
                     SDL_Surface * canvas, SDL_Surface * snapshot, int x,
                     int y, SDL_Rect * update_rect);
void lightning_set_color(magic_api * api, int which, SDL_Surface * canvas,
                         SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void lightning_drag(magic_api * api, int which, SDL_Surface * canvas,
                    SDL_Surface * snapshot, int ox, int oy, int x, int y,
                    SDL_Rect * update_rect);
void lightning_line_callback_drag(void *ptr, int which, SDL_Surface * canvas,
                                  SDL_Surface * snapshot, int x, int y);
void lightning_draw_bolt(void *ptr, SDL_Surface * canvas,
                         SDL_Surface * snapshot, float sx, float sy,
                         float angle, float len, int thickness);
void lightning_release(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * snapshot, int x, int y,
                       SDL_Rect * update_rect);
void lightning_switchin(magic_api * api, int which, int mode,
                        SDL_Surface * canvas);
void lightning_switchout(magic_api * api, int which, int mode,
                         SDL_Surface * canvas);


Uint32 lightning_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int lightning_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/lightning.ogg",
           api->data_directory);
  snd_effect = Mix_LoadWAV(fname);

  return (1);
}

int lightning_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return (1);
}


SDL_Surface *lightning_get_icon(magic_api * api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/lightning.png",
           api->data_directory);

  return (IMG_Load(fname));
}

char *lightning_get_name(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext("Lightning"));
}

int lightning_get_group(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_ARTISTIC;
}

char *lightning_get_description(magic_api * api ATTRIBUTE_UNUSED,
                                int which ATTRIBUTE_UNUSED,
                                int mode ATTRIBUTE_UNUSED)
{
  return
    strdup(gettext
           ("Click, drag, and release to draw a lightning bolt between two points."));
}

int lightning_requires_colors(magic_api * api ATTRIBUTE_UNUSED,
                              int which ATTRIBUTE_UNUSED)
{
  return 1;
}

int lightning_modes(magic_api * api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}

void lightning_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (snd_effect != NULL)
    Mix_FreeChunk(snd_effect);
}


void
lightning_click(magic_api * api, int which, int mode ATTRIBUTE_UNUSED,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect)
{
  sx = x;
  sy = y;
  lightning_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


void
lightning_drag(magic_api * api, int which, SDL_Surface * canvas,
               SDL_Surface * snapshot, int ox ATTRIBUTE_UNUSED,
               int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect * update_rect)
{
  /* FIXME: This could be made more efficient
     (only blit and update between (sx,sy) and (x,y), though
     it should also cover the area extending to (ox,oy),
     to avoid leaving trails */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  SDL_BlitSurface(snapshot, update_rect, canvas, update_rect);

  api->line((void *) api, which, canvas, snapshot, sx, sy, x, y, 1,
            lightning_line_callback_drag);
}


void
lightning_release(magic_api * api, int which ATTRIBUTE_UNUSED,
                  SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                  SDL_Rect * update_rect)
{
  float a, b, len, angle;
  int thickness;

  /* FIXME: This could be made more efficient
     (only blit and update between (sx,sy) and (x,y), though
     it should also cover the area extending to (ox,oy),
     to avoid leaving trails */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;

  SDL_BlitSurface(snapshot, update_rect, canvas, update_rect);

  api->stopsound();
  api->playsound(snd_effect, (x * 255) / canvas->w, 255);

  a = (x - sx);
  b = (y - sy);

  len = sqrt((a * a) + (b * b));
  if (len < 100)
    len = 100;

  angle = acos((x - sx) / len) * 180.0 / M_PI;
  if (y < sy)
    angle = -angle;

#ifdef DEBUG
  printf
    ("(%d,%d)->(%d,%d) => a = %.2f, b = %.2f, c (len) = %.2f; angle = %.2f degrees\n",
     sx, sy, x, y, a, b, len, angle);
  fflush(stdout);
#endif

  thickness = len / 50;
  if (thickness < 4)
    thickness = 4;

  lightning_draw_bolt((void *) api, canvas, snapshot, (float) sx, (float) sy,
                      angle, len, thickness);
}

void lightning_draw_bolt(void *ptr, SDL_Surface * canvas,
                         SDL_Surface * snapshot, float sx, float sy,
                         float angle, float len, int thickness)
{
  magic_api *api = (magic_api *) ptr;
  float i;
  float x, y, orig_angle;
  int xx, yy, t;
  Uint8 r, g, b;
  float h, s, v, new_h, new_s, new_v;
  float adj;

  x = sx;
  y = sy;

  orig_angle = angle;
  t = thickness / 3;
  if (t < 1)
    t = 1;

  for (i = 0; i < len; i++)
  {
    x = x + cos(angle * M_PI / 180.0);
    y = y + sin(angle * M_PI / 180.0);

    angle = angle + ((float) (rand() % 15) - 7.5);
    if (angle < orig_angle - 10.0)
      angle = orig_angle - 10.0;
    else if (angle > orig_angle + 10.0)
      angle = orig_angle + 10.0;

    for (yy = -t; yy <= t; yy++)
    {
      for (xx = -t; xx <= t; xx++)
      {
        if (api->in_circle(xx, yy, t))
        {
          float light_h, light_s;

          light_h = lightning_h;
          light_s = lightning_s;

          SDL_GetRGB(api->getpixel(canvas, x + xx, y + yy), canvas->format,
                     &r, &g, &b);
          api->rgbtohsv(r, g, b, &h, &s, &v);

          adj = 1.0 - (sqrt((xx * xx) + (yy * yy)) / t);

          new_v = v + adj;
          if (new_v > 1.0)
          {
            light_s = light_s / (new_v * 2);
            new_v = 1.0;
          }

          if (light_h == -1)
          {
            new_h = h;
            new_s = (s * 25) / 100;
          }
          else
          {
            new_h = ((light_h * 75) + (h * 25)) / 100;
            new_s = ((light_s * 75) + (s * 25)) / 100;
          }

          api->hsvtorgb(new_h, new_s, new_v, &r, &g, &b);

          api->putpixel(canvas, x + xx, y + yy,
                        SDL_MapRGB(canvas->format, r, g, b));
        }
      }
    }

    if (((rand() % 50) == 0 || (int) i == (int) (len / 2)) && thickness > 1
        && len >= 4)
    {
      float new_angle;

      if ((rand() % 10) == 0)
      {
        new_angle = angle + ((float) (rand() % 180) - 90.0);
      }
      else
      {
        new_angle = angle + ((float) (rand() % 90) - 45.0);
      }

      lightning_draw_bolt((void *) api, canvas, snapshot, x, y,
                          new_angle,
                          ((len / 8) + (rand() % (int) (len / 4))),
                          thickness - 1);
    }
  }
}


void lightning_set_color(magic_api * api, int which, SDL_Surface * canvas,
                         SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect)
{
  api->rgbtohsv(r, g, b, &lightning_h, &lightning_s, &lightning_v);
}


void lightning_line_callback_drag(void *ptr, int which ATTRIBUTE_UNUSED,
                                  SDL_Surface * canvas,
                                  SDL_Surface * snapshot ATTRIBUTE_UNUSED,
                                  int x, int y)
{
  magic_api *api = (magic_api *) ptr;

  api->xorpixel(canvas, x, y);
}

void lightning_switchin(magic_api * api ATTRIBUTE_UNUSED,
                        int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                        SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void lightning_switchout(magic_api * api ATTRIBUTE_UNUSED,
                         int which ATTRIBUTE_UNUSED,
                         int mode ATTRIBUTE_UNUSED,
                         SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}
