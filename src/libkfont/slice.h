#ifndef SLICE_H
#define SLICE_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define INVALID_CODE_POINT INT32_MAX

#if CHAR_BIT != 8
# error CHAR_BIT != 8, I have no idea how to read octets on this system.
#endif

struct kfont_slice {
    unsigned char *ptr;
	unsigned char *end;
};

static inline uint16_t peek_uint16(uint8_t *data)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	// TODO(dmage): is unaligned access allowed?
	return *(uint16_t *)data;
#else
# error TODO
#endif
}

static inline uint32_t peek_uint32(uint8_t *data)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	// TODO(dmage): is unaligned access allowed?
	return *(uint32_t *)data;
#else
# error TODO
#endif
}

static inline bool read_uint16(struct kfont_slice *slice, uint16_t *out)
{
	if (slice->ptr + sizeof(uint16_t) > slice->end) {
		return false;
	}

	*out = peek_uint16(slice->ptr);
	slice->ptr += sizeof(uint16_t);
	return true;
}

static inline bool read_uint32(struct kfont_slice *slice, uint32_t *out)
{
	if (slice->ptr + sizeof(uint32_t) > slice->end) {
		return false;
	}

	*out = peek_uint32(slice->ptr);
	slice->ptr += sizeof(uint32_t);
	return true;
}

static inline bool read_utf8_rune(struct kfont_slice *slice, uint32_t *out)
{
	if (slice->ptr + 1 > slice->end) {
		return false;
	}

	uint8_t c = *slice->ptr++;
	if (c < 0x80) {
		*out = c;
		return true;
	}

	int need;
	uint32_t result;
	if ((c & 0xfe) == 0xfc) {
		need   = 5;
		result = c & 0x01;
	} else if ((c & 0xfc) == 0xf8) {
		need   = 4;
		result = c & 0x03;
	} else if ((c & 0xf8) == 0xf0) {
		need   = 3;
		result = c & 0x07;
	} else if ((c & 0xf0) == 0xe0) {
		need   = 2;
		result = c & 0x0f;
	} else if ((c & 0xe0) == 0xc0) {
		need   = 1;
		result = c & 0x1f;
	} else {
		*out = INVALID_CODE_POINT;
		return true;
	}

	for (int i = 0; i < need; i++) {
		if (slice->ptr == slice->end) {
			return false;
		}

		c = *slice->ptr;
		if ((c & 0xc0) != 0x80) {
			*out = INVALID_CODE_POINT;
			return true;
		}
		slice->ptr++;

		result = (result << 6) + (c & 0x3f);
	}

	*out = result;
	return true;
}

#endif
