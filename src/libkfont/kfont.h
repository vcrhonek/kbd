/**
 * @file kfont.h
 * @brief This file is a part of libkfont's public API.
 */
#ifndef KFONT_H
#define KFONT_H

/**
 * @brief Error codes.
 */
enum kfont_error {
	KFONT_ERROR_SUCCESS                  = 0,
	KFONT_ERROR_READ                     = -2,
	KFONT_ERROR_BAD_MAGIC                = -3,
	KFONT_ERROR_BAD_PSF1_HEADER          = -4,
	KFONT_ERROR_UNSUPPORTED_PSF1_MODE    = -5,
	KFONT_ERROR_BAD_PSF2_HEADER          = -6,
	KFONT_ERROR_UNSUPPORTED_PSF2_VERSION = -7,
	KFONT_ERROR_TRAILING_GARBAGE         = -8,
	KFONT_ERROR_SHORT_UNICODE_TABLE      = -9,
	KFONT_ERROR_FONT_OFFSET_TOO_BIG      = -10,
	KFONT_ERROR_CHAR_SIZE_ZERO           = -11,
	KFONT_ERROR_CHAR_SIZE_TOO_BIG        = -12,
	KFONT_ERROR_FONT_LENGTH_TOO_BIG      = -13,
	KFONT_ERROR_FONT_METRICS_MISMATCH    = -14,
};

/**
 * @brief Returns the error message string corresponding to an error code.
 */
const char *kfont_strerror(enum kfont_error err);

/**
 * @brief Configuration options for the font parser.
 */
struct kfont_parse_options {
	/**
	 * A desired font height for files with several point sizes.
	 * Value 0 means to reject such fonts.
	 */
	uint8_t iunit;

	/**
	 * Finds a path to a font specified in a combined PSF font file.
	 * Use NULL to disable combined fonts support.
	 */
	bool (*find_font)(const char *name, const char **filename);

	/**
	 * Releases buffer allocated by \ref find_font.
	 */
	void (*free)(const char *filename);
};

/**
 * @brief An opaque font descriptor.
 */
typedef struct kfont_handler *kfont_handler_t;

/**
 * @brief Loads a font from the given file.
 * If the font is loaded, it should be freed using @ref kfont_free.
 */
enum kfont_error kfont_load(const char *filename, struct kfont_parse_options opts, kfont_handler_t *font);

/**
 * @brief Loads a font from the given buffer.
 * If the font is loaded, it should be freed using @ref kfont_free.
 * The buffer itself should not be freed before freeing the font.
 */
enum kfont_error kfont_parse(unsigned char *buf, size_t size, struct kfont_parse_options opts, kfont_handler_t *font);

/**
 * @brief Combines two fonts. Resources associated with the font @p other would be released.
 */
enum kfont_error kfont_append(kfont_handler_t font, kfont_handler_t other);

/**
 * @brief Releases resources associated with the font.
 */
void kfont_free(kfont_handler_t font);

/**
 * @brief Returns the characters' width.
 */
uint32_t kfont_get_width(kfont_handler_t font);

/**
 * @brief Returns the characters' height.
 */
uint32_t kfont_get_height(kfont_handler_t font);

/**
 * @brief Returns the number of characters in the font.
 */
uint32_t kfont_get_char_count(kfont_handler_t font);

/**
 * @brief Returns a buffer with a symbol representation. It should not be freed.
 *
 * @code
 *  01234v67
 * 0-####---
 * >##--#x--
 * 2----##--
 * 3---##---
 * 4--##----
 * 5--------
 * 6--##----
 * 7--------
 *
 * row = 1
 * col = 5
 *
 * pitch = ceil(width/8)
 * byte_idx = row*pitch + floor(col/8)
 * bit_idx = col%8
 *
 * x = (buf[byte_idx] & (0x80 >> bit_idx))
 * @endcode
 */
const unsigned char *kfont_get_char_buffer(kfont_handler_t font, uint32_t font_pos);

/**
 * @brief A linked list of pairs (font_pos, seq[len]).
 */
struct kfont_unimap_node {
	/**
	 * A next element of the linked list or NULL.
	 */
	struct kfont_unimap_node *next;

	/**
	 * A font position.
	 */
	uint32_t font_pos;

	/**
	 * A length of the @ref seq.
	 */
	unsigned int len;

	/**
	 * A sequence of the Unicode characters represented by the font position @ref font_pos.
	 */
	uint32_t seq[];
};

/**
 * @brief Returns a Unicode description of the glyphs.
 * The result is associated with the font and should not be freed.
 */
struct kfont_unimap_node *kfont_get_unicode_map(kfont_handler_t font);

/**
 * @brief Loads a unicode map from the given file.
 * If the unicode map is loaded, it should be freed using @ref kfont_free_unimap.
 */
enum kfont_error kfont_load_unimap(const char *filename, struct kfont_unimap_node **unimap);

/**
 * @brief Releases resources associated with the unicode map.
 */
void kfont_free_unimap(struct kfont_unimap_node *unimap);

#endif /* KFONT_H */
