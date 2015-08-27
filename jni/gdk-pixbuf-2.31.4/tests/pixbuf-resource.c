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
test_resource (void)
{
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *pixbuf, *ref;

  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, "icc-profile.png", NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_resource ("/test/resource/icc-profile.png", &error);
  g_assert_no_error (error);
  g_assert (pixbuf_equal (pixbuf, ref));
  g_object_unref (pixbuf);
  
  pixbuf = gdk_pixbuf_new_from_resource ("/test/resource/icc-profile.pixdata", &error);
  g_assert_no_error (error);
  g_assert (pixdata_equal (pixbuf, ref, NULL));
  g_object_unref (pixbuf);

  pixbuf = gdk_pixbuf_new_from_resource ("/no/such/resource", &error);
  g_assert (pixbuf == NULL);
  g_assert_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND);
  g_clear_error (&error);

  pixbuf = gdk_pixbuf_new_from_resource ("resource:///test/resource/icc-profile.png", &error);
  g_assert (pixbuf == NULL);
  g_assert_error (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND);
  g_clear_error (&error);

  g_object_unref (ref);
}

static void
test_resource_at_scale (void)
{
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *pixbuf, *ref;

  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, "icc-profile.png", NULL);
  ref = gdk_pixbuf_new_from_file_at_scale (path, 40, 10, FALSE, &error);
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_new_from_resource_at_scale ("/test/resource/icc-profile.png", 40, 10, FALSE, &error);
  g_assert_no_error (error);
  g_assert (pixbuf_equal (pixbuf, ref));
  g_object_unref (pixbuf);
  
  g_object_unref (ref);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/resource", test_resource);
  g_test_add_func ("/pixbuf/resource/at-scale", test_resource_at_scale);

  return g_test_run ();
}
