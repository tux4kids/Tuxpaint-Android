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
#include "gdk-pixbuf/gdk-pixdata.h"
#include "test-common.h"
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
test_pixdata_deserialize (gconstpointer data)
{
  const gchar *filename = data;
  GdkPixbuf *pixbuf;
  GdkPixdata pixdata;
  GError *error = NULL;
  gchar *contents;
  gsize size;

  g_file_get_contents (g_test_get_filename (G_TEST_DIST, filename, NULL), &contents, &size, &error);
  g_assert_no_error (error);

  gdk_pixdata_deserialize (&pixdata, size, (const guint8 *) contents, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_from_pixdata (&pixdata, TRUE, &error);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE);
  g_clear_error (&error);
  g_free (contents);

  pixbuf = gdk_pixbuf_from_pixdata (&pixdata, FALSE, &error);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE);
  g_clear_error (&error);

  g_clear_object (&pixbuf);
}

static void
test_pixdata_success (void)
{
  const gchar *path;
  GdkPixbuf *pixbuf;
  GdkPixdata pixdata1, pixdata2;
  GError *error = NULL;
  GdkPixbuf *ref;
  gchar *contents;
  gsize size;

  path = g_test_get_filename (G_TEST_DIST, "test-image.png", NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);
  g_assert_no_error (error);

  g_file_get_contents (g_test_get_filename (G_TEST_DIST, "test-image.pixdata", NULL), &contents, &size, &error);
  g_assert_no_error (error);
  gdk_pixdata_deserialize (&pixdata1, size, (const guint8 *) contents, &error);
  g_assert_no_error (error);
  pixbuf = gdk_pixbuf_from_pixdata (&pixdata1, FALSE, &error);
  g_assert_no_error (error);

  pixdata_equal (ref, pixbuf, &error);
  g_assert_no_error (error);
  g_free (contents);
  g_object_unref (pixbuf);

  g_file_get_contents (g_test_get_filename (G_TEST_DIST, "test-image-rle.pixdata", NULL), &contents, &size, &error);
  g_assert_no_error (error);
  gdk_pixdata_deserialize (&pixdata2, size, (const guint8 *) contents, &error);
  g_assert_no_error (error);
  pixbuf = gdk_pixbuf_from_pixdata (&pixdata2, FALSE, &error);
  g_assert_no_error (error);

  pixdata_equal (ref, pixbuf, &error);
  g_assert_no_error (error);
  g_free (contents);
  g_object_unref (pixbuf);
  g_object_unref (ref);
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_pixdata (void)
{
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;

  ref = gdk_pixbuf_new_from_resource ("/test/resource/icc-profile.pixdata", &error);
  g_assert_no_error (error);
  g_object_unref (ref);

  ref = gdk_pixbuf_new_from_resource ("/test/resource/icc-profile-compressed.pixdata", &error);
  g_assert_no_error (error);
  g_object_unref (ref);

  path = g_test_get_filename (G_TEST_DIST, "test-image.pixdata", NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_UNKNOWN_TYPE);
  g_clear_error (&error);
  g_clear_object (&ref);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/pixdata", test_pixdata);
  g_test_add_func ("/pixbuf/pixdata/success", test_pixdata_success);
  g_test_add_data_func ("/pixbuf/pixdata/bug775693", "bug775693.pixdata", test_pixdata_deserialize);
  g_test_add_data_func ("/pixbuf/pixdata/bug775229", "bug775229.pixdata", test_pixdata_deserialize);

  return g_test_run ();
}
