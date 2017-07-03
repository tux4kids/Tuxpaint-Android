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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Matthias Clasen
 */

#include "config.h"
#include "test-common.h"
#include "gdk-pixbuf/gdk-pixbuf.h"

#include <string.h>

gboolean
format_supported (const gchar *filename)
{
  const gchar *name = NULL;
  GSList *formats, *l;
  gboolean retval;
  const gchar *names[] = { "png", "jpeg", "bmp", "gif", "ras",
    "tga", "xpm", "xbm", "ico", "tiff" };
  gint i;

  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (strstr (filename, names[i]))
        {
          name = names[i];
          break;
        }
    }
  if (name == NULL)
    return FALSE;

  retval = FALSE;
  formats = gdk_pixbuf_get_formats ();
  for (l = formats; l; l = l->next)
    {
      GdkPixbufFormat *format = l->data;

      if (g_str_equal (gdk_pixbuf_format_get_name (format), name))
        {
          retval = TRUE;
          break;
        }
    }
  g_slist_free (formats);

  return retval;
}

gboolean
pixdata_equal (GdkPixbuf *p1, GdkPixbuf *p2, GError **error)
{
  if (gdk_pixbuf_get_colorspace (p1) != gdk_pixbuf_get_colorspace (p2)) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "Colorspaces differ");
    return FALSE;
  }
  if (gdk_pixbuf_get_n_channels (p1) != gdk_pixbuf_get_n_channels (p2)) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "Channels differ");
    return FALSE;
  }
  if (gdk_pixbuf_get_bits_per_sample (p1) != gdk_pixbuf_get_bits_per_sample (p2)) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "BPP differ");
    return FALSE;
  }
  if (gdk_pixbuf_get_width (p1) != gdk_pixbuf_get_width (p2)) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "Widths differ");
    return FALSE;
  }
  if (gdk_pixbuf_get_height (p1) != gdk_pixbuf_get_height (p2)) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "Heights differ");
    return FALSE;
  }
  if (gdk_pixbuf_get_rowstride (p1) != gdk_pixbuf_get_rowstride (p2)) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "Rowstrides differ");
    return FALSE;
  }
  if (memcmp (gdk_pixbuf_get_pixels (p1), gdk_pixbuf_get_pixels (p2),
          gdk_pixbuf_get_byte_length (p1)) != 0) {
    g_set_error_literal (error, GDK_PIXBUF_ERROR, 0, "Data differ");
    return FALSE;
  }

  return TRUE;
}
