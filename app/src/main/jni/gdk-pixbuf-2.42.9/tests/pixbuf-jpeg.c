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

#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"
#include <string.h>

static void
test_inverted_cmyk_jpeg (void)
{
  GError *error = NULL;
  GdkPixbuf *ref, *ref2;
  gboolean ret;

  if (!format_supported ("jpeg") || !format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "premature-end.jpg", NULL), &error);
  g_assert_no_error (error);

  ref2 = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "premature-end.png", NULL), &error);
  g_assert_no_error (error);

  ret = pixdata_equal (ref, ref2, &error);
  g_assert_no_error (error);
  g_assert (ret);

  g_object_unref (ref);
  g_object_unref (ref2);
}

static void
test_type9_rotation_exif_tag (void)
{
  GError *error = NULL;
  GdkPixbuf *ref, *ref1, *ref2;
  gboolean ret;

  if (!format_supported ("jpeg") || !format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "bug725582-testrotate.jpg", NULL), &error);
  g_assert_no_error (error);
  ref1 = gdk_pixbuf_apply_embedded_orientation (ref);

  ref2 = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "bug725582-testrotate.png", NULL), &error);
  g_assert_no_error (error);

  ret = pixdata_equal (ref1, ref2, &error);
  g_assert_no_error (error);
  g_assert (ret);

  g_assert_cmpstr (gdk_pixbuf_get_option (ref, "orientation"), ==, "6");

  g_object_unref (ref);
}

static void
test_bug_775218 (void)
{
  GError *error = NULL;
  GdkPixbuf *ref;

  if (!format_supported ("jpeg"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "bug775218.jpg", NULL), &error);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE);
  g_clear_object (&ref);
}

static void
test_comment(void)
{
  GError *error = NULL;
  GdkPixbuf *ref;

  if (!format_supported ("jpeg") || !format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "bug143608-comment.jpg", NULL), &error);
  g_assert_no_error (error);

  g_assert_cmpstr (gdk_pixbuf_get_option (ref, "comment"), ==, "COMMENT HERE");
  g_object_unref (ref);
}

static void
test_at_size (void)
{
  GError *error = NULL;
  GdkPixbuf *ref;

  if (!format_supported ("jpeg") || !format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "bug753605-atsize.jpg", NULL), &error);
  g_assert_no_error (error);
  g_object_unref (ref);

  ref = gdk_pixbuf_new_from_file_at_size (g_test_get_filename (G_TEST_DIST, "bug753605-atsize.jpg", NULL),
					  50, 50, &error);
  g_assert_no_error (error);
  g_object_unref (ref);
}

static void
test_jpeg_markers (void)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  gchar *contents;
  gsize size;

  if (!format_supported ("jpeg"))
    {
      g_test_skip ("format not supported");
      return;
    }

  g_file_get_contents (g_test_get_filename (G_TEST_DIST, "issue70.jpg", NULL), &contents, &size, &error);
  g_assert_no_error (error);

  loader = gdk_pixbuf_loader_new ();

  gdk_pixbuf_loader_write (loader, (const guchar*)contents, size, &error);
  g_assert_no_error (error);

  gdk_pixbuf_loader_close (loader, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  g_assert_nonnull (pixbuf);

  g_object_unref (loader);
  g_free (contents);
}

static void
test_jpeg_fbfbfbfb (void)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  gchar *contents;
  gsize size;

  if (!format_supported ("jpeg"))
    {
      g_test_skip ("format not supported");
      return;
    }

  g_test_message ("Load JPEG with size 0xfbfbfbfb (issue: 250)");

  g_file_get_contents (g_test_get_filename (G_TEST_DIST, "issue205.jpg", NULL), &contents, &size, &error);
  g_assert_no_error (error);

  loader = gdk_pixbuf_loader_new ();

  gdk_pixbuf_loader_write (loader, (const guchar*)contents, size, &error);
  g_assert_no_error (error);

  gdk_pixbuf_loader_close (loader, &error);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE);

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  g_assert_nonnull (pixbuf);

  g_object_unref (loader);
  g_free (contents);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/jpeg/inverted_cmyk_jpeg", test_inverted_cmyk_jpeg);
  g_test_add_func ("/pixbuf/jpeg/type9_rotation_exif_tag", test_type9_rotation_exif_tag);
  g_test_add_func ("/pixbuf/jpeg/bug775218", test_bug_775218);
  g_test_add_func ("/pixbuf/jpeg/comment", test_comment);
  g_test_add_func ("/pixbuf/jpeg/at_size", test_at_size);
  g_test_add_func ("/pixbuf/jpeg/issue70", test_jpeg_markers);
  g_test_add_func ("/pixbuf/jpeg/issue205", test_jpeg_fbfbfbfb);

  return g_test_run ();
}
