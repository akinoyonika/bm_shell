#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "bms/font.h"

static int _initialize(struct bms_font *this, char const *path, int index, int size);
static void _finalize(struct bms_font *this);
static int _get(
	struct bms_font *this,
	uint8_t const *fg_color,
	uint8_t const *bg_color,
	uint32_t code,
	uint8_t *bitmap,
	int height,
	int width
);

struct bms_font_ops const bms_font[] = {{
	_initialize,
	_finalize,
	_get
}};

#define _FT_PIXEL_B (0)
#define _FT_PIXEL_G (1)
#define _FT_PIXEL_R (2)
#define _FT_PIXEL_A (3)

static int _initialize(struct bms_font *this, char const *path, int index, int size)
{
	int ret = 0;
	int i;
	int j;
	int ix;

	if (FT_Init_FreeType(&(this->lib)) != 0) {
		ret = 1;
		goto INIT_FAILURE_EXIT;
	}

	if (FT_New_Face(this->lib, path, 0, &(this->face)) != 0) {
		ret = 1;
		goto NEW_FACE_FAILURE_EXIT;
	}

	this->size = size;
	this->selected_size = size;
	if (FT_Set_Pixel_Sizes(this->face, index, size) != 0) {
		for (i = 0, this->selected_size = 0, ix = -1 ; i < this->face->num_fixed_sizes ; i++) {
			j = this->face->available_sizes[i].height;
			if (this->selected_size < j) {
				this->selected_size = j;
				ix = i;
			}
		}
		if ((ix == -1) || (FT_Select_Size(this->face, ix) != 0)) {
			ret = 1;
			goto SET_PIXEL_SIZE_FAILURE_EXIT;
		}
	}

SET_PIXEL_SIZE_FAILURE_EXIT:
	if (0 < ret) FT_Done_Face(this->face);
NEW_FACE_FAILURE_EXIT:
	if (0 < ret) FT_Done_FreeType(this->lib);
INIT_FAILURE_EXIT:

	return ret;
}

static void _finalize(struct bms_font *this)
{
	FT_Done_Face(this->face);
	FT_Done_FreeType(this->lib);
	return;
}

static int _get(
	struct bms_font *this,
	uint8_t const *fg_color,
	uint8_t const *bg_color,
	uint32_t code,
	uint8_t *bitmap,
	int height,
	int width
)
{
	int ret = 0;
	int i;
	int ix;
	int x;
	int y;
	int rx;
	int ry;
	uint8_t *b;
	uint8_t c;
	uint8_t *bitmap_buffer;
	int do_scale(struct bms_font *this, int n)
	{
		return (n * this->selected_size * 2 / this->size + 1) / 2;
	}
	int no_scale(struct bms_font *this, int n)
	{
		(void)this;
		return n;
	}
	int (*scale)(struct bms_font *this, int n) = do_scale;
	int normal_conv_x(struct bms_font *this, int (*scale)(struct bms_font *this, int n), int width, int height, int x)
	{
		(void)width;
		(void)height;
		return scale(this, x) - (int)(this->face->glyph->bitmap_left);
	}
	int normal_conv_y(struct bms_font *this, int (*scale)(struct bms_font *this, int n), int width, int height, int y)
	{
		(void)width;
		(void)height;
		return scale(this, y) + (int)(this->face->glyph->bitmap_top) - (int)(this->face->size->metrics.ascender >> 6);
	}
	int emoji_conv_x(struct bms_font *this, int (*scale)(struct bms_font *this, int n), int width, int height, int x)
	{
		(void)width;
		(void)height;
		return scale(this, x) - (int)(this->face->glyph->bitmap_left);
	}
	int emoji_conv_y(struct bms_font *this, int (*scale)(struct bms_font *this, int n), int width, int height, int y)
	{
		(void)width;
		return scale(this, y) + (height - (int)(this->face->glyph->bitmap.rows)) / 2;
	}
	int (*conv_x)(struct bms_font *this, int (*scale)(struct bms_font *this, int n), int width, int height, int x) = normal_conv_x;
	int (*conv_y)(struct bms_font *this, int (*scale)(struct bms_font *this, int n), int width, int height, int y) = normal_conv_y;

	ix = FT_Get_Char_Index(this->face, code);

	if (FT_Load_Char(this->face, code, FT_LOAD_RENDER | FT_LOAD_COLOR) != 0) {
		ret = 1;
		goto EXIT;
	}

	if (this->selected_size == this->size) {
		scale = no_scale;
	}
	if ((this->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) || !FT_IS_SCALABLE(this->face)) {
		conv_x = emoji_conv_x;
		conv_y = emoji_conv_y;
	}

	for (y = 0, bitmap_buffer = this->face->glyph->bitmap.buffer ; y < height ; y++) {
		b = bitmap + y * width * 3;
		for (x = 0 ; x < width ; x++) {
			rx = conv_x(this, scale, width, height, x);
			ry = conv_y(this, scale, width, height, y);
			if (
				(0 <= rx) && (rx < this->face->glyph->bitmap.width) &&
				(0 <= ry) && (ry < this->face->glyph->bitmap.rows)
			) {
				if (this->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
					i = ry * this->face->glyph->bitmap.pitch + rx * 4;
					c = *b;
					*b++ =
						bitmap_buffer[i + _FT_PIXEL_R] * bitmap_buffer[i + _FT_PIXEL_A] / 255 +
						c * (255 - bitmap_buffer[i + _FT_PIXEL_A]) / 255;
					c = *b;
					*b++ =
						bitmap_buffer[i + _FT_PIXEL_G] * bitmap_buffer[i + _FT_PIXEL_A] / 255 +
						c * (255 - bitmap_buffer[i + _FT_PIXEL_A]) / 255;
					c = *b;
					*b++ =
						bitmap_buffer[i + _FT_PIXEL_B] * bitmap_buffer[i + _FT_PIXEL_A] / 255 +
						c * (255 - bitmap_buffer[i + _FT_PIXEL_A]) / 255;
				} else {
					i = ry * this->face->glyph->bitmap.pitch + rx;
					if (bitmap_buffer[i] == 0) {
						b++;
						b++;
						b++;
					} else {
						*b++ = bitmap_buffer[i] * fg_color[BMS_FONT_R] / 255;
						*b++ = bitmap_buffer[i] * fg_color[BMS_FONT_G] / 255;
						*b++ = bitmap_buffer[i] * fg_color[BMS_FONT_B] / 255;
					}
				}
			} else {
				b++;
				b++;
				b++;
			}
		}
	}

	if (ix == 0) {
		ret = 1;
		goto EXIT;
	}

EXIT:

	return ret;
}
