/* GdkPixbuf library
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GDK_PIXBUF_BUFFER_QUEUE_H__
#define __GDK_PIXBUF_BUFFER_QUEUE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GdkPixbufBufferQueue GdkPixbufBufferQueue;

GdkPixbufBufferQueue *  gdk_pixbuf_buffer_queue_new             (void);

GdkPixbufBufferQueue *  gdk_pixbuf_buffer_queue_ref             (GdkPixbufBufferQueue   *queue);
void                    gdk_pixbuf_buffer_queue_unref           (GdkPixbufBufferQueue   *queue);

gsize                   gdk_pixbuf_buffer_queue_get_size        (GdkPixbufBufferQueue   *queue);
gsize                   gdk_pixbuf_buffer_queue_get_offset      (GdkPixbufBufferQueue   *queue);

void                    gdk_pixbuf_buffer_queue_flush           (GdkPixbufBufferQueue   *queue,
                                                                 gsize                   n_bytes);
void                    gdk_pixbuf_buffer_queue_clear           (GdkPixbufBufferQueue   *queue);
void                    gdk_pixbuf_buffer_queue_push            (GdkPixbufBufferQueue   *queue,
                                                                 GBytes                 *buffer);
GBytes *                gdk_pixbuf_buffer_queue_pull            (GdkPixbufBufferQueue   *queue,
                                                                 gsize                   length);
GBytes *                gdk_pixbuf_buffer_queue_pull_buffer     (GdkPixbufBufferQueue   *queue);
GBytes *                gdk_pixbuf_buffer_queue_peek            (GdkPixbufBufferQueue   *queue,
                                                                 gsize                   length);
GBytes *                gdk_pixbuf_buffer_queue_peek_buffer     (GdkPixbufBufferQueue   *queue);

G_END_DECLS
#endif

