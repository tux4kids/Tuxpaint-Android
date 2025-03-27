/* rivulet.c

   Applys a lense effect like rivulets of water dripping
   down a pane of glass.  Applies an additive brush at
   the mouse pointer, and a subtractive brush slightly
   above (to simulate the water breaking up due to
   evaporation), only allowing the draw path to go downwards,
   and the left/right delta to change slightly (will not
   follow the mouse precisely).  Upon release, the lense
   effect will be applied.

  Last updated: January 25, 2023
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

// #define DEBUG_RADIUS
// #define DEBUG_ALPHA
// #define DEBUG_ANGLE

Mix_Chunk *snd_effect = NULL;
SDL_Surface *rivulet_img_brush_add = NULL,
  *rivulet_img_brush_alpha = NULL, *rivulet_img_brush_sub = NULL, *rivulet_img_angles = NULL;
SDL_Surface *rivulet_snapshot = NULL;
int riv_x, riv_y;
Uint8 *riv_radii = NULL, *riv_alpha = NULL, *riv_angles = NULL;

Uint32 rivulet_api_version(void);
int rivulet_init(magic_api * api);
int rivulet_get_tool_count(magic_api * api);
SDL_Surface *rivulet_get_icon(magic_api * api, int which);
char *rivulet_get_name(magic_api * api, int which);
int rivulet_get_group(magic_api * api, int which);
char *rivulet_get_description(magic_api * api, int which, int mode);
int rivulet_requires_colors(magic_api * api, int which);
int rivulet_modes(magic_api * api, int which);
void rivulet_shutdown(magic_api * api);
void rivulet_click(magic_api * api, int which, int mode,
                   SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void rivulet_set_color(magic_api * api, int which, SDL_Surface * canvas,
                       SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void rivulet_drag(magic_api * api, int which, SDL_Surface * canvas,
                  SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void rivulet_line_callback_drag(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void rivulet_release(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void rivulet_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void rivulet_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
void zero_riv_arrays(SDL_Surface * canvas);


Uint32 rivulet_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int rivulet_init(magic_api *api)
{
  char fname[1024];

  /* FIXME */
//  snprintf(fname, sizeof(fname), "%ssounds/magic/rivulet.ogg",
//           api->data_directory);
//  snd_effect = Mix_LoadWAV(fname);

  /* Load our images */

  snprintf(fname, sizeof(fname), "%simages/magic/rivulet-brush-add.png", api->data_directory);
  rivulet_img_brush_add = IMG_Load(fname);
  if (rivulet_img_brush_add == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  snprintf(fname, sizeof(fname), "%simages/magic/rivulet-brush-alpha.png", api->data_directory);
  rivulet_img_brush_alpha = IMG_Load(fname);
  if (rivulet_img_brush_alpha == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  snprintf(fname, sizeof(fname), "%simages/magic/rivulet-brush-sub.png", api->data_directory);
  rivulet_img_brush_sub = IMG_Load(fname);
  if (rivulet_img_brush_sub == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  snprintf(fname, sizeof(fname), "%simages/magic/rivulet-angles.png", api->data_directory);
  rivulet_img_angles = IMG_Load(fname);
  if (rivulet_img_angles == NULL)
  {
    fprintf(stderr, "Can't open %s\n", fname);
    return 0;
  }

  return (1);
}

int rivulet_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (1);
}


SDL_Surface *rivulet_get_icon(magic_api *api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/rivulet.png", api->data_directory);

  return (IMG_Load(fname));
}

char *rivulet_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext("Rivulet"));
}

int rivulet_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_DISTORTS;
}

char *rivulet_get_description(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return (gettext("Click and drag downward to add water rivulets to your drawing"));
}

int rivulet_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;
}

int rivulet_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MODE_PAINT;
}

void rivulet_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (snd_effect != NULL)
    Mix_FreeChunk(snd_effect);

  if (rivulet_img_brush_add != NULL)
    SDL_FreeSurface(rivulet_img_brush_add);

  if (rivulet_img_brush_alpha != NULL)
    SDL_FreeSurface(rivulet_img_brush_alpha);

  if (rivulet_img_brush_sub != NULL)
    SDL_FreeSurface(rivulet_img_brush_sub);

  if (rivulet_img_angles != NULL)
    SDL_FreeSurface(rivulet_img_angles);

  if (riv_radii != NULL)
    free(riv_radii);

  if (riv_alpha != NULL)
    free(riv_alpha);

  if (riv_angles != NULL)
    free(riv_angles);
}


void
rivulet_click(magic_api *api, int which, int mode ATTRIBUTE_UNUSED,
              SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  riv_x = x;
  riv_y = y - 1;

  if (riv_radii == NULL || rivulet_snapshot == NULL)
    return;

  if (snd_effect != NULL)
  {
    api->stopsound();
    api->playsound(snd_effect, (x * 255) / canvas->w, 255);
  }

  rivulet_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


void
rivulet_drag(magic_api *api ATTRIBUTE_UNUSED, int which, SDL_Surface *canvas,
             SDL_Surface *snapshot, int ox ATTRIBUTE_UNUSED,
             int oy ATTRIBUTE_UNUSED, int x, int y, SDL_Rect *update_rect)
{
  int old_riv_x, old_riv_y;

  if (riv_radii == NULL || rivulet_snapshot == NULL)
    return;

  /* Don't go backwards */
  if (y <= riv_y)
    return;

  /* Don't stray too far left/right */
  if (x < riv_x - 2)
    x = riv_x - 2;
  if (x > riv_x + 2)
    x = riv_x + 2;

  old_riv_x = riv_x;
  old_riv_y = riv_y;

  riv_x = x;
  riv_y = y;

  api->line((void *)api, which, canvas, snapshot, old_riv_x, old_riv_y, riv_x, riv_y, 1, rivulet_line_callback_drag);

  /* FIXME */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void rivulet_release(magic_api *api, int which ATTRIBUTE_UNUSED, SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y,     /* ignored and reused in a for-loop */
                     SDL_Rect *update_rect)
{
  int src_x, src_y, idx;
  double radius, angle_deg, angle_rad;
  Uint8 alpha;
  Uint32 pix;
  Uint8 r, g, b, r1, g1, b1, r2, g2, b2;


  if (riv_radii == NULL || rivulet_snapshot == NULL)
    return;

  /* Undo all of the placeholder drawings */
  SDL_BlitSurface(rivulet_snapshot, NULL, canvas, NULL);

  /* Apply the lense effect */
  for (y = 0; y < canvas->h; y++)
  {
    for (x = 0; x < canvas->w; x++)
    {
      idx = (y * canvas->w) + x;

#if defined(DEBUG_RADIUS) || defined(DEBUG_ALPHA) || defined(DEBUG_ANGLE)
      int v;

#if defined(DEBUG_RADIUS)
      v = riv_radii[idx];
#elif defined(DEBUG_ALPHA)
      v = riv_alpha[idx];
#else
      v = riv_angles[idx];
#endif

      api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, v, 64, 64));
#else

      radius = ((double)riv_radii[idx]);
      alpha = riv_alpha[idx] / 2;

      if (radius != 0.0)
      {
        /* Angle is stored as 0-255 (so 256 would be 360 degrees) */
        angle_deg = ((((double)riv_angles[idx]) / 256.0) * 360.0);

        angle_rad = (angle_deg * M_PI) / 180.0;

        src_x = x + cos(angle_rad) * radius;
        src_y = y - sin(angle_rad) * radius;

        pix = api->getpixel(rivulet_snapshot, x, y);
        SDL_GetRGB(pix, rivulet_snapshot->format, &r1, &g1, &b1);

        pix = api->getpixel(rivulet_snapshot, src_x, src_y);
        SDL_GetRGB(pix, rivulet_snapshot->format, &r2, &g2, &b2);

        r = ((r2 * alpha) / 255) + ((r1 * (255 - alpha) / 255));
        g = ((g2 * alpha) / 255) + ((g1 * (255 - alpha) / 255));
        b = ((b2 * alpha) / 255) + ((b1 * (255 - alpha) / 255));

        if (riv_radii[idx] > 128)
        {
          r = (r + riv_radii[idx]) / 2;
          g = (g + riv_radii[idx]) / 2;
          b = (b + riv_radii[idx]) / 2;
        }

        api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format, r, g, b));
      }
#endif
    }
  }

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void rivulet_set_color(magic_api *api, int which, SDL_Surface *canvas,
                       SDL_Surface *last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect *update_rect)
{
}


void rivulet_line_callback_drag(void *ptr, int which ATTRIBUTE_UNUSED,
                                SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  magic_api *api;
  SDL_Rect dest;
  int w, h, half_w, half_h, idx;
  int src_x, src_y, dest_x, dest_y;
  Uint32 pix;
  Uint8 intensity, tmp;
  int alpha, radius;

  api = (magic_api *) ptr;

  w = rivulet_img_brush_add->w;
  h = rivulet_img_brush_add->h;
  half_w = w / 2;
  half_h = h / 2;


  /* Draw placeholder onto the image
     (no lense effect yet; will do at the very end) */
  dest.x = x - half_w;
  dest.y = y - half_h;
  dest.w = rivulet_img_brush_add->w;
  dest.h = rivulet_img_brush_add->h;

  SDL_BlitSurface(rivulet_img_brush_add, NULL, canvas, &dest);


  /* Adjust the displacement maps */
  for (src_y = 0; src_y <= h; src_y++)
  {
    dest_y = (y - half_h) + src_y;
    if (dest_y >= 0 && dest_y < canvas->h)
    {
      for (src_x = 0; src_x < w; src_x++)
      {
        dest_x = (x - half_w) + src_x;
        if (dest_x >= 0 && dest_x < canvas->w)
        {
          idx = (dest_y * canvas->w) + dest_x;

          /* Add alpha */
          pix = api->getpixel(rivulet_img_brush_alpha, src_x, src_y);
          SDL_GetRGB(pix, rivulet_img_brush_alpha->format, &intensity, &tmp, &tmp);

          alpha = ((int)riv_alpha[idx] + (int)(intensity));
          if (alpha > 255)
            alpha = 255;
          riv_alpha[idx] = (Uint8) alpha;


          /* Apply radius brush to radius displacement map */
          if (alpha > 0)
          {
            pix = api->getpixel(rivulet_img_brush_add, src_x, src_y);
            SDL_GetRGB(pix, rivulet_img_brush_add->format, &intensity, &tmp, &tmp);

            radius = ((int)riv_radii[idx] + (int)intensity - 128);
            if (radius < 0)
              radius = 0;
            if (radius > 255)
              radius = 255;
            riv_radii[idx] = radius;
          }


          /* Apply angle brush to angle displacement map */
          pix = api->getpixel(rivulet_img_angles, src_x, src_y);
          SDL_GetRGB(pix, rivulet_img_angles->format, &intensity, &tmp, &tmp);

          riv_angles[idx] = intensity;
        }
      }
    }
  }
}

void rivulet_switchin(magic_api *api ATTRIBUTE_UNUSED,
                      int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas)
{
  if (riv_radii == NULL)
  {
    riv_radii = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);
    if (riv_radii == NULL)
    {
      fprintf(stderr, "rivulet: Cannot malloc() riv_radii!\n");
      return;
    }

    riv_alpha = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);
    if (riv_alpha == NULL)
    {
      free(riv_radii);
      riv_radii = NULL;

      fprintf(stderr, "rivulet: Cannot malloc() riv_alpha!\n");
      return;
    }

    riv_angles = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);
    if (riv_angles == NULL)
    {
      free(riv_radii);
      riv_radii = NULL;

      free(riv_alpha);
      riv_alpha = NULL;

      fprintf(stderr, "rivulet: Cannot malloc() riv_angles!\n");
      return;
    }
  }

  zero_riv_arrays(canvas);

  if (rivulet_snapshot == NULL)
    rivulet_snapshot =
      SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h,
                           canvas->format->BitsPerPixel,
                           canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, canvas->format->Amask);

  if (rivulet_snapshot != NULL)
    SDL_BlitSurface(canvas, NULL, rivulet_snapshot, NULL);
}

void rivulet_switchout(magic_api *api ATTRIBUTE_UNUSED,
                       int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
  zero_riv_arrays(canvas);
}

void zero_riv_arrays(SDL_Surface *canvas)
{
  if (riv_radii != NULL)
    memset(riv_radii, 0, (canvas->w * canvas->h));
  if (riv_alpha != NULL)
    memset(riv_alpha, 0, (canvas->w * canvas->h));
  if (riv_angles != NULL)
    memset(riv_angles, 0, (canvas->w * canvas->h));
}
