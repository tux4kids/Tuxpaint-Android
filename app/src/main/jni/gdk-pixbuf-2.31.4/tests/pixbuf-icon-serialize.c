#include "config.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"
#include <string.h>
#include <glib.h>

static void
test_serialize (void)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf;
  GdkPixbuf *pixbuf2;
  GVariant *data;
  GIcon *icon;
  GInputStream *stream;

  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  pixbuf = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "test-image.png", NULL), &error);
  g_assert_no_error (error);
  g_assert (pixbuf != NULL);

  /* turn it into a GVariant */
  data = g_icon_serialize (G_ICON (pixbuf));

  /* back to a GIcon, but this will be a GBytesIcon, not GdkPixbuf */
  icon = g_icon_deserialize (data);
  g_assert (G_IS_BYTES_ICON (icon));

  /* but since that is a GLoadableIcon, we can load it again */
  stream = g_loadable_icon_load (G_LOADABLE_ICON (icon), 0, NULL, NULL, &error);
  g_assert_no_error (error);
  pixbuf2 = gdk_pixbuf_new_from_stream (stream, NULL, &error);
  g_assert_no_error (error);

  /* make sure that the pixels are the same.
   * our _serialize() uses png, so this should be perfect.
   */
  {
    guchar *pixels_a, *pixels_b;
    guint len_a, len_b;
    pixels_a = gdk_pixbuf_get_pixels_with_length (pixbuf, &len_a);
    pixels_b = gdk_pixbuf_get_pixels_with_length (pixbuf2, &len_b);
    g_assert (len_a == len_b);
    g_assert (memcmp (pixels_a, pixels_b, len_a) == 0);
  }

  g_object_unref (pixbuf2);
  g_object_unref (pixbuf);
  g_object_unref (stream);
  g_variant_unref (data);

}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/icon/serialize", test_serialize);

  return g_test_run ();
}
