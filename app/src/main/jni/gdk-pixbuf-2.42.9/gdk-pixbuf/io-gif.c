/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GdkPixbuf library - GIF image loader
 *
 * Copyright (C) 1999 Mark Crichton
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Jonathan Blandford <jrb@redhat.com>
 *          Adapted from the gimp gif filter written by Adam Moss <adam@gimp.org>
 *          Gimp work based on earlier work.
 *          Permission to relicense under the LGPL obtained.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* This loader is very hairy code.
 *
 * The main loop was not designed for incremental loading, so when it was hacked
 * in it got a bit messy.  Basically, every function is written to expect a failed
 * read_gif, and lets you call it again assuming that the bytes are there.
 *
 * Return vals.
 * Unless otherwise specified, these are the return vals for most functions:
 *
 *  0 -> success
 * -1 -> more bytes needed.
 * -2 -> failure; abort the load
 * -3 -> control needs to be passed back to the main loop
 *        \_ (most of the time returning 0 will get this, but not always)
 *
 * >1 -> for functions that get a guchar, the char will be returned.
 *
 * -jrb (11/03/1999)
 */

/*
 * If you have any images that crash this code, please, please let me know and
 * send them to me.
 *                                            <jrb@redhat.com>
 */



#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib/gi18n-lib.h>
#include "gdk-pixbuf-io.h"
#include "io-gif-animation.h"



#undef DUMP_IMAGE_DETAILS 

#define MAXCOLORMAPSIZE  256

#define INTERLACE          0x40
#define LOCALCOLORMAP      0x80
#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))
#define LM_to_uint(a,b)         (((b)<<8)|(a))



typedef unsigned char CMap[3][MAXCOLORMAPSIZE];

/* Possible states we can be in. */
enum {
	GIF_START,
	GIF_GET_COLORMAP,
	GIF_GET_NEXT_STEP,
	GIF_GET_FRAME_INFO,
	GIF_GET_EXTENSION,
	GIF_GET_COLORMAP2,
	GIF_PREPARE_LZW,
	GIF_GET_LZW,
	GIF_DONE
};

typedef struct _GifContext GifContext;
struct _GifContext
{
	int state; /* really only relevant for progressive loading */
	unsigned int width;
	unsigned int height;

        gboolean has_global_cmap;

        gint global_colormap_size;
        unsigned int global_bit_pixel;

        CMap frame_color_map;
        gint frame_colormap_size;
        unsigned int frame_bit_pixel;

	GdkPixbufGifAnim *animation;
	GdkPixbufFrame *frame;
	int transparent_index;
	int delay_time;
	int disposal;

	/* stuff per frame. */
	int frame_len;
	int frame_height;
	int frame_interlace;
	int x_offset;
	int y_offset;

	/* Static read only */
	FILE *file;

	/* progressive read, only. */
	GdkPixbufModuleSizeFunc size_func;
	GdkPixbufModulePreparedFunc prepared_func;
	GdkPixbufModuleUpdatedFunc updated_func;
	gpointer user_data;
	GByteArray *buf;

	/* extension context */
	guchar extension_label;
	guchar extension_flag;
        gboolean in_loop_extension;

	/* get block context */
	guchar block_count;
	guchar block_buf[280];

	guchar lzw_set_code_size;

        /* error pointer */
        GError **error;
};

/* Returns TRUE if read is OK,
 * FALSE if more memory is needed. */
static gboolean
gif_read (GifContext *context, guchar *buffer, size_t len)
{
	gboolean retval;
	if (context->file) {
		retval = (fread (buffer, 1, len, context->file) == len);

                if (!retval && ferror (context->file)) {
                        gint save_errno = errno;
                        g_set_error (context->error,
                                     G_FILE_ERROR,
                                     g_file_error_from_errno (save_errno),
                                     _("Failure reading GIF: %s"), 
                                     g_strerror (save_errno));
                }

		return retval;
	} else {
		if (context->buf->len >= len) {
			memcpy (buffer, context->buf->data, len);
			g_byte_array_remove_range (context->buf, 0, len);
			return TRUE;
		}
	}
	return FALSE;
}

/* Changes the stage to be GIF_GET_COLORMAP */
static void
gif_set_get_colormap (GifContext *context)
{
	context->global_colormap_size = 0;
	context->state = GIF_GET_COLORMAP;
}

static void
gif_set_get_colormap2 (GifContext *context)
{
	context->state = GIF_GET_COLORMAP2;
}

static gint
gif_get_colormap (GifContext *context)
{
	unsigned char rgb[3];

	while (context->global_colormap_size < context->global_bit_pixel) {
		if (!gif_read (context, rgb, sizeof (rgb))) {
			return -1;
		}

		context->animation->color_map[context->global_colormap_size * 3 + 0] = rgb[0];
		context->animation->color_map[context->global_colormap_size * 3 + 1] = rgb[1];
		context->animation->color_map[context->global_colormap_size * 3 + 2] = rgb[2];

		context->global_colormap_size ++;
	}

	return 0;
}


static gint
gif_get_colormap2 (GifContext *context)
{
	unsigned char rgb[3];

	while (context->frame_colormap_size < context->frame_bit_pixel) {
		if (!gif_read (context, rgb, sizeof (rgb))) {
			return -1;
		}

		context->frame_color_map[0][context->frame_colormap_size] = rgb[0];
		context->frame_color_map[1][context->frame_colormap_size] = rgb[1];
		context->frame_color_map[2][context->frame_colormap_size] = rgb[2];

		context->frame_colormap_size ++;
	}

	return 0;
}

/*
 * in order for this function to work, we need to perform some black magic.
 * We want to return -1 to let the calling function know, as before, that it needs
 * more bytes.  If we return 0, we were able to successfully read all block->count bytes.
 * Problem is, we don't want to reread block_count every time, so we check to see if
 * context->block_count is 0 before we read in the function.
 *
 * As a result, context->block_count MUST be 0 the first time the get_data_block is called
 * within a context, and cannot be 0 the second time it's called.
 */

static int
get_data_block (GifContext *context,
		unsigned char *buf,
		gint *empty_block)
{

	if (context->block_count == 0) {
		if (!gif_read (context, &context->block_count, 1)) {
			return -1;
		}
	}

	if (context->block_count == 0)
		if (empty_block) {
			*empty_block = TRUE;
			return 0;
		}

	if (!gif_read (context, buf, context->block_count)) {
		return -1;
	}

	return 0;
}

static void
gif_set_get_extension (GifContext *context)
{
	context->state = GIF_GET_EXTENSION;
	context->extension_flag = TRUE;
	context->extension_label = 0;
	context->block_count = 0;
}

static int
gif_get_extension (GifContext *context)
{
	gint retval;
	gint empty_block = FALSE;

	if (context->extension_flag) {
		if (context->extension_label == 0) {
			/* I guess bad things can happen if we have an extension of 0 )-: */
			/* I should look into this sometime */
			if (!gif_read (context, & context->extension_label , 1)) {
				return -1;
			}
		}

		switch (context->extension_label) {
                case 0xf9:			/* Graphic Control Extension */
                        retval = get_data_block (context, (unsigned char *) context->block_buf, NULL);
			if (retval != 0)
				return retval;

			if (context->frame == NULL) {
				/* I only want to set the transparency if I haven't
				 * created the frame yet.
                                 */
				context->disposal = (context->block_buf[0] >> 2) & 0x7;
				context->delay_time = LM_to_uint (context->block_buf[1], context->block_buf[2]);
				
				if ((context->block_buf[0] & 0x1) != 0) {
					context->transparent_index = context->block_buf[3];
				} else {
					context->transparent_index = -1;
				}
			}

			/* Now we've successfully loaded this one, we continue on our way */
			context->block_count = 0;
			context->extension_flag = FALSE;
			break;
                case 0xff: /* application extension */
                        if (!context->in_loop_extension) { 
                                retval = get_data_block (context, (unsigned char *) context->block_buf, NULL);
                                if (retval != 0)
                                        return retval;
                                if (!strncmp ((gchar *)context->block_buf, "NETSCAPE2.0", 11) ||
                                    !strncmp ((gchar *)context->block_buf, "ANIMEXTS1.0", 11)) {
                                        context->in_loop_extension = TRUE;
                                }
                                context->block_count = 0;
                        }
                        if (context->in_loop_extension) {
                                do {
                                        retval = get_data_block (context, (unsigned char *) context->block_buf, &empty_block);
                                        if (retval != 0)
                                                return retval;
                                        if (context->block_buf[0] == 0x01) {
                                                context->animation->loop = context->block_buf[1] + (context->block_buf[2] << 8);
                                                if (context->animation->loop != 0) 
                                                        context->animation->loop++;
                                        }
                                        context->block_count = 0;
                                }
                                while (!empty_block);
                                context->in_loop_extension = FALSE;
                                context->extension_flag = FALSE;
                                return 0;
                        }
			break;                          
		default:
			/* Unhandled extension */
			break;
		}
	}
	/* read all blocks, until I get an empty block, in case there was an extension I didn't know about. */
	do {
		retval = get_data_block (context, (unsigned char *) context->block_buf, &empty_block);
		if (retval != 0)
			return retval;
		context->block_count = 0;
	} while (!empty_block);

	return 0;
}

static void
gif_set_get_lzw (GifContext *context)
{
	context->state = GIF_GET_LZW;
}

static int
gif_get_lzw (GifContext *context)
{
	if (context->frame == NULL) {
                int rowstride;
                guint64 len;

                rowstride = gdk_pixbuf_calculate_rowstride (GDK_COLORSPACE_RGB,
                                                            TRUE,
                                                            8,
                                                            context->frame_len,
                                                            context->frame_height);
                if (rowstride < 0 ||
                    !g_uint64_checked_mul (&len, rowstride, context->frame_height) ||
                    len >= G_MAXINT) {
                        g_set_error_literal (context->error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                             _("Not enough memory to load GIF file"));
                        return -2;
                }

                context->frame = g_new0 (GdkPixbufFrame, 1);

                context->frame->lzw_data = g_byte_array_new ();
                context->frame->lzw_code_size = context->lzw_set_code_size;

                context->frame->width = context->frame_len;
                context->frame->height = context->frame_height;
                context->frame->x_offset = context->x_offset;
                context->frame->y_offset = context->y_offset;
                context->frame->interlace = context->frame_interlace;

                if (context->frame_colormap_size > 0) {
                        int i;

                        context->frame->color_map = g_malloc (256 * 3);
                        context->frame->color_map_allocated = TRUE;
                        for (i = 0; i < 256; i++) {
                                context->frame->color_map[i * 3 + 0] = context->frame_color_map[0][i];
                                context->frame->color_map[i * 3 + 1] = context->frame_color_map[1][i];
                                context->frame->color_map[i * 3 + 2] = context->frame_color_map[2][i];
                        }
                }
                else {
                        context->frame->color_map = context->animation->color_map;
                }

                context->frame->transparent_index = context->transparent_index;

                /* GIF delay is in hundredths, we want thousandths */
                context->frame->delay_time = context->delay_time * 10;

                /* GIFs with delay time 0 are mostly broken, but they
                 * just want a default, "not that fast" delay.
                 */
                if (context->frame->delay_time == 0)
                        context->frame->delay_time = 100;

                /* No GIFs gets to play faster than 50 fps. They just
                 * lock up poor gtk.
                 */
                if (context->frame->delay_time < 20)
                        context->frame->delay_time = 20; /* 20 = "fast" */

                context->frame->elapsed = context->animation->total_time;
                context->animation->total_time += context->frame->delay_time;                
                
                switch (context->disposal) {
                case 0:
                case 1:
                        context->frame->action = GDK_PIXBUF_FRAME_RETAIN;
                        break;
                case 2:
                        context->frame->action = GDK_PIXBUF_FRAME_DISPOSE;
                        break;
                case 3:
                        context->frame->action = GDK_PIXBUF_FRAME_REVERT;
                        break;
                default:
                        context->frame->action = GDK_PIXBUF_FRAME_RETAIN;
                        break;
                }

                context->animation->frames = g_list_append (context->animation->frames, context->frame);

		/* Notify when have first frame */
		if (context->animation->frames->next == NULL) {
			GdkPixbuf *pixbuf = gdk_pixbuf_animation_get_static_image (GDK_PIXBUF_ANIMATION (context->animation));
			if (pixbuf != NULL)
				(* context->prepared_func) (pixbuf,
                                                            GDK_PIXBUF_ANIMATION (context->animation),
                                                            context->user_data);
		}
        }

	/* read all blocks */
	while (TRUE) {
		gint retval, empty_block = FALSE;

		retval = get_data_block (context, (unsigned char *) context->block_buf, &empty_block);

		/* Notify frame update */
		if ((retval != 0 || empty_block) && context->animation->frames->next == NULL) {
			GdkPixbuf *pixbuf = gdk_pixbuf_animation_get_static_image (GDK_PIXBUF_ANIMATION (context->animation));
			if (pixbuf)
				(* context->updated_func) (pixbuf,
                                                           0, 0, context->frame->width, context->frame->height,
                                                           context->user_data);
		}

		if (retval != 0)
			return retval;

		if (empty_block) {
			context->frame = NULL;
			context->state = GIF_GET_NEXT_STEP;
			return 0;
		}

		g_byte_array_append (context->frame->lzw_data, context->block_buf, context->block_count);
		if (context->animation->last_frame == context->frame)
			context->animation->last_frame = NULL;
		context->block_count = 0;
	}
}

static void
gif_set_prepare_lzw (GifContext *context)
{
	context->state = GIF_PREPARE_LZW;
}
static int
gif_prepare_lzw (GifContext *context)
{
	if (!gif_read (context, &(context->lzw_set_code_size), 1)) {
		/*g_message (_("GIF: EOF / read error on image data\n"));*/
		return -1;
	}

        if (context->lzw_set_code_size >= 12) {
                g_set_error_literal (context->error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("GIF image is corrupt (incorrect LZW compression)"));
                return -2;
        }

	gif_set_get_lzw (context);

	return 0;
}

/*
 * Read the GIF signature and screen descriptor.
 *
 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9  | 10     | 11 | 12 |
 * |-----------|-----------|-----------------------------------|
 * | magic     | version   | screen descriptor                 |
 * | G | I | F | 8 | 9 | a | width | height | colors | ignored |
 */
static gint
gif_init (GifContext *context)
{
	unsigned char buf[13];
	char version[4];
        gint width, height;

	if (!gif_read (context, buf, 13)) {
		/* Unable to read magic number,
                 * gif_read() should have set error
                 */
		return -1;
	}

	if (strncmp ((char *) buf, "GIF", 3) != 0) {
		/* Not a GIF file */
                g_set_error_literal (context->error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("File does not appear to be a GIF file"));
		return -2;
	}

	strncpy (version, (char *) buf + 3, 3);
	version[3] = '\0';

	if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0)) {
		gchar *escaped_version;

		/* bad version number, not '87a' or '89a' */
		escaped_version = g_strescape (version, NULL);
                g_set_error (context->error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                             _("Version %s of the GIF file format is not supported"),
                             escaped_version);
		g_free (escaped_version);
		return -2;
	}

	context->width = LM_to_uint (buf[6], buf[7]);
	context->height = LM_to_uint (buf[8], buf[9]);
        /* The 4th byte is
         * high bit: whether to use the background index
         * next 3:   color resolution
         * next:     whether colormap is sorted by priority of allocation
         * last 3:   size of colormap
         */
	context->global_bit_pixel = 2 << (buf[10] & 0x07);
        context->has_global_cmap = (buf[10] & 0x80) != 0;

        context->animation->width = context->width;
        context->animation->height = context->height;

        width = context->width;
        height = context->height;

        (*context->size_func) (&width, &height, context->user_data);

        if (width == 0 || height == 0) {
                g_set_error_literal (context->error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("Resulting GIF image has zero size"));
                return -2;
        }

	if (context->has_global_cmap) {
		gif_set_get_colormap (context);
	} else {
		context->state = GIF_GET_NEXT_STEP;
	}

#ifdef DUMP_IMAGE_DETAILS
        g_print (">Image width: %d height: %d global_cmap: %d\n",
                 context->width, context->height, context->has_global_cmap);
#endif
        
	return 0;
}

static void
gif_set_get_frame_info (GifContext *context)
{
	context->state = GIF_GET_FRAME_INFO;
}

static gint
gif_get_frame_info (GifContext *context)
{
	unsigned char buf[9];
        
	if (!gif_read (context, buf, 9)) {
		return -1;
	}

	/* Okay, we got all the info we need.  Lets record it */
	context->frame_len = LM_to_uint (buf[4], buf[5]);
	context->frame_height = LM_to_uint (buf[6], buf[7]);
	context->x_offset = LM_to_uint (buf[0], buf[1]);
	context->y_offset = LM_to_uint (buf[2], buf[3]);

	context->frame_interlace = BitSet (buf[8], INTERLACE);

#ifdef DUMP_IMAGE_DETAILS
        g_print (">width: %d height: %d xoffset: %d yoffset: %d disposal: %d delay: %d transparent: %d interlace: %d\n",
                 context->frame_len, context->frame_height, context->x_offset, context->y_offset,
                 context->disposal, context->delay_time, context->transparent_index, context->frame_interlace);
#endif
        
	context->frame_colormap_size = 0;
	if (BitSet (buf[8], LOCALCOLORMAP)) {

#ifdef DUMP_IMAGE_DETAILS
                g_print (">has local colormap\n");
#endif
                
		/* Does this frame have it's own colormap. */
		/* really only relevant when looking at the first frame
		 * of an animated gif. */
		/* if it does, we need to re-read in the colormap,
		 * the gray_scale, and the bit_pixel */
		context->frame_bit_pixel = 1 << ((buf[8] & 0x07) + 1);
		gif_set_get_colormap2 (context);
		return 0;
	}

        if (!context->has_global_cmap) {
                context->state = GIF_DONE;
                
                g_set_error_literal (context->error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("GIF image has no global colormap, and a frame inside it has no local colormap."));
                
		return -2;
        }

	gif_set_prepare_lzw (context);
	return 0;

}

static gint
gif_get_next_step (GifContext *context)
{
	unsigned char c;
	while (TRUE) {
		if (!gif_read (context, &c, 1)) {
			return -1;
		}
		if (c == ';') {
			/* GIF terminator */
			/* hmm.  Not 100% sure what to do about this.  Should
			 * i try to return a blank image instead? */
			context->state = GIF_DONE;
			return 0;
		}

		if (c == '!') {
			/* Check the extension */
			gif_set_get_extension (context);
			return 0;
		}

		/* look for frame */
		if (c != ',') {
			/* Not a valid start character */
			continue;
		}
		/* load the frame */
		gif_set_get_frame_info (context);
		return 0;
	}
}


#define LOG(x) /* g_print ("%s: %s\n", G_STRLOC, x); */

static gint
gif_main_loop (GifContext *context)
{
	gint retval = 0;

	do {
		switch (context->state) {
		case GIF_START:
                        LOG("start\n");
			retval = gif_init (context);
			break;

		case GIF_GET_COLORMAP:
                        LOG("get_colormap\n");
			retval = gif_get_colormap (context);
			if (retval == 0)
				context->state = GIF_GET_NEXT_STEP;
			break;

		case GIF_GET_NEXT_STEP:
                        LOG("next_step\n");
			retval = gif_get_next_step (context);
			break;

		case GIF_GET_FRAME_INFO:
                        LOG("frame_info\n");
			retval = gif_get_frame_info (context);
			break;

		case GIF_GET_EXTENSION:
                        LOG("get_extension\n");
			retval = gif_get_extension (context);
			if (retval == 0)
				context->state = GIF_GET_NEXT_STEP;
			break;

		case GIF_GET_COLORMAP2:
                        LOG("get_colormap2\n");
			retval = gif_get_colormap2 (context);
			if (retval == 0)
				gif_set_prepare_lzw (context);
			break;

		case GIF_PREPARE_LZW:
                        LOG("prepare_lzw\n");
			retval = gif_prepare_lzw (context);
			break;

		case GIF_GET_LZW:
                        LOG("get_lzw\n");
			retval = gif_get_lzw (context);
			break;

		case GIF_DONE:
                        LOG("done\n");
                        /* fall through */
		default:
			retval = 0;
			goto done;
		};
	} while ((retval == 0) || (retval == -3));
 done:
	return retval;
}

static GifContext *
new_context (GdkPixbufModuleSizeFunc size_func,
             GdkPixbufModulePreparedFunc prepared_func,
             GdkPixbufModuleUpdatedFunc updated_func,
             gpointer user_data)
{
	GifContext *context;

        g_assert (size_func != NULL);
        g_assert (prepared_func != NULL);
        g_assert (updated_func != NULL);

	context = g_try_malloc (sizeof (GifContext));
        if (context == NULL)
                return NULL;

        memset (context, 0, sizeof (GifContext));
        
        context->animation = g_object_new (GDK_TYPE_PIXBUF_GIF_ANIM, NULL);
	context->frame = NULL;
	context->transparent_index = -1;
	context->file = NULL;
	context->state = GIF_START;
	context->size_func = size_func;
	context->prepared_func = prepared_func;
	context->updated_func = updated_func;
	context->user_data = user_data;
	context->buf = g_byte_array_new ();
        context->animation->loop = 1;
        context->in_loop_extension = FALSE;

	return context;
}

static void
noop_size_notify (gint     *width,
		  gint     *height,
		  gpointer  data)
{
}

static void
noop_prepared_notify (GdkPixbuf *pixbuf,
                      GdkPixbufAnimation *anim,
                      gpointer user_data)
{
}

static void
noop_updated_notify (GdkPixbuf *pixbuf,
		     int        x,
		     int        y,
		     int        width,
		     int        height,
		     gpointer   user_data)
{
}

static GifContext *
new_context_without_callbacks (void)
{
        return new_context (noop_size_notify, noop_prepared_notify, noop_updated_notify, NULL);
}

/* Shared library entry point */
static GdkPixbuf *
gdk_pixbuf__gif_image_load (FILE *file, GError **error)
{
	GifContext *context;
	GdkPixbuf *pixbuf;
        gint retval;

	g_return_val_if_fail (file != NULL, NULL);

	context = new_context_without_callbacks ();

        if (context == NULL) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Not enough memory to load GIF file"));
                return NULL;
        }
        
	context->file = file;
        context->error = error;

        retval = gif_main_loop (context);
	if (retval == -1 || context->animation->frames == NULL) {
                if (context->error && *(context->error) == NULL)
                        g_set_error_literal (context->error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                             _("GIF file was missing some data (perhaps it was truncated somehow?)"));
        }
        else if (retval == -2) {
                pixbuf = NULL;
                goto out;
        }
        
        pixbuf = gdk_pixbuf_animation_get_static_image (GDK_PIXBUF_ANIMATION (context->animation));

        if (pixbuf)
                g_object_ref (pixbuf);

out:
        g_object_unref (context->animation);
        
	g_byte_array_unref (context->buf);
	g_free (context);
 
	return pixbuf;
}

static gpointer
gdk_pixbuf__gif_image_begin_load (GdkPixbufModuleSizeFunc size_func,
                                  GdkPixbufModulePreparedFunc prepared_func,
				  GdkPixbufModuleUpdatedFunc updated_func,
				  gpointer user_data,
                                  GError **error)
{
	GifContext *context;

        g_assert (size_func != NULL);
        g_assert (prepared_func != NULL);
        g_assert (updated_func != NULL);

	context = new_context (size_func, prepared_func, updated_func, user_data);

        if (context == NULL) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Not enough memory to load GIF file"));
                return NULL;
        }
        
        context->error = error;

	return (gpointer) context;
}

static gboolean
gdk_pixbuf__gif_image_stop_load (gpointer data, GError **error)
{
	GifContext *context = (GifContext *) data;
        gboolean retval = TRUE;
        
        if (context->animation->frames == NULL) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("GIF image was truncated or incomplete."));

                retval = FALSE;
        } else if (context->state != GIF_DONE) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION,
                                     _("Not all frames of the GIF image were loaded."));

                retval = FALSE;
        }

        g_object_unref (context->animation);

	g_byte_array_unref (context->buf);
	g_free (context);

        return retval;
}

static gboolean
gdk_pixbuf__gif_image_load_increment (gpointer data,
                                      const guchar *buf, guint size,
                                      GError **error)
{
	gint retval;
	GifContext *context = (GifContext *) data;

        context->error = error;

	g_byte_array_append (context->buf, buf, size);

	retval = gif_main_loop (context);
	if (retval == -2)
		return FALSE;

	return TRUE;
}

static GdkPixbufAnimation *
gdk_pixbuf__gif_image_load_animation (FILE *file,
                                      GError **error)
{
	GifContext *context;
	GdkPixbufAnimation *animation;

	g_return_val_if_fail (file != NULL, NULL);

	context = new_context_without_callbacks ();

        if (context == NULL) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Not enough memory to load GIF file"));
                return NULL;
        }
        
        context->error = error;
	context->file = file;

	if (gif_main_loop (context) == -1 || context->animation->frames == NULL) {
                if (context->error && *(context->error) == NULL)
                        g_set_error_literal (context->error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                             _("GIF file was missing some data (perhaps it was truncated somehow?)"));

                g_object_unref (context->animation);
                context->animation = NULL;
        }

        if (context->animation)
                animation = GDK_PIXBUF_ANIMATION (context->animation);
        else
                animation = NULL;

        if (context->error && *(context->error))
                g_print ("%s\n", (*(context->error))->message);
        
	g_byte_array_unref (context->buf);
	g_free (context);
	return animation;
}

#ifndef INCLUDE_gif
#define MODULE_ENTRY(function) G_MODULE_EXPORT void function
#else
#define MODULE_ENTRY(function) void _gdk_pixbuf__gif_ ## function
#endif

MODULE_ENTRY (fill_vtable) (GdkPixbufModule *module)
{
        module->load = gdk_pixbuf__gif_image_load;
        module->begin_load = gdk_pixbuf__gif_image_begin_load;
        module->stop_load = gdk_pixbuf__gif_image_stop_load;
        module->load_increment = gdk_pixbuf__gif_image_load_increment;
        module->load_animation = gdk_pixbuf__gif_image_load_animation;
}

MODULE_ENTRY (fill_info) (GdkPixbufFormat *info)
{
        static const GdkPixbufModulePattern signature[] = {
                { "GIF8", NULL, 100 },
                { NULL, NULL, 0 }
        };
	static const gchar *mime_types[] = {
		"image/gif",
		NULL
	};
	static const gchar *extensions[] = {
		"gif",
		NULL
	};

	info->name = "gif";
        info->signature = (GdkPixbufModulePattern *) signature;
	info->description = NC_("image format", "GIF");
	info->mime_types = (gchar **) mime_types;
	info->extensions = (gchar **) extensions;
	info->flags = GDK_PIXBUF_FORMAT_THREADSAFE;
	info->license = "LGPL";
}
