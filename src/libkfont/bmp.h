#ifndef BMP_H
#define BMP_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct bmp {
	FILE *f;
	uint32_t width, height;

	unsigned char b;
	unsigned int bavail;
	uint32_t bytes;
};

static void bmp_write_buf(FILE *f, const unsigned char *buf, size_t size)
{
	if (fwrite(buf, size, 1, f) == 0) {
		fprintf(stderr, "bmp_write_buf error\n");
		abort();
	}
}

static void bmp_write_uint32(FILE *f, uint32_t v)
{
	unsigned char buf[4] = {
		v & 0xff,
		(v >> 8) & 0xff,
		(v >> 16) & 0xff,
		v >> 24,
	};
	bmp_write_buf(f, buf, 4);
}

static void bmp_b_write(struct bmp *image, unsigned char c, unsigned int bits)
{
	assert(image->bavail >= bits);
	assert((c & ~((1 << bits) - 1)) == 0);

	image->b = (image->b << bits) + c;
	image->bavail -= bits;
	if (image->bavail == 0) {
		bmp_write_buf(image->f, &image->b, 1);
		image->b = 0;
		image->bavail = 8;
		image->bytes++;
	}
}

static void bmp_open(struct bmp *image, const char *filename)
{
	image->f = fopen(filename, "wb");
	if (!image->f) {
		fprintf(stderr, "bmp_open: fopen %s failed\n", filename);
		exit(1);
	}
}

static void bmp_write_header(struct bmp *image, uint32_t width, uint32_t height)
{
	image->width = width;
	image->height = height;
	image->b = 0;
	image->bavail = 8;
	image->bytes = 0;

	const uint32_t bmp_header_size = 14;
	const uint32_t dib_header_size = 40;
	const uint32_t color_table_size = 8;

	uint32_t bytes_per_row = (width + 31) / 32 * 4;

	uint32_t bitmap_offset = bmp_header_size + dib_header_size + color_table_size;
	uint32_t bitmap_size = height * bytes_per_row;
	uint32_t file_size = bitmap_offset + bitmap_size;

	// BMP header
	bmp_write_buf(image->f, (const unsigned char*)"BM", 2);
	bmp_write_uint32(image->f, file_size);
	bmp_write_uint32(image->f, 0); // unused
	bmp_write_uint32(image->f, bitmap_offset);

	// DIB header
	bmp_write_uint32(image->f, dib_header_size);
	bmp_write_uint32(image->f, width);
	bmp_write_uint32(image->f, height);
	uint32_t tmp = 0x0001; // number of planes, must be 1
	tmp += 0x0001 << 16; // bits per pixel
	bmp_write_uint32(image->f, tmp);
	bmp_write_uint32(image->f, 0); // BI_RGB, no compression
	bmp_write_uint32(image->f, bitmap_size);
	bmp_write_uint32(image->f, 0x0B13); // 72 DPI
	bmp_write_uint32(image->f, 0x0B13); // 72 DPI
	bmp_write_uint32(image->f, 2); // colors in the palette
	bmp_write_uint32(image->f, 0); // 0 means all colors are important

	// Color table
	bmp_write_uint32(image->f, 0x00ffffff); // 0xAARRGGBB
	bmp_write_uint32(image->f, 0x00000000);
}

static void bmp_write_bits(struct bmp *image, const unsigned char *buf, unsigned int bits)
{
	unsigned int chunk = bits < 8 ? bits : 8;
	unsigned char c = *buf++ >> (8 - chunk);
	while (1) {
		if (chunk <= image->bavail) {
			bmp_b_write(image, c, chunk);
			bits -= chunk;
			if (bits == 0) {
				break;
			}
			chunk = bits < 8 ? bits : 8;
			c = *buf++ >> (8 - chunk);
		} else {
			unsigned int bavail = image->bavail;
			bmp_b_write(image, c >> (chunk - bavail), bavail);
			bits -= bavail;
			chunk -= bavail;
			c = c & ((1 << chunk) - 1);
		}
	}
}

static void bmp_end_row(struct bmp *image)
{
	if (image->bavail != 8) {
		bmp_b_write(image, 0, image->bavail);
	}

	uint32_t p = (image->width + 31) / 32 * 4;
	unsigned char zero = 0;
	for (uint32_t i = image->bytes; i < p; i++) {
		bmp_write_buf(image->f, &zero, 1);
	}
	image->bytes = 0;
}

static void bmp_close(struct bmp *image)
{
	fclose(image->f);
}

#endif
