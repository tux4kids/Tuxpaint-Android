/*
  realrainbow.c

  Draws an arc with semi-transparent rainbow colors.

  by Bill Kendrick <bill@newbreedsoftware.com>
  Math assistance by Jeff Newmiller <jdnewmil@dcn.davis.ca.us>

  2009.04.02 - 2014.08.14

FIXME:
* Color/alpha art needs improvement.
* Pixel gaps appear in lines sometimes (esp. larger rainbows).
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "SDL_image.h"

#include "tp_magic_api.h"

Mix_Chunk * realrainbow_snd;
int realrainbow_x1, realrainbow_y1, realrainbow_x2, realrainbow_y2;
SDL_Rect realrainbow_rect;
SDL_Surface * realrainbow_colors[2];
Uint8 realrainbow_blendr, realrainbow_blendg, realrainbow_blendb, realrainbow_blenda;


void realrainbow_arc(magic_api * api, int which, SDL_Surface * canvas, SDL_Surface * last,
                     int x1, int y1, int x2, int y2,
                     int fulldraw, SDL_Rect * update_rect);
static void realrainbow_linecb(void * ptr, int which,
                               SDL_Surface * canvas, SDL_Surface * last,
                               int x, int y);
Uint32 realrainbow_api_version(void);
int realrainbow_init(magic_api * api);
int realrainbow_get_tool_count(magic_api * api);
SDL_Surface * realrainbow_get_icon(magic_api * api, int which);
char * realrainbow_get_name(magic_api * api, int which);
char * realrainbow_get_description(magic_api * api, int which, int mode);
int realrainbow_modes(magic_api * api, int which);
int realrainbow_requires_colors(magic_api * api, int which);
void realrainbow_shutdown(magic_api * api);
void realrainbow_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b);
void realrainbow_click(magic_api * api, int which, int mode,
                       SDL_Surface * canvas, SDL_Surface * last,
                       int x, int y,
                       SDL_Rect * update_rect);
void realrainbow_drag(magic_api * api, int which,
                      SDL_Surface * canvas, SDL_Surface * last,
                      int ox, int oy, int x, int y,
                      SDL_Rect * update_rect);
void realrainbow_release(magic_api * api, int which,
                         SDL_Surface * canvas, SDL_Surface * last,
                         int x, int y,
                         SDL_Rect * update_rect);
void realrainbow_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void realrainbow_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);


Uint32 realrainbow_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}

int realrainbow_init(magic_api * api)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%s/images/magic/realrainbow-colors.png", api->data_directory);
  realrainbow_colors[0] = IMG_Load(fname);
  if (realrainbow_colors[0] == NULL)
    return(0);

  snprintf(fname, sizeof(fname), "%s/images/magic/realrainbow-roygbiv-colors.png", api->data_directory);
  realrainbow_colors[1] = IMG_Load(fname);
  if (realrainbow_colors[1] == NULL)
    return(0);

  snprintf(fname, sizeof(fname), "%s/sounds/magic/realrainbow.ogg",
           api->data_directory);
  realrainbow_snd = Mix_LoadWAV(fname);

  return(1);
}

int realrainbow_get_tool_count(magic_api * api ATTRIBUTE_UNUSED)
{
  return(2);
}

SDL_Surface * realrainbow_get_icon(magic_api * api, int which)
{
  char fname[1024];

  if (which == 0)
    snprintf(fname, sizeof(fname), "%s/images/magic/realrainbow.png",
             api->data_directory);
  else
    snprintf(fname, sizeof(fname), "%s/images/magic/realrainbow-roygbiv.png",
             api->data_directory);

  return(IMG_Load(fname));
}

char * realrainbow_get_name(magic_api * api ATTRIBUTE_UNUSED, int which)
{
  if (which == 0)
    return(strdup(gettext_noop("Real Rainbow")));
  else
    return(strdup(gettext_noop("ROYGBIV Rainbow")));
}

char * realrainbow_get_description(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return(strdup(gettext_noop("Click where you want your rainbow to start, drag to where you want it to end, and then let go to draw a rainbow.")));
}

int realrainbow_modes(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(MODE_PAINT_WITH_PREVIEW);
}

int realrainbow_requires_colors(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return(0);
}

void realrainbow_shutdown(magic_api * api ATTRIBUTE_UNUSED)
{
  if (realrainbow_colors[0] != NULL)
    SDL_FreeSurface(realrainbow_colors[0]);
  if (realrainbow_colors[1] != NULL)
    SDL_FreeSurface(realrainbow_colors[1]);
  if (realrainbow_snd != NULL)
    Mix_FreeChunk(realrainbow_snd);
}

void realrainbow_set_color(magic_api * api ATTRIBUTE_UNUSED, Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED, Uint8 b ATTRIBUTE_UNUSED)
{
}

void realrainbow_click(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                       SDL_Surface * canvas ATTRIBUTE_UNUSED, SDL_Surface * last ATTRIBUTE_UNUSED,
                       int x, int y,
                       SDL_Rect * update_rect ATTRIBUTE_UNUSED)
{
  realrainbow_x1 = x;
  realrainbow_y1 = y;

  realrainbow_rect.x = x;
  realrainbow_rect.y = y;
  realrainbow_rect.w = 1;
  realrainbow_rect.h = 1;
}

void realrainbow_drag(magic_api * api, int which,
                      SDL_Surface * canvas, SDL_Surface * last,
                      int ox ATTRIBUTE_UNUSED, int oy ATTRIBUTE_UNUSED, int x, int y,
                      SDL_Rect * update_rect)
{
  int rx1, ry1, rx2, ry2;
  SDL_Rect rect;

  realrainbow_x2 = x;
  realrainbow_y2 = y;

  SDL_BlitSurface(last, &realrainbow_rect, canvas, &realrainbow_rect);

  realrainbow_arc(api, which, canvas, last, realrainbow_x1, realrainbow_y1, realrainbow_x2, realrainbow_y2, 0, update_rect);

  memcpy(&rect, &realrainbow_rect, sizeof(SDL_Rect));
  memcpy(&realrainbow_rect, update_rect, sizeof(SDL_Rect));

  rx1 = update_rect->x;
  ry1 = update_rect->y;
  rx2 = update_rect->x + update_rect->w;
  ry2 = update_rect->y + update_rect->h;

  if (rect.x < rx1)
    rx1 = rect.x;
  if (rect.x + rect.w > rx2)
    rx2 = rect.x + rect.w;
  if (rect.y < ry1)
    ry1 = rect.y;
  if (rect.y + rect.h > ry2)
    ry2 = rect.y + rect.h;

  update_rect->x = rx1;
  update_rect->y = ry1;
  update_rect->w = rx2 - rx1 + 1;
  update_rect->h = ry2 - ry1 + 1;
}

void realrainbow_release(magic_api * api, int which,
                         SDL_Surface * canvas, SDL_Surface * last,
                         int x, int y,
                         SDL_Rect * update_rect)
{
  int rx1, ry1, rx2, ry2;
  SDL_Rect rect;

  realrainbow_x2 = x;
  realrainbow_y2 = y;

  SDL_BlitSurface(last, &realrainbow_rect, canvas, &realrainbow_rect);

  realrainbow_arc(api, which, canvas, last, realrainbow_x1, realrainbow_y1, realrainbow_x2, realrainbow_y2, 1, update_rect);

  memcpy(&rect, &realrainbow_rect, sizeof(SDL_Rect));
  memcpy(&realrainbow_rect, update_rect, sizeof(SDL_Rect));

  rx1 = update_rect->x;
  ry1 = update_rect->y;
  rx2 = update_rect->x + update_rect->w;
  ry2 = update_rect->y + update_rect->h;

  if (rect.x < rx1)
    rx1 = rect.x;
  if (rect.x + rect.w > rx2)
    rx2 = rect.x + rect.w;
  if (rect.y < ry1)
    ry1 = rect.y;
  if (rect.y + rect.h > ry2)
    ry2 = rect.y + rect.h;

  update_rect->x = rx1;
  update_rect->y = ry1;
  update_rect->w = rx2 - rx1 + 1;
  update_rect->h = ry2 - ry1 + 1;

  api->playsound(realrainbow_snd, 128, 255);
}

void realrainbow_switchin(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}

void realrainbow_switchout(magic_api * api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface * canvas ATTRIBUTE_UNUSED)
{
}


void realrainbow_arc(magic_api * api, int which, SDL_Surface * canvas, SDL_Surface * last, int x1, int y1, int x2, int y2, int fulldraw, SDL_Rect * update_rect)
{
  int lowx, lowy, hix, hiy, xm, ym, xc, yc, r, a1, atan2_a, atan2_b;
  int a, oa, ox, oy, nx, ny, step, thick, rr, done;
  float slope, theta;
  int colorindex;

  if (abs(x2 - x1) < 50)
  {
    if (x2 > x1)
      x2 = x1 + 50;
    else
      x2 = x1 - 50;
  }

  if (y1 == y2)
  {
    xc = x1 + (x2 - x1) / 2;
    yc = y1;
    r = abs(xc - x1);

    a1 = 0;
    theta = -180;
  }
  else
  {
    if (y1 > y2)
    {
      lowx = x1;
      lowy = y1;
      hix = x2;
      hiy = y2;
    }
    else
    {
      lowx = x2;
      lowy = y2;
      hix = x1;
      hiy = y1;
    }

    xm = (lowx + hix) / 2;
    ym = (lowy + hiy) / 2;

    if (hix == lowx)
      return;

    slope = (float)(hiy - lowy) / (float)(hix - lowx);

    yc = lowy;
    xc = slope * (ym - yc) + xm;

    r = abs(xc - lowx);
    atan2_b = hix - xc;
    atan2_a = hiy - yc;
    theta = atan2(atan2_a, atan2_b) * (180.0 / M_PI);

    if (slope > 0)
      a1 = 0;
    else
      a1 = -180;
  }

  if (fulldraw)
  {
    step = 1;
    /* thick = (r / 5); */
  }
  else
  {
    step = 30;
    /* thick = 1; */
  }
  thick = (r / 5);

  if (theta < a1)
    step = -step;
  done = 0;

  oa = a1;

  for (a = (a1 + step); done < 2; a = a + step)
  {
    for (rr = r - (thick / 2); rr <= r + (thick / 2); rr++)
    {
      ox = (rr * cos(oa * M_PI / 180.0)) + xc;
      oy = (rr * sin(oa * M_PI / 180.0)) + yc;

      nx = (rr * cos(a * M_PI / 180.0)) + xc;
      ny = (rr * sin(a * M_PI / 180.0)) + yc;

      colorindex = realrainbow_colors[which]->h - 1 - (((rr - r + (thick / 2)) * realrainbow_colors[which]->h) / thick);

      SDL_GetRGBA(api->getpixel(realrainbow_colors[which], 0, colorindex),
                  realrainbow_colors[which]->format, &realrainbow_blendr, &realrainbow_blendg, &realrainbow_blendb, &realrainbow_blenda);

      if (!fulldraw)
        realrainbow_blenda = 255;

      api->line((void *) api, 0, canvas, last, ox, oy, nx, ny, 1, realrainbow_linecb);
    }

    oa = a;

    if ((step > 0 && a + step > theta) ||
        (step < 0 && a + step < theta))
    {
      done++;
      a = theta - step;
    }
  }

  update_rect->y = yc - r - thick - 2;
  update_rect->h = r + thick * 2 + 4;
  update_rect->x = xc - r - thick;
  update_rect->w = r * 2 + thick * 2;
}

static void realrainbow_linecb(void * ptr, int which ATTRIBUTE_UNUSED,
                               SDL_Surface * canvas, SDL_Surface * last,
                               int x, int y)
{
  magic_api * api = (magic_api *) ptr;
  Uint8 origr, origg, origb;
  Uint8 newr, newg, newb;

  SDL_GetRGB(api->getpixel(last, x, y),
              last->format, &origr, &origg, &origb);

  newr = ((realrainbow_blendr * realrainbow_blenda) / 255) + ((origr * (255 - realrainbow_blenda)) / 255);
  newg = ((realrainbow_blendg * realrainbow_blenda) / 255) + ((origg * (255 - realrainbow_blenda)) / 255);
  newb = ((realrainbow_blendb * realrainbow_blenda) / 255) + ((origb * (255 - realrainbow_blenda)) / 255);

  api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, newr, newg, newb));
}

