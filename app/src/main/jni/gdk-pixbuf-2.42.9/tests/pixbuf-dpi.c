/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2014 Canonical Ltd.
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
 * Author: Robert Ancell
 */

#include <string.h>

#include "config.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"

static void
test_incremental (gconstpointer data)
{
  const gchar *filename = data;
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  const gchar *x_dpi, *y_dpi;
  gchar *contents;
  gsize size;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  g_file_get_contents (g_test_get_filename (G_TEST_DIST, filename, NULL), &contents, &size, &error);
  g_assert_no_error (error);

  loader = gdk_pixbuf_loader_new ();

  gdk_pixbuf_loader_write (loader, (const guchar*)contents, size, &error);
  g_assert_no_error (error);
  
  gdk_pixbuf_loader_close (loader, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  g_assert_nonnull (pixbuf);
  x_dpi = gdk_pixbuf_get_option (pixbuf, "x-dpi");
  y_dpi = gdk_pixbuf_get_option (pixbuf, "y-dpi");
  g_assert_nonnull (x_dpi);
  g_assert_nonnull (y_dpi);
  g_assert_cmpstr (x_dpi, ==, "300");
  g_assert_cmpstr (y_dpi, ==, "600");

  g_object_unref (loader);
  g_free (contents);
}

static void
test_nonincremental (gconstpointer data)
{
  const gchar *filename = data;
  GError *error = NULL;
  GdkPixbuf *pixbuf;
  const gchar *x_dpi, *y_dpi;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  pixbuf = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, filename, NULL), &error);
  g_assert_no_error (error);

  x_dpi = gdk_pixbuf_get_option (pixbuf, "x-dpi");
  y_dpi = gdk_pixbuf_get_option (pixbuf, "y-dpi");
  g_assert_nonnull (x_dpi);
  g_assert_nonnull (y_dpi);
  g_assert_cmpstr (x_dpi, ==, "300");
  g_assert_cmpstr (y_dpi, ==, "600");

  g_object_unref (pixbuf);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/pixbuf/dpi/png", "dpi.png", test_nonincremental);
  g_test_add_data_func ("/pixbuf/dpi/png-incremental", "dpi.png", test_incremental);
  g_test_add_data_func ("/pixbuf/dpi/jpeg", "dpi.jpeg", test_nonincremental);
  g_test_add_data_func ("/pixbuf/dpi/jpeg-incremental", "dpi.jpeg", test_incremental);
  g_test_add_data_func ("/pixbuf/dpi/tiff", "dpi.tiff", test_nonincremental);
  g_test_add_data_func ("/pixbuf/dpi/tiff-incremental", "dpi.tiff", test_incremental);

  return g_test_run ();
}
