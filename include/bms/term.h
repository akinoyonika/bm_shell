#ifndef __BMS_TERM_H__
#define __BMS_TERM_H__

struct screen_state {
	int rows;
	int cols;
	VTermPos cursor_pos;
	VTermPos cursor_oldpos;
	int is_cursor_visible;
	int cursor_shape;
	int num_bell;
	int is_cell_dirty[1];
};

#define SIZE_OF_STRUCT_SCREEN_STATE(rows, cols) (                          \
	offsetof(struct screen_state, is_cell_dirty) +                         \
	sizeof(*(((struct screen_state *)0)->is_cell_dirty)) * (rows) * (cols) \
)

struct bms_term {
	void (*disp_get_cursor_pos)(int *row, int *col);
	void (*disp_set_cursor_pos)(int row, int col);
	void (*disp_put_chars)(
		int fg_red, int fg_green, int fg_blue,
		int bg_red, int bg_green, int bg_blue,
		int bold, int underline, int reverse,
		int width,
		uint32_t const *chars, int max_num_chars
	);
	void (*disp_bell)(void);
	struct screen_state *screen_state;
	VTerm *vt;
	VTermScreen *vtsc;
	VTermScreenCallbacks vtsc_cs[1];
	VTermPos prev_pos[1];
	VTermScreenCell prev_cell[1];
};

struct bms_term_ops {
	int (*initialize)(
		struct bms_term *this,
		int rows, int cols,
		void (*disp_get_cursor_pos)(int *row, int *col),
		void (*disp_set_cursor_pos)(int row, int col),
		void (*disp_put_chars)(
			int fg_red, int fg_green, int fg_blue,
			int bg_red, int bg_green, int bg_blue,
			int bold, int underline, int reverse,
			int width,
			uint32_t const *chars, int max_num_chars
		),
		void (*disp_bell)(void)
	);
	void (*finalize)(struct bms_term *this);
	int (*set_size)(struct bms_term *this, int rows, int cols);
	void (*set_callbacks)(
		struct bms_term *this,
		void (*disp_get_cursor_pos)(int *row, int *col),
		void (*disp_set_cursor_pos)(int row, int col),
		void (*disp_put_chars)(
			int fg_red, int fg_green, int fg_blue,
			int bg_red, int bg_green, int bg_blue,
			int bold, int underline, int reverse,
			int width,
			uint32_t const *chars, int max_num_chars
		),
		void (*disp_bell)(void)
	);
	void (*write)(struct bms_term *this, char const *buffer, size_t buffer_size);
};

extern struct bms_term_ops const bms_term[];

#endif /* __BMS_TERM_H__ */
