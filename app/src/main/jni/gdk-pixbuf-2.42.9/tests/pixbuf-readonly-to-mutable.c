/* -*- Mode: C; c-basic-offset: 2; -*- */
/* GdkPixbuf library - test loaders
 *
 * Copyright (C) 2014 Red Hat, Inc.
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

#include "gdk-pixbuf/gdk-pixbuf.h"
#include "test-common.h"
#include <errno.h>
#include <string.h>

#ifdef G_OS_UNIX
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef G_OS_UNIX

typedef struct {
  void *buf;
  gsize len;
} MappedBuf;

static void
destroy_buf_unmap (gpointer data)
{
  MappedBuf *buf = data;
  int r;

  r = munmap (buf->buf, buf->len);
  g_assert_cmpint (r, ==, 0);
  g_free (buf);
}
#endif

static GdkPixbuf *
get_readonly_pixbuf (void)
{
  GdkPixbuf *reference;
  GdkPixbuf *result;
  GBytes *bytes;
  GError *error = NULL;

  reference = gdk_pixbuf_new_from_file (g_test_get_filename (G_TEST_DIST, "test-image.png", NULL), &error);
  g_assert_no_error (error);
  
#ifdef G_OS_UNIX
  {
    MappedBuf *buf;
    int saved_errno;
    int pagesize;
    int pages;
    int r;
    int zero_fd;
    gsize pixlen;

    pagesize = sysconf (_SC_PAGESIZE);
    g_assert_cmpint (pagesize, >, 0);

    pixlen = gdk_pixbuf_get_byte_length (reference);
    pages = pixlen / pagesize + 1;

    buf = g_new0 (MappedBuf, 1);
    buf->len = pages * pagesize;
    zero_fd = open("/dev/zero", O_RDWR);
    g_assert(zero_fd != -1);
    saved_errno = errno;
    errno = 0;
    buf->buf = mmap (NULL, buf->len, PROT_READ | PROT_WRITE, MAP_PRIVATE, zero_fd, 0);
    g_assert_true (errno >= 0);
    errno = saved_errno;
    close(zero_fd);

    memcpy (buf->buf, gdk_pixbuf_get_pixels (reference), pixlen);

    r = mprotect (buf->buf, buf->len, PROT_READ);
    g_assert_cmpint (r, ==, 0);

    bytes = g_bytes_new_with_free_func (buf->buf, buf->len, destroy_buf_unmap, buf);
  }
#else
  bytes = g_bytes_new (gdk_pixbuf_get_pixels (reference), gdk_pixbuf_get_byte_length (reference));
#endif

  result = gdk_pixbuf_new_from_bytes (bytes,
				      gdk_pixbuf_get_colorspace (reference),
				      gdk_pixbuf_get_has_alpha (reference),
				      gdk_pixbuf_get_bits_per_sample (reference),
				      gdk_pixbuf_get_width (reference),
				      gdk_pixbuf_get_height (reference),
				      gdk_pixbuf_get_rowstride (reference));
  g_object_unref (reference);
  g_bytes_unref (bytes);

  return result;
}

static void
test_mutate_readonly (void)
{
  GdkPixbuf *src;
  GdkPixbuf *dest;

  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }
  
  src = get_readonly_pixbuf ();
  gdk_pixbuf_scale (src, src,
		    0, 0, 
		    gdk_pixbuf_get_width (src) / 4, 
		    gdk_pixbuf_get_height (src) / 4,
		    0, 0, 0.5, 0.5,
		    GDK_INTERP_NEAREST);
  g_object_unref (src);

  src = get_readonly_pixbuf ();
		    
  dest = gdk_pixbuf_scale_simple (src,
				  gdk_pixbuf_get_width (src) / 4, 
				  gdk_pixbuf_get_height (src) / 4,
				  GDK_INTERP_NEAREST);
  g_object_unref (dest);

  dest = gdk_pixbuf_composite_color_simple (src,
					    gdk_pixbuf_get_width (src) / 4, 
					    gdk_pixbuf_get_height (src) / 4,
					    GDK_INTERP_NEAREST,
					    128,
					    8,
					    G_MAXUINT32,
					    G_MAXUINT32/2);
  g_object_unref (dest);

  dest = gdk_pixbuf_rotate_simple (src, 180);
  g_object_unref (dest);

  dest = gdk_pixbuf_flip (src, TRUE);
  g_object_unref (dest);
  dest = gdk_pixbuf_flip (src, FALSE);
  g_object_unref (dest);

  g_object_unref (src);
}

static void
test_read_pixel_bytes (void)
{
  GdkPixbuf *src;
  GBytes *bytes;
  
  if (!format_supported ("png"))
    {
      g_test_skip ("format not supported");
      return;
    }
  
  src = get_readonly_pixbuf ();
  bytes = gdk_pixbuf_read_pixel_bytes (src);
  g_object_unref (src);
  g_bytes_unref (bytes);

  src = get_readonly_pixbuf ();
  /* Force a mutable conversion */
  (void) gdk_pixbuf_get_pixels (src);
  bytes = gdk_pixbuf_read_pixel_bytes (src);
  g_object_unref (src);
  g_bytes_unref (bytes);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pixbuf/readonly/mutate", test_mutate_readonly);
  g_test_add_func ("/pixbuf/readonly/readpixelbytes", test_read_pixel_bytes);

  return g_test_run ();
}
