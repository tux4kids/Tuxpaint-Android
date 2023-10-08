/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GdkPixbuf library - animated gif support
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Jonathan Blandford <jrb@redhat.com>
 *          Havoc Pennington <hp@redhat.com>
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

#include "config.h"
#include <string.h>
#include <errno.h>
#include "gdk-pixbuf-transform.h"
#include "io-gif-animation.h"
#include "lzw.h"

static void gdk_pixbuf_gif_anim_finalize   (GObject        *object);

static gboolean                gdk_pixbuf_gif_anim_is_static_image  (GdkPixbufAnimation *animation);
static GdkPixbuf*              gdk_pixbuf_gif_anim_get_static_image (GdkPixbufAnimation *animation);

static void                    gdk_pixbuf_gif_anim_get_size (GdkPixbufAnimation *anim,
                                                             int                *width,
                                                             int                *height);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static GdkPixbufAnimationIter* gdk_pixbuf_gif_anim_get_iter (GdkPixbufAnimation *anim,
                                                             const GTimeVal     *start_time);
G_GNUC_END_IGNORE_DEPRECATIONS
static GdkPixbuf*              gdk_pixbuf_gif_anim_iter_get_pixbuf (GdkPixbufAnimationIter *iter);



G_DEFINE_TYPE (GdkPixbufGifAnim, gdk_pixbuf_gif_anim, GDK_TYPE_PIXBUF_ANIMATION);

static void
gdk_pixbuf_gif_anim_init (GdkPixbufGifAnim *anim)
{
}

static void
gdk_pixbuf_gif_anim_class_init (GdkPixbufGifAnimClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GdkPixbufAnimationClass *anim_class = GDK_PIXBUF_ANIMATION_CLASS (klass);

        object_class->finalize = gdk_pixbuf_gif_anim_finalize;

        anim_class->is_static_image = gdk_pixbuf_gif_anim_is_static_image;
        anim_class->get_static_image = gdk_pixbuf_gif_anim_get_static_image;
        anim_class->get_size = gdk_pixbuf_gif_anim_get_size;
        anim_class->get_iter = gdk_pixbuf_gif_anim_get_iter;
}

static void
gdk_pixbuf_gif_anim_finalize (GObject *object)
{
        GdkPixbufGifAnim *gif_anim = GDK_PIXBUF_GIF_ANIM (object);

        GList *l;
        GdkPixbufFrame *frame;

        for (l = gif_anim->frames; l; l = l->next) {
                frame = l->data;
                g_byte_array_unref (frame->lzw_data);
                if (frame->color_map_allocated)
                        g_free (frame->color_map);
                g_free (frame);
        }

        g_list_free (gif_anim->frames);

        g_clear_object (&gif_anim->last_frame_data);
        g_clear_object (&gif_anim->last_frame_revert_data);

        G_OBJECT_CLASS (gdk_pixbuf_gif_anim_parent_class)->finalize (object);
}

static gboolean
gdk_pixbuf_gif_anim_is_static_image  (GdkPixbufAnimation *animation)
{
        GdkPixbufGifAnim *gif_anim;

        gif_anim = GDK_PIXBUF_GIF_ANIM (animation);

        return (gif_anim->frames != NULL &&
                gif_anim->frames->next == NULL);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static GdkPixbuf*
gdk_pixbuf_gif_anim_get_static_image (GdkPixbufAnimation *animation)
{
        GdkPixbufGifAnim *gif_anim;
        GdkPixbufAnimationIter *iter;
        GdkPixbuf *pixbuf;
        GTimeVal start_time = { 0, 0 };

        gif_anim = GDK_PIXBUF_GIF_ANIM (animation);

        if (gif_anim->frames == NULL)
                return NULL;

        iter = gdk_pixbuf_gif_anim_get_iter (animation, &start_time);
        pixbuf = gdk_pixbuf_gif_anim_iter_get_pixbuf (iter);
        g_object_unref (iter);

        return pixbuf;
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
gdk_pixbuf_gif_anim_get_size (GdkPixbufAnimation *anim,
                              int                *width,
                              int                *height)
{
        GdkPixbufGifAnim *gif_anim;

        gif_anim = GDK_PIXBUF_GIF_ANIM (anim);

        if (width)
                *width = gif_anim->width;

        if (height)
                *height = gif_anim->height;
}


static void
iter_clear (GdkPixbufGifAnimIter *iter)
{
        iter->current_frame = NULL;
}

static void
iter_restart (GdkPixbufGifAnimIter *iter)
{
        iter_clear (iter);

        iter->current_frame = iter->gif_anim->frames;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static GdkPixbufAnimationIter*
gdk_pixbuf_gif_anim_get_iter (GdkPixbufAnimation *anim,
                              const GTimeVal     *start_time)
{
        GdkPixbufGifAnimIter *iter;

        iter = g_object_new (GDK_TYPE_PIXBUF_GIF_ANIM_ITER, NULL);

        iter->gif_anim = GDK_PIXBUF_GIF_ANIM (anim);

        g_object_ref (iter->gif_anim);

        iter_restart (iter);

        iter->start_time = *start_time;
        iter->current_time = *start_time;
        iter->first_loop_slowness = 0;

        return GDK_PIXBUF_ANIMATION_ITER (iter);
}
G_GNUC_END_IGNORE_DEPRECATIONS



static void gdk_pixbuf_gif_anim_iter_finalize   (GObject                   *object);

static int        gdk_pixbuf_gif_anim_iter_get_delay_time             (GdkPixbufAnimationIter *iter);
static gboolean   gdk_pixbuf_gif_anim_iter_on_currently_loading_frame (GdkPixbufAnimationIter *iter);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static gboolean   gdk_pixbuf_gif_anim_iter_advance                    (GdkPixbufAnimationIter *iter,
                                                                       const GTimeVal         *current_time);
G_GNUC_END_IGNORE_DEPRECATIONS



G_DEFINE_TYPE (GdkPixbufGifAnimIter, gdk_pixbuf_gif_anim_iter, GDK_TYPE_PIXBUF_ANIMATION_ITER);

static void
gdk_pixbuf_gif_anim_iter_init (GdkPixbufGifAnimIter *iter)
{
}

static void
gdk_pixbuf_gif_anim_iter_class_init (GdkPixbufGifAnimIterClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GdkPixbufAnimationIterClass *anim_iter_class =
                GDK_PIXBUF_ANIMATION_ITER_CLASS (klass);

        object_class->finalize = gdk_pixbuf_gif_anim_iter_finalize;

        anim_iter_class->get_delay_time = gdk_pixbuf_gif_anim_iter_get_delay_time;
        anim_iter_class->get_pixbuf = gdk_pixbuf_gif_anim_iter_get_pixbuf;
        anim_iter_class->on_currently_loading_frame = gdk_pixbuf_gif_anim_iter_on_currently_loading_frame;
        anim_iter_class->advance = gdk_pixbuf_gif_anim_iter_advance;
}

static void
gdk_pixbuf_gif_anim_iter_finalize (GObject *object)
{
        GdkPixbufGifAnimIter *iter = GDK_PIXBUF_GIF_ANIM_ITER (object);

        iter_clear (iter);

        g_object_unref (iter->gif_anim);

        G_OBJECT_CLASS (gdk_pixbuf_gif_anim_iter_parent_class)->finalize (object);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static gboolean
gdk_pixbuf_gif_anim_iter_advance (GdkPixbufAnimationIter *anim_iter,
                                  const GTimeVal         *current_time)
{
        GdkPixbufGifAnimIter *iter;
        gint elapsed;
        gint loop;
        GList *tmp;
        GList *old;

        iter = GDK_PIXBUF_GIF_ANIM_ITER (anim_iter);

        iter->current_time = *current_time;

        /* We use milliseconds for all times */
        elapsed =
          (((iter->current_time.tv_sec - iter->start_time.tv_sec) * G_USEC_PER_SEC +
            iter->current_time.tv_usec - iter->start_time.tv_usec)) / 1000;

        if (elapsed < 0) {
                /* Try to compensate; probably the system clock
                 * was set backwards
                 */
                iter->start_time = iter->current_time;
                elapsed = 0;
        }

        g_assert (iter->gif_anim->total_time > 0);

        /* See how many times we've already played the full animation,
         * and subtract time for that.
         */

        /* If current_frame is NULL at this point, we have loaded the
         * animation from a source which fell behind the speed of the
         * display. We remember how much slower the first loop was due
         * to this and correct the position calculation in order to not
         * jump in the middle of the second loop.
         */
        if (iter->current_frame == NULL)
                iter->first_loop_slowness = MAX(0, elapsed - iter->gif_anim->total_time);

        loop = (elapsed - iter->first_loop_slowness) / iter->gif_anim->total_time;
        elapsed = (elapsed - iter->first_loop_slowness) % iter->gif_anim->total_time;

        iter->position = elapsed;

        /* Now move to the proper frame */
        if (iter->gif_anim->loop == 0 || loop < iter->gif_anim->loop)
                tmp = iter->gif_anim->frames;
        else
                tmp = NULL;
        while (tmp != NULL) {
                GdkPixbufFrame *frame = tmp->data;

                if (iter->position >= frame->elapsed &&
                    iter->position < (frame->elapsed + frame->delay_time))
                        break;

                tmp = tmp->next;
        }

        old = iter->current_frame;

        iter->current_frame = tmp;

        return iter->current_frame != old;
}
G_GNUC_END_IGNORE_DEPRECATIONS

int
gdk_pixbuf_gif_anim_iter_get_delay_time (GdkPixbufAnimationIter *anim_iter)
{
        GdkPixbufFrame *frame;
        GdkPixbufGifAnimIter *iter;

        iter = GDK_PIXBUF_GIF_ANIM_ITER (anim_iter);

        if (iter->current_frame) {
                frame = iter->current_frame->data;

#if 0
                g_print ("frame start: %d pos: %d frame len: %d frame remaining: %d\n",
                         frame->elapsed,
                         iter->position,
                         frame->delay_time,
                         frame->delay_time - (iter->position - frame->elapsed));
#endif

                return frame->delay_time - (iter->position - frame->elapsed);
        } else
                return -1; /* show last frame forever */
}

static void
composite_frame (GdkPixbufGifAnim *anim, GdkPixbufFrame *frame)
{
        LZWDecoder *lzw_decoder = NULL;
        guint8 *index_buffer = NULL;
        gsize n_indexes, i;
        guint16 *interlace_rows = NULL;
        guchar *pixels;

        anim->last_frame = frame;

        /* Store overwritten data if required */
        g_clear_object (&anim->last_frame_revert_data);
        if (frame->action == GDK_PIXBUF_FRAME_REVERT) {
                anim->last_frame_revert_data = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, frame->width, frame->height);
                if (anim->last_frame_revert_data != NULL)
                        gdk_pixbuf_copy_area (anim->last_frame_data,
                                              frame->x_offset, frame->y_offset, frame->width, frame->height,
                                              anim->last_frame_revert_data,
                                              0, 0);
        }

        lzw_decoder = lzw_decoder_new (frame->lzw_code_size + 1);
        index_buffer = g_new (guint8, frame->width * frame->height);
        if (index_buffer == NULL)
                goto out;

        interlace_rows = g_new (guint16, frame->height);
        if (interlace_rows == NULL)
                goto out;
        if (frame->interlace) {
                int row, n = 0;
                for (row = 0; row < frame->height; row += 8)
                        interlace_rows[n++] = row;
                for (row = 4; row < frame->height; row += 8)
                        interlace_rows[n++] = row;
                for (row = 2; row < frame->height; row += 4)
                        interlace_rows[n++] = row;
                for (row = 1; row < frame->height; row += 2)
                        interlace_rows[n++] = row;
        }
        else {
                int row;
                for (row = 0; row < frame->height; row++)
                        interlace_rows[row] = row;
        }

        n_indexes = lzw_decoder_feed (lzw_decoder, frame->lzw_data->data, frame->lzw_data->len, index_buffer, frame->width * frame->height);
        pixels = gdk_pixbuf_get_pixels (anim->last_frame_data);
        for (i = 0; i < n_indexes; i++) {
                guint8 index = index_buffer[i];
                guint x, y;
                gsize offset;

                if (index == frame->transparent_index)
                        continue;

                x = i % frame->width + frame->x_offset;
                y = interlace_rows[i / frame->width] + frame->y_offset;
                if (x >= anim->width || y >= anim->height)
                        continue;

                if (g_size_checked_mul (&offset, gdk_pixbuf_get_rowstride (anim->last_frame_data), y) &&
                    g_size_checked_add (&offset, offset, x * 4)) {
                        pixels[offset + 0] = frame->color_map[index * 3 + 0];
                        pixels[offset + 1] = frame->color_map[index * 3 + 1];
                        pixels[offset + 2] = frame->color_map[index * 3 + 2];
                        pixels[offset + 3] = 255;
                }
        }

out:
        g_clear_object (&lzw_decoder);
        g_free (index_buffer);
        g_free (interlace_rows);
}

GdkPixbuf*
gdk_pixbuf_gif_anim_iter_get_pixbuf (GdkPixbufAnimationIter *anim_iter)
{
        GdkPixbufGifAnimIter *iter = GDK_PIXBUF_GIF_ANIM_ITER (anim_iter);
        GdkPixbufGifAnim *anim = iter->gif_anim;
        GdkPixbufFrame *requested_frame;
        GList *link;

        if (iter->current_frame != NULL)
                requested_frame = iter->current_frame->data;
        else
                requested_frame = g_list_last (anim->frames)->data;

        /* If the previously rendered frame is not before this one, then throw it away */
        if (anim->last_frame != NULL) {
                link = g_list_find (anim->frames, anim->last_frame);
                while (link != NULL && link->data != requested_frame)
                        link = link->next;
                if (link == NULL)
                        anim->last_frame = NULL;
        }

        /* If no rendered frame, render the first frame */
        if (anim->last_frame == NULL) {
                gsize len = 0;
                if (anim->last_frame_data == NULL)
                        anim->last_frame_data = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, anim->width, anim->height);
                if (anim->last_frame_data == NULL)
                        return NULL;
                if (g_size_checked_mul (&len, gdk_pixbuf_get_rowstride (anim->last_frame_data), anim->height))
                        memset (gdk_pixbuf_get_pixels (anim->last_frame_data), 0, len);
                else
                        return NULL;
                composite_frame (anim, g_list_nth_data (anim->frames, 0));
        }

        /* If the requested frame is already rendered, then no action required */
        if (requested_frame == anim->last_frame)
                return anim->last_frame_data;

        /* Starting from the last rendered frame, render to the current frame */
        for (link = g_list_find (anim->frames, anim->last_frame); link->next != NULL && link->data != requested_frame; link = link->next) {
                GdkPixbufFrame *frame = link->data;
                guchar *pixels;
                int y, x_end, y_end;

                /* Remove last frame if required */
                switch (frame->action) {
                case GDK_PIXBUF_FRAME_RETAIN:
                        break;
                case GDK_PIXBUF_FRAME_DISPOSE:
                        /* Replace previous area with background */
                        pixels = gdk_pixbuf_get_pixels (anim->last_frame_data);
                        x_end = MIN (anim->last_frame->x_offset + anim->last_frame->width, anim->width);
                        y_end = MIN (anim->last_frame->y_offset + anim->last_frame->height, anim->height);
                        for (y = anim->last_frame->y_offset; y < y_end; y++) {
                                gsize offset;
                                if (g_size_checked_mul (&offset, gdk_pixbuf_get_rowstride (anim->last_frame_data), y) &&
                                    g_size_checked_add (&offset, offset, anim->last_frame->x_offset * 4)) {
                                         memset (pixels + offset, 0, (x_end - anim->last_frame->x_offset) * 4);
                                }
                        }
                        break;
                case GDK_PIXBUF_FRAME_REVERT:
                        /* Replace previous area with last retained area */
                        if (anim->last_frame_revert_data != NULL)
                                gdk_pixbuf_copy_area (anim->last_frame_revert_data,
                                                      0, 0, anim->last_frame->width, anim->last_frame->height,
                                                      anim->last_frame_data,
                                                      anim->last_frame->x_offset, anim->last_frame->y_offset);
                        break;
                }

                /* Render next frame */
                composite_frame (anim, link->next->data);
        }

        return anim->last_frame_data;
}

static gboolean
gdk_pixbuf_gif_anim_iter_on_currently_loading_frame (GdkPixbufAnimationIter *anim_iter)
{
        GdkPixbufGifAnimIter *iter;

        iter = GDK_PIXBUF_GIF_ANIM_ITER (anim_iter);

        return iter->current_frame == NULL || iter->current_frame->next == NULL;
}
