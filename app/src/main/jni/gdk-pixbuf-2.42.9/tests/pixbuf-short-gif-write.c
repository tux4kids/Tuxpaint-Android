#include "config.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include <glib.h>

/*
 * Reads count_bytes from the channel and write them to the loader.
 * Returns number of bytes written.
 * count_bytes = G_MAXSIZE means read as many bytes as possible.
 */
static gsize
loader_write_from_channel (GdkPixbufLoader *loader,
			   GIOChannel      *channel,
			   gsize            count_bytes)
{
    guchar* buffer = NULL;
    gsize bytes_read = 0;
    GIOStatus read_status;
    GError* error = NULL;
    gboolean ret;
    
    if(count_bytes < G_MAXSIZE) {
        //read no more than 'count_bytes' bytes
        buffer = g_malloc (count_bytes);
        read_status = g_io_channel_read_chars (channel, (gchar*)buffer, count_bytes, &bytes_read, NULL);
    } else {
        //read up to end
        read_status = g_io_channel_read_to_end (channel, (gchar**)&buffer, &bytes_read, NULL);
    }
    g_assert (read_status == G_IO_STATUS_NORMAL || read_status == G_IO_STATUS_EOF);

    ret = gdk_pixbuf_loader_write(loader, buffer, bytes_read, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_free(buffer);
    return bytes_read;
}

static void
test_short_gif_write (void)
{
    GdkPixbufLoader *loader;
    GIOChannel* channel = g_io_channel_new_file (g_test_get_filename (G_TEST_DIST, "test-animation.gif", NULL), "r", NULL);

    g_assert (channel != NULL);
    g_io_channel_set_encoding (channel, NULL, NULL);

    loader = gdk_pixbuf_loader_new_with_type ("gif", NULL);
    g_assert (loader != NULL);

    loader_write_from_channel (loader, channel, 10);
    loader_write_from_channel (loader, channel, G_MAXSIZE);

    g_io_channel_unref (channel);

    gdk_pixbuf_loader_close (loader, NULL);
    g_object_unref (loader);
}

static void
test_load_first_frame (void)
{
    GIOChannel* channel;
    gboolean has_frame = FALSE;
    GError *error = NULL;
    GdkPixbuf *pixbuf;
    GdkPixbufLoader *loader;

    channel = g_io_channel_new_file (g_test_get_filename (G_TEST_DIST, "1_partyanimsm2.gif", NULL), "r", NULL);
    g_assert (channel != NULL);
    g_io_channel_set_encoding (channel, NULL, NULL);

    loader = gdk_pixbuf_loader_new_with_type ("gif", NULL);
    g_assert (loader != NULL);

    while (!has_frame) {
        GdkPixbufAnimation *animation;

        loader_write_from_channel (loader, channel, 4096);
        animation = gdk_pixbuf_loader_get_animation (loader);
        if (animation) {
            GdkPixbufAnimationIter *iter = gdk_pixbuf_animation_get_iter (animation, NULL);
            if (!gdk_pixbuf_animation_iter_on_currently_loading_frame (iter))
                has_frame = TRUE;
            g_object_unref (iter);
        }
    }

    g_io_channel_unref (channel);

    gdk_pixbuf_loader_close (loader, &error);
    g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION);
    g_clear_error (&error);
    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    g_assert (pixbuf);
    g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 660);
    g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 666);
    g_object_unref (loader);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/animation/short_gif_write", test_short_gif_write);
  g_test_add_func ("/animation/load_first_frame", test_load_first_frame);

  return g_test_run ();
}
