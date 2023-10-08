/*
 * Copyright (C) 2018 Canonical Ltd.
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

#ifndef GDK_PIXBUF_LZW_H
#define GDK_PIXBUF_LZW_H

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (LZWDecoder, lzw_decoder, LZW, DECODER, GObject)

/* Maximum code size in bits */
#define LZW_CODE_MAX 12

LZWDecoder *lzw_decoder_new  (guint8     code_size);

gsize       lzw_decoder_feed (LZWDecoder *decoder,
                              guint8     *input,
                              gsize       input_length,
                              guint8     *output,
                              gsize       output_length);

G_END_DECLS

#endif
