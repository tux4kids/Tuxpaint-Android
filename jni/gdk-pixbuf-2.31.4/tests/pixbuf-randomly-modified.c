/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2001 Søren Sandmann (sandmann@daimi.au.dk)
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
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

static void
randomly_modify (const gchar *image, guint size)
{
  int i;

  guchar *img_copy = g_malloc (size);
  g_memmove (img_copy, image, size);
  
  for (i = 0; i < size / 4; i++)
    {
      FILE *f;
      GdkPixbufLoader *loader;
      
      guint index = g_random_int_range (0, size);
      guchar byte = g_random_int_range (0, 256);
      
      img_copy[index] = byte;
      f = fopen ("pixbuf-randomly-modified-image", "w");
      g_assert (f != NULL);
      fwrite (img_copy, size, 1, f);
      g_assert (!ferror (f));
      fclose (f);

      loader = gdk_pixbuf_loader_new ();
      gdk_pixbuf_loader_write (loader, img_copy, size, NULL);
      gdk_pixbuf_loader_close (loader, NULL);
      g_object_unref (loader);
    }
  g_free (img_copy);
}

static void
test_randomly_modified (gconstpointer data)
{
  const gchar *file = data;
  const gchar *path;
  gchar *buffer;
  gsize size;
  gint iterations;
  gint i;
  GError *error = NULL;

  path = g_test_get_filename (G_TEST_DIST, "test-images", file, NULL);
  g_file_get_contents (path, &buffer, &size, &error);
  g_assert_no_error (error);

  if (g_test_thorough ())
    iterations = 1000;
  else
    iterations = 1;

  for (i = 0; i < iterations; i++)
    randomly_modify (buffer, size);

  g_free (buffer);
}

int
main (int argc, char **argv)
{
  const gchar *name;
  gchar *test_images_dir;
  gchar *path;
  GDir *dir;
#ifdef HAVE_SETRLIMIT
  struct rlimit max_mem_size;

  max_mem_size.rlim_cur = 100 * 1024 * 1024; /* 100M */
  max_mem_size.rlim_max = max_mem_size.rlim_cur;
  setrlimit (RLIMIT_DATA, &max_mem_size);
#ifdef RLIMIT_AS
  setrlimit (RLIMIT_AS, &max_mem_size);
#endif
#endif

  g_test_init (&argc, &argv, NULL);

  test_images_dir = g_build_filename (g_test_get_dir (G_TEST_DIST), "test-images", NULL);
  dir = g_dir_open (test_images_dir, 0, NULL);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      path = g_strconcat ("/pixbuf/randomly-modified/", name, NULL);
      g_test_add_data_func_full (path, g_strdup (name), test_randomly_modified, g_free);
      g_free (path);
    }
  g_dir_close (dir);
  g_free (test_images_dir);

  g_test_message ("Modified image is written to pixbuf-randomly-modified-image");

  return g_test_run ();
}
