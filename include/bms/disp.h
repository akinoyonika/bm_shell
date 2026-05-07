#ifndef __BMS_DISP_H__
#define __BMS_DISP_H__

#define BMS_DISP_HEIGHT_PX_MIN (8)
#define BMS_DISP_HEIGHT_PX_MAX (65536)
#define BMS_DISP_WIDTH_PX_MIN (8)
#define BMS_DISP_WIDTH_PX_MAX (65536)
#define BMS_DISP_ROWS_MIN (1)
#define BMS_DISP_ROWS_MAX (2048)
#define BMS_DISP_COLS_MIN (1)
#define BMS_DISP_COLS_MAX (2048)

struct bms_disp_ops {
	int (*initialize)(int fd, char const *font, int mode, int line_height_px, int column_width_px, int rows, int cols);
	void (*finalize)(void);
	int (*set_size)(int line_height_px, int column_width_px, int rows, int cols);
	void (*get_cursor_pos)(int *row, int *col);
	void (*set_cursor_pos)(int row, int col);
	void (*put_chars)(
		int fg_red, int fg_green, int fg_blue,
		int bg_red, int bg_green, int bg_blue,
		int bold, int underline, int reverse,
		int width,
		uint32_t const *chars, int max_num_chars
	);
	void (*bell)(void);
	void (*sync)(void);
};

extern struct bms_disp_ops const *bms_disp;

#endif /* __BMS_DISP_H__ */
