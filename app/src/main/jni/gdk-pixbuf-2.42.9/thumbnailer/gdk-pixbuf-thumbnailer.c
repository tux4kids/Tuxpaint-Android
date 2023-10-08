/*
 * Copyright (C) 2016 Bastien Nocera <hadess@hadess.net>
 *
 * Authors: Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <string.h>
#include <glib.h>

#include "gnome-thumbnailer-skeleton.h"

GdkPixbuf *
file_to_pixbuf (const char  *path,
		guint        destination_size,
	        GError     **error)
{
	GdkPixbuf *pixbuf, *tmp_pixbuf;
	const char *original_width_str, *original_height_str;

	pixbuf = gdk_pixbuf_new_from_file_at_size (path, destination_size, destination_size, error);
	if (pixbuf == NULL)
		return NULL;

	/* The GIF codec throws GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION
	 * if it's closed without decoding all the frames. Since
	 * gdk_pixbuf_new_from_file_at_size only decodes the first
	 * frame, this specific error needs to be ignored.
	 */
	if (error != NULL && g_error_matches (*error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION))
		g_clear_error (error);

	tmp_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
	gdk_pixbuf_copy_options (pixbuf, tmp_pixbuf);
	gdk_pixbuf_remove_option (tmp_pixbuf, "orientation");
	g_object_unref (pixbuf);
	pixbuf = tmp_pixbuf;

	original_width_str = gdk_pixbuf_get_option (pixbuf, "original-width");
	original_height_str = gdk_pixbuf_get_option (pixbuf, "original-height");

	if (original_width_str != NULL)
		gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Width", original_width_str);

	if (original_height_str != NULL)
		gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Height", original_height_str);

	return pixbuf;
}
