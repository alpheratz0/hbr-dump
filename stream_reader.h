// ISC License (C) 2023 <alpheratz99@protonmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
// OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define hb_stream_reader_string_ascii_auto(stream, str) \
	hb_stream_reader_string_ascii(stream, hb_stream_reader_uint16(stream), \
			sizeof(str), &str[0])

struct hb_stream_reader {
	uint8_t *data;
	size_t len;
	size_t offset;
};

struct hb_stream_reader *hb_stream_reader_new(size_t len);
struct hb_stream_reader *hb_stream_reader_from_file(const char *path);
struct hb_stream_reader *hb_stream_reader_slice(struct hb_stream_reader *s, size_t len);

int8_t      hb_stream_reader_int8(struct hb_stream_reader *s);
uint8_t    hb_stream_reader_uint8(struct hb_stream_reader *s);
int16_t    hb_stream_reader_int16(struct hb_stream_reader *s);
uint16_t  hb_stream_reader_uint16(struct hb_stream_reader *s);
int32_t    hb_stream_reader_int32(struct hb_stream_reader *s);
uint32_t  hb_stream_reader_uint32(struct hb_stream_reader *s);
float      hb_stream_reader_float(struct hb_stream_reader *s);
double    hb_stream_reader_double(struct hb_stream_reader *s);
bool        hb_stream_reader_bool(struct hb_stream_reader *s);

void hb_stream_reader_string_ascii(struct hb_stream_reader *s,
                                   uint32_t len,
                                   size_t cap,
                                   char *str);

void hb_stream_reader_inflate(struct hb_stream_reader *s, bool raw);

void hb_stream_reader_free(struct hb_stream_reader *s);

#ifdef __cplusplus
}
#endif
