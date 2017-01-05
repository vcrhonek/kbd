#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "bmp.h"
#include "kfont.h"

static void render_row(struct bmp *image, uint32_t *data, size_t len, kfont_handler_t font)
{
	uint32_t font_width = kfont_get_width(font);
	uint32_t pitch = (font_width - 1) / 8 + 1;

	for (uint32_t row = kfont_get_height(font) - 1; ; row--) {
		for (size_t i = 0; i < len; i++) {
			bmp_write_bits(image, kfont_get_char_buffer(font, data[i]) + pitch * row, font_width);
		}
		bmp_end_row(image);
		if (row == 0) {
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	set_progname(argv[0]);

	if (argc != 3) {
		fprintf(stderr, "usage: %s FONT.psf OUTPUT.bmp\n", progname);
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

	uint32_t char_count = kfont_get_char_count(font);

	if (char_count <= 'f') {
		fprintf(stderr, "%s: %s does not contain enough glyphs to render labels (%lu glyphs)\n",
		        argv[0], argv[1], (unsigned long)char_count);
		exit(1);
	}

	unsigned long row_label_width = 0;
	for (uint32_t glyphs = char_count - 1; glyphs > 0; glyphs >>= 8) {
		row_label_width++;
	}

	unsigned long width = row_label_width + 0x10;
	unsigned long height = 1 + (char_count + 0x0f) / 0x10;

	uint32_t font_width = kfont_get_width(font);
	uint32_t font_height = kfont_get_height(font);

	uint32_t *data = alloca(width * sizeof(uint32_t));

	struct bmp image;
	bmp_open(&image, argv[2]);
	bmp_write_header(&image, width * font_width, height * font_height);
	for (unsigned long row = height - 2; ; row--) {
		for (unsigned long i = 0; i < row_label_width; i++) {
			data[i] = "0123456789abcdef"[(row >> (row_label_width - i - 1)*4) & 0xf];
		}
		for (unsigned long i = 0x00; i < 0x10; i++) {
			uint32_t r = (row << 4) + i;
			if (r >= char_count) {
				r = ' ';
			}
			data[row_label_width + i] = r;
		}
		render_row(&image, data, width, font);
		if (row == 0) {
			break;
		}
	}
	for (unsigned long i = 0; i < row_label_width; i++) {
		data[i] = ' ';
	}
	for (unsigned long i = 0x00; i < 0x10; i++) {
		data[row_label_width + i] = "0123456789abcdef"[i];
	}
	render_row(&image, data, width, font);
	bmp_close(&image);

	kfont_free(font);
}
