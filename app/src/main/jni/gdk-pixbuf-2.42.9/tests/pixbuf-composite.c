/* GdkPixbuf library - test compositing
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
 * Author: Matthias Clasen
 */

#include <gdk-pixbuf.h>
#include "test-common.h"

static void
test_composite1 (void)
{
  GdkPixbuf *red, *green, *out, *ref, *sub;

  red = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 24, 24);
  gdk_pixbuf_fill (red, 0xff000000);

  green = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 12, 12);
  gdk_pixbuf_fill (green, 0x00ff0000);

  out = gdk_pixbuf_copy (red);

  gdk_pixbuf_composite (green, out,
                        0, 0, 12, 12,
                        0, 0, 1.0, 1.0,
                        GDK_INTERP_NEAREST,
                        255);

  ref = gdk_pixbuf_copy (red);
  sub = gdk_pixbuf_new_subpixbuf (ref, 0, 0, 12, 12);

  gdk_pixbuf_fill (sub, 0x00ff0000);

  g_assert (pixdata_equal (out, ref, NULL));

  g_object_unref (red);
  g_object_unref (green);
  g_object_unref (out);
  g_object_unref (ref);
  g_object_unref (sub);
}

/*
 * Test for Bug 766842
 * https://bugzilla.gnome.org/show_bug.cgi?id=766842
 */
static void
test_composite2 (void)
{
  GdkPixbuf *src, *dest;
  guchar *pixels, *p;

  char *filename = g_test_get_filename (G_TEST_DIST, "test-image.png", NULL);
  if (!format_supported (filename))
    {
      g_test_skip ("PNG format not supported");
      return;
    }

  src = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "test-image.png", NULL), NULL);

  {
    GdkPixbuf *tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                     TRUE,
                                     gdk_pixbuf_get_bits_per_sample (src),
                                     gdk_pixbuf_get_width (src),
                                     gdk_pixbuf_get_height (src));
    gdk_pixbuf_fill (tmp, 0x00ccccff);
    gdk_pixbuf_composite (src, tmp,
                          0, 0, gdk_pixbuf_get_width (tmp), gdk_pixbuf_get_height (tmp),
                          0., 0., 1., 1.,
                          GDK_INTERP_NEAREST, 255);
    g_object_unref (src);
    src = tmp;
  }

  pixels = gdk_pixbuf_get_pixels (src);
  p = pixels;
  p[0] = 0xff;
  p[1] = 0x00;
  p[2] = 0xff;
  p = pixels + (gsize)((gdk_pixbuf_get_height (src) - 1) * gdk_pixbuf_get_rowstride (src)) + (gsize)((gdk_pixbuf_get_width (src) - 1) * gdk_pixbuf_get_n_channels (src));
  p[0] = 0xff;
  p[1] = 0xff;
  p[2] = 0x00;

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                         TRUE,
                         gdk_pixbuf_get_bits_per_sample (src),
                         gdk_pixbuf_get_width (src) + 80,
                         gdk_pixbuf_get_height (src) + 80);
  gdk_pixbuf_fill (dest, 0xffffffff);
  gdk_pixbuf_composite (src, dest,
                        0, 0, gdk_pixbuf_get_width (dest), gdk_pixbuf_get_height (dest),
                        10.0, 10.0, 1.0, 1.0,
                        GDK_INTERP_NEAREST, 255);

  pixels = gdk_pixbuf_get_pixels (dest);
  p = pixels;
  g_assert_cmpint (p[0], ==, 0xff);
  g_assert_cmpint (p[1], ==, 0x00);
  g_assert_cmpint (p[2], ==, 0xff);
  p = pixels + (gsize)((gdk_pixbuf_get_height (dest) - 1) * gdk_pixbuf_get_rowstride (dest)) + (gsize)((gdk_pixbuf_get_width (dest) - 1) * gdk_pixbuf_get_n_channels (dest));
  g_assert_cmpint (p[0], ==, 0xff);
  g_assert_cmpint (p[1], ==, 0xff);
  g_assert_cmpint (p[2], ==, 0x00);

  g_object_unref (dest);

  /* now try compositing into a pixbuf that is 1px less in width and height */
  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                         TRUE,
                         gdk_pixbuf_get_bits_per_sample (src),
                         gdk_pixbuf_get_width (src) - 1,
                         gdk_pixbuf_get_height (src) - 1);
  gdk_pixbuf_fill (dest, 0xffffffff);
  gdk_pixbuf_composite (src, dest,
                        0, 0, gdk_pixbuf_get_width (dest), gdk_pixbuf_get_height (dest),
                        -1.0, -2.0, 1.0, 1.0,
                        GDK_INTERP_NEAREST, 255);

  pixels = gdk_pixbuf_get_pixels (dest);
  p = pixels + (gsize)((gdk_pixbuf_get_height (dest) - 2) * gdk_pixbuf_get_rowstride (dest)) + (gsize)((gdk_pixbuf_get_width (dest) - 1) * gdk_pixbuf_get_n_channels (dest));
  g_assert_cmpint (p[0], ==, 0xff);
  g_assert_cmpint (p[1], ==, 0xff);
  g_assert_cmpint (p[2], ==, 0x00);
  p = pixels + (gsize)((gdk_pixbuf_get_height (dest) - 1) * gdk_pixbuf_get_rowstride (dest)) + (gsize)((gdk_pixbuf_get_width (dest) - 1) * gdk_pixbuf_get_n_channels (dest));
  g_assert_cmpint (p[0], ==, 0xff);
  g_assert_cmpint (p[1], ==, 0xff);
  g_assert_cmpint (p[2], ==, 0x00);

  g_object_unref (dest);

  g_object_unref (src);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/composite1", test_composite1);
  g_test_add_func ("/pixbuf/composite2", test_composite2);

  return g_test_run ();
}
