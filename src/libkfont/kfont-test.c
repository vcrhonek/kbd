#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "kfont.h"

static void draw_bit(int bit)
{
	if (bit) {
		printf("\x1b[7m  \x1b[0m");
	} else {
		printf("  ");
	}
}

int main(int argc, char *argv[])
{
	set_progname(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s FILENAME.psf\n", progname);
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

	uint32_t width = kfont_get_width(font);
	uint32_t height = kfont_get_height(font);
	uint32_t char_count = kfont_get_char_count(font);

	printf("kfont data:\n");
	printf("width       : %lu\n", (unsigned long)width);
	printf("height      : %lu\n", (unsigned long)height);
	printf("char_count  : %lu\n", (unsigned long)char_count);

	int row_size = (width + 7) / 8;
	for (unsigned int font_pos = 0; font_pos < char_count; font_pos++) {
		printf("position %u:\n", font_pos);
		const unsigned char *glyph = kfont_get_char_buffer(font, font_pos);
		for (unsigned int row = 0; row < height; row++) {
			printf("|");
			for (uint32_t col = 0; col < width; col++) {
				int value = glyph[row * row_size + col / 8] & (0x80 >> (col % 8));
				draw_bit(value);
			}
			printf("|\n");
		}
	}

	// struct kfont_unicode_pair *pair = font.unicode_map_head;
	// for ( ; pair; pair = pair->next) {
	// 	for (uint32_t i = 0; i < pair->seq_length; i++) {
	// 		printf("U+%04X ", pair->seq[i]);
	// 	}
	// 	printf("-> %u\n", pair->font_pos);
	// }

	kfont_free(font);
}
