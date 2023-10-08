/* -*- mode: C; c-file-style: "linux" -*- */
/* 
 * GdkPixbuf library - TGA image loader
 * Copyright (C) 1999 Nicola Girardi <nikke@swlibero.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Some NOTES about the TGA loader (2001/06/07, nikke@swlibero.org)
 *
 * - The TGAFooter isn't present in all TGA files.  In fact, there's an older
 *   format specification, still in use, which doesn't cover the TGAFooter.
 *   Actually, most TGA files I have are of the older type.  Anyway I put the 
 *   struct declaration here for completeness.
 *
 * - Error handling was designed to be very paranoid.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include "gdk-pixbuf-core.h"
#include "gdk-pixbuf-io.h"
#include "gdk-pixbuf-buffer-queue-private.h"

#undef DEBUG_TGA

#define TGA_INTERLEAVE_MASK     0xc0
#define TGA_INTERLEAVE_NONE     0x00
#define TGA_INTERLEAVE_2WAY     0x40
#define TGA_INTERLEAVE_4WAY     0x80

#define TGA_ORIGIN_MASK         0x30
#define TGA_ORIGIN_RIGHT        0x10
#define TGA_ORIGIN_UPPER        0x20

enum {
	TGA_TYPE_NODATA = 0,
	TGA_TYPE_PSEUDOCOLOR = 1,
	TGA_TYPE_TRUECOLOR = 2,
	TGA_TYPE_GRAYSCALE = 3,
	TGA_TYPE_RLE_PSEUDOCOLOR = 9,
	TGA_TYPE_RLE_TRUECOLOR = 10,
	TGA_TYPE_RLE_GRAYSCALE = 11
};

#define LE16(p) ((p)[0] + ((p)[1] << 8))

typedef struct _TGAHeader TGAHeader;
typedef struct _TGAFooter TGAFooter;

typedef struct _TGAColor TGAColor;
typedef struct _TGAColormap TGAColormap;

typedef struct _TGAContext TGAContext;

struct _TGAHeader {
	guint8 infolen;
	guint8 has_cmap;
	guint8 type;
	
	guint8 cmap_start[2];
	guint8 cmap_n_colors[2];
	guint8 cmap_bpp;
	
	guint8 x_origin[2];
	guint8 y_origin[2];
	
	guint8 width[2];
	guint8 height[2];
	guint8 bpp;
	
	guint8 flags;
};

struct _TGAFooter {
	guint32 extension_area_offset;
	guint32 developer_directory_offset;

	/* Standard TGA signature, "TRUEVISION-XFILE.\0". */
	union {
		gchar sig_full[18];
		struct {
			gchar sig_chunk[16];
			gchar dot, null;
		} sig_struct;
	} sig;
};

struct _TGAColor {
	guchar r, g, b, a;
};

struct _TGAColormap {
	guint n_colors;
	TGAColor colors[1];
};

typedef gboolean (* TGAProcessFunc) (TGAContext *ctx, GError **error);

struct _TGAContext {
	TGAHeader *hdr;

	TGAColormap *cmap;
	guint cmap_size;

	GdkPixbuf *pbuf;
	int pbuf_x;
	int pbuf_y;
	int pbuf_y_notified;

	GdkPixbufBufferQueue *input;

        TGAProcessFunc process;

	GdkPixbufModuleSizeFunc size_func;
	GdkPixbufModulePreparedFunc prepared_func;
	GdkPixbufModuleUpdatedFunc updated_func;
	gpointer user_data;
};

static TGAColormap *
colormap_new (guint n_colors)
{
  TGAColormap *cmap;

  g_assert (n_colors <= G_MAXUINT16);

  cmap = g_try_malloc0 (sizeof (TGAColormap) + (MAX (n_colors, 1) - 1) * sizeof (TGAColor));
  if (cmap == NULL)
    return NULL;

  cmap->n_colors = n_colors;

  return cmap;
}

static const TGAColor *
colormap_get_color (TGAColormap *cmap,
                    guint        id)
{
  static const TGAColor transparent_black = { 0, 0, 0, 0 };

  if (id >= cmap->n_colors)
    return &transparent_black;

  return &cmap->colors[id];
}

static void
colormap_set_color (TGAColormap    *cmap,
                    guint           id,
                    const TGAColor *color)
{
  if (id >= cmap->n_colors)
    return;

  cmap->colors[id] = *color;
}

static void
colormap_free (TGAColormap *cmap)
{
  g_free (cmap);
}

static gboolean
tga_skip_rest_of_image (TGAContext  *ctx,
                        GError     **err)
{
  gdk_pixbuf_buffer_queue_flush (ctx->input, gdk_pixbuf_buffer_queue_get_size (ctx->input));

  return TRUE;
}

static inline void
tga_write_pixel (TGAContext     *ctx,
                 const TGAColor *color)
{
  gint width = gdk_pixbuf_get_width (ctx->pbuf);
  gint height = gdk_pixbuf_get_height (ctx->pbuf);
  gint rowstride = gdk_pixbuf_get_rowstride (ctx->pbuf);
  gint n_channels = gdk_pixbuf_get_n_channels (ctx->pbuf);

  guint x = (ctx->hdr->flags & TGA_ORIGIN_RIGHT) ? width - ctx->pbuf_x - 1 : ctx->pbuf_x;
  guint y = (ctx->hdr->flags & TGA_ORIGIN_UPPER) ? ctx->pbuf_y : height - ctx->pbuf_y - 1;

  memcpy (gdk_pixbuf_get_pixels (ctx->pbuf) + y * rowstride + x * n_channels, color, n_channels);

  ctx->pbuf_x++;
  if (ctx->pbuf_x >= width)
    {
      ctx->pbuf_x = 0;
      ctx->pbuf_y++;
    }
}

static gsize
tga_pixels_remaining (TGAContext *ctx)
{
  gint width = gdk_pixbuf_get_width (ctx->pbuf);
  gint height = gdk_pixbuf_get_height (ctx->pbuf);

  return width * (height - ctx->pbuf_y) - ctx->pbuf_x;
}

static gboolean
tga_all_pixels_written (TGAContext *ctx)
{
  gint height = gdk_pixbuf_get_height (ctx->pbuf);

  return ctx->pbuf_y >= height;
}

static void
tga_emit_update (TGAContext *ctx)
{
  gint width = gdk_pixbuf_get_width (ctx->pbuf);
  gint height = gdk_pixbuf_get_height (ctx->pbuf);

  /* We only notify row-by-row for now.
   * I was too lazy to handle line-breaks.
   */
  if (ctx->pbuf_y_notified == ctx->pbuf_y)
    return;

  if (ctx->hdr->flags & TGA_ORIGIN_UPPER)
    (*ctx->updated_func) (ctx->pbuf,
                   0, ctx->pbuf_y_notified,
                   width, ctx->pbuf_y - ctx->pbuf_y_notified,
                   ctx->user_data);
  else
    (*ctx->updated_func) (ctx->pbuf,
                   0, height - ctx->pbuf_y,
                   width, ctx->pbuf_y - ctx->pbuf_y_notified,
                   ctx->user_data);

  ctx->pbuf_y_notified = ctx->pbuf_y;
}

static gboolean
tga_format_supported (guint type,
                      guint bits_per_pixel)
{
  switch (type)
    {
      case TGA_TYPE_PSEUDOCOLOR:
      case TGA_TYPE_RLE_PSEUDOCOLOR:
        return bits_per_pixel == 8;

      case TGA_TYPE_TRUECOLOR:
      case TGA_TYPE_RLE_TRUECOLOR:
        return bits_per_pixel == 16
            || bits_per_pixel == 24
            || bits_per_pixel == 32;

      case TGA_TYPE_GRAYSCALE:
      case TGA_TYPE_RLE_GRAYSCALE:
        return bits_per_pixel == 8
            || bits_per_pixel == 16;

      default:
        return FALSE;
    }
}

static inline void
tga_read_pixel (TGAContext   *ctx,
                const guchar *data,
                TGAColor     *color)
{
  switch (ctx->hdr->type)
    {
      case TGA_TYPE_PSEUDOCOLOR:
      case TGA_TYPE_RLE_PSEUDOCOLOR:
        *color = *colormap_get_color (ctx->cmap, data[0]);
        break;

      case TGA_TYPE_TRUECOLOR:
      case TGA_TYPE_RLE_TRUECOLOR:
        if (ctx->hdr->bpp == 16)
          {
            guint16 col = data[0] + (data[1] << 8);
            color->r = (col >> 7) & 0xf8;
            color->r |= color->r >> 5;
            color->g = (col >> 2) & 0xf8;
            color->g |= color->g >> 5;
            color->b = col << 3;
            color->b |= color->b >> 5;
            color->a = 255;
          }
        else
          {
            color->b = data[0];
            color->g = data[1];
            color->r = data[2];
            if (ctx->hdr->bpp == 32)
              color->a = data[3];
            else
              color->a = 255;
          }
        break;

      case TGA_TYPE_GRAYSCALE:
      case TGA_TYPE_RLE_GRAYSCALE:
        color->r = color->g = color->b = data[0];
        if (ctx->hdr->bpp == 16)
          color->a = data[1];
        else
          color->a = 255;
        break;

      default:
        g_assert_not_reached ();
    }
}

static gboolean fill_in_context(TGAContext *ctx, GError **err)
{
	gboolean alpha;
	guint w, h;

	g_return_val_if_fail(ctx != NULL, FALSE);

        ctx->cmap_size = ((ctx->hdr->cmap_bpp + 7) >> 3) *
                LE16(ctx->hdr->cmap_n_colors);
        ctx->cmap = colormap_new (LE16(ctx->hdr->cmap_n_colors));
	if (!ctx->cmap) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate colormap"));
		return FALSE;
	}

	alpha = ((ctx->hdr->bpp == 16) || 
		 (ctx->hdr->bpp == 32) ||
		 (ctx->hdr->has_cmap && (ctx->hdr->cmap_bpp == 32)));

	w = LE16(ctx->hdr->width);
	h = LE16(ctx->hdr->height);

	{
		gint wi = w;
		gint hi = h;

		(*ctx->size_func) (&wi, &hi, ctx->user_data);

		if (wi == 0 || hi == 0)
			return FALSE;
	}

	ctx->pbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, alpha, 8, w, h);

	if (!ctx->pbuf) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate new pixbuf"));
		return FALSE;
	}

	return TRUE;
}

static gboolean
tga_load_image (TGAContext  *ctx,
                GError     **err)
{
  TGAColor color;
  GBytes *bytes;
  gsize i, size, bytes_per_pixel;
  const guchar *data;

  bytes_per_pixel = (ctx->hdr->bpp + 7) / 8;
  size = gdk_pixbuf_buffer_queue_get_size (ctx->input) / bytes_per_pixel;
  size = MIN (size, tga_pixels_remaining (ctx));

  bytes = gdk_pixbuf_buffer_queue_pull (ctx->input, size * bytes_per_pixel);
  g_assert (bytes != NULL);

  data = g_bytes_get_data (bytes, NULL);

  for (i = 0; i < size; i++)
    {
      tga_read_pixel (ctx, data, &color);
      tga_write_pixel (ctx, &color);
      data += bytes_per_pixel;
    }

  g_bytes_unref (bytes);

  tga_emit_update (ctx);
  
  if (tga_all_pixels_written (ctx))
    ctx->process = tga_skip_rest_of_image;
  return TRUE;
}

static gboolean
tga_load_rle_image (TGAContext  *ctx,
                    GError     **err)
{
        GBytes *bytes;
	TGAColor color;
	guint rle_num, raw_num;
	const guchar *s;
        guchar tag;
	gsize n, size, bytes_per_pixel;

        bytes_per_pixel = (ctx->hdr->bpp + 7) / 8;
        bytes = gdk_pixbuf_buffer_queue_peek (ctx->input, gdk_pixbuf_buffer_queue_get_size (ctx->input));
	s = g_bytes_get_data (bytes, &size);

	for (n = 0; n < size; ) {
		tag = *s;
		s++, n++;
		if (tag & 0x80) {
			if (n + bytes_per_pixel > size) {
				--n;
                                break;
			} else {
				rle_num = (tag & 0x7f) + 1;
                                tga_read_pixel (ctx, s, &color);
				s += bytes_per_pixel;
				n += bytes_per_pixel;
                                rle_num = MIN (rle_num, tga_pixels_remaining (ctx));
                                for (; rle_num; rle_num--)
                                  {
                                    tga_write_pixel (ctx, &color);
                                  }
	                        if (tga_all_pixels_written (ctx))
                                        break;
			}
		} else {
			raw_num = tag + 1;
			if (n + (raw_num * bytes_per_pixel) > size) {
			        --n;
                                break;
			} else {
                                raw_num = MIN (raw_num, tga_pixels_remaining (ctx));
				for (; raw_num; raw_num--) {
                                        tga_read_pixel (ctx, s, &color);
					s += bytes_per_pixel;
					n += bytes_per_pixel;
                                        tga_write_pixel (ctx, &color);
				}
				
	                        if (tga_all_pixels_written (ctx))
                                        break;
			}
		}
	}

        g_bytes_unref (bytes);
        gdk_pixbuf_buffer_queue_flush (ctx->input, n);

        tga_emit_update (ctx);

	if (tga_all_pixels_written (ctx))
                ctx->process = tga_skip_rest_of_image;
        return TRUE;
}

static gboolean
tga_load_colormap (TGAContext  *ctx,
                   GError     **err)
{
  GBytes *bytes;
  TGAColor color;
  const guchar *p;
  guint i, n_colors;

  if (ctx->hdr->has_cmap)
    {
      bytes = gdk_pixbuf_buffer_queue_pull (ctx->input, ctx->cmap_size);
      if (bytes == NULL)
        return TRUE;

      n_colors = LE16(ctx->hdr->cmap_n_colors);

      p = g_bytes_get_data (bytes, NULL);
      color.a = 255;

      for (i = 0; i < n_colors; i++)
        {
          if ((ctx->hdr->cmap_bpp == 15) || (ctx->hdr->cmap_bpp == 16))
            {
              guint16 col = p[0] + (p[1] << 8);
              color.b = (col >> 7) & 0xf8;
              color.g = (col >> 2) & 0xf8;
              color.r = col << 3;
              p += 2;
            }
          else if ((ctx->hdr->cmap_bpp == 24) || (ctx->hdr->cmap_bpp == 32))
            {
              color.b = *p++;
              color.g = *p++;
              color.r = *p++;
              if (ctx->hdr->cmap_bpp == 32)
                color.a = *p++;
            }
          else
            {
              g_set_error_literal (err, GDK_PIXBUF_ERROR, 
                                   GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                   _("Unexpected bitdepth for colormap entries"));
              g_bytes_unref (bytes);
              return FALSE;
            }
          colormap_set_color (ctx->cmap, i, &color);
        }

      g_bytes_unref (bytes);
    }
  else
    {
      if ((ctx->hdr->type == TGA_TYPE_PSEUDOCOLOR)
          || (ctx->hdr->type == TGA_TYPE_RLE_PSEUDOCOLOR))
        {
          g_set_error_literal (err, GDK_PIXBUF_ERROR, 
                               GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                               _("Pseudocolor image does not contain a colormap"));
          return FALSE;
        }
    }
  
  if ((ctx->hdr->type == TGA_TYPE_RLE_PSEUDOCOLOR)
      || (ctx->hdr->type == TGA_TYPE_RLE_TRUECOLOR)
      || (ctx->hdr->type == TGA_TYPE_RLE_GRAYSCALE))
    ctx->process = tga_load_rle_image;
  else
    ctx->process = tga_load_image;

  return TRUE;
}

static gboolean
tga_read_info (TGAContext  *ctx,
               GError     **err)
{
  if (gdk_pixbuf_buffer_queue_get_size (ctx->input) < ctx->hdr->infolen)
    return TRUE;
  
  gdk_pixbuf_buffer_queue_flush (ctx->input, ctx->hdr->infolen);

  ctx->process = tga_load_colormap;
  return TRUE;
}

static gboolean
tga_load_header (TGAContext  *ctx,
                 GError     **err)
{
  GBytes *bytes;
  
  bytes = gdk_pixbuf_buffer_queue_pull (ctx->input, sizeof (TGAHeader));
  if (bytes == NULL)
    return TRUE;

  ctx->hdr = g_try_malloc (sizeof (TGAHeader));
  if (!ctx->hdr)
    {
      g_set_error_literal (err, GDK_PIXBUF_ERROR,
                           GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                           _("Cannot allocate TGA header memory"));
      return FALSE;
  }
  memmove(ctx->hdr, g_bytes_get_data (bytes, NULL), sizeof(TGAHeader));
  g_bytes_unref (bytes);
#ifdef DEBUG_TGA
  g_print ("infolen %d "
           "has_cmap %d "
           "type %d "
           "cmap_start %d "
           "cmap_n_colors %d "
           "cmap_bpp %d "
           "x %d y %d width %d height %d bpp %d "
           "flags %#x",
           ctx->hdr->infolen,
           ctx->hdr->has_cmap,
           ctx->hdr->type,
           LE16(ctx->hdr->cmap_start),
           LE16(ctx->hdr->cmap_n_colors),
           ctx->hdr->cmap_bpp,
           LE16(ctx->hdr->x_origin),
           LE16(ctx->hdr->y_origin),
           LE16(ctx->hdr->width),
           LE16(ctx->hdr->height),
           ctx->hdr->bpp,
           ctx->hdr->flags);
#endif
  if (LE16(ctx->hdr->width) == 0 || 
      LE16(ctx->hdr->height) == 0) {
          g_set_error_literal(err, GDK_PIXBUF_ERROR,
                              GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                              _("TGA image has invalid dimensions"));
          return FALSE;
  }
  if ((ctx->hdr->flags & TGA_INTERLEAVE_MASK) != TGA_INTERLEAVE_NONE) {
          g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                              GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                              _("TGA image type not supported"));
          return FALSE;
  }
  if (!tga_format_supported (ctx->hdr->type, ctx->hdr->bpp))
    {
      g_set_error_literal(err, GDK_PIXBUF_ERROR,
                          GDK_PIXBUF_ERROR_UNKNOWN_TYPE,
                          _("TGA image type not supported"));
      return FALSE;
    }

  if (!fill_in_context(ctx, err))
          return FALSE;

  (*ctx->prepared_func) (ctx->pbuf, NULL, ctx->user_data);

  ctx->process = tga_read_info;
  return TRUE;
}

static gpointer gdk_pixbuf__tga_begin_load(GdkPixbufModuleSizeFunc size_func,
                                           GdkPixbufModulePreparedFunc prepared_func,
					   GdkPixbufModuleUpdatedFunc updated_func,
					   gpointer user_data, GError **err)
{
	TGAContext *ctx;

	g_assert (size_func != NULL);
	g_assert (prepared_func != NULL);
	g_assert (updated_func != NULL);

	ctx = g_try_malloc(sizeof(TGAContext));
	if (!ctx) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, 
                                    GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate memory for TGA context struct"));
		return NULL;
	}

	ctx->hdr = NULL;

	ctx->cmap = NULL;
	ctx->cmap_size = 0;

	ctx->pbuf = NULL;
        ctx->pbuf_x = 0;
        ctx->pbuf_y = 0;
        ctx->pbuf_y_notified = 0;

	ctx->input = gdk_pixbuf_buffer_queue_new ();

        ctx->process = tga_load_header;

	ctx->size_func     = size_func;
	ctx->prepared_func = prepared_func;
	ctx->updated_func  = updated_func;

	ctx->user_data = user_data;

	return ctx;
}

static gboolean gdk_pixbuf__tga_load_increment(gpointer data,
					       const guchar *buffer,
					       guint size,
					       GError **err)
{
	TGAContext *ctx = (TGAContext*) data;
        TGAProcessFunc process;

	g_return_val_if_fail(buffer != NULL, TRUE);
        gdk_pixbuf_buffer_queue_push (ctx->input, g_bytes_new (buffer, size));

        do
          {
            process = ctx->process;

            if (!process (ctx, err))
              return FALSE;
          }
        while (process != ctx->process);

	return TRUE;
}

static gboolean gdk_pixbuf__tga_stop_load(gpointer data, GError **err)
{
	TGAContext *ctx = (TGAContext *) data;
        gboolean result = TRUE;

	g_return_val_if_fail (ctx != NULL, FALSE);

        if (ctx->pbuf == NULL || tga_pixels_remaining (ctx))
          {
            g_set_error_literal (err,
                                 GDK_PIXBUF_ERROR,
                                 GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                 _("TGA image was truncated or incomplete."));

            result = FALSE;
          }

	g_free (ctx->hdr);
	if (ctx->cmap)
          colormap_free (ctx->cmap);
	if (ctx->pbuf)
          g_object_unref (ctx->pbuf);
	gdk_pixbuf_buffer_queue_unref (ctx->input);
	g_free (ctx);

	return result;
}

#ifndef INCLUDE_tga
#define MODULE_ENTRY(function) G_MODULE_EXPORT void function
#else
#define MODULE_ENTRY(function) void _gdk_pixbuf__tga_ ## function
#endif

MODULE_ENTRY (fill_vtable) (GdkPixbufModule *module)
{
	module->begin_load = gdk_pixbuf__tga_begin_load;
	module->stop_load = gdk_pixbuf__tga_stop_load;
	module->load_increment = gdk_pixbuf__tga_load_increment;
}

MODULE_ENTRY (fill_info) (GdkPixbufFormat *info)
{
	static const GdkPixbufModulePattern signature[] = {
		{ " \x1\x1", "x  ", 100 },
		{ " \x1\x9", "x  ", 100 },
		{ "  \x2", "xz ",  99 }, /* only 99 since .CUR also matches this */
		{ "  \x3", "xz ", 100 },
		{ "  \xa", "xz ", 100 },
		{ "  \xb", "xz ", 100 },
		{ NULL, NULL, 0 }
	};
	static const gchar *mime_types[] = {
		"image/x-tga",
		NULL
	};
	static const gchar *extensions[] = {
		"tga",
		"targa",
		NULL
	};

	info->name = "tga";
	info->signature = (GdkPixbufModulePattern *) signature;
	info->description = NC_("image format", "Targa");
	info->mime_types = (gchar **) mime_types;
	info->extensions = (gchar **) extensions;
	info->flags = GDK_PIXBUF_FORMAT_THREADSAFE;
	info->license = "LGPL";
}
