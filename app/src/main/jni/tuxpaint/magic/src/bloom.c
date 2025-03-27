/* bloom.c

   Applies a "bloom" effect to the image.
   (https://en.wikipedia.org/wiki/Bloom_(shader_effect))

   Last updated: January 16, 2024
*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <math.h>

#include "tp_magic_api.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

/* Radius of the painting tool */
static int BLOOM_PAINT_RADIUS = 24;

/* Overall weight to apply the sampled pixels */
#define BLOOM_WEIGHT_CONST 0.05

/* Length of spike shape */
static int BLOOM_SPIKE_LENGTH = 5;

/* From https://www.shadertoy.com/view/lsXGWn */
//float sample_weights[9] = {
//  0.05, 0.09, 0.12, 0.15, 0.16, 0.15, 0.12, 0.09, 0.05
//};

/* Take N digits consecutive digits (1-n), calculate SIN() of
 * (PI/(N+1))*n, take the SQRT(), and scale by a constant
 * so they all SUM() to approx. 1.0000 */
#define NUM_SAMPLE_WEIGHTS 13
float sample_weights[NUM_SAMPLE_WEIGHTS] = {
  0.0449, 0.0627, 0.0752, 0.0842, 0.0904, 0.0940, 0.0952, 0.0940, 0.0904,
  0.0842, 0.0752, 0.0627, 0.0449
};

Mix_Chunk *snd_effects = NULL;
Uint8 *bloom_mask = NULL;
int bloom_scale;

Uint32 bloom_api_version(void);
int bloom_init(magic_api * api, Uint8 disabled_features, Uint8 complexity_level);
int bloom_get_tool_count(magic_api * api);
SDL_Surface *bloom_get_icon(magic_api * api, int which);
char *bloom_get_name(magic_api * api, int which);
int bloom_get_group(magic_api * api, int which);
int bloom_get_order(int which);
char *bloom_get_description(magic_api * api, int which, int mode);
int bloom_requires_colors(magic_api * api, int which);
int bloom_modes(magic_api * api, int which);
void bloom_shutdown(magic_api * api);
void bloom_click(magic_api * api, int which, int mode,
                 SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void bloom_set_color(magic_api * api, int which, SDL_Surface * canvas,
                     SDL_Surface * last, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect);
void bloom_drag(magic_api * api, int which, SDL_Surface * canvas,
                SDL_Surface * snapshot, int ox, int oy, int x, int y, SDL_Rect * update_rect);
void bloom_line_callback_drag(void *ptr, int which, SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y);
void bloom_release(magic_api * api, int which, SDL_Surface * canvas,
                   SDL_Surface * snapshot, int x, int y, SDL_Rect * update_rect);
void bloom_apply_effect(magic_api * api, SDL_Surface * canvas, SDL_Surface * snapshot);
void bloom_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas);
void bloom_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas);
float luminance(float r, float g, float b);
float change_luminance(float c_in, float l_in, float l_out);
Uint8 bloom_accepted_sizes(magic_api * api, int which, int mode);
Uint8 bloom_default_size(magic_api * api, int which, int mode);
void bloom_set_size(magic_api * api, int which, int mode,
                    SDL_Surface * canvas, SDL_Surface * last, Uint8 size, SDL_Rect * update_rect);


Uint32 bloom_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}

int bloom_init(magic_api *api, Uint8 disabled_features ATTRIBUTE_UNUSED, Uint8 complexity_level ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%ssounds/magic/bloom.ogg", api->data_directory);
  snd_effects = Mix_LoadWAV(fname);

  bloom_scale = sqrt(2 * (BLOOM_PAINT_RADIUS * BLOOM_PAINT_RADIUS));

  return (1);
}

int bloom_get_tool_count(magic_api *api ATTRIBUTE_UNUSED)
{
  return (1);
}

SDL_Surface *bloom_get_icon(magic_api *api, int which ATTRIBUTE_UNUSED)
{
  char fname[1024];

  snprintf(fname, sizeof(fname), "%simages/magic/bloom.png", api->data_directory);

  return (IMG_Load(fname));
}

char *bloom_get_name(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return strdup(gettext("Bloom"));
}

int bloom_get_group(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return MAGIC_TYPE_COLOR_FILTERS;
}

int bloom_get_order(int which ATTRIBUTE_UNUSED)
{
  return 900;
}

char *bloom_get_description(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
  {
    return strdup(gettext("Click and drag to apply a glowing \"bloom\" effect to parts of your image."));
  }
  else
  {
    return strdup(gettext("Click to apply a glowing \"bloom\" effect to your entire image."));
  }
}

int bloom_requires_colors(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED)
{
  return 0;                     /* TODO: Maybe some day? */
}

int bloom_modes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED ATTRIBUTE_UNUSED)
{
  return (MODE_PAINT | MODE_FULLSCREEN);
}

void bloom_shutdown(magic_api *api ATTRIBUTE_UNUSED)
{
  if (snd_effects != NULL)
  {
    Mix_FreeChunk(snd_effects);
    snd_effects = NULL;
  }

  if (bloom_mask != NULL)
  {
    free(bloom_mask);
    bloom_mask = NULL;
  }
}

void
bloom_click(magic_api *api, int which, int mode,
            SDL_Surface *canvas, SDL_Surface *snapshot, int x, int y, SDL_Rect *update_rect)
{
  if (bloom_mask == NULL)
    return;

  if (snd_effects != NULL)
    api->stopsound();

  if (mode == MODE_PAINT)
  {
    memset(bloom_mask, 0, (canvas->w * canvas->h));
    bloom_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
  }
  else
  {
    if (snd_effects != NULL)
    {
      api->playsound(snd_effects, (x * 255) / canvas->w, 255);
    }

    memset(bloom_mask, 128, (canvas->w * canvas->h));
    bloom_apply_effect(api, canvas, snapshot);

    update_rect->x = 0;
    update_rect->y = 0;
    update_rect->w = canvas->w;
    update_rect->h = canvas->h;
  }
}


void
bloom_drag(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED,
           SDL_Surface *canvas, SDL_Surface *snapshot, int ox, int oy, int x, int y, SDL_Rect *update_rect)
{
  if (bloom_mask == NULL)
    return;

  api->line((void *)api, which, canvas, snapshot, ox, oy, x, y, 1 /* FIXME: Consider fewer iterations? */ ,
            bloom_line_callback_drag);

  /* FIXME: Would be good to only update the area around the line (ox,oy)->(x,y) (+/- the maxium radius of the effect) */
  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}


void bloom_release(magic_api *api, int which ATTRIBUTE_UNUSED,
                   SDL_Surface *canvas,
                   SDL_Surface *snapshot, int x ATTRIBUTE_UNUSED, int y ATTRIBUTE_UNUSED, SDL_Rect *update_rect)
{
  if (bloom_mask == NULL)
    return;

  if (snd_effects != NULL)
    api->stopsound();

  bloom_apply_effect(api, canvas, snapshot);

  update_rect->x = 0;
  update_rect->y = 0;
  update_rect->w = canvas->w;
  update_rect->h = canvas->h;
}

void bloom_apply_effect(magic_api *api, SDL_Surface *canvas, SDL_Surface *snapshot)
{
  int sample, offset, offset_flip, x, y, xx, yy;
  Uint8 r, g, b;
  float rf, gf, bf, mask_weight, lum;
  float sums[3];
  Uint32 color;

  SDL_BlitSurface(snapshot, NULL, canvas, NULL);

  for (y = 0; y < canvas->h; y++)
  {
    if (y % 10 == 0)
    {
      api->update_progress_bar();
    }

    for (x = 0; x < canvas->w; x++)
    {
      if (bloom_mask[y * canvas->w + x] > 0)
      {
        sums[0] = 0.0;
        sums[1] = 0.0;
        sums[2] = 0.0;

        /* (Pull from snapshot) */
        for (sample = 0; sample < NUM_SAMPLE_WEIGHTS; sample++)
        {
          /* Horizontal samples */
          color = api->getpixel(snapshot, x - ((NUM_SAMPLE_WEIGHTS - 1) / 2) + sample, y);
          SDL_GetRGB(color, snapshot->format, &r, &g, &b);
          sums[0] += r * sample_weights[sample];
          sums[1] += g * sample_weights[sample];
          sums[2] += b * sample_weights[sample];

          /* Vertical samples */
          color = api->getpixel(snapshot, x, y - ((NUM_SAMPLE_WEIGHTS - 1) / 2) + sample);
          SDL_GetRGB(color, snapshot->format, &r, &g, &b);
          sums[0] += r * sample_weights[sample];
          sums[1] += g * sample_weights[sample];
          sums[2] += b * sample_weights[sample];
        }

        /* (Blend an "X" shape, additively, onto target canvas) */
        for (offset = -BLOOM_SPIKE_LENGTH; offset <= BLOOM_SPIKE_LENGTH; offset++)
        {
          for (offset_flip = -1; offset <= 1; offset += 2)
          {
            xx = x + offset;
            yy = y + (offset * offset_flip);

            if (xx >= 0 && xx < canvas->w && yy >= 0 && yy < canvas->h)
            {
              color = api->getpixel(snapshot, xx, yy);
              SDL_GetRGB(color, snapshot->format, &r, &g, &b);

              mask_weight = (float)(bloom_mask[(yy) * canvas->w + xx] / 255.0);
              mask_weight *= BLOOM_WEIGHT_CONST;
              mask_weight *= ((BLOOM_SPIKE_LENGTH + 1) - (abs(offset)) / BLOOM_SPIKE_LENGTH);

              rf = (((float)r) + (sums[0] * mask_weight)) / 255.0;
              gf = (((float)g) + (sums[1] * mask_weight)) / 255.0;
              bf = (((float)b) + (sums[2] * mask_weight)) / 255.0;

              /* Reinhard Tonemap (Luminence) */
              lum = luminance(rf, gf, bf);

              if (lum > 0.0)
              {
                float numerator = lum * (1.0f + lum);
                float l_new = numerator / (1.0f + lum);

                rf = change_luminance(rf, lum, l_new);
                gf = change_luminance(gf, lum, l_new);
                bf = change_luminance(bf, lum, l_new);
              }

              /* Clamp */
              if (rf > 1.0)
                rf = 1.0;
              if (gf > 1.0)
                gf = 1.0;
              if (bf > 1.0)
                bf = 1.0;

              rf *= 255.0;
              gf *= 255.0;
              bf *= 255.0;

              api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, (Uint8) rf, (Uint8) gf, (Uint8) bf));
            }
          }
        }
      }
    }
  }
}


void bloom_set_color(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED ATTRIBUTE_UNUSED,
                     SDL_Surface *canvas ATTRIBUTE_UNUSED,
                     SDL_Surface *last ATTRIBUTE_UNUSED,
                     Uint8 r ATTRIBUTE_UNUSED, Uint8 g ATTRIBUTE_UNUSED,
                     Uint8 b ATTRIBUTE_UNUSED, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  /* TODO: Maybe some day? */
}


void bloom_line_callback_drag(void *ptr, int which ATTRIBUTE_UNUSED,
                              SDL_Surface *canvas, SDL_Surface *snapshot ATTRIBUTE_UNUSED, int x, int y)
{
  int xrad, yrad, xx, yy, chg, n;
  magic_api *api = (magic_api *) ptr;

  if (snd_effects != NULL)
    api->playsound(snd_effects, (x * 255) / canvas->w, 255);

  for (yrad = -BLOOM_PAINT_RADIUS; yrad < BLOOM_PAINT_RADIUS; yrad++)
  {
    yy = y + yrad;
    if (yy >= 0 && yy < canvas->h)
    {
      for (xrad = -BLOOM_PAINT_RADIUS; xrad < BLOOM_PAINT_RADIUS; xrad++)
      {
        xx = x + xrad;
        if (xx >= 0 && xx < canvas->w)
        {
          if (api->in_circle(xrad, yrad, BLOOM_PAINT_RADIUS))
          {
            /* Add to the bloom mask */
            n = (int)bloom_mask[yy * canvas->w + xx];
            chg = sqrt(bloom_scale - sqrt((xrad * xrad) + (yrad * yrad)));
            n += chg;
            if (n > 255)
            {
              n = 255;
            }
            bloom_mask[yy * canvas->w + xx] = (Uint8) n;

            /* Draw on the canvas temporarily */
            api->putpixel(canvas, xx, yy, SDL_MapRGB(canvas->format, n, n, n));
          }
        }
      }
    }
  }
}


void bloom_switchin(magic_api *api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED ATTRIBUTE_UNUSED, int mode, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
  if (bloom_mask == NULL)
    bloom_mask = (Uint8 *) malloc(sizeof(Uint8) * canvas->w * canvas->h);

  if (mode == MODE_FULLSCREEN)
    bloom_set_size(api, which, mode, NULL, NULL, bloom_default_size(api, which, mode), NULL);
}

void bloom_switchout(magic_api *api ATTRIBUTE_UNUSED,
                     int which ATTRIBUTE_UNUSED ATTRIBUTE_UNUSED,
                     int mode ATTRIBUTE_UNUSED, SDL_Surface *canvas ATTRIBUTE_UNUSED)
{
}

float luminance(float r, float g, float b)
{
  return (r * 0.2126) + (g * 0.7152) + (b * 0.0722);
}

float change_luminance(float c_in, float l_in, float l_out)
{
  return c_in * (l_out / l_in);
}


Uint8 bloom_accepted_sizes(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode)
{
  if (mode == MODE_PAINT)
    return 4;
  else
    return 0;
}

Uint8 bloom_default_size(magic_api *api ATTRIBUTE_UNUSED, int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED)
{
  return 2;
}

void bloom_set_size(magic_api *api ATTRIBUTE_UNUSED,
                    int which ATTRIBUTE_UNUSED, int mode ATTRIBUTE_UNUSED,
                    SDL_Surface *canvas ATTRIBUTE_UNUSED,
                    SDL_Surface *last ATTRIBUTE_UNUSED, Uint8 size, SDL_Rect *update_rect ATTRIBUTE_UNUSED)
{
  BLOOM_PAINT_RADIUS = size * 12;
  BLOOM_SPIKE_LENGTH = sqrt(BLOOM_PAINT_RADIUS + 1);
  bloom_scale = sqrt(2 * (BLOOM_PAINT_RADIUS * BLOOM_PAINT_RADIUS));
}
