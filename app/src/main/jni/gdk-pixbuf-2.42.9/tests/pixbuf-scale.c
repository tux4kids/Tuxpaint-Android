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

static void
test_scale (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;
  GdkPixbuf *pixbuf;
  gint width, height;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);
  g_assert_no_error (error);

  width = gdk_pixbuf_get_width (ref);
  height = gdk_pixbuf_get_height (ref);

  pixbuf = gdk_pixbuf_new_from_file_at_scale (path, 2 * width, 3 * height, FALSE, &error);
  g_assert_no_error (error);

  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 2 * width);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 3 * height);
  
  g_object_unref (pixbuf);

  pixbuf = gdk_pixbuf_new_from_file_at_scale (path, 4 * width, 2 * height, TRUE, &error);
  g_assert_no_error (error);

  g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 2 * width);
  g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 2 * height);
  
  g_object_unref (pixbuf);

  g_object_unref (ref);
}

static void
test_scale_down (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;
  GdkPixbuf *pixbuf;
  gint width, height;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);

  if (skip_if_insufficient_memory (&error))
    return;
  g_assert_no_error (error);

  width = gdk_pixbuf_get_width (ref);
  height = gdk_pixbuf_get_height (ref);

  pixbuf = gdk_pixbuf_scale_simple (ref, width / 10, height / 10, GDK_INTERP_BILINEAR);
  g_assert (pixbuf != NULL);

  g_object_unref (ref);
}

static void
test_add_alpha (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;
  GdkPixbuf *pixbuf;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);

  if (skip_if_insufficient_memory (&error))
    return;
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_add_alpha (ref, FALSE, 0, 0, 0);

  if (pixbuf == NULL)
    {
      g_test_skip ("Couldn't add alpha to the image - your system probably lacks sufficient memory.");
      g_object_unref (ref);
      return;
    }

  g_object_unref (pixbuf);

  pixbuf = gdk_pixbuf_add_alpha (ref, TRUE, 0, 0, 255);
  g_assert (pixbuf != NULL);
  g_object_unref (pixbuf);

  g_object_unref (ref);
}

static void
test_rotate (gconstpointer data)
{
  const gchar *filename = data;
  const gchar *path;
  GError *error = NULL;
  GdkPixbuf *ref;
  GdkPixbuf *pixbuf;

  if (!format_supported (filename))
    {
      g_test_skip ("format not supported");
      return;
    }

  path = g_test_get_filename (G_TEST_DIST, filename, NULL);
  ref = gdk_pixbuf_new_from_file (path, &error);

  if (skip_if_insufficient_memory (&error))
    return;
  g_assert_no_error (error);

  pixbuf = gdk_pixbuf_rotate_simple (ref, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);

  if (pixbuf == NULL)
    g_test_skip ("Couldn't rotate the image - your system probably lacks sufficient memory.");
  else
    g_object_unref (pixbuf);

  g_object_unref (ref);
}

/* Test images creation functions */

/* Create a 256x256 checkerboard of (1,1,1) and (255,255,255) pixels,
 * scale it to exactly half size and check that all pixels are (128,128,128). */
static void
test_halve_checkerboard (gconstpointer data)
{
  GdkInterpType interp_type = *(GdkInterpType *) data;
  const GdkPixbuf *source;              /* Source image */
  gint width = 256, height = 256;       /* Size of source image */
  gint scaled_width, scaled_height;     /* Size of scaled image */
  GdkPixbuf *scaled;                    /* Scaled version */
  guchar *row;                          /* Pointer to start of row of pixels within the image */
  guchar *pixel;                        /* Pointer to current pixel data in row */
  guint x, y;
  guchar expected;                      /* Expected color of all pixels */

  expected = (interp_type == GDK_INTERP_NEAREST) ? 255 : 128;

  source = make_checkerboard (width, height);

  scaled = gdk_pixbuf_scale_simple (source, width / 2, height / 2, interp_type);
  scaled_width = gdk_pixbuf_get_width (scaled);
  scaled_height = gdk_pixbuf_get_height (scaled);
  g_assert_cmpint (scaled_width, >, 0);
  g_assert_cmpint (scaled_height, >, 0);

  /* Check that the result is all gray (or all white in the case of NEAREST) */
  for (y = 0, row = gdk_pixbuf_get_pixels (scaled);
       y < (guint) scaled_height;
       y++, row += gdk_pixbuf_get_rowstride (scaled))
    {
      for (x = 0, pixel = row;
           x < (guint) scaled_width;
           x++, pixel += gdk_pixbuf_get_n_channels (scaled))
        {
          if (!(pixel[0] == expected && pixel[1] == expected && pixel[2] == expected))
            {
              /* Expected failure: HYPER has a different opinion about the color
               * of the corner pixels: (126,126,126) and (130,130,130) */
              if (interp_type == GDK_INTERP_HYPER &&
                  (x == 0 || x == scaled_width - 1) &&
                  (y == 0 || y == scaled_height - 1))
                {
                  continue;
                }
              g_test_fail ();
            }
        }
    }

  g_object_unref (G_OBJECT (scaled));
  g_object_unref (G_OBJECT (source));
}

/* Crop a region out of a source image using subpixbuf() and using the scaler,
 * and check that the results are the same */
static void
crop_n_compare (const GdkPixbuf *source,
                gint             offset_x,
                gint             offset_y,
                guint            width,
                guint            height,
                GdkInterpType    interp_type)
{
  GdkPixbuf *cropped, *scaled;
  guchar *crow, *srow;                  /* Pointer to current row in image data */
  guchar *cpixel, *spixel;              /* Pointer to current pixel in row */
  guint x, y;
  gint scaled_width, scaled_height;     /* Size of scaled image */

  cropped = gdk_pixbuf_new_subpixbuf ((GdkPixbuf *)source, offset_x, offset_y, width, height);
  g_assert_nonnull (cropped);

  scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, width, height);
  g_assert_nonnull (scaled);
  gdk_pixbuf_scale (source, scaled,
                    0, 0,                             /* dest_[xy] */
                    width, height,                    /* dest_width/height */
                    -1.0 * offset_x, -1.0 * offset_y, /* offset_[xy] */
                    1.0, 1.0,                         /* scale_[xy] */
                    interp_type);

  scaled_width = gdk_pixbuf_get_width (scaled);
  scaled_height = gdk_pixbuf_get_height (scaled);
  g_assert_cmpint (scaled_width, >, 0);
  g_assert_cmpint (scaled_height, >, 0);

  for (y = 0, crow = gdk_pixbuf_get_pixels (cropped),
              srow = gdk_pixbuf_get_pixels (scaled);
       y < scaled_height;
       y++, crow += gdk_pixbuf_get_rowstride (cropped),
            srow += gdk_pixbuf_get_rowstride (scaled))
    {
      for (x = 0, cpixel = crow, spixel = srow;
           x < scaled_width;
           x++, cpixel += gdk_pixbuf_get_n_channels (cropped),
                spixel += gdk_pixbuf_get_n_channels (scaled))
        {
          if (!(spixel[0] == cpixel[0] &&
                spixel[1] == cpixel[1] &&
                spixel[2] == cpixel[2]))
            {
              /* Expected failure: HYPER has a different opinion about the
               * colors of the edge pixels */
              if (interp_type == GDK_INTERP_HYPER &&
                  ((x == 0 || x == scaled_width - 1) ||
                   (y == 0 || y == scaled_height - 1)))
                {
                  continue;
                }
              g_test_fail();
            }
        }
    }

  g_object_unref (G_OBJECT (cropped));
  g_object_unref (G_OBJECT (scaled));
}

/* Check that offsets work.
 * We should be able to copy a region of an image using the scaler using
 * negative offsets. */
static void
test_offset (gconstpointer data)
{
  GdkInterpType interp_type = *(GdkInterpType *) data;
  gint width = 256, height = 256;
  const GdkPixbuf *source;  /* Source image */

  source = make_rg (width, height);

  /* Check that the scaler correctly crops out an image half the size of the
   * original from each corner and from the middle */
  crop_n_compare (source, 0,         0,          width / 2, height / 2, interp_type);
  crop_n_compare (source, width / 2, 0,          width / 2, height / 2, interp_type);
  crop_n_compare (source, 0,         height / 2, width / 2, height / 2, interp_type);
  crop_n_compare (source, width / 2, height / 2, width / 2, height / 2, interp_type);
  crop_n_compare (source, width / 4, height / 4, width / 2, height / 2, interp_type);

  g_object_unref (G_OBJECT (source));
}

/* Test the dest_x and dest_y fields by making a copy of an image by
 * scaling 1:1 and using dest to copy the four quadrants separately.
 *
 * When scaled, images are:
 * 1) scaled by the scale factors with respect to the top-left corner
 * 2) translated by the offsets (negative to shift the image left/up in its
 *    frame)
 * 3) a region of size dest_width x dest-height starting at (dest_x,dest_y)
 *    in the scaled-and-offset image is copied to (dest_x,dest_y) in the
 *    destination image. See the illustration at
 *    https://developer.gnome.org/gdk-pixbuf/2.22/gdk-pixbuf-scaling.html#gdk-pixbuf-composite */
static void
test_dest (gconstpointer data)
{
  GdkInterpType interp_type = *(GdkInterpType *) data;
  gint width = 256, height = 256;
  const GdkPixbuf *source;  /* Source image */
  GdkPixbuf *copy;          /* Destination image */
  gint x, y;
  guchar *srow, *crow;	    /* Pointer to current row in source and copy data */
  guchar *spixel, *cpixel;  /* Pointer to current pixel in row */

  source = make_rg (width, height);

  copy = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, width, height);
  g_assert_nonnull (copy);

  /* Copy the four quadrants with a no-op scale */
  gdk_pixbuf_scale ((const GdkPixbuf *)source, copy,
                    0, 0,                  /* dest_[xy] */
                    width / 2, height / 2, /* dest_width/height */
                    0.0, 0.0,              /* offset_[xy] */
                    1.0, 1.0,              /* scale_[xy] */
                    interp_type);
  gdk_pixbuf_scale ((const GdkPixbuf *)source, copy,
                    width / 2, 0,          /* dest_[xy] */
                    width / 2, height / 2, /* dest_width/height */
                    0.0, 0.0,              /* offset_[xy] */
                    1.0, 1.0,              /* scale_[xy] */
                    interp_type);
  gdk_pixbuf_scale ((const GdkPixbuf *)source, copy,
                    0, height / 2,         /* dest_[xy] */
                    width / 2, height / 2, /* dest_width/height */
                    0.0, 0.0,              /* offset_[xy] */
                    1.0, 1.0,              /* scale_[xy] */
                    interp_type);
  gdk_pixbuf_scale ((const GdkPixbuf *)source, copy,
                    width / 2, height / 2, /* dest_[xy] */
                    width / 2, height / 2, /* dest_width/height */
                    0.0, 0.0,              /* offset_[xy] */
                    1.0, 1.0,              /* scale_[xy] */
                    interp_type);

  /* Compare the original and the copy */
  for (y = 0, srow = gdk_pixbuf_get_pixels (source),
              crow = gdk_pixbuf_get_pixels (copy);
       y < gdk_pixbuf_get_height (source);
       y++, srow += gdk_pixbuf_get_rowstride (source),
            crow += gdk_pixbuf_get_rowstride (copy))
    {
      for (x = 0, spixel = srow, cpixel = crow;
           x < gdk_pixbuf_get_width (source);
           x++, spixel += gdk_pixbuf_get_n_channels (source),
                cpixel += gdk_pixbuf_get_n_channels (copy))
        {
          if (!(spixel[0] == cpixel[0] &&
                spixel[1] == cpixel[1] &&
                spixel[2] == cpixel[2]))
            {
              g_test_fail();
            }
        }
    }

  g_object_unref (G_OBJECT (source));
  g_object_unref (G_OBJECT (copy));
}

int
main (int argc, char **argv)
{
  GdkInterpType nearest = GDK_INTERP_NEAREST;
  GdkInterpType tiles = GDK_INTERP_TILES;
  GdkInterpType bilinear = GDK_INTERP_BILINEAR;
  GdkInterpType hyper = GDK_INTERP_HYPER;

  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/pixbuf/scale/png", "test-images/randomly-modified/valid.1.png", test_scale);
  g_test_add_data_func ("/pixbuf/scale/bmp", "test-images/randomly-modified/valid.1.bmp", test_scale);
  g_test_add_data_func ("/pixbuf/scale/gif", "test-images/randomly-modified/valid.1.gif", test_scale);
  g_test_add_data_func ("/pixbuf/scale/jpeg", "test-images/randomly-modified/valid.1.jpeg", test_scale);
  g_test_add_data_func ("/pixbuf/scale/tga", "test-images/randomly-modified/valid.1.tga", test_scale);
  g_test_add_data_func ("/pixbuf/scale/xpm", "test-images/randomly-modified/valid.1.xpm", test_scale);
  g_test_add_data_func ("/pixbuf/scale/xbm", "test-images/randomly-modified/valid.1.xbm", test_scale);
  if (g_test_slow ())
    {
      g_test_add_data_func ("/pixbuf/scale/png/large", "large.png", test_scale_down);
      g_test_add_data_func ("/pixbuf/scale/jpeg/large", "large.jpg", test_scale_down);
      g_test_add_data_func ("/pixbuf/add-alpha/large", "large.png", test_add_alpha);
      g_test_add_data_func ("/pixbuf/rotate/large", "large.png", test_rotate);
    }

  g_test_add_data_func ("/pixbuf/scale/halve-checkerboard/nearest", &nearest, test_halve_checkerboard);
  g_test_add_data_func ("/pixbuf/scale/halve-checkerboard/tiles", &tiles, test_halve_checkerboard);
  g_test_add_data_func ("/pixbuf/scale/halve-checkerboard/bilinear", &bilinear, test_halve_checkerboard);
  g_test_add_data_func ("/pixbuf/scale/halve-checkerboard/hyper", &hyper, test_halve_checkerboard);

  g_test_add_data_func ("/pixbuf/scale/offset/nearest", &nearest, test_offset);
  g_test_add_data_func ("/pixbuf/scale/offset/tiles", &tiles, test_offset);
  g_test_add_data_func ("/pixbuf/scale/offset/bilinear", &bilinear, test_offset);
  g_test_add_data_func ("/pixbuf/scale/offset/hyper", &hyper, test_offset);

  g_test_add_data_func ("/pixbuf/scale/dest/nearest", &nearest, test_dest);
  g_test_add_data_func ("/pixbuf/scale/dest/tiles", &tiles, test_dest);
  g_test_add_data_func ("/pixbuf/scale/dest/bilinear", &bilinear, test_dest);
  /* Don't bother with hyper as it changes the edge pixels */

  return g_test_run ();
}
