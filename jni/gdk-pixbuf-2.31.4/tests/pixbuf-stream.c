/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2013 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */

#include "config.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"
#include <string.h>

#define compare_option(p1, p2, key) \
  g_strcmp0 (gdk_pixbuf_get_option (p1, key), gdk_pixbuf_get_option (p2, key))

static gboolean
pixbuf_equal (GdkPixbuf *p1, GdkPixbuf *p2)
{
  if (gdk_pixbuf_get_colorspace (p1) != gdk_pixbuf_get_colorspace (p2))
    return FALSE;
  if (gdk_pixbuf_get_n_channels (p1) != gdk_pixbuf_get_n_channels (p2))
    return FALSE;
  if (gdk_pixbuf_get_bits_per_sample (p1) != gdk_pixbuf_get_bits_per_sample (p2))
    return FALSE;
  if (gdk_pixbuf_get_width (p1) != gdk_pixbuf_get_width (p2))
    return FALSE;
  if (gdk_pixbuf_get_height (p1) != gdk_pixbuf_get_height (p2))
    return FALSE;
  if (gdk_pixbuf_get_rowstride (p1) != gdk_pixbuf_get_rowstride (p2))
    return FALSE;
  if (memcmp (gdk_pixbuf_get_pixels (p1), gdk_pixbuf_get_pixels (p2),
          gdk_pixbuf_get_byte_length (p1)) != 0)
    return FALSE;
  if (compare_option (p1, p2, "Title") != 0)
    return FALSE;
  if (compare_option (p1, p2, "Artist") != 0)
    return FALSE;
  if (compare_option (p1, p2, "x_hot") != 0)
    return FALSE;
  if (compare_option (p1, p2, "y_hot") != 0)
    return FALSE;
  if (compare_option (p1, p2, "orientation") != 0)
    return FALSE;
  if (compare_option (p1, p2, "icc-profile") != 0)
    return FALSE;

  return TRUE;
}

static void
test_stream (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *pixbuf, *ref;
  GFile *file;
  GInputStream *stream;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);
  g_assert_no_error (error);

  file = g_file_new_for_path (path);
  stream = (GInputStream *)g_file_read (file, NULL, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
  g_assert_no_error (error);
  g_assert (pixbuf_equal (pixbuf, ref));
  g_object_unref (pixbuf);
  
  g_object_unref (stream);
  g_object_unref (file);
  g_object_unref (ref);
}

static void
async_done_cb (GObject *source, GAsyncResult *res, gpointer data)
{
  GdkPixbuf *ref = data;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, &error);
  g_assert_no_error (error);

  g_assert (pixbuf_equal (pixbuf, ref));

  g_object_unref (pixbuf);
  g_object_unref (ref);
}

static void
test_stream_async (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;
  gchar *buffer;
  gsize size;
  GInputStream *stream;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);
  g_assert_no_error (error);

  g_file_get_contents (path, &buffer, &size, &error);
  g_assert_no_error (error);

  stream = g_memory_input_stream_new_from_data (buffer, size, g_free);
  gdk_pixbuf_new_from_stream_async (stream, NULL, async_done_cb, ref);
  g_object_unref (stream);
}

static void
test_stream_at_scale (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *pixbuf, *ref;
  GFile *file;
  GInputStream *stream;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file_at_scale (path, 20, 30, TRUE, &error);
  g_assert_no_error (error);

  file = g_file_new_for_path (path);
  stream = (GInputStream *)g_file_read (file, NULL, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_stream_at_scale (stream, 20, 30, TRUE, NULL, &error);
  g_assert_no_error (error);
  g_assert (pixbuf_equal (pixbuf, ref));
  g_object_unref (pixbuf);
  
  g_object_unref (stream);
  g_object_unref (file);
  g_object_unref (ref);
}

static void
test_stream_at_scale_async (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;
  gchar *buffer;
  gsize size;
  GInputStream *stream;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file_at_scale (path, 40, 10, FALSE, &error);
  g_assert_no_error (error);

  g_file_get_contents (path, &buffer, &size, &error);
  g_assert_no_error (error);

  stream = g_memory_input_stream_new_from_data (buffer, size, g_free);
  gdk_pixbuf_new_from_stream_at_scale_async (stream, 40, 10, FALSE, NULL, async_done_cb, ref);
  g_object_unref (stream);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/pixbuf/stream", "icc-profile.png", test_stream);
  g_test_add_data_func ("/pixbuf/stream/async", "icc-profile.png", test_stream_async);
  g_test_add_data_func ("/pixbuf/stream/scale", "icc-profile.png", test_stream_at_scale);
  g_test_add_data_func ("/pixbuf/stream/scale/async", "icc-profile.png", test_stream_at_scale_async);

  return g_test_run ();
}
