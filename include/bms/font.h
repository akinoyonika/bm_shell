#ifndef __BMS_FONT_H__
#define __BMS_FONT_H__

#define BMS_FONT_R (0)
#define BMS_FONT_G (1)
#define BMS_FONT_B (2)

struct bms_font {
	FT_Library lib;
	FT_Face face;
	int size;
	int selected_size;
};

struct bms_font_ops {
	int (*initialize)(struct bms_font *this, char const *path, int index, int size);
	void (*finalize)(struct bms_font *this);
	int (*get)(
		struct bms_font *this,
		uint8_t const *fg_color,
		uint8_t const *bg_color,
		uint32_t code,
		uint8_t *bitmap,
		int height,
		int width
	);
};

extern struct bms_font_ops const bms_font[];

#endif /* __BMS_FONT_H__ */
