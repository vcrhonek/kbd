#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "bmp.h"
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

	struct bmp image;
	bmp_open(&image, argv[2]);
	bmp_write_header(&image, width * font_width, height * font_height);
	// Bitmap data
	for (unsigned long line = 0; line < height; line++)
	{
		for (row = font_height - 1; ; row--)
		{
			for (size_t i = lines[height - line - 1]; i < buflen; i++)
			{
				if (buf[i] == '\n')
				{
					break;
				}
				const unsigned char *glyph = kfont_get_char_buffer(font, buf[i]) + pitch * row;
				bmp_write_bits(&image, glyph, font_width);
			}
			bmp_end_row(&image);
			if (row == 0)
			{
				break;
			}
		}
	}
	bmp_close(&image);

	xfree(lines);
	kfont_free(font);
}
