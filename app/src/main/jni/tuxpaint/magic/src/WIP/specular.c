/* specular.c

   A specular reflection Magic tool for Tux Paint
   by Bill Kendrick <bill@newbreedsoftware.com>

   Last updated: March 2, 2024
*/


/* Inclusion of header files */
/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Our global variables: */
/* ---------------------------------------------------------------------- */

/* Sound effects: */
Mix_Chunk *snd_effect;

/* The size the user has selected in Tux Paint: */
Uint8 specular_size = 32;


void specular_drag(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * snapshot, int old_x, int old_y, int x, int y, SDL_Rect * update_rect);

void specular_line_callback(void *pointer, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);


Uint32 specular_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int specular_init(magic_api *api, Uint8 disabled_features, Uint8 complexity_level)
{
  int i;
  char filename[1024];

  snprintf(filename, sizeof(filename), "%ssounds/magic/%s", api->data_directory, "reflection.ogg");     // FIXME
  snd_effect = Mix_LoadWAV(filename);

  return (1);
}


int specular_get_tool_count(magic_api *api)
{
  return (1);
}

SDL_Surface *specular_get_icon(magic_api *api, int which)
{
  char filename[1024];

  snprintf(filename, sizeof(filename), "%simages/magic/%s", api->data_directory, "reflection.png");     // FIXME

  return (IMG_Load(filename));
}


char *specular_get_name(magic_api *api, int which)
{
  return strdup(gettext("Specular Reflection"));
}


int specular_get_group(magic_api *api, int which)
{
  return MAGIC_TYPE_PAINTING;
}


int specular_get_order(int which)
{
  return 1710;
}


char *specular_get_description(magic_api *api, int which, int mode)
{
  return
    strdup(gettext
           ("Click and drag on the bottom half of your picture to draw a specular reflection — like a puddle, pond, or lake — that mirrors the top half of your picture."));
}


int specular_requires_colors(magic_api *api, int which)
{
  return 0;
}


int specular_modes(magic_api *api, int which)
{
  return MODE_PAINT;
}


Uint8 specular_accepted_sizes(magic_api *api, int which, int mode)
{
  return 4;
}


Uint8 specular_default_size(magic_api *api, int which, int mode)
{
  return 2;
}


void specular_shutdown(magic_api *api)
{
  if (snd_effect != NULL)
  {
    Mix_FreeChunk(snd_effect);
  }
}


void
specular_click(magic_api *api, int which, int mode,
               SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  specular_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


void
specular_drag(magic_api *api, int which,
              SDL_Surface *canvas, SDL_Surface *snapshot, int old_x, int old_y, int x, int y, SDL_Rect *update_rect)
{
  SDL_LockSurface(snapshot);
  SDL_LockSurface(canvas);

  api->line((void *)api, which, canvas, snapshot, old_x, old_y, x, y, 1, specular_line_callback);

  SDL_UnlockSurface(canvas);
  SDL_UnlockSurface(snapshot);

  if (old_x > x)
  {
    int temp = old_x;

    old_x = x;
    x = temp;
  }
  if (old_y > y)
  {
    int temp = old_y;

    old_y = y;
    y = temp;
  }

  update_rect->x = old_x - specular_size;
  update_rect->y = old_y - specular_size;
  update_rect->w = (x + specular_size) - update_rect->x + 1;
  update_rect->h = (y + specular_size) - update_rect->y + 1;

  api->playsound(snd_effect, (x * 255) / canvas->w, 255);
}

void
specular_release(magic_api *api, int which,
                 SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
}

void specular_set_color(magic_api *api, int which, SDL_Surface *canvas, SDL_Surface *snapshot, Uint8 r, Uint8 g,
                        Uint8 b, SDL_Rect *update_rect)
{
}

void specular_set_size(magic_api *api, int which, int mode, SDL_Surface *canvas, SDL_Surface *snapshot, Uint8 size,
                       SDL_Rect *update_rect)
{
  specular_size = size * 16;
}


void specular_line_callback(void *pointer, int which, SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y)
{
  magic_api *api = (magic_api *) pointer;
  int xx, yy, ysrc;
  Uint8 r, g, b, r1, g1, b1, r2, g2, b2, fade;

  for (yy = -specular_size / 16; yy < specular_size / 16; yy++)
  {
    ysrc = (snapshot->h - y - yy) + sin(y - yy) * 4;

    for (xx = -specular_size; xx < specular_size; xx++)
    {
      if (api->in_circle(xx, yy * 16, specular_size))
      {
        SDL_GetRGB(api->getpixel(snapshot, x + xx + sin((y + yy) / 4) * 4, ysrc), snapshot->format, &r1, &g1, &b1);
        SDL_GetRGB(api->getpixel(snapshot, x + xx + sin((y + yy) / 4) * 4 + 1, ysrc), snapshot->format, &r2, &g2, &b2);

        fade = (((y + yy) * 255) / canvas->h);
        r = (((r1 + r2) * 2) + fade) / 5;
        g = (((g1 + g2) * 2) + fade) / 5;
        b = (((b1 + b2) * 2) + fade) / 5;

        api->putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format, r, g, b));
      }
    }
  }
}

void specular_switchin(magic_api *api, int which, int mode, SDL_Surface *canvas)
{
}

void specular_switchout(magic_api *api, int which, int mode, SDL_Surface *canvas)
{
}
