#include "config.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include <glib.h>

static void
test_gif_circular_table (void)
{
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "circular-table.gif", NULL), &error);
  g_assert_no_error (error);

  g_object_unref (pixbuf);
}

int main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/gif/circular-table", test_gif_circular_table);

  return g_test_run ();
}
