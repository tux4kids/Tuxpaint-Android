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

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include "test-common.h"

static void
randomly_modify (const gchar *image, guint size)
{
  int i, n;

  guchar *img_copy = g_malloc (size);
  memmove (img_copy, image, size);
  
  n = MIN (100, size / 4);

  for (i = 0; i < n; i++)
    {
      FILE *f;
      GdkPixbufLoader *loader;
      
      guint index = g_test_rand_int_range (0, size);
      guchar byte = g_test_rand_int_range (0, 256);
      
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
  GFile *file = G_FILE (data);
  gchar *buffer;
  gsize size;
  gint iterations;
  gint i;
  GError *error = NULL;

  g_file_load_contents (file, NULL, &buffer, &size, NULL, &error);
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
  gchar *base_dir;
  GFile *base, *test_images;
#ifdef HAVE_SETRLIMIT
  struct rlimit max_mem_size;

  max_mem_size.rlim_cur = 100 * 1024 * 1024; /* 100M */
  max_mem_size.rlim_max = max_mem_size.rlim_cur;
  setrlimit (RLIMIT_DATA, &max_mem_size);
#endif

  g_test_init (&argc, &argv, NULL);

  base_dir = g_build_filename (g_test_get_dir (G_TEST_DIST), "test-images", NULL);
  base = g_file_new_for_path (base_dir);

  test_images = g_file_get_child (base, "randomly-modified");
  add_test_for_all_images ("/pixbuf", base, test_images, test_randomly_modified, NULL);
  g_object_unref (test_images);

  test_images = g_file_get_child (base, "fail");
  add_test_for_all_images ("/pixbuf/randomly-modified", base, test_images, test_randomly_modified, NULL);
  g_object_unref (test_images);

  g_object_unref (base);
  g_free (base_dir);

  g_test_message ("Modified image is written to pixbuf-randomly-modified-image");

  return g_test_run ();
}
