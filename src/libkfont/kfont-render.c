#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "kfont.h"
#include "xmalloc.h"

static bool read_all(FILE *f, unsigned char **buffer, size_t *size)
{
	size_t buflen      = 1024; /* actually an arbitrary value */
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

static void fwrite_buf(FILE *f, const unsigned char *buf, size_t size)
{
	if (fwrite(buf, size, 1, f) == 0) {
		fprintf(stderr, "fwrite error\n");
		abort();
	}
}

static void fwrite_uint32(FILE *f, uint32_t v)
{
	unsigned char buf[4] = {
		v & 0xff,
		(v >> 8) & 0xff,
		(v >> 16) & 0xff,
		v >> 24,
	};
	fwrite_buf(f, buf, 4);
}

int main(int argc, char *argv[])
{
	set_progname(argv[0]);

	if (argc != 3) {
		fprintf(stderr, "usage: %s FILENAME.psf OUTPUT.bmp\n", progname);
		exit(1);
	}

	kfont_handler_t font;
	struct kfont_parse_options opts = {
		.iunit = 0,
		.find_font = NULL,
		.free = NULL,
	};
	enum kfont_error err = kfont_load(argv[1], opts, &font);
	if (err != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_load: %d\n", err);
		exit(1);
	}

	unsigned long width = 0;
	unsigned long height = 0;

	unsigned char *buf;
	size_t buflen;
	if (!read_all(stdin, &buf, &buflen)) {
		fprintf(stderr, "read_all failed\n");
		exit(1);
	}

	unsigned long row = 1;
	unsigned long col = 0;
	for (size_t i = 0; i < buflen; i++)
	{
		if (row > height) {
			height = row;
		}
		if (buf[i] == '\n') {
			row++;
			col = 0;
			continue;
		}
		col++;
		if (col > width) {
			width = col;
		}
	}

	size_t *lines = xmalloc(height * sizeof(size_t));
	lines[0] = 0;
	if (height > 1) {
		row = 0;
		for (size_t i = 0; i < buflen; i++)
		{
			if (buf[i] == '\n') {
				row++;
				lines[row] = i + 1;
				if (row == height - 1) {
					break;
				}
			}
		}
	}

	uint32_t font_width = kfont_get_width(font);
	uint32_t font_height = kfont_get_height(font);
	uint32_t pitch = (font_width - 1) / 8 + 1;
	unsigned long pwidth = width * font_width;
	unsigned long pheight = height * font_height;

	unsigned long words_per_row = (pwidth + 31) / 32;
	unsigned long bytes_per_row = words_per_row * 4;

	FILE *f = fopen(argv[2], "wb");
	if (!f) {
		fprintf(stderr, "fopen %s failed\n", argv[2]);
		exit(1);
	}

	// BMP header
	fwrite_buf(f, (const unsigned char*)"BM", 2);
	fwrite_uint32(f, 14 + 40 + 8 + pheight * bytes_per_row); // file size
	fwrite_uint32(f, 0); // unused
	fwrite_uint32(f, 62); // bitmap offset
	// DIB header
	fwrite_uint32(f, 40); // size of DIB header
	fwrite_uint32(f, pwidth); // width
	fwrite_uint32(f, pheight); // height
	uint32_t tmp = 0x0001; // number of planes, must be 1
	tmp += 0x0001 << 16; // bits per pixel
	fwrite_uint32(f, tmp);
	fwrite_uint32(f, 0); // BI_RGB, no compression
	fwrite_uint32(f, pheight * bytes_per_row); // size of raw bitmap data
	fwrite_uint32(f, 0x0B13); // 72 DPI
	fwrite_uint32(f, 0x0B13); // 72 DPI
	fwrite_uint32(f, 2); // colors in the palette
	fwrite_uint32(f, 0); // 0 means all colors are important
	// Color table
	fwrite_uint32(f, 0x00ffffff); // 0xAARRGGBB
	fwrite_uint32(f, 0x00000000);
	// Bitmap data
	for (unsigned long line = 0; line < height; line++)
	{
		for (row = font_height - 1; ; row--)
		{
			unsigned long bytes = 0;
			unsigned char b = 0;
			unsigned char bits = 8;
			for (size_t i = lines[height - line - 1]; i < buflen; i++)
			{
				if (buf[i] == '\n')
				{
					break;
				}
				const unsigned char *glyph = kfont_get_char_buffer(font, buf[i]) + pitch * row;
				unsigned char g = *glyph++;
				unsigned long chunk = font_width >= 8 ? 8 : font_width;
				unsigned long gbits = font_width;
				while (1) {
					if (chunk <= bits) {
						b = (b << chunk) + g;
						bits -= chunk;
						if (bits == 0) {
							fwrite_buf(f, &b, 1);
							bytes++;
							b = 0;
							bits = 8;
						}
						gbits -= chunk;
						if (gbits == 0) {
							break;
						}
						chunk = gbits >= 8 ? 8 : gbits;
						g = *glyph++ >> (8 - chunk);
					} else {
						b = (b << bits) + (g >> (chunk - bits));
						chunk -= bits;
						gbits -= bits;
						g = g & ((1 << chunk) - 1);
						fwrite_buf(f, &b, 1);
						bytes++;
						b = 0;
						bits = 8;
					}
				}
			}
			if (bits != 8) {
				b = b << bits;
				fwrite_buf(f, &b, 1);
				bytes++;
			}
			for (size_t i = bytes; i < bytes_per_row; i++) {
				b = 0;
				fwrite_buf(f, &b, 1);
			}
			if (row == 0)
			{
				break;
			}
		}
	}

	fclose(f);

	xfree(lines);
	kfont_free(font);
}
