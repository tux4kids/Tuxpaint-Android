/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2018 Canonical Ltd.
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

#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"
#include <string.h>

const gchar *
get_test_path (const gchar *filename)
{
  return g_test_get_filename (G_TEST_DIST, "test-images", "gif-test-suite", filename, NULL);
}

static gboolean
pixels_match (GBytes *pixels0, GBytes *pixels1)
{
  gsize size0, size1, i;
  guint8 *data0, *data1;

  data0 = g_bytes_get_data (pixels0, &size0);
  data1 = g_bytes_get_data (pixels1, &size1);

  if (size0 != size1)
    return FALSE;

  for (i = 0; i < size0; i += 4)
   {
     guint8 red0, green0, blue0, alpha0;
     guint8 red1, green1, blue1, alpha1;

     red0 = data0[i + 0];
     green0 = data0[i + 1];
     blue0 = data0[i + 2];
     alpha0 = data0[i + 3];
     red1 = data1[i + 0];
     green1 = data1[i + 1];
     blue1 = data1[i + 2];
     alpha1 = data1[i + 3];
     if (alpha0 == 0 && alpha1 == 0)
        ; /* Transparent, don't care what the RGB is set to */
     else if (red0 != red1 || blue0 != blue1 || green0 != green1 || alpha0 != alpha1)
        return FALSE;
   }

  return TRUE;
}

static void
run_gif_test (gconstpointer data)
{
  const gchar *name = data;
  GKeyFile *config_file;
  gchar *config_filename, *input_filename;
  gint width, height;
  GFile *input_file;
  GBytes *input_bytes;
  GdkPixbufLoader *loader;
  GdkPixbufAnimation *animation = NULL;
  GdkPixbufAnimationIter *iter = NULL;
  GTimeVal animation_time;
  GStrv frames;
  int i;
  GError *error = NULL;

  if (!format_supported ("gif"))
    {
      g_test_skip ("GIF format not supported");
      return;
    }

  config_file = g_key_file_new ();
  g_key_file_set_list_separator (config_file, ',');
  config_filename = g_strdup_printf ("%s.conf", name);
  g_key_file_load_from_file (config_file, get_test_path (config_filename), G_KEY_FILE_NONE, &error);
  g_free (config_filename);
  g_assert_no_error (error);

  input_filename = g_key_file_get_string (config_file, "config", "input", &error);
  g_assert_no_error (error);
  input_file = g_file_new_for_path (get_test_path (input_filename));
  g_free (input_filename);
  input_bytes = g_file_load_bytes (input_file, NULL, NULL, &error);
  g_clear_object (&input_file);
  g_assert_no_error (error);

  width = g_key_file_get_integer (config_file, "config", "width", &error);
  g_assert_no_error (error);
  height = g_key_file_get_integer (config_file, "config", "height", &error);
  g_assert_no_error (error);

  loader = gdk_pixbuf_loader_new ();
  gdk_pixbuf_loader_write_bytes (loader, input_bytes, &error);
  g_clear_pointer (&input_bytes, g_bytes_unref);
  g_assert_no_error (error);

  gdk_pixbuf_loader_close (loader, &error);
  if (width == 0 || height == 0) {
    g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE);
    g_clear_error (&error);
  }
  else {
    g_assert_no_error (error);
    animation = gdk_pixbuf_loader_get_animation (loader);
    g_assert_nonnull (animation);
  }

  frames = g_key_file_get_string_list (config_file, "config", "frames", NULL, &error);
  g_assert_no_error (error);
  animation_time.tv_sec = 0;
  animation_time.tv_usec = 0;
  for (i = 0; frames[i]; i++)
    {
      const gchar *frame = frames[i];
      GdkPixbuf *pixbuf;
      gint delay_time, expected_delay_time = 100;
      gchar *pixels_filename;
      GFile *pixels_file;
      GBytes *expected_pixels, *pixels;

      g_assert_nonnull (animation);

      if (iter == NULL)
        iter = gdk_pixbuf_animation_get_iter (animation, &animation_time);
      else
        gdk_pixbuf_animation_iter_advance (iter, &animation_time);
      delay_time = gdk_pixbuf_animation_iter_get_delay_time (iter);
      g_time_val_add (&animation_time, gdk_pixbuf_animation_iter_get_delay_time (iter) * 1000);

      if (g_key_file_has_key (config_file, frame, "delay", &error))
        expected_delay_time = g_key_file_get_integer (config_file, frame, "delay", &error) * 10;
      g_assert_no_error (error);

      g_assert_cmpint (delay_time, ==, expected_delay_time);

      pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (iter);

      g_assert_cmpint (width, ==, gdk_pixbuf_get_width (pixbuf));
      g_assert_cmpint (height, ==, gdk_pixbuf_get_height (pixbuf));

      pixels_filename = g_key_file_get_string (config_file, frame, "pixels", &error);
      g_assert_no_error (error);
      pixels_file = g_file_new_for_path (get_test_path (pixels_filename));
      g_free (pixels_filename);
      expected_pixels = g_file_load_bytes (pixels_file, NULL, NULL, &error);
      g_clear_object (&pixels_file);
      g_assert_no_error (error);

      g_assert_cmpint (gdk_pixbuf_get_colorspace (pixbuf), ==, GDK_COLORSPACE_RGB);
      g_assert_cmpint (gdk_pixbuf_get_n_channels (pixbuf), ==, 4);
      g_assert_true (gdk_pixbuf_get_has_alpha (pixbuf));
      g_assert_cmpint (gdk_pixbuf_get_rowstride (pixbuf), ==, width * 4);
      pixels = g_bytes_new_static (gdk_pixbuf_read_pixels (pixbuf), gdk_pixbuf_get_byte_length (pixbuf));
      g_assert_true (pixels_match (pixels, expected_pixels));
      g_clear_pointer (&pixels, g_bytes_unref);
      g_clear_pointer (&expected_pixels, g_bytes_unref);
    }
  g_strfreev (frames);
  g_clear_object (&iter);

  /* FIXME: We should check here if there's more frames than we were expecting, but gdk-pixbuf doesn't return this information */

  g_clear_object (&loader);
  g_clear_pointer (&config_file, g_key_file_unref);
}

int
main (int argc, char **argv)
{
  gchar *path, *contents;
  GStrv lines;
  int i, result;
  GError *error = NULL;

  g_test_init (&argc, &argv, NULL);

  path = g_build_filename (g_test_get_dir (G_TEST_DIST), "test-images", "gif-test-suite", "TESTS", NULL);
  g_file_get_contents (path, &contents, NULL, &error);
  g_free (path);
  g_assert_no_error (error);
  lines = g_strsplit (contents, "\n", -1);
  g_free (contents);
  for (i = 0; lines[i]; i++)
    {
      const gchar *name = g_strstrip (lines[i]);
      gchar *test_name;

      if (g_strcmp0 (name, "") == 0 || name[0] == '#')
        continue;

      test_name = g_strdup_printf ("/pixbuf/gif/%s", name);
      g_test_add_data_func (test_name, name, run_gif_test);
      g_free (test_name);
    }

  result = g_test_run ();

  g_strfreev (lines);

  return result;
}
