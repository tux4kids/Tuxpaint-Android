/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2004 Matthias Clasen <mclasen@redhat.com>
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
 */

#include "config.h"
#include <glib/gstdio.h>
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"

static void
load_image (gpointer data, 
	    gpointer user_data)
{
  gchar *filename = data;
  const gchar *path;
  FILE *file;
  int nbytes;
  guchar buffer[1024];
  GdkPixbufLoader *loader;
  GError *error = NULL;

  loader = gdk_pixbuf_loader_new ();
  path = g_test_get_filename (G_TEST_DIST, "test-images/randomly-modified", filename, NULL);

  g_test_message ("reading %s", path); 
  file = g_fopen (path, "rb");
  g_assert (file != NULL);

  while (!feof (file)) 
    {
      nbytes = fread (buffer, 1, sizeof (buffer), file);
      gdk_pixbuf_loader_write (loader, buffer, nbytes, &error);
      g_assert_no_error (error);
      g_thread_yield ();      
    }

  fclose (file);

  gdk_pixbuf_loader_close (loader, &error);
  g_assert_no_error (error);

  g_object_unref (loader);
}

static void
test_threads (void)
{
  GThreadPool *pool;
  gint iterations;
  gint i;

  pool = g_thread_pool_new (load_image, NULL, 20, FALSE, NULL);

  if (g_test_thorough ())
    iterations = 100;
  else
    iterations = 1;

  for (i = 0; i < iterations; i++)
    {
      if (format_supported ("jpeg"))
        g_thread_pool_push (pool, "valid.1.jpeg", NULL);
      if (format_supported ("png"))
        g_thread_pool_push (pool, "valid.1.png", NULL);
      if (format_supported ("gif"))
        g_thread_pool_push (pool, "valid.1.gif", NULL);
      if (format_supported ("bmp"))
        g_thread_pool_push (pool, "valid.1.bmp", NULL);
      if (format_supported ("jpeg"))
        g_thread_pool_push (pool, "valid.2.jpeg", NULL);
      if (format_supported ("xpm"))
        g_thread_pool_push (pool, "valid.1.xpm", NULL);
      if (format_supported ("tga"))
        g_thread_pool_push (pool, "valid.1.tga", NULL);
      if (format_supported ("tiff"))
        g_thread_pool_push (pool, "valid.1.tiff", NULL);
    }

  g_thread_pool_free (pool, FALSE, TRUE);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/threads", test_threads);

  return g_test_run ();
}
