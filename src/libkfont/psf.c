#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

struct kfont_psf1_header {
	uint8_t mode;      /* PSF font mode */
	uint8_t char_size; /* character size */
};

struct kfont_psf2_header {
	uint32_t version;
	uint32_t header_size; /* offset of bitmaps in file */
	uint32_t flags;
	uint32_t length;        /* number of glyphs */
	uint32_t char_size;     /* number of bytes for each character */
	uint32_t height, width; /* max dimensions of glyphs */
	                        /* charsize = height * ((width + 7) / 8) */
};

struct kfont_handler {
	uint32_t width;
	uint32_t height;
	uint32_t char_size;
	uint32_t char_count;
	const unsigned char *glyphs;

	struct kfont_unimap_node *unimap_head;
	struct kfont_unimap_node *unimap_tail;

	unsigned char *blob;
};

static bool kfont_blob_read(FILE *f, unsigned char **buffer, size_t *size)
{
	size_t buflen      = MAXFONTSIZE / 4; /* actually an arbitrary value */
	size_t n           = 0;
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
	*size   = n;

	return true;
}

enum kfont_error kfont_load(const char *filename, struct kfont_parse_options opts, kfont_handler_t *font)
{
	FILE *f = fopen(filename, "rb");
	if (!f) {
		return errno;
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

	return KFONT_ERROR_SUCCESS;
}

static bool kfont_read_psf1_header(struct kfont_slice *p, struct kfont_psf1_header *header)
{
	return read_uint8(p, &header->mode) &&
	       read_uint8(p, &header->char_size);
}

static bool kfont_read_psf2_header(struct kfont_slice *p, struct kfont_psf2_header *header)
{
	return read_uint32(p, &header->version) &&
	       read_uint32(p, &header->header_size) &&
	       read_uint32(p, &header->flags) &&
	       read_uint32(p, &header->length) &&
	       read_uint32(p, &header->char_size) &&
	       read_uint32(p, &header->height) &&
	       read_uint32(p, &header->width);
}

static bool kfont_read_psf1_unicode(struct kfont_slice *p, struct kfont_unimap_node **unimap)
{
	uint16_t uc;
	if (!read_uint16(p, &uc)) {
		return false;
	}

	if (uc == PSF1_SEPARATOR) {
		*unimap = NULL;
		return true;
	}

	if (uc == PSF1_START_SEQ) {
		fprintf(stderr, "psf1 read unicode map: <start seq>\n");
		abort();
		// TODO(dmage): handle <seq>*
	}

	*unimap = xmalloc(sizeof(struct kfont_unimap_node) + sizeof(uint32_t));

	(*unimap)->len    = 1;
	(*unimap)->seq[0] = uc;

	return true;
}

static bool kfont_read_psf2_unicode(struct kfont_slice *p, struct kfont_unimap_node **unimap)
{
	if (p->ptr + 1 > p->end) {
		return false;
	}

	if (*p->ptr == PSF2_SEPARATOR) {
		p->ptr++;
		*unimap = NULL;
		return true;
	}

	if (*p->ptr == PSF2_START_SEQ) {
		fprintf(stderr, "psf2 read unicode map: <start seq>\n");
		abort();
		// TODO(dmage): handle <seq>*
	}

	uint32_t uc;
	if (!read_utf8_code_point(p, &uc)) {
		return false;
	}

	*unimap = xmalloc(sizeof(struct kfont_unimap_node) + sizeof(uint32_t));

	(*unimap)->len    = 1;
	(*unimap)->seq[0] = uc;

	return true;
}

static enum kfont_error kfont_parse_psf_unimap(struct kfont_slice *p,
                                               kfont_handler_t font,
                                               bool (*read_unicode)(struct kfont_slice *p, struct kfont_unimap_node **unimap))
{
	for (uint32_t font_pos = 0; font_pos < font->char_count; font_pos++) {
		while (1) {
			struct kfont_unimap_node *unimap;
			if (!read_unicode(p, &unimap)) {
				return KFONT_ERROR_SHORT_UNICODE_TABLE;
			}
			if (!unimap) {
				// end of unicode map for this font position
				break;
			}

			if (unimap->len == 0) {
				// XXX: free(unimap)
				fprintf(stderr, "read unicode map %d: <empty sequence>\n", font_pos);
				abort(); // TODO(dmage)
			}
			for (unsigned int i = 0; i < unimap->len; i++) {
				if (unimap->seq[i] == INVALID_CODE_POINT) {
					// XXX: free(unimap)
					fprintf(stderr, "read unicode map %d: <invalid utf8>\n", font_pos);
					abort(); // TODO(dmage)
				}
			}

			unimap->font_pos = font_pos;

			if (font->unimap_tail) {
				font->unimap_tail->next = unimap;
			} else {
				font->unimap_head = unimap;
			}
			font->unimap_tail = unimap;
		}
	}
	return KFONT_ERROR_SUCCESS;
}

static enum kfont_error kfont_parse_legacy(struct kfont_slice *p, kfont_handler_t font)
{
	size_t len = p->ptr - p->end;
	if (len == 9780) {
		fprintf(stderr, "kfont_parse_legacy: 9780\n");
		abort(); // TODO(dmage)
	} else if (len == 32768) {
		fprintf(stderr, "kfont_parse_legacy: 32768\n");
		abort(); // TODO(dmage)
	} else if (len % 256 == 0 || len % 256 == 40) {
		if (len % 256 == 40) {
			fprintf(stderr, "kfont_parse_legacy: +40\n");
			p->ptr += 40;
		}
		fprintf(stderr, "kfont_parse_legacy: height=%lu\n", (unsigned long)((p->end - p->ptr) / 256));
		abort(); // TODO(dmage)
	}
	return KFONT_ERROR_BAD_MAGIC;
}

static enum kfont_error kfont_parse_combined(struct kfont_slice *p, kfont_handler_t font)
{
	const unsigned char magic[] = "# combine partial fonts\n";
	if (!read_buf_magic(p, magic, sizeof(magic) - 1)) {
		return KFONT_ERROR_BAD_MAGIC;
	}

	while (1) {
		const unsigned char *filename = p->ptr;
		while (p->ptr != p->end) {
			if (*p->ptr == '\0') {
				return KFONT_ERROR_TRAILING_GARBAGE; // FIXME(dmage): \0 in text file
			}
			if (*p->ptr == '\n') {
				*p->ptr = '\0';
				break;
			}
			p->ptr++;
		}
		while (p->ptr == p->end) {
			return KFONT_ERROR_TRAILING_GARBAGE; // FIXME(dmage): no \n at the end of the file
		}
		p->ptr++;

		fprintf(stderr, "TODO: load [%s]\n", filename);

		if (p->ptr == p->end) {
			break;
		}
	}

	fprintf(stderr, "kfont_parse_combined\n");
	abort(); // TODO(dmage)
}

static enum kfont_error kfont_parse_psf1(struct kfont_slice *p, kfont_handler_t font)
{
	if (!read_uint16_magic(p, PSF1_MAGIC)) {
		return KFONT_ERROR_BAD_MAGIC;
	}

	struct kfont_psf1_header psf1_header;
	if (!kfont_read_psf1_header(p, &psf1_header)) {
		return KFONT_ERROR_BAD_PSF1_HEADER;
	}

	if (psf1_header.mode > PSF1_MAXMODE) {
		return KFONT_ERROR_UNSUPPORTED_PSF1_MODE;
	}

	/* TODO: check char_size */
	/* TODO: check char_count */

	font->width      = 8;
	font->height     = psf1_header.char_size;
	font->char_size  = psf1_header.char_size;
	font->char_count = (psf1_header.mode & PSF1_MODE512 ? 512 : 256);
	font->glyphs     = p->ptr;

	if (psf1_header.mode & (PSF1_MODE_HAS_TAB | PSF1_MODE_HAS_SEQ)) {
		p->ptr += font->char_size * font->char_count;

		enum kfont_error err = kfont_parse_psf_unimap(p, font, kfont_read_psf1_unicode);
		if (err != KFONT_ERROR_SUCCESS) {
			return err;
		}

		if (p->ptr != p->end) {
			return KFONT_ERROR_TRAILING_GARBAGE;
		}
	}

	return KFONT_ERROR_SUCCESS;
}

static enum kfont_error kfont_parse_psf2(struct kfont_slice *p, kfont_handler_t font)
{
	unsigned char *begin = p->ptr;

	if (!read_uint32_magic(p, PSF2_MAGIC)) {
		return KFONT_ERROR_BAD_MAGIC;
	}

	struct kfont_psf2_header psf2_header;
	if (!kfont_read_psf2_header(p, &psf2_header)) {
		return KFONT_ERROR_BAD_PSF2_HEADER;
	}

	if (psf2_header.version > PSF2_MAXVERSION) {
		return KFONT_ERROR_UNSUPPORTED_PSF2_VERSION;
	}

	/* check that the buffer is large enough for the header and glyphs */
	size_t size = p->end - begin;
	if (psf2_header.header_size > size) {
		return KFONT_ERROR_FONT_OFFSET_TOO_BIG;
	}
	if (psf2_header.char_size == 0) {
		return KFONT_ERROR_CHAR_SIZE_ZERO;
	}
	if (psf2_header.char_size > size - psf2_header.header_size) {
		return KFONT_ERROR_CHAR_SIZE_TOO_BIG;
	}
	if (psf2_header.length > (size - psf2_header.header_size) / psf2_header.char_size) {
		return KFONT_ERROR_FONT_LENGTH_TOO_BIG;
	}

	/* check that width, height and char_size are consistent */
	if (psf2_header.width == 0) {
		// FIXME(dmage): another error code
		return KFONT_ERROR_CHAR_SIZE_ZERO;
	}
	uint32_t pitch = (psf2_header.width - 1) / 8 + 1;
	if (psf2_header.height != psf2_header.char_size / pitch) {
		// FIXME(dmage): another error code
		return KFONT_ERROR_CHAR_SIZE_TOO_BIG;
	}

	font->width      = psf2_header.width;
	font->height     = psf2_header.height;
	font->char_size  = psf2_header.char_size;
	font->char_count = psf2_header.length;
	font->glyphs     = begin + psf2_header.header_size;

	if (psf2_header.flags & PSF2_HAS_UNICODE_TABLE) {
		p->ptr = begin + psf2_header.header_size + font->char_size * font->char_count;

		enum kfont_error err = kfont_parse_psf_unimap(p, font, kfont_read_psf2_unicode);
		if (err != KFONT_ERROR_SUCCESS) {
			return err;
		}

		if (p->ptr != p->end) {
			return KFONT_ERROR_TRAILING_GARBAGE;
		}
	}

	return KFONT_ERROR_SUCCESS;
}

enum kfont_error kfont_parse(unsigned char *buf, size_t size, struct kfont_parse_options opts, kfont_handler_t *font)
{
	struct kfont_slice p;
	p.ptr = buf;
	p.end = buf + size;

	*font = xmalloc(sizeof(struct kfont_handler));

	(*font)->unimap_head = NULL;
	(*font)->unimap_tail = NULL;
	(*font)->blob        = NULL;

	enum kfont_error err = kfont_parse_psf2(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

	err = kfont_parse_psf1(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

	err = kfont_parse_combined(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

	err = kfont_parse_legacy(&p, *font);
	if (err != KFONT_ERROR_BAD_MAGIC) {
		goto ret;
	}

ret:
	if (err != KFONT_ERROR_SUCCESS) {
		kfont_free(*font);
	}
	return err;
}

enum kfont_error kfont_append(kfont_handler_t font, kfont_handler_t other)
{
	if (font->width != other->width || font->height != other->height || font->char_size != other->char_size) {
		return KFONT_ERROR_FONT_METRICS_MISMATCH;
	}

	/* TODO(dmage): check overflows */
	uint32_t char_count  = font->char_count + other->char_count;
	unsigned char *glyphs = xmalloc(font->char_size * char_count);

	memmove(glyphs,
	        font->glyphs,
	        font->char_size * font->char_count);
	memmove(glyphs + font->char_size * font->char_count,
	        other->glyphs,
	        font->char_size * other->char_count);

	font->glyphs = glyphs;
	xfree(font->blob);
	font->blob = glyphs;

	if (font->unimap_tail) {
		font->unimap_tail->next = other->unimap_head;
		font->unimap_tail = other->unimap_tail;
	} else {
		font->unimap_head = other->unimap_head;
		font->unimap_tail = other->unimap_tail;
	}
	other->unimap_head = NULL;
	other->unimap_tail = NULL;

	kfont_free(other);

	return KFONT_ERROR_SUCCESS;
}

void kfont_free(kfont_handler_t font)
{
	struct kfont_unimap_node *unimap = font->unimap_head;
	while (unimap) {
		struct kfont_unimap_node *next = unimap->next;
		xfree(unimap);
		unimap = next;
	}
	font->unimap_head = NULL;
	font->unimap_tail = NULL;

	if (font->blob) {
		xfree(font->blob);
		font->blob = NULL;
	}

	xfree(font);
}

uint32_t kfont_get_width(kfont_handler_t font)
{
	return font->width;
}

uint32_t kfont_get_height(kfont_handler_t font)
{
	return font->height;
}

uint32_t kfont_get_char_count(kfont_handler_t font)
{
	return font->char_count;
}

const unsigned char *kfont_get_char_buffer(kfont_handler_t font, uint32_t font_pos)
{
	if (font_pos >= font->char_count) {
		return NULL;
	}
	return font->glyphs + font_pos * font->char_size;
}

struct kfont_unimap_node *kfont_get_unicode_map(kfont_handler_t font)
{
	return font->unimap_head;
}
