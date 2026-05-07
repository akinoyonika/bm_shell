#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "bms/disp_xterm.h"

static int _initialize(int line_height_px, int column_width_px, int rows, int cols);
static void _finalize(void);
static int _set_size(int line_height_px, int column_width_px, int rows, int cols);
static void _get_cursor_pos(int *row, int *col);
static void _set_cursor_pos(int row, int col);
static void _set_fg_color(int red, int green, int blue);
static void _set_bg_color(int red, int green, int blue);
static void _put_chars(
	int fg_red, int fg_green, int fg_blue,
	int bg_red, int bg_green, int bg_blue,
	int bold, int underline, int reverse,
	int width,
	uint32_t const *chars, int max_num_chars
);
static void _bell(void);
static void _sync(void);

struct bms_disp_xterm_ops const bms_disp_xterm[] = {{
	_initialize,
	_finalize,
	_set_size,
	_get_cursor_pos,
	_set_cursor_pos,
	_put_chars,
	_bell,
	_sync
}};

#define _IS_ESC_SEQ_COLOR_INDEXED (0)

static int _rows;
static int _cols;
static int _row;
static int _col;

#if _IS_ESC_SEQ_COLOR_INDEXED
	static int _rgb_to_indexed(int red, int green, int blue)
	{
		int ret;
		int index(int color)
		{
			int const c[] = {0, 95, 135, 175, 215, 255};
			int i;
			int d;
			int dmin;
			for (i = 0, dmin = 256 ; i < (sizeof(c) / sizeof(c[0])) ; i++) {
				d = color - c[i];
				d = (0 < d) ? d : -d;
				if (d < dmin) {
					dmin = d;
				} else {
					break;
				}
			}
			return i - 1;
		}

		switch ((red << 16) | (green << 8) | blue) {
		case 0x000000:
			ret = 0;
			break;
		case 0x800000:
			ret = 1;
			break;
		case 0x008000:
			ret = 2;
			break;
		case 0x808000:
			ret = 3;
			break;
		case 0x000080:
			ret = 4;
			break;
		case 0x800080:
			ret = 5;
			break;
		case 0x008080:
			ret = 6;
			break;
		case 0xc0c0c0:
			ret = 7;
			break;
		case 0x808080:
			ret = 8;
			break;
		case 0xff0000:
			ret = 9;
			break;
		case 0x00ff00:
			ret = 10;
			break;
		case 0xffff00:
			ret = 11;
			break;
		case 0x0000ff:
			ret = 12;
			break;
		case 0xff00ff:
			ret = 13;
			break;
		case 0x00ffff:
			ret = 14;
			break;
		case 0xffffff:
			ret = 15;
			break;
		case 0x080808:
			ret = 232;
			break;
		case 0x121212:
			ret = 233;
			break;
		case 0x1c1c1c:
			ret = 234;
			break;
		case 0x262626:
			ret = 235;
			break;
		case 0x303030:
			ret = 236;
			break;
		case 0x3a3a3a:
			ret = 237;
			break;
		case 0x444444:
			ret = 238;
			break;
		case 0x4e4e4e:
			ret = 239;
			break;
		case 0x585858:
			ret = 240;
			break;
		case 0x626262:
			ret = 241;
			break;
		case 0x6c6c6c:
			ret = 242;
			break;
		case 0x767676:
			ret = 243;
			break;
		//case 0x808080:
		//	ret = 244;
		//	break;
		case 0x8a8a8a:
			ret = 245;
			break;
		case 0x949494:
			ret = 246;
			break;
		case 0x9e9e9e:
			ret = 247;
			break;
		case 0xa8a8a8:
			ret = 248;
			break;
		case 0xb2b2b2:
			ret = 249;
			break;
		case 0xbcbcbc:
			ret = 250;
			break;
		case 0xc6c6c6:
			ret = 251;
			break;
		case 0xd0d0d0:
			ret = 252;
			break;
		case 0xdadada:
			ret = 253;
			break;
		case 0xe4e4e4:
			ret = 254;
			break;
		case 0xeeeeee:
			ret = 255;
			break;
		default:
			ret = 16 + 36 * index(red) + 6 * index(green) + index(blue);
			break;
		}

		return ret;
	}
#endif

static uint32_t _utf32_to_utf8(uint32_t ch)
{
	uint8_t ret[4] = {0};
	if        (ch <= 0x7f)     {
		ret[0] = ch;
	} else if (ch <= 0x7ff)    {
		ret[0] = 0xc0 | (ch >> 6);
		ret[1] = 0x80 | (ch & 0x3f);
	} else if (ch <= 0xffff)   {
		ret[0] = 0xe0 | (ch >> 12);
		ret[1] = 0x80 | ((ch >> 6) & 0x3f);
		ret[2] = 0x80 | (ch & 0x3f);
	} else if (ch <= 0x10ffff) {
		ret[0] = 0xf0 | (ch >> 18);
		ret[1] = 0x80 | ((ch >> 12) & 0x3f);
		ret[2] = 0x80 | ((ch >> 6) & 0x3f);
		ret[3] = 0x80 | (ch & 0x3f);
	}
	return *(uint32_t *)ret;
}

static ssize_t _write(int fd, uint8_t const *buffer, size_t buffer_size)
{
	size_t st;
	ssize_t sst;
	for (st = 0, errno = 0 ; st < buffer_size ; st += sst) {
		sst = write(fd, buffer + st, buffer_size - st);
		if (sst == -1) {
			if (errno == EAGAIN) {
				sst = 0;
			} else {
				break;
			}
		}
	}
	if (st < buffer_size) {
		sst = -1;
	} else {
		sst = st;
	}
	return sst;
}

#define _printf(...) do{char *s; int i = asprintf(&s, __VA_ARGS__); if (0 <= i) {_write(STDOUT_FILENO, (uint8_t *)s, i); free(s);}}while (0)

#define _putchar(c) do{char s1 = (c); _write(STDOUT_FILENO, (uint8_t *)&s1, 1);}while (0)

static int _initialize(int line_height_px, int column_width_px, int rows, int cols)
{
	int ret = 0;

	(void)line_height_px;
	(void)column_width_px;

	if (
		(rows < BMS_DISP_XTERM_ROWS_MIN) ||
		(BMS_DISP_XTERM_ROWS_MAX < rows) ||
		(cols < BMS_DISP_XTERM_COLS_MIN) ||
		(BMS_DISP_XTERM_COLS_MAX < cols)
	) {
		ret = 1;
		goto EXIT;
	}

	_put_chars(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
	_printf("\x1b[0H");   // cursor home
	_printf("\x1b[?25l"); // disable cursor
	_printf("\x1b[?7l");  // disable wrap
	_rows = rows;
	_cols = cols;
	_row = _col = -1;

EXIT:

	return ret;
}

static void _finalize(void)
{
	_printf("\x1b[0m");   // reset color and attr
	_printf("\x1b[?7h");  // enable wrap
	_printf("\x1b[?25h"); // enable cursor
}
static int _set_size(int line_height_px, int column_width_px, int rows, int cols)
{
	int ret = 0;

	(void)line_height_px;
	(void)column_width_px;

	if (
		(rows < BMS_DISP_XTERM_ROWS_MIN) ||
		(BMS_DISP_XTERM_ROWS_MAX < rows) ||
		(cols < BMS_DISP_XTERM_COLS_MIN) ||
		(BMS_DISP_XTERM_COLS_MAX < cols)
	) {
		ret = 1;
		goto EXIT;
	}

	_rows = rows;
	_cols = cols;
	_row = _col = -1;

EXIT:

	return ret;
}

static void _get_cursor_pos(int *row, int *col)
{
	*row = _row;
	*col = _col;
}

static void _set_cursor_pos(int row, int col)
{
	if ((0 <= row) && (row < _rows) && (0 <= col) && (col < _cols)) {
		_printf("\x1b[%d;%dH", row + 1, col + 1);
		_row = row;
		_col = col;
	} else {
		_row = -1;
		_col = -1;
	}
}

static void _set_color(int is_fg, int red, int green, int blue)
{
#	if _IS_ESC_SEQ_COLOR_INDEXED
		_printf("\x1b[%d;5;%dm", is_fg ? 38 : 48, _rgb_to_indexed(red, green, blue));
#	else
		_printf("\x1b[%d;2;%d;%d;%dm", is_fg ? 38 : 48, red, green, blue);
#	endif
}

static void _set_fg_color(int red, int green, int blue)
{
	_set_color(1, red, green, blue);
}

static void _set_bg_color(int red, int green, int blue)
{
	_set_color(0, red, green, blue);
}

static void _set_bold(int bold)
{
	_printf("\x1b[%dm", (bold == 0) ? 22 : 1);
}

static void _set_underline(int underline)
{
	_printf("\x1b[%dm", (underline == 0) ? 24 : 4);
}

static void _set_reverse(int reverse)
{
	_printf("\x1b[%dm", (reverse == 0) ? 27 : 7);
}

static void _save_cursor()
{
	_printf("\x1b[s");
}

static void _restore_cursor()
{
	_printf("\x1b[u");
}

static void _forward_cursor(int n)
{
	_printf("\x1b[%dC", n);
}

static void _put_chars(
	int fg_red, int fg_green, int fg_blue,
	int bg_red, int bg_green, int bg_blue,
	int bold, int underline, int reverse,
	int width,
	uint32_t const *chars, int max_num_chars
)
{
	int i;
	int j;
	uint8_t c[4];
	static int prev_fg_red = -1;
	static int prev_fg_green = -1;
	static int prev_fg_blue = -1;
	static int prev_bg_red = -1;
	static int prev_bg_green = -1;
	static int prev_bg_blue = -1;
	static int prev_bold = -1;
	static int prev_underline = -1;
	static int prev_reverse = -1;

	if (chars == NULL) {
		prev_fg_red = -1;
		prev_fg_green = -1;
		prev_fg_blue = -1;
		prev_bg_red = -1;
		prev_bg_green = -1;
		prev_bg_blue = -1;
		prev_bold = -1;
		prev_underline = -1;
		prev_reverse = -1;
		goto EXIT;
	}

	if ((fg_red != prev_fg_red) || (fg_green != prev_fg_green) || (fg_blue != prev_fg_blue)) _set_fg_color(fg_red, fg_green, fg_blue);
	if ((bg_red != prev_bg_red) || (bg_green != prev_bg_green) || (bg_blue != prev_bg_blue)) _set_bg_color(bg_red, bg_green, bg_blue);
	if (bold != prev_bold) _set_bold(bold);
	if (underline != prev_underline) _set_underline(underline);
	if (reverse != prev_reverse) _set_reverse(reverse);

	_save_cursor();

	for (i = 0 ; (i < max_num_chars) && (chars[i] != 0) ; i++) {
		*(uint32_t *)c = _utf32_to_utf8(chars[i]);
		for (j = 0 ; (j < 4) && (c[j] != 0) ; j++) {
			_putchar(c[j]);
		}
	}

	if (i == 0) {
		_putchar(' ');
	}

	_restore_cursor();
	_forward_cursor(width);

	prev_fg_red = fg_red;
	prev_fg_green = fg_green;
	prev_fg_blue = fg_blue;
	prev_bg_red = bg_red;
	prev_bg_green = bg_green;
	prev_bg_blue = bg_blue;
	prev_bold = bold;
	prev_underline = underline;
	prev_reverse = reverse;

	_col += width;
	if (_cols <= _col) {
		_col = -1;
	}

EXIT:

	return;
}

static void _bell()
{
	_putchar('\a');
}

static void _sync()
{
	fsync(STDOUT_FILENO);
}
