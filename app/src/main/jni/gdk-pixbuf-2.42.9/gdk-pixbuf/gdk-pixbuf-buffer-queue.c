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

#include "config.h"

#include "gdk-pixbuf-buffer-queue-private.h"

#include <string.h>

struct _GdkPixbufBufferQueue
{
  GSList *	first_buffer;		/* pointer to first buffer */
  GSList *	last_buffer;		/* pointer to last buffer (for fast appending) */
  gsize		size;			/* amount of bytes in the queue */
  gsize		offset;			/* amount of data already flushed out of the queue */
  
  int		ref_count;
};

/**
 * GdkPixbufBufferQueue:
 *
 * A #GdkPixbufBufferQueue is a queue of continuous buffers that allows reading
 * its data in chunks of pre-defined sizes. It is used to transform a data 
 * stream that was provided by buffers of random sizes to buffers of the right
 * size.
 */

/**
 * gdk_pixbuf_buffer_queue_new:
 *
 * Creates a new empty buffer queue.
 *
 * Returns: a new buffer queue. Use gdk_pixbuf_buffer_queue_unref () to free it.
 **/
GdkPixbufBufferQueue *
gdk_pixbuf_buffer_queue_new (void)
{
  GdkPixbufBufferQueue *buffer_queue;

  buffer_queue = g_new0 (GdkPixbufBufferQueue, 1);
  buffer_queue->ref_count = 1;

  return buffer_queue;
}

/**
 * gdk_pixbuf_buffer_queue_get_size:
 * @queue: a #GdkPixbufBufferQueue
 *
 * Returns the number of bytes currently in @queue.
 *
 * Returns: amount of bytes in @queue.
 **/
gsize
gdk_pixbuf_buffer_queue_get_size (GdkPixbufBufferQueue *queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->size;
}

/**
 * gdk_pixbuf_buffer_queue_get_offset:
 * @queue: a #GdkPixbufBufferQueue
 *
 * Queries the amount of bytes that has already been pulled out of
 * @queue using functions like gdk_pixbuf_buffer_queue_pull().
 *
 * Returns: Number of bytes that were already pulled from this queue.
 **/
gsize
gdk_pixbuf_buffer_queue_get_offset (GdkPixbufBufferQueue * queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->offset;
}

/**
 * gdk_pixbuf_buffer_queue_flush:
 * @queue: a #GdkPixbufBufferQueue
 * @n_bytes: amount of bytes to flush from the queue
 *
 * Removes the first @n_bytes bytes from the queue.
 */
void
gdk_pixbuf_buffer_queue_flush (GdkPixbufBufferQueue *queue, gsize n_bytes)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (n_bytes <= queue->size);

  queue->size -= n_bytes;
  queue->offset += n_bytes;

  while (n_bytes > 0)
    {
      GBytes *bytes;
      gsize size;
      
      bytes = queue->first_buffer->data;
      size = g_bytes_get_size (bytes);

      if (size <= n_bytes)
        {
          n_bytes -= size;
          queue->first_buffer = g_slist_remove (queue->first_buffer, bytes);
        }
      else
        {
          queue->first_buffer->data = g_bytes_new_from_bytes (bytes,
	                                                      n_bytes,
                                                              size - n_bytes);
          n_bytes = 0;
        }
      g_bytes_unref (bytes);
    }

  if (queue->first_buffer == NULL)
    queue->last_buffer = NULL;
}

/**
 * gdk_pixbuf_buffer_queue_clear:
 * @queue: a #GdkPixbufBufferQueue
 *
 * Resets @queue into to initial state. All buffers it contains will be 
 * released and the offset will be reset to 0.
 **/
void
gdk_pixbuf_buffer_queue_clear (GdkPixbufBufferQueue *queue)
{
  g_return_if_fail (queue != NULL);

  g_slist_free_full (queue->first_buffer, (GDestroyNotify) g_bytes_unref);
  queue->first_buffer = NULL;
  queue->last_buffer = NULL;
  queue->size = 0;
  queue->offset = 0;
}

/**
 * gdk_pixbuf_buffer_queue_push:
 * @queue: a #GdkPixbufBufferQueue
 * @bytes: #GBytes to append to @queue
 *
 * Appends the given @bytes to the buffers already in @queue. This function
 * will take ownership of the given @buffer. Use g_bytes_ref () before
 * calling this function to keep a reference.
 **/
void
gdk_pixbuf_buffer_queue_push (GdkPixbufBufferQueue *queue,
                              GBytes               *bytes)
{
  gsize size;

  g_return_if_fail (queue != NULL);
  g_return_if_fail (bytes != NULL);

  size = g_bytes_get_size (bytes);
  if (size == 0)
    {
      g_bytes_unref (bytes);
      return;
    }

  queue->last_buffer = g_slist_append (queue->last_buffer, bytes);
  if (queue->first_buffer == NULL)
    queue->first_buffer = queue->last_buffer;
  else
    queue->last_buffer = queue->last_buffer->next;

  queue->size += size;
}

/**
 * gdk_pixbuf_buffer_queue_peek:
 * @queue: a #GdkPixbufBufferQueue to read from
 * @length: amount of bytes to peek
 *
 * Creates a new buffer with the first @length bytes from @queue, but unlike 
 * gdk_pixbuf_buffer_queue_pull(), does not remove them from @queue.
 *
 * Returns: NULL if the requested amount of data wasn't available or a new 
 *          #GBytes. Use g_bytes_unref() after use.
 **/
GBytes *
gdk_pixbuf_buffer_queue_peek (GdkPixbufBufferQueue *queue,
                              gsize                 length)
{
  GSList *g;
  GBytes *result, *bytes;

  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->size < length)
    return NULL;

  /* need to special case here, because the queue may be empty */
  if (length == 0)
    return g_bytes_new (NULL, 0);

  g = queue->first_buffer;
  bytes = g->data;
  if (g_bytes_get_size (bytes) == length)
    {
      result = g_bytes_ref (bytes);
    }
  else if (g_bytes_get_size (bytes) > length)
    {
      result = g_bytes_new_from_bytes (bytes, 0, length);
    }
  else
    {
      guchar *data;
      gsize amount, offset;

      data = g_malloc (length);
      
      for (offset = 0; offset < length; offset += amount)
        {
          bytes = g->data;
          amount = MIN (length - offset, g_bytes_get_size (bytes));
          memcpy (data + offset, g_bytes_get_data (bytes, NULL), amount);
          g = g->next;
        }

      result = g_bytes_new_take (data, length);
    }

  return result;
}

/**
 * gdk_pixbuf_buffer_queue_pull:
 * @queue: a #GdkPixbufBufferQueue
 * @length: amount of bytes to pull
 *
 * If enough data is still available in @queue, the first @length bytes are 
 * put into a new buffer and that buffer is returned. The @length bytes are
 * removed from the head of the queue. If not enough data is available, %NULL
 * is returned.
 *
 * Returns: a new #GBytes or %NULL
 **/
GBytes *
gdk_pixbuf_buffer_queue_pull (GdkPixbufBufferQueue * queue, gsize length)
{
  GBytes *result;

  g_return_val_if_fail (queue != NULL, NULL);

  result = gdk_pixbuf_buffer_queue_peek (queue, length);
  if (result == NULL)
    return NULL;

  gdk_pixbuf_buffer_queue_flush (queue, length);

  return result;
}

/**
 * gdk_pixbuf_buffer_queue_peek_buffer:
 * @queue: a #GdkPixbufBufferQueue
 *
 * Gets the first buffer out of @queue and returns it. This function is 
 * equivalent to calling gdk_pixbuf_buffer_queue_peek() with the size of the
 * first buffer in it.
 *
 * Returns: The first buffer in @queue or %NULL if @queue is empty. Use 
 *          g_bytes_unref() after use.
 **/
GBytes *
gdk_pixbuf_buffer_queue_peek_buffer (GdkPixbufBufferQueue * queue)
{
  GBytes *bytes;

  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->first_buffer == NULL)
    return NULL;

  bytes = queue->first_buffer->data;

  return g_bytes_ref (bytes);
}

/**
 * gdk_pixbuf_buffer_queue_pull_buffer:
 * @queue: a #GdkPixbufBufferQueue
 *
 * Pulls the first buffer out of @queue and returns it. This function is 
 * equivalent to calling gdk_pixbuf_buffer_queue_pull() with the size of the
 * first buffer in it.
 *
 * Returns: The first buffer in @queue or %NULL if @queue is empty.
 **/
GBytes *
gdk_pixbuf_buffer_queue_pull_buffer (GdkPixbufBufferQueue *queue)
{
  GBytes *bytes;

  g_return_val_if_fail (queue != NULL, NULL);

  bytes = gdk_pixbuf_buffer_queue_peek_buffer (queue);
  if (bytes)
    gdk_pixbuf_buffer_queue_flush (queue, g_bytes_get_size (bytes));

  return bytes;
}

/**
 * gdk_pixbuf_buffer_queue_ref:
 * @queue: a #GdkPixbufBufferQueue
 *
 * increases the reference count of @queue by one.
 *
 * Returns: The passed in @queue.
 **/
GdkPixbufBufferQueue *
gdk_pixbuf_buffer_queue_ref (GdkPixbufBufferQueue * queue)
{
  g_return_val_if_fail (queue != NULL, NULL);
  g_return_val_if_fail (queue->ref_count > 0, NULL);

  queue->ref_count++;
  return queue;
}

/**
 * gdk_pixbuf_buffer_queue_unref:
 * @queue: a #GdkPixbufBufferQueue
 *
 * Decreases the reference count of @queue by one. If no reference 
 * to this buffer exists anymore, the buffer and the memory 
 * it manages are freed.
 **/
void
gdk_pixbuf_buffer_queue_unref (GdkPixbufBufferQueue * queue)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (queue->ref_count > 0);

  queue->ref_count--;
  if (queue->ref_count > 0)
    return;

  gdk_pixbuf_buffer_queue_clear (queue);
  g_free (queue);
}

