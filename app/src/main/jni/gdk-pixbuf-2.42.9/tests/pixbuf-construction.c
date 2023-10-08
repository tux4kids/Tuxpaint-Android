/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* GdkPixbuf library - tests for GdkPixbuf constructors
 *
 * Copyright (C) 2018 Federico Mena Quintero
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
#include "test-common.h"

static void
test_no_construct_properties (void)
{
  GdkPixbuf *pixbuf = g_object_new (GDK_TYPE_PIXBUF, NULL);
  GBytes *bytes;
  guchar *pixels;

  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 1);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 1);

  g_object_get (pixbuf, "pixel-bytes", &bytes, NULL);
  g_assert (bytes != NULL);
  g_bytes_unref (bytes);

  g_object_get (pixbuf, "pixels", &pixels, NULL);
  g_assert (pixels != NULL);
}

#define WIDTH 10
#define HEIGHT 20
#define BUFFER_SIZE (WIDTH * HEIGHT * 4)
#define ROWSTRIDE (WIDTH * 4)

static void
test_pixels (void)
{
  guchar *pixels = g_new0 (guchar, BUFFER_SIZE);

  GdkPixbuf *pixbuf = g_object_new (GDK_TYPE_PIXBUF,
				    "width", WIDTH,
				    "height", HEIGHT,
				    "rowstride", ROWSTRIDE,
				    "bits-per-sample", 8,
				    "n-channels", 3,
				    "has-alpha", TRUE,
				    "pixels", pixels,
				    NULL);

  g_assert (gdk_pixbuf_get_pixels (pixbuf) == pixels);
  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, WIDTH);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, HEIGHT);
  g_assert_cmpint (gdk_pixbuf_get_rowstride (pixbuf), ==, ROWSTRIDE);

  g_object_unref (pixbuf);
  g_free (pixels);
}

static void
test_bytes (void)
{
  guchar *pixels = g_new0 (guchar, BUFFER_SIZE);
  GBytes *bytes = g_bytes_new_take (pixels, BUFFER_SIZE);
  GdkPixbuf *pixbuf = g_object_new (GDK_TYPE_PIXBUF,
				    "width", WIDTH,
				    "height", HEIGHT,
				    "rowstride", ROWSTRIDE,
				    "bits-per-sample", 8,
				    "n-channels", 3,
				    "has-alpha", TRUE,
				    "pixel-bytes", bytes,
				    NULL);

  GBytes *read_bytes = gdk_pixbuf_read_pixel_bytes (pixbuf);

  g_assert (read_bytes == bytes);
  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, WIDTH);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, HEIGHT);
  g_assert_cmpint (gdk_pixbuf_get_rowstride (pixbuf), ==, ROWSTRIDE);

  g_bytes_unref (read_bytes);
  g_object_unref (pixbuf);
  g_bytes_unref (bytes);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/construction/no_construct_properties", test_no_construct_properties);
  g_test_add_func ("/pixbuf/construction/pixels", test_pixels);
  g_test_add_func ("/pixbuf/construction/bytes", test_bytes);

  return g_test_run ();
}
