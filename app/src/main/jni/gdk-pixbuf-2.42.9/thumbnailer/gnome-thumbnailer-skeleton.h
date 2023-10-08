/*
 * Copyright (C) 2013 Bastien Nocera <hadess@hadess.net>
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef THUMBNAILER_RETURNS_PIXBUF
#ifdef THUMBNAILER_RETURNS_DATA
#error "Only one of THUMBNAILER_RETURNS_PIXBUF or THUMBNAILER_RETURNS_DATA must be set"
#else
GdkPixbuf * file_to_pixbuf (const char  *path,
			    guint        destination_size,
			    GError     **error);
#endif
#elif THUMBNAILER_RETURNS_DATA
char * file_to_data (const char  *path,
		     gsize       *ret_length,
		     GError     **error);
#else
#error "One of THUMBNAILER_RETURNS_PIXBUF or THUMBNAILER_RETURNS_DATA must be set"
#endif
