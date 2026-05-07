#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <vterm.h>
#include "bms/term.h"

static int _initialize(
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
static void _finalize(struct bms_term *this);
static int _set_size(struct bms_term *this, int rows, int cols);
static void _set_callbacks(
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
static void _write(struct bms_term *this, char const *buffer, size_t buffer_size);

struct bms_term_ops const bms_term[] = {{
	_initialize,
	_finalize,
	_set_size,
	_set_callbacks,
	_write
}};

static int _compo_rgb(int red, int green, int blue)
{
	return (red << 16) | (green << 8) | blue;
}

static int _indexed_to_rgb(int idx)
{
	int ret;
	int const code[] = {0, 95, 135, 175, 215, 255};

	switch (idx) {
	case 0:
		ret = 0x000000;
		break;
	case 1:
		ret = 0x800000;
		break;
	case 2:
		ret = 0x008000;
		break;
	case 3:
		ret = 0x808000;
		break;
	case 4:
		ret = 0x000080;
		break;
	case 5:
		ret = 0x800080;
		break;
	case 6:
		ret = 0x008080;
		break;
	case 7:
		ret = 0xc0c0c0;
		break;
	case 8:
		ret = 0x808080;
		break;
	case 9:
		ret = 0xff0000;
		break;
	case 10:
		ret = 0x00ff00;
		break;
	case 11:
		ret = 0xffff00;
		break;
	case 12:
		ret = 0x0000ff;
		break;
	case 13:
		ret = 0xff00ff;
		break;
	case 14:
		ret = 0x00ffff;
		break;
	case 15:
		ret = 0xffffff;
		break;
	case 232:
		ret = 0x080808;
		break;
	case 233:
		ret = 0x121212;
		break;
	case 234:
		ret = 0x1c1c1c;
		break;
	case 235:
		ret = 0x262626;
		break;
	case 236:
		ret = 0x303030;
		break;
	case 237:
		ret = 0x3a3a3a;
		break;
	case 238:
		ret = 0x444444;
		break;
	case 239:
		ret = 0x4e4e4e;
		break;
	case 240:
		ret = 0x585858;
		break;
	case 241:
		ret = 0x626262;
		break;
	case 242:
		ret = 0x6c6c6c;
		break;
	case 243:
		ret = 0x767676;
		break;
	case 244:
		ret = 0x808080;
		break;
	case 245:
		ret = 0x8a8a8a;
		break;
	case 246:
		ret = 0x949494;
		break;
	case 247:
		ret = 0x9e9e9e;
		break;
	case 248:
		ret = 0xa8a8a8;
		break;
	case 249:
		ret = 0xb2b2b2;
		break;
	case 250:
		ret = 0xbcbcbc;
		break;
	case 251:
		ret = 0xc6c6c6;
		break;
	case 252:
		ret = 0xd0d0d0;
		break;
	case 253:
		ret = 0xdadada;
		break;
	case 254:
		ret = 0xe4e4e4;
		break;
	case 255:
		ret = 0xeeeeee;
		break;
	default:
		ret = idx - 16;
		ret = (code[ret / 36] << 16) | (code[(ret % 36) / 6] << 8) | code[ret % 6];
		break;
	}

	return ret;
}

static int _vterm_color_to_rgb(VTermColor const *vtc)
{
	int ret;

	if        (VTERM_COLOR_IS_RGB(vtc))     {
		ret = _compo_rgb((int)(vtc->rgb.red), (int)(vtc->rgb.green), (int)(vtc->rgb.blue));
	} else if (VTERM_COLOR_IS_INDEXED(vtc)) {
		ret = _indexed_to_rgb(vtc->indexed.idx);
	} else                                  {
		ret = -1;
	}

	return ret;
}

static void _set_dirty(VTermRect const *rect, struct screen_state *screen_state)
{
	int i;
	int j;

	if (
		(0 <= rect->start_row) && (rect->start_row < screen_state->rows) &&
		(0 < rect->end_row) && (rect->end_row <= screen_state->rows) &&
		(0 <= rect->start_col) && (rect->start_col < screen_state->cols) &&
		(0 < rect->end_col) && (rect->end_col <= screen_state->cols)
	) {
		for (i = rect->start_row ; i < rect->end_row ; i++) {
			for (j = rect->start_col ; j < rect->end_col ; j++) {
				screen_state->is_cell_dirty[i * screen_state->cols + j] = 1;
			}
		}
	} else {
		for (i = 0 ; i < screen_state->rows ; i++) {
			for (j = 0 ; j < screen_state->cols ; j++) {
				screen_state->is_cell_dirty[i * screen_state->cols + j] = 1;
			}
		}
	}
}

static int _vterm_screen_damage(VTermRect rect, void *param)
{
	struct screen_state **screen_state = (struct screen_state **)param;

	_set_dirty(&rect, *screen_state);

	return 1;
}

static int _vterm_screen_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *param)
{
	struct screen_state **screen_state = (struct screen_state **)param;
	VTermRect rect[1];

	(*screen_state)->cursor_pos = pos;
	(*screen_state)->cursor_oldpos = oldpos;
	(*screen_state)->is_cursor_visible = visible;
	rect->start_row = pos.row;
	rect->end_row = rect->start_row + 1;
	rect->start_col = pos.col;
	rect->end_col = rect->start_col + 1;
	_set_dirty(rect, *screen_state);

	rect->start_row = oldpos.row;
	rect->end_row = rect->start_row + 1;
	rect->start_col = oldpos.col;
	rect->end_col = rect->start_col + 1;
	_set_dirty(rect, *screen_state);

	return 1;
}

static int _vterm_screen_settermprop(VTermProp prop, VTermValue *val, void *param)
{
	struct screen_state **screen_state = (struct screen_state **)param;

	if        (prop == VTERM_PROP_CURSORSHAPE)   {
		(*screen_state)->cursor_shape = val->number;
	} else if (prop == VTERM_PROP_CURSORVISIBLE) {
		(*screen_state)->is_cursor_visible = val->boolean;
	}

	return 1;
}

static int _vterm_screen_resize(int rows, int cols, void *param)
{
	struct screen_state **screen_state = (struct screen_state **)param;
	int ret = 0;
	struct screen_state *new_screen_state;
	VTermRect rect[1];

	new_screen_state = (struct screen_state *)realloc(
		*screen_state,
		SIZE_OF_STRUCT_SCREEN_STATE(rows, cols)
	);

	if (new_screen_state != NULL) {
		new_screen_state->rows = rows;
		new_screen_state->cols = cols;
		rect->start_row = 0;
		rect->end_row = rows;
		rect->start_col = 0;
		rect->end_col = cols;
		_set_dirty(rect, new_screen_state);
		*screen_state = new_screen_state;
		ret = 1;
	}

	return ret;
}

static int _vterm_screen_bell(void *param)
{
	struct screen_state **screen_state = (struct screen_state **)param;
	
	((*screen_state)->num_bell)++;

	return 1;
}

static void _disp_put_cell(struct bms_term *this, VTermPos const *pos, VTermScreenCell const *cell)
{
	VTermScreenCell sc[1];
	int rgb;
	VTermPos p[1];

	if ((pos == NULL) || (cell == NULL)) {
		this->prev_pos->row = -1;
		this->prev_pos->col = -1;
		this->prev_cell->width = 0;
		this->prev_cell->fg.type = VTERM_COLOR_RGB;
		this->prev_cell->fg.rgb.red = -1;
		this->prev_cell->fg.rgb.green = -1;
		this->prev_cell->fg.rgb.blue = -1;
		this->prev_cell->bg.type = VTERM_COLOR_RGB;
		this->prev_cell->bg.rgb.red = -1;
		this->prev_cell->bg.rgb.green = -1;
		this->prev_cell->bg.rgb.blue = -1;
		goto EXIT;
	}

	this->disp_get_cursor_pos(&(p->col), &(p->row));

	if ((p->row != pos->row) || (p->col != pos->col)) {
		this->disp_set_cursor_pos(pos->row, pos->col);
	}

	memcpy(sc, cell, sizeof(*sc));
	if (!VTERM_COLOR_IS_RGB(&(sc->fg))) {
		rgb = _vterm_color_to_rgb(&(sc->fg));
		vterm_color_rgb(&(sc->fg), (rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	if (!VTERM_COLOR_IS_RGB(&(sc->bg))) {
		rgb = _vterm_color_to_rgb(&(sc->bg));
		vterm_color_rgb(&(sc->bg), (rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
	}

	if (0x80000000 <= sc->chars[0]) {
		this->disp_set_cursor_pos(pos->row, pos->col + sc->width);
	} else {
		this->disp_put_chars(
			sc->fg.rgb.red, sc->fg.rgb.green, sc->fg.rgb.blue,
			sc->bg.rgb.red, sc->bg.rgb.green, sc->bg.rgb.blue,
			sc->attrs.bold, sc->attrs.underline, sc->attrs.reverse,
			sc->width,
			sc->chars, VTERM_MAX_CHARS_PER_CELL
		);
	}

	memcpy(this->prev_pos, pos, sizeof(*this->prev_pos));
	memcpy(this->prev_cell, sc, sizeof(*this->prev_cell));

EXIT:

	return;
}

static void _disp(struct bms_term *this)
{
	VTermPos pos[1];
	VTermScreenCell cell[1];

	vterm_screen_flush_damage(this->vtsc);
	_disp_put_cell(this, NULL, NULL);
	for (pos->row = 0 ; pos->row < this->screen_state->rows ; pos->row++) {
		for (pos->col = 0 ; pos->col < this->screen_state->cols ; pos->col++) {
			if (!this->screen_state->is_cell_dirty[pos->row * this->screen_state->cols + pos->col]) continue;
			memset(cell, 0, sizeof(*cell));
			if (vterm_screen_get_cell(this->vtsc, *pos, cell) == 0) continue;
			if (cell->width == 0) continue;
			if (
				(this->screen_state->cursor_pos.col == pos->col) &&
				(this->screen_state->cursor_pos.row == pos->row) &&
				this->screen_state->is_cursor_visible
			) {
				switch (this->screen_state->cursor_shape) {
				//case VTERM_PROP_CURSORSHAPE_BLOCK:
				//	cell->attrs.reverse = !cell->attrs.reverse;
				//	break;
				case VTERM_PROP_CURSORSHAPE_UNDERLINE:
					cell->attrs.underline = !cell->attrs.underline;
					break;
				//case VTERM_PROP_CURSORSHAPE_BAR_LEFT:
				//	cell->attrs.reverse = !cell->attrs.reverse;
				//	break;
				default:
					cell->attrs.reverse = !cell->attrs.reverse;
					break;
				}
			}
			_disp_put_cell(this, pos, cell);
			this->screen_state->is_cell_dirty[pos->row * this->screen_state->cols + pos->col] = 0;
		}
	}
	for ( ; 0 < this->screen_state->num_bell ; (this->screen_state->num_bell)--) this->disp_bell();
}

static int _initialize(
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
)
{
	int ret = 0;

	if (
		(disp_get_cursor_pos == NULL) ||
		(disp_set_cursor_pos == NULL) ||
		(disp_put_chars      == NULL) ||
		(disp_bell           == NULL)
	) {
		ret = 1;
		goto NULL_CALLBACK_EXIT;
	}
	this->disp_get_cursor_pos = disp_get_cursor_pos;
	this->disp_set_cursor_pos = disp_set_cursor_pos;
	this->disp_put_chars = disp_put_chars;
	this->disp_bell = disp_bell;

	this->screen_state = (struct screen_state *)malloc(SIZE_OF_STRUCT_SCREEN_STATE(rows, cols));
	if (this->screen_state == NULL) {
		ret = 1;
		goto SCREEN_STATE_MALLOC_FAILURE_EXIT;
	}
	memset(this->screen_state, 0, SIZE_OF_STRUCT_SCREEN_STATE(rows, cols));
	this->screen_state->rows = rows;
	this->screen_state->cols = cols;

	this->vt = vterm_new(rows, cols);
	if (this->vt == NULL) {
		ret = 1;
		goto VTERM_NEW_FAILURE_EXIT;
	}

	vterm_set_utf8(this->vt, 1);

	this->vtsc = vterm_obtain_screen(this->vt);
	if (this->vtsc == NULL) {
		ret = 1;
		goto VTERM_OBTAIN_SCREEN_FAILURE_EXIT;
	}

	vterm_screen_set_damage_merge(this->vtsc, VTERM_DAMAGE_CELL);
	memset(this->vtsc_cs, 0, sizeof(*(this->vtsc_cs)));
	this->vtsc_cs->damage = _vterm_screen_damage;
	this->vtsc_cs->movecursor = _vterm_screen_movecursor;
	this->vtsc_cs->settermprop = _vterm_screen_settermprop;
	this->vtsc_cs->resize = _vterm_screen_resize;
	this->vtsc_cs->bell = _vterm_screen_bell;
	vterm_screen_set_callbacks(this->vtsc, this->vtsc_cs, &(this->screen_state));

	vterm_screen_reset(this->vtsc, 1);

VTERM_OBTAIN_SCREEN_FAILURE_EXIT:
	if (0 != ret) vterm_free(this->vt);
VTERM_NEW_FAILURE_EXIT:
	if (0 != ret) free(this->screen_state);
SCREEN_STATE_MALLOC_FAILURE_EXIT:
NULL_CALLBACK_EXIT:

	return ret;
}

static void _finalize(struct bms_term *this)
{
	vterm_free(this->vt);
	free(this->screen_state);
}

static int _set_size(struct bms_term *this, int rows, int cols)
{
	vterm_set_size(this->vt, rows, cols);
	return 0;
}

static void _set_callbacks(
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
)
{
	if (disp_get_cursor_pos != NULL) this->disp_get_cursor_pos = disp_get_cursor_pos;
	if (disp_set_cursor_pos != NULL) this->disp_set_cursor_pos = disp_set_cursor_pos;
	if (disp_put_chars      != NULL) this->disp_put_chars = disp_put_chars;
	if (disp_bell           != NULL) this->disp_bell = disp_bell;
}

static void _write(struct bms_term *this, char const *buffer, size_t buffer_size)
{
	size_t st;

	for (st = 0 ; st < buffer_size ; st += vterm_input_write(this->vt, buffer + st, buffer_size - st));
	if (0 < st) _disp(this);
}
