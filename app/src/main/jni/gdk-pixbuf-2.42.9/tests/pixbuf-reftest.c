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

/* As defined in gdk-pixbuf-private.h */
#define DEFAULT_FILL_COLOR 0x979899ff

static void
loader_size_prepared (GdkPixbufLoader  *loader,
                      int               w,
                      int               h,
                      GdkPixbuf       **pixbuf)
{
  g_assert (*pixbuf == NULL);

  *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, w, h);
  g_assert (*pixbuf != NULL);
  /* likely!! */
  gdk_pixbuf_fill (*pixbuf, DEFAULT_FILL_COLOR);
}

static void
loader_area_prepared (GdkPixbufLoader  *loader,
                      GdkPixbuf       **pixbuf)
{
  g_assert (*pixbuf != NULL);

  if (gdk_pixbuf_get_has_alpha (gdk_pixbuf_loader_get_pixbuf (loader)))
    {
      GdkPixbuf *alpha = gdk_pixbuf_add_alpha (*pixbuf, FALSE, 0, 0, 0);

      g_object_unref (*pixbuf);
      *pixbuf = alpha;
    }

  g_assert (*pixbuf != NULL);
}

static void
loader_area_updated (GdkPixbufLoader  *loader,
                     int               x,
                     int               y,
                     int               w,
                     int               h,
                     GdkPixbuf       **pixbuf)
{
  gdk_pixbuf_copy_area (gdk_pixbuf_loader_get_pixbuf (loader),
                        x, y,
                        w, h,
                        *pixbuf,
                        x, y);
}

static GFile *
make_ref_file (GFile *file)
{
  char *uri, *ref_uri;
  GFile *result;

  uri = g_file_get_uri (file);
  ref_uri = g_strconcat (uri, ".ref.png", NULL);

  result = g_file_new_for_uri (ref_uri);

#ifdef G_OS_WIN32
  /* XXX: The .ref.png files are symlinks and on Windows git will create
   * files containing the symlink target instead of symlinks. */
  {
    char *contents;
    gboolean success;

    success = g_file_load_contents (result, NULL, &contents, NULL, NULL, NULL);
    if (success)
      {
        if (g_str_is_ascii (contents))
          {
            GFile *parent = g_file_get_parent (result);
            g_object_unref (result);
            result = g_file_get_child (parent, contents);
            g_object_unref (parent);
          }
        g_free (contents);
      }
  }
#endif

  g_free (ref_uri);
  g_free (uri);

  return result;
}

static gboolean
is_not_ref_image (GFile *file)
{
  char *uri;
  gboolean result;

  uri = g_file_get_uri (file);

  result = !g_str_has_suffix (uri, ".ref.png");

  g_free (uri);

  return result;
}

static void
test_reftest (gconstpointer data)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *reference, *loaded = NULL;
  GError *error = NULL;
  GFile *file, *ref_file;
  GInputStream *stream;
  guchar *contents;
  gsize i, contents_length;
  char *filename;
  gboolean success;

  file = G_FILE (data);
  if (!file_supported (file))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref_file = make_ref_file (file);
  if (!file_supported (ref_file))
    {
      g_test_skip ("format not supported for reference file");
      return;
    }

  filename = g_file_get_path (file);

  g_test_message ("Loading ref file '%s' for test file '%s'", g_file_peek_path (ref_file), filename);

  stream = G_INPUT_STREAM (g_file_read (ref_file, NULL, &error));
  g_assert_no_error (error);
  g_assert (stream != NULL);
  reference = gdk_pixbuf_new_from_stream (stream, NULL, &error);
  g_assert_no_error (error);
  g_assert (reference != NULL);
  g_object_unref (stream);

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
  g_signal_connect (loader, "size-prepared", G_CALLBACK (loader_size_prepared), &loaded);
  g_signal_connect (loader, "area-prepared", G_CALLBACK (loader_area_prepared), &loaded);
  g_signal_connect (loader, "area-updated", G_CALLBACK (loader_area_updated), &loaded);

  for (i = 0; i < contents_length; i++)
    {
      success = gdk_pixbuf_loader_write (loader, &contents[i], 1, &error);
      g_assert_no_error (error);
      g_assert (success);
    }
  
  success = gdk_pixbuf_loader_close (loader, &error);
  g_assert_no_error (error);
  g_assert (success);

  g_assert (loaded != NULL);

  success = pixdata_equal (loaded, reference, &error);
  g_assert_no_error (error);
  g_assert (success);

  g_free (contents);
  g_object_unref (loaded);
  g_object_unref (loader);
  g_object_unref (reference);
  g_object_unref (ref_file);
  g_free (filename);
}

int
main (int argc, char **argv)
{

  g_test_init (&argc, &argv, NULL);

  if (argc < 2)
    {
      GFile *dir;
      gchar *test_images;

      test_images = g_build_filename (g_test_get_dir (G_TEST_DIST), "test-images/reftests", NULL);
      dir = g_file_new_for_path (test_images);
      
      add_test_for_all_images ("/pixbuf/reftest", dir, dir, test_reftest, is_not_ref_image);

      g_object_unref (dir);
      g_free (test_images);
    }
  else
    {
      guint i;

      for (i = 1; i < argc; i++)
        {
          GFile *file = g_file_new_for_commandline_arg (argv[i]);

          add_test_for_all_images ("/pixbuf/reftest", NULL, file, test_reftest, is_not_ref_image);

          g_object_unref (file);
        }
    }

  return g_test_run ();
}
