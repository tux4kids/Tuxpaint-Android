/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Andrey Tsyvarev
 *         Bastien Nocera
 */

#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* maximum number of frames in animation(before repeating) */
#define MAX_NUMBER_FRAMES 1000

/* Information about currently loading frame of animation.
 *
 * pixbuf == NULL means that animation is fully loaded
 * (no currently loading frames exist).
 */
typedef struct {
    GdkPixbufAnimationIter* iter; /* iterator pointed to the frame */
    GTimeVal time; /* time when this frame appears */
    GdkPixbuf *pixbuf; /* current content of the frame */
} FrameData;

/* Auxiliary function, returns numeric representation of pixel.
 * For RGB format it is rrggbb(in hex), for RGBA - rrggbbaa. */
static guint32
get_pixel (GdkPixbuf *pixbuf, int x, int y)
{
    guchar *colors;
    guint32 pixel;

    colors = ((guchar*)gdk_pixbuf_get_pixels(pixbuf)
	      + gdk_pixbuf_get_n_channels(pixbuf) * x
	      + gdk_pixbuf_get_rowstride(pixbuf) * y);
    pixel = (colors[0] << 16) | (colors[1] << 8) | colors[2];

    if (gdk_pixbuf_get_n_channels (pixbuf) == 4)
        pixel = (pixel << 8) | colors[3];

    return pixel;
}
/* Verify whether all pixels outside the updated area
 * have the same values in pixbuf_old and pixbuf_new. */
static gboolean
pixbuf_not_changed_outside_area (GdkPixbuf *pixbuf_new,
				 GdkPixbuf *pixbuf_old,
				 int        x,
				 int        y,
				 int        width,
				 int        height)
{
    int pixbuf_width, pixbuf_height;
    int x_curr, y_curr;

    pixbuf_width = gdk_pixbuf_get_width (pixbuf_new);
    pixbuf_height = gdk_pixbuf_get_height (pixbuf_new);

    for (x_curr = 0; x_curr < pixbuf_width; x_curr++) {
        for (y_curr = 0; y_curr < pixbuf_height; y_curr++) {
            if ((x_curr >= x)
                && (x_curr < x + width)
                && (y_curr >= y)
                && (y_curr < y + height)) {
                continue; /* inside area - don't compare pixels. */
            }
            if (get_pixel (pixbuf_new, x_curr, y_curr) != get_pixel (pixbuf_old, x_curr, y_curr)) {
                printf("Pixel at (%d, %d) changed from %x to %x,\n",
                       x_curr, y_curr,
                       get_pixel (pixbuf_old, x_curr, y_curr),
                       get_pixel (pixbuf_new, x_curr, y_curr));
                printf("But it is outside of area with ");
                printf("x=%d, y=%d, width=%d, height=%d\n",
                       x, y, width, height);

#if 0
                gdk_pixbuf_save (pixbuf_old, "old.png", "png", NULL, NULL);
                gdk_pixbuf_save (pixbuf_new, "new.png", "png", NULL, NULL);
#endif

                g_assert_not_reached ();
            }
        }
    }

    return TRUE;
}

static void
callback_area_updated (GdkPixbufLoader *loader,
                       int              x,
                       int              y,
                       int              width,
                       int              height,
                       GdkPixbuf       *pixbuf_old)
{
    GdkPixbuf *pixbuf_new;

    pixbuf_new = gdk_pixbuf_loader_get_pixbuf (loader);

    pixbuf_not_changed_outside_area (pixbuf_new, pixbuf_old, x, y, width, height);
    /* update copy of pixbuf */
    gdk_pixbuf_copy_area (pixbuf_new, x, y, width, height, pixbuf_old, x, y);
}

/* free copy of pixbuf used in area-updated callback. */
static void
callback_closed (GdkPixbufLoader *loader,
                 GdkPixbuf       *pixbuf_copy)
{
    g_object_unref (pixbuf_copy);
}

/* prepare copy of pixbuf and connect other callbacks */
static void
callback_area_prepared (GdkPixbufLoader *loader)
{
    GdkPixbuf *pixbuf_copy;

    pixbuf_copy = gdk_pixbuf_copy (gdk_pixbuf_loader_get_pixbuf (loader));
    /* connect callbacks for another signals for not use pointer to pointer in them. */
    g_signal_connect (loader, "area-updated",
                      (GCallback) callback_area_updated, (gpointer) pixbuf_copy);
    g_signal_connect (loader, "closed",
                      (GCallback) callback_closed, (gpointer) pixbuf_copy);
}

/* Read count_bytes from the channel and write them to the loader.
 * Returns number of bytes written.
 * count_bytes = G_MAXSIZE means read as many bytes as possible. */
static gsize
loader_write_from_channel (GdkPixbufLoader *loader,
                           GIOChannel      *channel,
                           gsize            count_bytes)
{
    guchar* buffer;
    gsize bytes_read;
    GIOStatus read_status;

    if(count_bytes < G_MAXSIZE) {
        /* read no more than 'count_bytes' bytes */
        buffer = g_malloc(count_bytes);
        read_status = g_io_channel_read_chars (channel, (gchar*) buffer, count_bytes, &bytes_read, NULL);
    } else {
        /*read up to end */
        read_status = g_io_channel_read_to_end (channel, (gchar**) &buffer, &bytes_read, NULL);
    }

    if ((read_status != G_IO_STATUS_NORMAL) && (read_status != G_IO_STATUS_EOF))
        g_assert_not_reached ();

    if (!gdk_pixbuf_loader_write(loader, buffer, bytes_read, NULL))
        g_assert_not_reached ();

    g_free (buffer);
    return bytes_read;
}

static void
test_area_updated_ico (gconstpointer data)
{
    const char *filename;
    GIOChannel *channel;
    GdkPixbufLoader *loader;

    filename = g_test_get_filename (G_TEST_DIST, data, NULL);

    /* create channel */
    channel = g_io_channel_new_file(filename, "r", NULL);
    g_assert_nonnull (channel);
    g_io_channel_set_encoding (channel, NULL, NULL);
    /* create loader */
    loader = gdk_pixbuf_loader_new ();
    g_assert_nonnull (loader);

    g_signal_connect(loader, "area-prepared",
                    (GCallback) callback_area_prepared, NULL);
    /* callbacks for "area-updated" and "closed" signals will be connected
     * in callback_area_prepared() */

    /* read image byte by byte */
    while (loader_write_from_channel(loader, channel, 1) == 1);
    /* or read full image at once */
#if 0
    loader_write_from_channel(loader, channel, G_MAXSIZE);
#endif

    /* free resources */
    g_io_channel_unref (channel);

    gdk_pixbuf_loader_close (loader, NULL);
    g_object_unref (loader);
}

/* Auxiliary function - look for frame that's currently loading. */
static void
update_currently_loaded_frame (FrameData* frame)
{
    int tmp_count;

    if (gdk_pixbuf_animation_iter_on_currently_loading_frame(frame->iter))
        return; /* frame is currently being loaded */
    /* clear old content of pixbuf */
    if (frame->pixbuf)
        g_object_unref (frame->pixbuf);
    frame->pixbuf = NULL;

    tmp_count = 0;
    do {
        int delay_time;

        if (++tmp_count > MAX_NUMBER_FRAMES) {
            /* protection against frames repeating */
            return;
        }

        delay_time = gdk_pixbuf_animation_iter_get_delay_time (frame->iter);
        if (delay_time < 0) {
            /* this is last frame in the animation */
            return;
        }
        g_time_val_add (&frame->time, delay_time * 1000);
        gdk_pixbuf_animation_iter_advance (frame->iter, &frame->time);
    } while (!gdk_pixbuf_animation_iter_on_currently_loading_frame (frame->iter));
    /* store current content of the frame */
    frame->pixbuf = gdk_pixbuf_copy (gdk_pixbuf_animation_iter_get_pixbuf (frame->iter));
}

static void
callback_area_updated_anim (GdkPixbufLoader *loader,
                            int              x,
                            int              y,
                            int              width,
                            int              height,
                            FrameData       *frame_old)
{
    GdkPixbuf *pixbuf_new;

    /* "area-updated" signal was emitted after animation had fully loaded. */
    g_assert_nonnull (frame_old->pixbuf);

    pixbuf_new = gdk_pixbuf_animation_iter_get_pixbuf (frame_old->iter);
    pixbuf_not_changed_outside_area (pixbuf_new, frame_old->pixbuf, x, y, width, height);
    gdk_pixbuf_copy_area (pixbuf_new, x, y, width, height, frame_old->pixbuf, x, y);
    update_currently_loaded_frame (frame_old);
}

/* free resources used in callback_area_updated_anim */
static void
callback_closed_anim(GdkPixbufLoader *loader, FrameData *frame_copy)
{
    g_object_unref (frame_copy->iter);
    if(frame_copy->pixbuf != NULL)
        g_object_unref (frame_copy->pixbuf);
    g_free (frame_copy);
}
/* prepare frame information and register other callbacks */
static void
callback_area_prepared_anim (GdkPixbufLoader* loader)
{
    GdkPixbufAnimation *anim;
    FrameData* frame_copy = g_new (FrameData, 1);

    g_signal_connect (loader, "area-updated",
                      (GCallback) callback_area_updated_anim, (gpointer) frame_copy);
    g_signal_connect (loader, "closed",
                      (GCallback) callback_closed_anim, (gpointer) frame_copy);

    frame_copy->time.tv_sec = frame_copy->time.tv_usec = 0; /* some time */

    anim = gdk_pixbuf_loader_get_animation (loader);
    frame_copy->iter = gdk_pixbuf_animation_get_iter (anim, &frame_copy->time);
    frame_copy->pixbuf = gdk_pixbuf_copy (gdk_pixbuf_animation_iter_get_pixbuf (frame_copy->iter));
    update_currently_loaded_frame (frame_copy);
}

static void
test_area_updated_anim (gconstpointer data)
{
    const char *filename;
    GIOChannel *channel;
    GdkPixbufLoader *loader;

    filename = g_test_get_filename (G_TEST_DIST, data, NULL);

    channel = g_io_channel_new_file (filename, "r", NULL);
    g_assert_nonnull (channel);
    g_io_channel_set_encoding(channel, NULL, NULL);
    /*create loader */
    loader = gdk_pixbuf_loader_new ();
    g_assert_nonnull (loader);

    g_signal_connect (loader, "area-prepared",
                      (GCallback) callback_area_prepared_anim, NULL);
    /* other callbacks will be registered inside callback_area_prepared_anim */

    /*read animation by portions of bytes */
#if 0
    while(loader_write_from_channel(loader, channel, 20) == 20);
#endif
    /* or read it at once */
    loader_write_from_channel (loader, channel, G_MAXSIZE);

    /* free resources */
    g_io_channel_unref (channel);
    gdk_pixbuf_loader_close (loader, NULL);
    g_object_unref (loader);
}

int main(int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/pixbuf/area-updated/ico", (gconstpointer) "test-images/reftests/squares.ico", test_area_updated_ico);
  g_test_add_data_func ("/pixbuf/area-updated/gif", (gconstpointer) "aero.gif", test_area_updated_anim);
  g_test_add_data_func ("/pixbuf/area-updated/gif2", (gconstpointer) "1_partyanimsm2.gif", test_area_updated_anim);
  g_test_add_data_func ("/pixbuf/area-updated/gif3", (gconstpointer) "test-animation.gif", test_area_updated_anim);

  return g_test_run ();
}
