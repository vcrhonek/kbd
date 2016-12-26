#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kfont.h>
#include "slice.h"
#include "xmalloc.h"

// psffontop.h
#define MAXFONTSIZE 65536

// new internal header
#define PSF1_MAGIC 0x0436
#define PSF2_MAGIC 0x864ab572

#define PSF1_MODE512 0x01
#define PSF1_MODE_HAS_TAB 0x02
#define PSF1_MODE_HAS_SEQ 0x04
#define PSF1_MAXMODE 0x05 // TODO(dmage): why not 0x07?

#define PSF1_SEPARATOR 0xFFFF
#define PSF1_START_SEQ 0xFFFE

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION 0

/* UTF8 separators */
#define PSF2_SEPARATOR 0xFF
#define PSF2_START_SEQ 0xFE

struct kfont_handler {
	unsigned char *blob;
};

static bool kfont_blob_read(FILE *f, unsigned char **buffer, size_t *size)
{
	size_t buflen = MAXFONTSIZE / 4; /* actually an arbitrary value */
	size_t n      = 0;
	unsigned char *buf = xmalloc(buflen);

	while (1) {
		if (n == buflen) {
			if (buflen > SIZE_MAX / 2) {
				xfree(buf);
				return false;
			}
			buflen *= 2;
			buf = xrealloc(buf, buflen);
		}
		n += fread(buf + n, 1, buflen - n, f);
		if (ferror(f)) {
			xfree(buf);
			return false;
		}
		if (feof(f)) {
			break;
		}
	}

	*buffer = buf;
	*size = n;

	return true;
}

// struct kfont_psf1_header {
// 	uint8_t mode;      /* PSF font mode */
// 	uint8_t char_size; /* character size */
// };
//
// struct kfont_psf2_header {
// 	uint32_t version;
// 	uint32_t header_size; /* offset of bitmaps in file */
// 	uint32_t flags;
// 	uint32_t length;        /* number of glyphs */
// 	uint32_t char_size;     /* number of bytes for each character */
// 	uint32_t height, width; /* max dimensions of glyphs */
// 	                        /* charsize = height * ((width + 7) / 8) */
// };
//
// static bool kfont_read_psf1_header(struct kfont_slice *slice, struct kfont_psf1_header *header)
// {
// 	if (slice->ptr + 2 > slice->end) {
// 		return false;
// 	}
//
// 	header->mode      = slice->ptr[0];
// 	header->char_size = slice->ptr[1];
// 	slice->ptr += 2;
// 	return true;
// }
//
// static bool kfont_read_psf2_header(struct kfont_slice *slice, struct kfont_psf2_header *header)
// {
// 	return read_uint32(slice, &header->version) &&
// 	       read_uint32(slice, &header->header_size) &&
// 	       read_uint32(slice, &header->flags) &&
// 	       read_uint32(slice, &header->length) &&
// 	       read_uint32(slice, &header->char_size) &&
// 	       read_uint32(slice, &header->height) &&
// 	       read_uint32(slice, &header->width);
// }
//
// static bool kfont_read_unicode_map(struct kfont_slice *slice, unsigned int font_pos, enum kfont_version version, struct kfont_unicode_pair **out)
// {
// 	if (version == KFONT_VERSION_PSF1) {
// 		while (1) {
// 			uint16_t uc;
// 			if (!read_uint16(slice, &uc)) {
// 				return false;
// 			}
// 			if (uc == PSF1_START_SEQ) {
// 				printf("kfont_read_unicode_map %d: <start seq>\n", font_pos);
// 				abort(); // TODO(dmage): handle <seq>*
// 			}
// 			if (uc == PSF1_SEPARATOR) {
// 				break;
// 			}
//
// 			struct kfont_unicode_pair *pair = xmalloc(sizeof(struct kfont_unicode_pair));
//
// 			pair->font_pos   = font_pos;
// 			pair->seq_length = 1;
// 			pair->seq[0]     = uc;
//
// 			pair->next = *out;
// 			*out       = pair;
// 		}
// 	} else if (version == KFONT_VERSION_PSF2) {
// 		while (1) {
// 			if (slice->ptr == slice->end) {
// 				return false;
// 			}
// 			if (*slice->ptr == PSF2_START_SEQ) {
// 				printf("kfont_read_unicode_map %d: <start seq>\n", font_pos);
// 				abort(); // TODO(dmage): handle <seq>*
// 			}
// 			if (*slice->ptr == PSF2_SEPARATOR) {
// 				slice->ptr++;
// 				break;
// 			}
//
// 			uint32_t rune;
// 			if (!read_utf8_rune(slice, &rune)) {
// 				return false;
// 			}
// 			if (rune == INVALID_RUNE) {
// 				printf("kfont_read_unicode_map %d: <invalid utf8 sequence>\n", font_pos);
// 				abort(); // TODO(dmage)
// 			}
//
// 			struct kfont_unicode_pair *pair = xmalloc(sizeof(struct kfont_unicode_pair));
//
// 			pair->font_pos   = font_pos;
// 			pair->seq_length = 1;
// 			pair->seq[0]     = rune;
//
// 			pair->next = *out;
// 			*out       = pair;
// 		}
// 	} else {
// 		printf("kfont_read_unicode_map %d: unknown font version\n", font_pos);
// 		abort();
// 	}
// 	return true;
// }
//
// bool kfont_read_file(FILE *f, struct kfont *font)
// {
// 	size_t buflen = MAXFONTSIZE / 4; /* actually an arbitrary value */
// 	size_t n      = 0;
// 	uint8_t *buf  = xmalloc(buflen);
//
// 	while (1) {
// 		if (n == buflen) {
// 			if (buflen > UINT32_MAX / 2) {
// 				xfree(buf);
// 				return false;
// 			}
// 			buflen *= 2;
// 			buf = xrealloc(buf, buflen);
// 		}
// 		n += fread(buf + n, 1, buflen - n, f);
// 		if (ferror(f)) {
// 			return false;
// 		}
// 		if (feof(f)) {
// 			break;
// 		}
// 	}
//
// 	font->content.data     = buf;
// 	font->content.size     = n;
// 	font->unicode_map_head = NULL;
//
// 	return true;
// }
//
// static void kfont_free_string(struct kfont_string *string)
// {
// 	xfree(string->data);
// 	string->data = 0;
// 	string->size = 0;
// }
//
// void kfont_free(struct kfont *font)
// {
// 	kfont_free_string(&font->content);
// 	// TODO(dmage): free unicode map
// }
//
// enum kfont_error kfont_parse_psf_font(struct kfont *font)
// {
// 	uint32_t has_table = 0;
//
// 	struct kfont_slice p;
// 	p.ptr = font->content.data;
// 	p.end = font->content.data + font->content.size;
//
// 	if (font->content.size >= 4 && peek_uint32(font->content.data) == PSF2_MAGIC) {
// 		p.ptr += 4;
//
// 		struct kfont_psf2_header psf2_header;
// 		if (!kfont_read_psf2_header(&p, &psf2_header)) {
// 			return KFONT_ERROR_BAD_PSF2_HEADER;
// 		}
//
// 		// FIXME(dmage): remove these printfs
// 		printf("PSF2\n");
// 		printf("version     : %lu\n", (unsigned long)psf2_header.version);
// 		printf("header size : %lu\n", (unsigned long)psf2_header.header_size);
// 		printf("flags       : %lu\n", (unsigned long)psf2_header.flags);
// 		printf("length      : %lu\n", (unsigned long)psf2_header.length);
// 		printf("char size   : %lu\n", (unsigned long)psf2_header.char_size);
// 		printf("height      : %lu\n", (unsigned long)psf2_header.height);
// 		printf("width       : %lu\n", (unsigned long)psf2_header.width);
//
// 		if (psf2_header.version > PSF2_MAXVERSION) {
// 			return KFONT_ERROR_UNSUPPORTED_PSF2_VERSION;
// 		}
//
// 		font->version     = KFONT_VERSION_PSF2;
// 		font->font_len    = psf2_header.length;
// 		font->char_size   = psf2_header.char_size;
// 		font->font_offset = psf2_header.header_size;
// 		font->font_width  = psf2_header.width;
// 		has_table         = (psf2_header.flags & PSF2_HAS_UNICODE_TABLE);
// 	} else if (font->content.size >= 2 && peek_uint16(font->content.data) == PSF1_MAGIC) {
// 		p.ptr += 2;
//
// 		struct kfont_psf1_header psf1_header;
// 		if (!kfont_read_psf1_header(&p, &psf1_header)) {
// 			return KFONT_ERROR_BAD_PSF1_HEADER;
// 		}
//
// 		// FIXME(dmage): remove these printfs
// 		printf("PSF1\n");
// 		printf("mode      : %u\n", (unsigned int)psf1_header.mode);
// 		printf("char size : %u\n", (unsigned int)psf1_header.char_size);
//
// 		if (psf1_header.mode > PSF1_MAXMODE) {
// 			return KFONT_ERROR_UNSUPPORTED_PSF1_MODE;
// 		}
//
// 		font->version     = KFONT_VERSION_PSF1;
// 		font->font_len    = (psf1_header.mode & PSF1_MODE512 ? 512 : 256);
// 		font->char_size   = psf1_header.char_size;
// 		font->font_offset = 4;
// 		font->font_width  = 8;
// 		has_table         = (psf1_header.mode & (PSF1_MODE_HAS_TAB | PSF1_MODE_HAS_SEQ));
// 	} else {
// 		return KFONT_ERROR_BAD_MAGIC;
// 	}
//
// 	if (font->font_offset > font->content.size) {
// 		return KFONT_ERROR_FONT_OFFSET_TOO_BIG;
// 	}
//
// 	if (font->char_size == 0) {
// 		return KFONT_ERROR_CHAR_SIZE_ZERO;
// 	}
//
// 	if (font->char_size > font->content.size - font->font_offset) {
// 		return KFONT_ERROR_CHAR_SIZE_TOO_BIG;
// 	}
//
// 	if (font->font_len > (font->content.size - font->font_offset) / font->char_size) {
// 		return KFONT_ERROR_FONT_LENGTH_TOO_BIG;
// 	}
//
// 	if (has_table) {
// 		p.ptr = font->content.data + font->font_offset + font->font_len * font->char_size;
// 		p.end = font->content.data + font->content.size;
//
// 		for (uint32_t i = 0; i < font->font_len; i++) {
// 			if (!kfont_read_unicode_map(&p, i, font->version, &font->unicode_map_head)) {
// 				return KFONT_ERROR_SHORT_UNICODE_TABLE;
// 			}
// 		}
//
// 		if (p.ptr != p.end) {
// 			return KFONT_ERROR_TRAILING_GARBAGE;
// 		}
// 	}
//
// 	return KFONT_ERROR_SUCCESS;
// }

enum kfont_error kfont_load(const char *filename, struct kfont_parse_options opts, kfont_handler_t *font)
{
	FILE *f = fopen(filename, "rb");
	if (!f) {
		return KFONT_ERROR_ERRNO;
	}

	unsigned char *buf;
	size_t size;
	if (!kfont_blob_read(f, &buf, &size)) {
		fclose(f);
		return KFONT_ERROR_READ;
	}

	fclose(f);

	enum kfont_error err = kfont_parse(buf, size, opts, font);
	if (err != KFONT_ERROR_SUCCESS) {
		xfree(buf);
		return err;
	}

	(*font)->blob = buf;

	printf("%s: %d\n", filename, size);
	return KFONT_ERROR_SUCCESS;
}

enum kfont_error kfont_parse(unsigned char *buf, size_t size, struct kfont_parse_options opts, kfont_handler_t *font)
{
	struct kfont_slice slice;
	slice.ptr = buf;
	slice.end = buf + size;

	*font = xmalloc(sizeof(struct kfont_handler));
	(*font)->blob = NULL;

	if (read_uint32_magic(&slice, PSF2_MAGIC)) {
		printf("PSF2\n");
	}

	return KFONT_ERROR_SUCCESS;
}

void kfont_free(kfont_handler_t font)
{
	if (font->blob) {
		xfree(font->blob);
		font->blob = NULL;
	}
	xfree(font);
}
