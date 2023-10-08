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
#include <glib/gstdio.h>

#define compare_option(p1, p2, key) \
  g_strcmp0 (gdk_pixbuf_get_option (p1, key), gdk_pixbuf_get_option (p2, key))

static gboolean
pixbuf_equal (GdkPixbuf *p1, GdkPixbuf *p2)
{
  if (!pixdata_equal (p1, p2, NULL))
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
test_save_roundtrip (void)
{
  GError *error = NULL;
  GdkPixbuf *ref;
  GdkPixbuf *pixbuf;

  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "test-image.png", NULL), &error);
  g_assert_no_error (error);

  gdk_pixbuf_save (ref, "pixbuf-save-roundtrip", "png", &error, NULL);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_file ("pixbuf-save-roundtrip", &error);
  g_assert_no_error (error);

  g_assert (pixbuf_equal (pixbuf, ref));

  g_object_unref (pixbuf);
  g_object_unref (ref);

  g_unlink ("pixbuf-save-roundtrip");
}

static void
test_save_ico (void)
{
  GError *error = NULL;
  GdkPixbuf *ref, *ref2;
  GdkPixbuf *pixbuf;

  if (!format_supported ("ico") || !format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "test-image.png", NULL), &error);
  g_assert_no_error (error);

  ref2 = gdk_pixbuf_scale_simple (ref, 256, 256, GDK_INTERP_NEAREST);
  g_object_unref (ref);
  ref = ref2;

  gdk_pixbuf_save (ref, "pixbuf-save-roundtrip", "ico", &error, NULL);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_file ("pixbuf-save-roundtrip", &error);
  g_assert_no_error (error);

  g_assert (pixbuf_equal (pixbuf, ref));

  g_object_unref (pixbuf);
  g_object_unref (ref);

  g_unlink ("pixbuf-save-roundtrip");
}

static void
test_save_options (void)
{
  GdkPixbuf *ref;
  GdkPixbuf *pixbuf, *pixbuf2;
  GError *error = NULL;

  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  ref = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 10, 10);
  gdk_pixbuf_fill (ref, 0xff00ff00);

  gdk_pixbuf_save (ref, "pixbuf-save-options", "png", &error,
                   "tEXt::option1", "Some text to transport via option",
                   "tEXt::long-option-name123456789123456789123456789", "",
#ifdef PNG_iTXt_SUPPORTED
                   "tEXt::3", "αβγδ",
#endif
                   NULL);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_file ("pixbuf-save-options", &error);
  g_assert_no_error (error);

  g_assert_cmpstr (gdk_pixbuf_get_option (pixbuf, "tEXt::option1"), ==, "Some text to transport via option");
  g_assert_cmpstr (gdk_pixbuf_get_option (pixbuf, "tEXt::long-option-name123456789123456789123456789"), ==, "");
#ifdef PNG_iTXt_SUPPORTED
  g_assert_cmpstr (gdk_pixbuf_get_option (pixbuf, "tEXt::3"), ==, "αβγδ");
#endif

  pixbuf2 = gdk_pixbuf_copy (pixbuf);
  g_assert_null (gdk_pixbuf_get_option (pixbuf2, "tEXt::option1"));
  gdk_pixbuf_copy_options (pixbuf, pixbuf2);
  g_assert_cmpstr (gdk_pixbuf_get_option (pixbuf2, "tEXt::option1"), ==, "Some text to transport via option");
  g_assert_true (gdk_pixbuf_remove_option (pixbuf2, "tEXt::option1"));
  g_assert_null (gdk_pixbuf_get_option (pixbuf2, "tEXt::option1"));
  g_assert_false (gdk_pixbuf_remove_option (pixbuf2, "tEXt::option1"));
#ifdef PNG_iTXt_SUPPORTED
  gdk_pixbuf_remove_option (pixbuf2, "tEXt::3");
#endif
  gdk_pixbuf_remove_option (pixbuf2, "tEXt::long-option-name123456789123456789123456789");
  g_assert_false (gdk_pixbuf_remove_option (pixbuf2, "tEXt::option1"));

  g_object_unref (pixbuf2);
  g_object_unref (pixbuf);
  g_object_unref (ref);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/save/roundtrip", test_save_roundtrip);
  g_test_add_func ("/pixbuf/save/options", test_save_options);
  g_test_add_func ("/pixbuf/save/ico", test_save_ico);

  return g_test_run ();
}
