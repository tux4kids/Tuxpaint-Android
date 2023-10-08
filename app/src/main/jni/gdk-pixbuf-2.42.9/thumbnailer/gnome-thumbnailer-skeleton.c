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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <locale.h>
#include <math.h>
#include <stdlib.h>

#include "gnome-thumbnailer-skeleton.h"

#ifndef THUMBNAILER_USAGE
#error "THUMBNAILER_USAGE must be set"
#endif

static int output_size = 256;
static gboolean g_fatal_warnings = FALSE;
static char **filenames = NULL;

#if !GDK_PIXBUF_CHECK_VERSION(2,36,5)
/**
 * gnome_desktop_thumbnail_scale_down_pixbuf:
 * @pixbuf: a #GdkPixbuf
 * @dest_width: the desired new width
 * @dest_height: the desired new height
 *
 * Scales the pixbuf to the desired size. This function
 * is a lot faster than gdk-pixbuf when scaling down by
 * large amounts.
 *
 * Return value: (transfer full): a scaled pixbuf
 * 
 * Since: 2.2
 **/
GdkPixbuf *
gnome_desktop_thumbnail_scale_down_pixbuf (GdkPixbuf *pixbuf,
					   int dest_width,
					   int dest_height)
{
	int source_width, source_height;
	int s_x1, s_y1, s_x2, s_y2;
	int s_xfrac, s_yfrac;
	int dx, dx_frac, dy, dy_frac;
	div_t ddx, ddy;
	int x, y;
	int r, g, b, a;
	int n_pixels;
	gboolean has_alpha;
	guchar *dest, *src, *xsrc, *src_pixels;
	GdkPixbuf *dest_pixbuf;
	int pixel_stride;
	int source_rowstride, dest_rowstride;

	if (dest_width == 0 || dest_height == 0) {
		return NULL;
	}
	
	source_width = gdk_pixbuf_get_width (pixbuf);
	source_height = gdk_pixbuf_get_height (pixbuf);

	g_assert (source_width >= dest_width);
	g_assert (source_height >= dest_height);

	ddx = div (source_width, dest_width);
	dx = ddx.quot;
	dx_frac = ddx.rem;
	
	ddy = div (source_height, dest_height);
	dy = ddy.quot;
	dy_frac = ddy.rem;

	g_assert (dx >= 1);
	g_assert (dy >= 1);

	has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	source_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	src_pixels = gdk_pixbuf_get_pixels (pixbuf);

	dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8,
				      dest_width, dest_height);
	dest = gdk_pixbuf_get_pixels (dest_pixbuf);
	dest_rowstride = gdk_pixbuf_get_rowstride (dest_pixbuf);

	pixel_stride = (has_alpha)?4:3;
	
	s_y1 = 0;
	s_yfrac = -dest_height/2;
	while (s_y1 < source_height) {
		s_y2 = s_y1 + dy;
		s_yfrac += dy_frac;
		if (s_yfrac > 0) {
			s_y2++;
			s_yfrac -= dest_height;
		}

		s_x1 = 0;
		s_xfrac = -dest_width/2;
		while (s_x1 < source_width) {
			s_x2 = s_x1 + dx;
			s_xfrac += dx_frac;
			if (s_xfrac > 0) {
				s_x2++;
				s_xfrac -= dest_width;
			}

			/* Average block of [x1,x2[ x [y1,y2[ and store in dest */
			r = g = b = a = 0;
			n_pixels = 0;

			src = src_pixels + s_y1 * source_rowstride + s_x1 * pixel_stride;
			for (y = s_y1; y < s_y2; y++) {
				xsrc = src;
				if (has_alpha) {
					for (x = 0; x < s_x2-s_x1; x++) {
						n_pixels++;
						
						r += xsrc[3] * xsrc[0];
						g += xsrc[3] * xsrc[1];
						b += xsrc[3] * xsrc[2];
						a += xsrc[3];
						xsrc += 4;
					}
				} else {
					for (x = 0; x < s_x2-s_x1; x++) {
						n_pixels++;
						r += *xsrc++;
						g += *xsrc++;
						b += *xsrc++;
					}
				}
				src += source_rowstride;
			}

			g_assert (n_pixels > 0);

			if (has_alpha) {
				if (a != 0) {
					*dest++ = r / a;
					*dest++ = g / a;
					*dest++ = b / a;
					*dest++ = a / n_pixels;
				} else {
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
				}
			} else {
				*dest++ = r / n_pixels;
				*dest++ = g / n_pixels;
				*dest++ = b / n_pixels;
			}
			
			s_x1 = s_x2;
		}
		s_y1 = s_y2;
		dest += dest_rowstride - dest_width * pixel_stride;
	}
	
	return dest_pixbuf;
}
#endif

static char *
get_target_uri (GFile *file)
{
	GFileInfo *info;
	char *target;

	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (info == NULL)
		return NULL;
	target = g_strdup (g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI));
	g_object_unref (info);

	return target;
}

static char *
get_target_path (GFile *input)
{
	if (g_file_has_uri_scheme (input, "trash") != FALSE ||
	    g_file_has_uri_scheme (input, "recent") != FALSE) {
		GFile *file;
		char *input_uri;
		char *input_path;

		input_uri = get_target_uri (input);
		file = g_file_new_for_uri (input_uri);
		g_free (input_uri);
		input_path = g_file_get_path (file);
		g_object_unref (file);
		return input_path;
	}
	return g_file_get_path (input);
}

static const GOptionEntry entries[] = {
	{ "size", 's', 0, G_OPTION_ARG_INT, &output_size, "Size of the thumbnail in pixels", NULL },
	{"g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, "Make all warnings fatal", NULL},
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, "[INPUT FILE] [OUTPUT FILE]" },
	{ NULL }
};

int main (int argc, char **argv)
{
	char *input_filename;
	GdkPixbuf *pixbuf;
	GError *error = NULL;
	GOptionContext *context;
	GFile *input;
	const char *output;

#ifdef THUMBNAILER_RETURNS_PIXBUF
	/* Nothing */
#elif THUMBNAILER_RETURNS_DATA
	char *data = NULL;
	gsize length;
#endif

	setlocale (LC_ALL, "");

#if !GLIB_CHECK_VERSION(2, 36, 0)
	g_type_init ();
#endif

	/* Options parsing */
	context = g_option_context_new (THUMBNAILER_USAGE);
	g_option_context_add_main_entries (context, entries, NULL);

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_warning ("Couldn't parse command-line options: %s", error->message);
		g_error_free (error);
		return 1;
	}

	/* Set fatal warnings if required */
	if (g_fatal_warnings) {
		GLogLevelFlags fatal_mask;

		fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
		fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
		g_log_set_always_fatal (fatal_mask);
	}

	if (filenames == NULL || g_strv_length (filenames) != 2) {
		g_print ("Expects an input and an output file\n");
		return 1;
	}

	input = g_file_new_for_commandline_arg (filenames[0]);
	input_filename = get_target_path (input);
	g_object_unref (input);

	if (input_filename == NULL) {
		g_warning ("Could not get file path for %s", filenames[0]);
		return 1;
	}

	output = filenames[1];

#ifdef THUMBNAILER_RETURNS_PIXBUF
	pixbuf = file_to_pixbuf (input_filename, output_size, &error);
	if (pixbuf != NULL) {
		int width, height;

		width = gdk_pixbuf_get_width (pixbuf);
		height = gdk_pixbuf_get_height (pixbuf);

		/* Handle naive thumbnailers that don't resize */
		if (output_size != 0 &&
		    (height > output_size || width > output_size)) {
			GdkPixbuf *scaled;
			double scale;

			scale = (double)output_size / MAX (width, height);

#if !GDK_PIXBUF_CHECK_VERSION(2,36,5)
			scaled = gnome_desktop_thumbnail_scale_down_pixbuf (pixbuf,
									    floor (width * scale + 0.5),
									    floor (height * scale + 0.5));
#else
			scaled = gdk_pixbuf_scale_simple (pixbuf,
							  floor (width * scale + 0.5),
							  floor (height * scale + 0.5),
							  GDK_INTERP_BILINEAR);
#endif
			gdk_pixbuf_copy_options (pixbuf, scaled);
			g_object_unref (pixbuf);
			pixbuf = scaled;
		}
	}
#elif THUMBNAILER_RETURNS_DATA
	data = file_to_data (input_filename, &length, &error);
	if (data) {
		GInputStream *mem_stream;

		mem_stream = g_memory_input_stream_new_from_data (data, length, g_free);
		pixbuf = gdk_pixbuf_new_from_stream_at_scale (mem_stream, output_size, -1, TRUE, NULL, &error);
		g_object_unref (mem_stream);
	} else {
		pixbuf = NULL;
	}
#else
#error "One of THUMBNAILER_RETURNS_PIXBUF or THUMBNAILER_RETURNS_DATA must be set"
#endif
	g_free (input_filename);

	if (!pixbuf) {
		g_warning ("Could not thumbnail '%s': %s", filenames[0],
			   error ? error->message : "Thumbnailer failed without returning an error");
		g_clear_error (&error);
		g_strfreev (filenames);
		return 1;
	}

	if (gdk_pixbuf_save (pixbuf, output, "png", &error, NULL) == FALSE) {
		g_warning ("Couldn't save the thumbnail '%s' for file '%s': %s", output, filenames[0], error->message);
		g_error_free (error);
		return 1;
	}

	g_object_unref (pixbuf);

	return 0;
}
