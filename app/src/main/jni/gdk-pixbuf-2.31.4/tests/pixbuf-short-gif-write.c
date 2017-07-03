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
    if(count_bytes < G_MAXSIZE) {
        //read no more than 'count_bytes' bytes
        buffer = g_malloc (count_bytes);
        read_status = g_io_channel_read_chars (channel, (gchar*)buffer, count_bytes, &bytes_read, NULL);
    } else {
        //read up to end
        read_status = g_io_channel_read_to_end (channel, (gchar**)&buffer, &bytes_read, NULL);
    }
    g_assert (read_status == G_IO_STATUS_NORMAL || read_status == G_IO_STATUS_EOF);

    g_assert (gdk_pixbuf_loader_write(loader, buffer, bytes_read, &error));
    g_assert_no_error (error);
    g_free(buffer);
    return bytes_read;
}

static void
test_short_gif_write (void)
{
    GIOChannel* channel = g_io_channel_new_file (g_test_get_filename (G_TEST_DIST, "test-animation.gif", NULL), "r", NULL);
    g_assert (channel != NULL);
    g_io_channel_set_encoding (channel, NULL, NULL);

    GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type ("gif", NULL);
    g_assert (loader != NULL);

    loader_write_from_channel (loader, channel, 10);
    loader_write_from_channel (loader, channel, G_MAXSIZE);

    g_io_channel_unref (channel);

    gdk_pixbuf_loader_close (loader, NULL);
    g_object_unref (loader);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/animation/short_gif_write", test_short_gif_write);

  return g_test_run ();
}
