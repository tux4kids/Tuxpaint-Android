/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 *  Copyright (C) 2016 Martin Guy <martinwguy@gmail.com>
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

/*
 * Test the two-step scaler speed optimization in gdk-pixbuf/pixops.c
 * for result correctness. See https://bugzilla.gnome.org/show_bug.cgi?id=80925
 *
 * The two-step scaler kicks in when ceil(1/scale_x + 1) * ceil(1/scale_y + 1)
 * (the number of 2KB filters created by make_filter_table()) exceeds 1000 so
 * test cases wanting it have to do drastic image reductions, such as reducing
 * both dimensions by a factor of more that sqrt(1000) - say by 40 both ways.
 *
 * There is a global boolean in the two-step-scaler allowing us to disable the
 * two-step optimization to be able to compare the results with and without it.
 */

#include "config.h"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"

/* Size of images to be scaled */
#define SOURCE_WIDTH 400
#define SOURCE_HEIGHT 400

/* To trigger the two-step scaler, we need to reduce the total area by
 * more than 1000.  40x40 is 1600. */
#define SCALE_FACTOR (1.0/40.0)

/* First, some functions to make test images */

/*
 * pixdata_almost_equal(): A helper function to check whether the pixels in
 * two images are almost the same: pixel color values are allowed to differ
 * by at most one.
 */

/* Are two values the same or differ by one? */
#define within_one(a, b) \
	((a) == (b) || (a) == (b) + 1 || (a) + 1 == (b))

static gboolean
pixdata_almost_equal (GdkPixbuf *one, GdkPixbuf *two)
{
  guchar *one_row;      /* Pointer to start of row of pixels in one */
  guchar *one_pixel;    /* Pointer to current pixel data in one */
  guchar *two_row;      /* Pointer to start of row of pixels in two */
  guchar *two_pixel;    /* Pointer to current pixel data in two */
  guint x, y;
  gint width_one, height_one;

  width_one = gdk_pixbuf_get_width (one);
  height_one = gdk_pixbuf_get_height (one);

  g_assert_cmpint (height_one, >=, 0);
  g_assert_cmpint (width_one, >=, 0);
  g_assert_cmpint (gdk_pixbuf_get_height (two), >=, 0);
  g_assert_cmpint (gdk_pixbuf_get_width (two), >=, 0);

  if (width_one != gdk_pixbuf_get_width (two) ||
      height_one != gdk_pixbuf_get_height (two))
    {
      g_test_fail();
    }

  for (y = 0, one_row = gdk_pixbuf_get_pixels (one),
              two_row = gdk_pixbuf_get_pixels (two);
       y < height_one;
       y++, one_row += gdk_pixbuf_get_rowstride (one),
            two_row += gdk_pixbuf_get_rowstride (two))
    {
      for (x = 0, one_pixel = one_row, two_pixel = two_row;
	   x < width_one;
	   x++, one_pixel += gdk_pixbuf_get_n_channels (one),
	        two_pixel += gdk_pixbuf_get_n_channels (two))
        {
	  if (!(within_one(one_pixel[0], two_pixel[0]) &&
	        within_one(one_pixel[1], two_pixel[1]) &&
	        within_one(one_pixel[2], two_pixel[2])))
	    {
	      return FALSE;
	    }
        }
    }

  return TRUE;
}

/* Create a checkerboard of (1,1,1) and (255,255,255) pixels and
 * scale it to one fortieth of the image dimensions.
 * See if the results are similar enough with and without the two-step
 * optimization. */
static void
test_old_and_new (gconstpointer data)
{
  GdkInterpType interp_type = *(GdkInterpType *) data;
  const GdkPixbuf *source;              /* Source image */
  gint width = SOURCE_WIDTH;            /* Size of source image */
  gint height = SOURCE_HEIGHT;
  GdkPixbuf *one;                       /* Version scaled by the old code */
  GdkPixbuf *two;                       /* Version scaled in two steps */

  /* Use an extreme source image, checkerboard, to exacerbate any artifacts */
  source = make_checkerboard (width, height);

  /* Scale it without and with the two-step optimization */
  g_setenv ("GDK_PIXBUF_DISABLE_TWO_STEP_SCALER", "1", TRUE);
  one = gdk_pixbuf_scale_simple (source,
				 (int) (width * SCALE_FACTOR + 0.5),
				 (int) (height * SCALE_FACTOR + 0.5),
				 interp_type);
  g_unsetenv("GDK_PIXBUF_DISABLE_TWO_STEP_SCALER");
  two = gdk_pixbuf_scale_simple (source,
				 (int) (width * SCALE_FACTOR + 0.5),
				 (int) (height * SCALE_FACTOR + 0.5),
				 interp_type);

  /* Check that the results are almost the same. An error of one color value
   * is admissible because the intermediate image's color values are stored
   * in 8-bit color values. In practice, with the checkerboard pattern all
   * pixels are (129,129,129) with the two-step scaler instead of (128,128,128)
   * while the rg pattern gives identical results.
   */
  if (!pixdata_almost_equal(one, two))
    g_test_fail();

  g_object_unref (G_OBJECT (one));
  g_object_unref (G_OBJECT (two));
  g_object_unref (G_OBJECT (source));
}

/* Crop a region out of a scaled image using gdk_pixbuf_new_subpixbuf() on a
 * scaled image and by cropping it as part of the scaling, and check that the
 * results are identical. */
static void
crop_n_compare(const GdkPixbuf *source,	/* The source image */
	       double           scale_factor,  /* is scaled by this amount */
	       gint             offset_x,      /* and from this offset in the scaled image */
	       gint             offset_y,
	       gint             width,         /* a region of this size is cropped out */
	       gint             height,
	       GdkInterpType    interp_type)
{
  GdkPixbuf *whole_scaled;     /* The whole image scaled but not cropped */
  GdkPixbuf *cropped;          /* The scaled-then-cropped result */
  GdkPixbuf *scaled;           /* The cropped-while-scaled result */
  guint new_width, new_height; /* Size of whole scaled image */

  /* First, scale the whole image and crop it */
  new_width = (int) (gdk_pixbuf_get_width (source) * scale_factor + 0.5);
  new_height = (int) (gdk_pixbuf_get_height (source) * scale_factor + 0.5);
  g_assert_cmpint (new_width, ==, 10);
  g_assert_cmpint (new_height, ==, 10);

  whole_scaled = gdk_pixbuf_scale_simple (source, new_width, new_height, interp_type);
  g_assert_nonnull (whole_scaled);
  cropped = gdk_pixbuf_new_subpixbuf (whole_scaled, offset_x, offset_y, width, height);
  g_assert_nonnull (cropped);

  /* Now crop and scale it in one go */
  scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, width, height);
  g_assert_nonnull (scaled);
  gdk_pixbuf_scale (source, scaled,
		    0, 0,                              /* dest_[xy] */
		    width, height,                     /* dest_width/height */
		    -1.0 * offset_x, -1.0 * offset_y,
		    scale_factor, scale_factor,
		    interp_type);

  /* and check that the results are the same.
   * We can't use pixdata_equal() because it fails if rowstride is different
   * and gdk_pixbuf_new_subpixbuf() reuses whole_scaled's image data with its
   * larger rowstride. */
  {
    guchar *scaled_row;     /* Pointer to start of row of pixels in scaled */
    guchar *scaled_pixel;   /* Pointer to current pixel data in scaled */
    guchar *cropped_row;    /* Pointer to start of row of pixels in cropped */
    guchar *cropped_pixel;  /* Pointer to current pixel data in cropped */
    guint x, y;
    gint scaled_width, scaled_height;

    scaled_width = gdk_pixbuf_get_width (scaled);
    scaled_height = gdk_pixbuf_get_height (scaled);

    g_assert (scaled_width > 0);
    g_assert (scaled_height > 0);

    if (scaled_width != gdk_pixbuf_get_width (cropped) ||
	scaled_height != gdk_pixbuf_get_height (cropped))
      {
        g_test_fail();
      }

    for (y = 0, scaled_row = gdk_pixbuf_get_pixels (scaled),
	       cropped_row = gdk_pixbuf_get_pixels (cropped);
	 y < scaled_height;
	 y++, scaled_row += gdk_pixbuf_get_rowstride (scaled),
	      cropped_row += gdk_pixbuf_get_rowstride (cropped))
      {
        for (x = 0, scaled_pixel = scaled_row, cropped_pixel = cropped_row;
	    x < scaled_width;
	    x++, scaled_pixel += gdk_pixbuf_get_n_channels (scaled),
	         cropped_pixel += gdk_pixbuf_get_n_channels (cropped))
	  {
	    if (!(scaled_pixel[0] == cropped_pixel[0] &&
	          scaled_pixel[1] == cropped_pixel[1] &&
	          scaled_pixel[2] == cropped_pixel[2]))
	    {
	      g_test_fail();
	    }
	  }
      }
  }

  g_object_unref (G_OBJECT (whole_scaled));
  g_object_unref (G_OBJECT (cropped));
  g_object_unref (G_OBJECT (scaled));
}

/* Check that offsets work.
 * We should be able to crop a region of an image using the scaler using
 * negative offsets. */
static void
test_offset (gconstpointer data)
{
  GdkInterpType interp_type = *(GdkInterpType *) data;
  const GdkPixbuf *source;                     /* Source image */
  gint swidth = SOURCE_WIDTH;                  /* Size of source image */
  gint sheight = SOURCE_HEIGHT;
  gint dwidth = (swidth * SCALE_FACTOR + 0.5); /* Size of scaled image */
  gint dheight = (sheight * SCALE_FACTOR + 0.5);

  source = make_rg (swidth, sheight);

  /* Check that the scaler correctly crops out an image half the size of the
   * original from each corner and from the middle */
  crop_n_compare (source, SCALE_FACTOR, 0,          0,           dwidth / 2, dheight / 2, interp_type);
  crop_n_compare (source, SCALE_FACTOR, 0,          dheight / 2, dwidth / 2, dheight / 2, interp_type);
  crop_n_compare (source, SCALE_FACTOR, dwidth / 2, 0,           dwidth / 2, dheight / 2, interp_type);
  crop_n_compare (source, SCALE_FACTOR, dwidth / 2, dheight / 2, dwidth / 2, dheight / 2, interp_type);
  crop_n_compare (source, SCALE_FACTOR, dwidth / 4, dheight / 4, dwidth / 2, dheight / 2, interp_type);

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
  const GdkPixbuf *source;             /* Source image */
  GdkPixbuf *scaled;                   /* Scaled whole image */
  GdkPixbuf *copy;                     /* Destination image */
  gint swidth = SOURCE_WIDTH;          /* Size of source image */
  gint sheight = SOURCE_HEIGHT;
  gint dwidth = swidth * SCALE_FACTOR; /* Size of scaled image */
  gint dheight = sheight * SCALE_FACTOR;

  source = make_checkerboard (swidth, sheight);

  copy = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, dwidth, dheight);
  g_assert_nonnull (copy);

  /* Copy the four quadrants separately */
  gdk_pixbuf_scale ((const GdkPixbuf *) source, copy,
		    0, 0,                    /* dest_[xy] */
		    dwidth / 2, dheight / 2, /* dest_width/height */
		    0.0, 0.0,                /* offset_[xy] */
		    SCALE_FACTOR,  SCALE_FACTOR,
		    interp_type);
  gdk_pixbuf_scale ((const GdkPixbuf *) source, copy,
		    dwidth / 2, 0,           /* dest_[xy] */
		    dwidth / 2, dheight / 2, /* dest_width/height */
		    0.0, 0.0,                /* offset_[xy] */
		    SCALE_FACTOR,  SCALE_FACTOR,
		    interp_type);
  gdk_pixbuf_scale ((const GdkPixbuf *)source, copy,
		    0, dheight / 2,          /* dest_[xy] */
		    dwidth / 2, dheight / 2, /* dest_width/height */
		    0.0, 0.0,                /* offset_[xy] */
		    SCALE_FACTOR,  SCALE_FACTOR,
		    interp_type);
  gdk_pixbuf_scale ((const GdkPixbuf *)source, copy,
		    dwidth / 2, dheight / 2, /* dest_[xy] */
		    dwidth / 2, dheight / 2, /* dest_width/height */
		    0.0, 0.0,                /* offset_[xy] */
		    SCALE_FACTOR,  SCALE_FACTOR,
		    interp_type);

  scaled = gdk_pixbuf_scale_simple (source, dwidth, dheight, interp_type);
  g_assert_nonnull (scaled);

  /* Compare the all-at-once and the composite images */
  {
    GError *error = NULL;
    if (!pixdata_equal(scaled, copy, &error))
      g_test_fail();
  }

  g_object_unref (G_OBJECT (source));
  g_object_unref (G_OBJECT (copy));
  g_object_unref (G_OBJECT (scaled));
}

int
main (int argc, char **argv)
{
  GdkInterpType tiles = GDK_INTERP_TILES;
  GdkInterpType bilinear = GDK_INTERP_BILINEAR;
  GdkInterpType hyper = GDK_INTERP_HYPER;
  g_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/pixbuf/scale/two-step/tiles", &tiles, test_old_and_new);
  g_test_add_data_func ("/pixbuf/scale/two-step/bilinear", &bilinear, test_old_and_new);
  g_test_add_data_func ("/pixbuf/scale/two-step/hyper", &hyper, test_old_and_new);

  g_test_add_data_func ("/pixbuf/scale/two-step/offset/tiles", &tiles, test_offset);
  g_test_add_data_func ("/pixbuf/scale/two-step/offset/bilinear", &bilinear, test_offset);
  g_test_add_data_func ("/pixbuf/scale/two-step/offset/hyper", &hyper, test_offset);

  g_test_add_data_func ("/pixbuf/scale/two-step/dest/tiles", &tiles, test_dest);
  g_test_add_data_func ("/pixbuf/scale/two-step/dest/bilinear", &bilinear, test_dest);
  g_test_add_data_func ("/pixbuf/scale/two-step/dest/hyper", &hyper, test_dest);

  return g_test_run ();
}
