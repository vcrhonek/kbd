/**
 * @file kfont.h
 * @brief This file should be considered as libkfont's public API.
 */
#ifndef KFONT_H
#define KFONT_H

/**
 * @brief Error codes.
 */
enum kfont_error {
	KFONT_ERROR_SUCCESS                  = 0,
	KFONT_ERROR_BAD_MAGIC                = -1,
	KFONT_ERROR_BAD_PSF1_HEADER          = -2,
	KFONT_ERROR_UNSUPPORTED_PSF1_MODE    = -3,
	KFONT_ERROR_BAD_PSF2_HEADER          = -4,
	KFONT_ERROR_UNSUPPORTED_PSF2_VERSION = -5,
	KFONT_ERROR_TRAILING_GARBAGE         = -6,
	KFONT_ERROR_SHORT_UNICODE_TABLE      = -7,
	KFONT_ERROR_FONT_OFFSET_TOO_BIG      = -8,
	KFONT_ERROR_CHAR_SIZE_ZERO           = -9,
	KFONT_ERROR_CHAR_SIZE_TOO_BIG        = -10,
	KFONT_ERROR_FONT_LENGTH_TOO_BIG      = -11,
};

/**
 * @brief TODO kfont_parse_options
 */
struct kfont_parse_options {
	uint8_t codepage; /* iunit; 0 - reject, 8, 14, 16 */
	int (*findfile)(const char *fnam, char **fp);
	void (*free_findfile)(char *fp);
};

/**
 * @brief Opaque font descriptor.
 */
typedef struct kfont_handler *kfont_handler_t;

/**
 * @brief Loads a font from the given file.
 * If the font is loaded, it should be freed using @ref kfont_free.
 */
extern enum kfont_error kfont_load(const char *filename, struct kfont_parse_options opts, kfont_handler_t *font);

/**
 * @brief TODO kfont_parse
 */
extern enum kfont_error kfont_parse(unsigned char *buf, size_t size, struct kfont_parse_options opts, kfont_handler_t *font);

/**
 * @brief Combines two fonts. Resources associated with the font @p other would be released.
 */
extern enum kfont_error kfont_append(kfont_handler_t *font, kfont_handler_t *other);

/**
 * @brief Releases resources associated with the font.
 */
extern void kfont_free(kfont_handler_t *font);

/**
 * @brief Returns the characters' width.
 */
extern unsigned int kfont_get_width(kfont_handler_t *font);

/**
 * @brief Returns the characters' height.
 */
extern unsigned int kfont_get_height(kfont_handler_t *font);

/**
 * @brief Returns the number of characters in the font.
 */
extern unsigned int kfont_get_char_count(kfont_handler_t *font);

/**
 * @brief Returns a buffer with a symbol representation.
 *
 * @code
 *  01234v67
 * 0-||||---
 * >||--|x--
 * 2----||--
 * 3---||---
 * 4--||----
 * 5--------
 * 6--||----
 * 7--------
 *
 * row = 1
 * col = 5
 *
 * pitch = ceil(width/8)
 * byte_idx = row*pitch + floor(col/8)
 * bit_idx = col%8
 *
 * x = BIT_VALUE(buf, col, row) = (buf[byte_idx] & (0x80 >> bit_idx))
 * @endcode
 */
extern unsigned char *kfont_get_char_buffer(kfont_handler_t *font, unsigned int index);

/**
 * @brief TODO kfont_unicode_pair
 */
struct kfont_unicode_pair {
	struct kfont_unicode_pair *next;
	unsigned int font_pos;
	uint32_t seq_length;
	uint32_t seq[1];
};

/**
 * @brief TODO kfont_get_unicode_map
 */
extern struct kfont_unicode_pair *kfont_get_unicode_map(kfont_handler_t *font);

#endif /* KFONT_H */
