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

#include "stream_reader.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <zlib.h>

#define HB_STREAM_READER_MAX_INFLATED_SIZE (1024*1024*100)

#define HB_STREAM_READER_VALID_READ_ASSERT(s, req) \
	assert(s->len - s->offset >= (req))

#define HB_STREAM_READER_XXX(s, type) do { \
	uint8_t data[sizeof(type)]; \
	hb_stream_reader_uint8_array_rev(s, sizeof(type), &data[0]); \
	type ret = *((type *)(&data[0])); \
	return ret; \
} while (0)

struct hb_stream_reader *hb_stream_reader_new(size_t len)
{
	struct hb_stream_reader *s = malloc(sizeof(struct hb_stream_reader));
	assert(s != NULL);
	s->offset = 0;
	s->len = len;
	s->data = malloc(s->len);
	assert(s->data != NULL);
	return s;
}

struct hb_stream_reader *hb_stream_reader_from_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	assert(fp != NULL);
	fseek(fp, 0, SEEK_END);
	struct hb_stream_reader *s = hb_stream_reader_new(ftell(fp));
	assert(s != NULL);
	fseek(fp, 0, SEEK_SET);
	fread(s->data, s->len, 1, fp);
	fclose(fp);
	return s;
}

struct hb_stream_reader *hb_stream_reader_slice(struct hb_stream_reader *s,
                                                size_t len)
{
	HB_STREAM_READER_VALID_READ_ASSERT(s, len);
	struct hb_stream_reader *slice = hb_stream_reader_new(len);
	if (!slice) return NULL;
	memcpy(&slice->data[0], &s->data[s->offset], len);
	s->offset += len;
	return slice;
}

static void hb_stream_reader_uint8_array_rev(struct hb_stream_reader *s,
                                             size_t len,
                                             uint8_t *arr)
{
	HB_STREAM_READER_VALID_READ_ASSERT(s, len);

	for (size_t i = 0; i < len; ++i)
		arr[len - i - 1] = s->data[s->offset + i];

	s->offset += len;
}

int8_t      hb_stream_reader_int8(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,    int8_t); }
uint8_t    hb_stream_reader_uint8(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,   uint8_t); }
int16_t    hb_stream_reader_int16(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,   int16_t); }
uint16_t  hb_stream_reader_uint16(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,  uint16_t); }
int32_t    hb_stream_reader_int32(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,   int32_t); }
uint32_t  hb_stream_reader_uint32(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,  uint32_t); }
float      hb_stream_reader_float(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,     float); }
double    hb_stream_reader_double(struct hb_stream_reader *s) { HB_STREAM_READER_XXX(s,    double); }
bool        hb_stream_reader_bool(struct hb_stream_reader *s) { return !!hb_stream_reader_uint8(s); }

void hb_stream_reader_string_ascii(struct hb_stream_reader *s,
                                   uint32_t len,
                                   size_t cap,
                                   char *str)
{
	assert(cap != 0);
	HB_STREAM_READER_VALID_READ_ASSERT(s, len);
	if (len == 0) { str[0] = '\0'; return; }
	size_t read_count = len >= cap ? cap - 1 : len;
	memcpy(str, &s->data[s->offset], read_count);
	str[read_count] = '\0';
	s->offset += len;
}

void hb_stream_reader_inflate(struct hb_stream_reader *s, bool raw)
{
	uint8_t *inflated = malloc(HB_STREAM_READER_MAX_INFLATED_SIZE);

	z_stream strm = { .zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL };
	strm.avail_in = s->len - s->offset;
	strm.next_in = &s->data[s->offset];
	strm.avail_out = HB_STREAM_READER_MAX_INFLATED_SIZE;
	strm.next_out = inflated;

	assert((raw ? inflateInit2(&strm, -15) : inflateInit(&strm)) == Z_OK);
	assert((inflate(&strm, Z_FINISH)) == Z_STREAM_END);

	free(s->data);
	s->offset = 0;
	s->len = HB_STREAM_READER_MAX_INFLATED_SIZE - strm.avail_out;
	s->data = malloc(s->len);

	memcpy(s->data, inflated, s->len);
	inflateEnd(&strm);
}

void hb_stream_reader_free(struct hb_stream_reader *s)
{
	free(s->data);
	free(s);
}
