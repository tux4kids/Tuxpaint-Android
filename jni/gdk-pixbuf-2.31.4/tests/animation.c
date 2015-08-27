#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>

static void
test_animation (const gchar *filename)
{
  GError *error = NULL;
  GdkPixbufAnimation* result = NULL;

  result = gdk_pixbuf_animation_new_from_file (g_test_get_filename (G_TEST_DIST, filename, NULL), &error);
  g_assert_no_error (error);
  g_assert (result != NULL);

  g_object_unref (result);
}

static void
test_gif_animation (void)
{
  test_animation ("test-animation.gif");
}

static void
test_ani_animation (void)
{
  test_animation ("test-animation.ani");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/animation/gif", test_gif_animation);
  g_test_add_func ("/animation/ani", test_ani_animation);

  return g_test_run ();
}
