/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2015 Red Hat, Inc.
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
 * Author: Benjamin Otte
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "test-common.h"

static void
test_fail_size (GFile *file,
		guint  chunk_size)
{
  GdkPixbufLoader *loader;
  GError *error = NULL;
  guchar *contents;
  gsize i, contents_length;
  char *filename;
  gboolean success;

  if (!file_supported (file))
    {
      g_test_skip ("format not supported");
      return;
    }

  filename = g_file_get_path (file);

  success = g_file_load_contents (file, NULL, (gchar **) &contents, &contents_length, NULL, &error);
  g_assert_no_error (error);
  g_assert (success);

#ifdef GDK_PIXBUF_USE_GIO_MIME
  {
    char *mime_type, *content_type;

    content_type = g_content_type_guess (filename, contents, contents_length, NULL);
    mime_type = g_content_type_get_mime_type (content_type);
    g_assert (mime_type);
    loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, &error);
    g_free (mime_type);
    g_free (content_type);
  }
#else
  {
    char *format;

    success = find_format (filename, &format);
    g_assert_true (success);
    loader = gdk_pixbuf_loader_new_with_type (format, &error);
    g_free (format);
  }
#endif
  g_assert_no_error (error);
  g_assert (loader != NULL);

  for (i = 0; i < contents_length; i += chunk_size)
    {
      success = gdk_pixbuf_loader_write (loader, &contents[i], MIN(chunk_size, contents_length - i), &error);
      if (!success)
        {
          g_assert (error);
          g_clear_error (&error);
          goto out;
        }
      g_assert_no_error (error);
    }
  
  success = gdk_pixbuf_loader_close (loader, &error);
  g_assert (!success);
  g_assert (error);
  g_clear_error (&error);

out:
  g_free (contents);
  g_object_unref (loader);
  g_free (filename);
}

static void
test_fail_tiny (gconstpointer data)
{
  GFile *file = (GFile *) data;

  test_fail_size (file, 1);
}

static void
test_fail_huge (gconstpointer data)
{
  GFile *file = (GFile *) data;

  test_fail_size (file, G_MAXUINT);
}

int
main (int argc, char **argv)
{

  g_test_init (&argc, &argv, NULL);

  if (argc < 2)
    {
      GFile *dir;
      gchar *test_images;

      test_images = g_build_filename (g_test_get_dir (G_TEST_DIST), "test-images/fail", NULL);
      dir = g_file_new_for_path (test_images);
      
      add_test_for_all_images ("/pixbuf/fail_tiny", dir, dir, test_fail_tiny, NULL);
      add_test_for_all_images ("/pixbuf/fail_huge", dir, dir, test_fail_huge, NULL);

      g_object_unref (dir);
      g_free (test_images);
    }
  else
    {
      guint i;

      for (i = 1; i < argc; i++)
        {
          GFile *file = g_file_new_for_commandline_arg (argv[i]);

          add_test_for_all_images ("/pixbuf/fail_tiny", NULL, file, test_fail_tiny, NULL);
          add_test_for_all_images ("/pixbuf/fail_huge", NULL, file, test_fail_huge, NULL);

          g_object_unref (file);
        }
    }

  return g_test_run ();
}
