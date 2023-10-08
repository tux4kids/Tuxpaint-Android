/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
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

#include "lzw.h"

/* Maximum number of codes */
#define MAX_CODES (1 << LZW_CODE_MAX)

typedef struct
{
        /* Last index this code represents */
        guint8 index;

        /* Codeword of previous index or the EOI code if doesn't extend */
        guint16 extends;
} LZWCode;

struct _LZWDecoder
{
        GObject parent_instance;

        /* Initial code size */
        int min_code_size;

        /* Current code size */
        int code_size;

        /* Code table and special codes */
        int clear_code;
        int eoi_code;
        LZWCode code_table[MAX_CODES];
        int code_table_size;

        /* Current code being assembled */
        int code;
        int code_bits;

        /* Last code processed */
        int last_code;
};

G_DEFINE_TYPE (LZWDecoder, lzw_decoder, G_TYPE_OBJECT)

static void
add_code (LZWDecoder *self,
          int         code)
{
        /* Find the first index of the given code */
        int c = code;
        while (self->code_table[c].extends != self->eoi_code)
                c = self->code_table[c].extends;

        /* Make a new code that extends the previous code */
        self->code_table[self->code_table_size].index = self->code_table[c].index;
        self->code_table[self->code_table_size].extends = self->last_code;
        self->code_table_size++;
}

static gsize
write_indexes (LZWDecoder *self,
               guint8     *output,
               gsize       output_length)
{
        int c;
        gsize index_count = 1, offset;

        /* Ignore invalid codeword */
        if (self->code >= self->code_table_size)
                return 0;

        /* Work out how many indexes this code represents... */
        c = self->code;
        while (self->code_table[c].extends != self->eoi_code) {
                c = self->code_table[c].extends;
                index_count++;
        }

        /* ...then write the indexes in backwards */
        c = self->code;
        offset = index_count - 1;
        while (TRUE) {
                if (offset < output_length)
                        output[offset] = self->code_table[c].index;

                if (self->code_table[c].extends == self->eoi_code)
                        return index_count;

                c = self->code_table[c].extends;
                offset--;
        }
}

void
lzw_decoder_class_init (LZWDecoderClass *klass)
{
}

void
lzw_decoder_init (LZWDecoder *self)
{
}

LZWDecoder *
lzw_decoder_new (guint8 code_size)
{
        LZWDecoder *self;
        int i;

        g_return_val_if_fail (code_size <= LZW_CODE_MAX, NULL);

        self = g_object_new (lzw_decoder_get_type (), NULL);

        self->min_code_size = code_size;
        self->code_size = code_size;

        /* Add special clear and end of information codes */
        self->clear_code = 1 << (code_size - 1);
        self->eoi_code = self->clear_code + 1;

        for (i = 0; i <= self->eoi_code; i++) {
                self->code_table[i].index = i;
                self->code_table[i].extends = self->eoi_code;
                self->code_table_size++;
        }

        /* Start with an empty codeword following an implicit clear codeword */
        self->code = 0;
        self->last_code = self->clear_code;

        return self;
}

gsize
lzw_decoder_feed (LZWDecoder *self,
                  guint8     *input,
                  gsize       input_length,
                  guint8     *output,
                  gsize       output_length)
{
        gsize i, n_written = 0;

        g_return_val_if_fail (LZW_IS_DECODER (self), 0);

        /* Ignore data after "end of information" codeword */
        if (self->last_code == self->eoi_code)
                return 0;

        /* Processes each octet of input */
        for (i = 0; i < input_length; i++) {
                guint8 d = input[i];
                int n_available;

                /* Process the bits of the octet into codewords */
                for (n_available = 8; n_available > 0; ) {
                        int n_bits, new_bits;

                        /* Extract up the the required number of bits from the octet */
                        n_bits = MIN (self->code_size - self->code_bits, n_available);
                        new_bits = d & ((1 << n_bits) - 1);
                        d = d >> n_bits;
                        n_available -= n_bits;

                        /* Add the new bits to the code until we have a full codeword */
                        self->code = new_bits << self->code_bits | self->code;
                        self->code_bits += n_bits;
                        if (self->code_bits < self->code_size)
                                continue;

                        /* Stop on "end of information" codeword */
                        if (self->code == self->eoi_code) {
                                self->last_code = self->code;
                                return n_written;
                        }

                        /* Reset the code table on "clear" */
                        if (self->code == self->clear_code) {
                                self->code_table_size = self->eoi_code + 1;
                                self->code_size = self->min_code_size;
                        } else {
                                /* Add a new code word if space.
                                 * The first code after a clear is skipped */
                                if (self->last_code != self->clear_code && self->code_table_size < MAX_CODES) {
                                        if (self->code < self->code_table_size)
                                                add_code (self, self->code);
                                        else
                                                add_code (self, self->last_code);

                                        /* When table is full increase code size */
                                        if (self->code_table_size == (1 << self->code_size) && self->code_size < LZW_CODE_MAX)
                                                self->code_size++;
                                }

                                /* Invalid code received - just stop here */
                                if (self->code >= self->code_table_size) {
                                        self->last_code = self->eoi_code;
                                        return output_length;
                                }

                                /* Convert codeword into indexes */
                                n_written += write_indexes (self, output + n_written, output_length - n_written);
                        }

                        self->last_code = self->code;
                        self->code = 0;
                        self->code_bits = 0;

                        /* Out of space */
                        if (n_written >= output_length)
                                return output_length;
                }
        }

        return n_written;
}
