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

#ifndef __TEST_COMMON_H__

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

typedef gboolean (* AddTestFunc) (GFile *file);

gboolean format_supported (const gchar *filename);
gboolean file_supported (GFile *file);
gboolean find_format (const gchar *filename, gchar **found_format);
gboolean skip_if_insufficient_memory (GError **err);
gboolean pixdata_equal (GdkPixbuf *test, GdkPixbuf *ref, GError **error);
GdkPixbuf *make_checkerboard (int width, int height);
GdkPixbuf *make_rg (int width, int height);
void add_test_for_all_images (const gchar   *prefix,
                              GFile         *base,
                              GFile         *file,
                              GTestDataFunc  test_func,
                              AddTestFunc    add_test_func);


G_END_DECLS

#endif  /* __TEST_COMMON_H__ */
