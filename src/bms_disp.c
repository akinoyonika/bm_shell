#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <fontconfig/fontconfig.h>
#include <bms/fff.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "bms/font.h"
#include "bms/disp_xterm.h"
#include "bms/disp.h"

static int _initialize(int fd, char const *font, int mode, int line_height_px, int column_width_px, int rows, int cols);
static void _finalize(void);
static int _set_size(int line_height_px, int column_width_px, int rows, int cols);
static void _get_cursor_pos(int *row, int *col);
static void _set_cursor_pos(int row, int col);
static void _put_chars(
	int fg_red, int fg_green, int fg_blue,
	int bg_red, int bg_green, int bg_blue,
	int bold, int underline, int reverse,
	int width,
	uint32_t const *chars, int max_num_chars
);
static void _bell(void);
static void _sync(void);

static struct bms_disp_ops _bms_disp[] = {{
	_initialize,
	_finalize,
	_set_size,
	_get_cursor_pos,
	_set_cursor_pos,
	_put_chars,
	_bell,
	_sync
}};

struct bms_disp_ops const *bms_disp = (struct bms_disp_ops const *)_bms_disp;

static struct bms_fff _fff[1];
static char _font_file_name[256] = {'\0'};
static int _font_file_index = -1;
static struct bms_font _font[1];
static int _fd;
static int _mode;
static int _lhpx;
static int _cwpx;
static int _line_height_px;
static int _column_width_px;
static int _rows;
static int _cols;
static int _l;
static uint8_t _r;
static uint8_t _g;
static uint8_t _b;
static int _row;
static int _col;

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

static void _send(uint8_t *buffer, size_t buffer_size)
{
	extern int _fd;
	static uint8_t *buf = NULL;
	static size_t sizeof_buf = 0;
	static size_t buf_size = 0;
	size_t st;
	uint8_t *b;
	if (buffer_size == 0) {
		if (0 < buf_size) _write(_fd, buf, buf_size);
		buf_size = 0;
		if (buffer == NULL) {
			if (buf != NULL) free(buf);
			buf = NULL;
			sizeof_buf = 0;
		}
	}
	while (0 < buffer_size) {
		if ((sizeof_buf - buf_size) < buffer_size) {
			st = buf_size + buffer_size + 1;
			b = realloc(buf, st);
			if (b != NULL) {
				buf = b;
				sizeof_buf = st;
			}
		}
		st = sizeof_buf - buf_size;
		if (buffer_size < st) st = buffer_size;
		memcpy(buf + buf_size, buffer, st);
		buffer_size -= st;
		buf_size += st;
		if (buf_size == sizeof_buf) {
			_write(_fd, buf, buf_size);
			buf_size = 0;
		}
	}
}
#define _flush() do{_send((uint8_t *)(!NULL), 0); fsync(_fd);}while (0)

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

static int _initialize(int fd, char const *font, int mode, int line_height_px, int column_width_px, int rows, int cols)
{
	int ret = 0;

	if (fd == 0) {
		ret = bms_disp_xterm->initialize(line_height_px, column_width_px, rows, cols);
		if (ret != 0) {
			goto EXIT;
		}
		_bms_disp->finalize = bms_disp_xterm->finalize;
		_bms_disp->set_size = bms_disp_xterm->set_size;
		_bms_disp->get_cursor_pos = bms_disp_xterm->get_cursor_pos;
		_bms_disp->set_cursor_pos = bms_disp_xterm->set_cursor_pos;
		_bms_disp->put_chars = bms_disp_xterm->put_chars;
		_bms_disp->bell = bms_disp_xterm->bell;
		_bms_disp->sync = bms_disp_xterm->sync;
	} else {
		if (
			(rows < BMS_DISP_ROWS_MIN) ||
			(BMS_DISP_ROWS_MAX < rows) ||
			(cols < BMS_DISP_COLS_MIN) ||
			(BMS_DISP_COLS_MAX < cols) ||
			((line_height_px * rows) < BMS_DISP_HEIGHT_PX_MIN) ||
			(BMS_DISP_HEIGHT_PX_MAX < (line_height_px * rows)) ||
			((column_width_px * cols) < BMS_DISP_WIDTH_PX_MIN) ||
			(BMS_DISP_WIDTH_PX_MAX < (column_width_px * cols))
		) {
			ret = 1;
			goto EXIT;
		}

		_send(NULL, 0);
		ret = bms_fff->initialize(_fff, font);
		if (ret != 0) {
			goto EXIT;
		}
		_font_file_name[0] = '\0';
		_bms_disp->finalize = _finalize;
		_bms_disp->set_size = _set_size;
		_bms_disp->get_cursor_pos = _get_cursor_pos;
		_bms_disp->set_cursor_pos = _set_cursor_pos;
		_bms_disp->put_chars = _put_chars;
		_bms_disp->bell = _bell;
		_bms_disp->sync = _sync;
	}

	_fd = fd;
	_mode = mode;
	_line_height_px = _lhpx = line_height_px;
	_column_width_px = _cwpx = column_width_px;
	if (_lhpx < 16) {
		_line_height_px = 16;
	}
	if (_cwpx < 16) {
		_column_width_px = 16;
	}
	_rows = rows;
	_cols = cols;
	_l = 0;
	_r = _g = _b = 0;
	_row = _col = -1;
	
EXIT:

	return ret;
}

static void _finalize(void)
{
	if (_font_file_name[0] != '\0') {
		bms_font->finalize(_font);
		_font_file_name[0] = '\0';
	}
	_send(NULL, 0);
	_bms_disp->finalize = _finalize;
	_bms_disp->set_size = _set_size;
	_bms_disp->get_cursor_pos = _get_cursor_pos;
	_bms_disp->set_cursor_pos = _set_cursor_pos;
	_bms_disp->put_chars = _put_chars;
	_bms_disp->bell = _bell;
}

static int _make_pxl_command(int *l, int mode, uint8_t r, uint8_t g, uint8_t b, uint8_t *buffer)
{
	int ret;
	int i;
	int const l1 = *l - 1;
	int ll;
	for (i = 0, ll = l1 >> 7 ; 0 < ll ; i++, ll >>= 5);
	if (i == 0) {
		buffer[0] = 0b00000000 | l1;
		switch (mode & 0b11) {
		case 1:
			buffer[1] = (r + g + b) / 3;
			ret = 2;
			break;
		case 2:
			buffer[1] = _rgb_to_indexed(r, g, b);
			ret = 2;
			break;
		default:
			buffer[1] = r;
			buffer[2] = g;
			buffer[3] = b;
			ret = 4;
			break;
		}
		*l = 0;
	} else {
		i = (i - 1) * 5 + 7;
		ll = l1 >> i;
		buffer[0] = 0b11000000 | ll;
		ret = 1;
		*l -= (ll << i);
	}
	return ret;
}

static int _set_size(int line_height_px, int column_width_px, int rows, int cols)
{
	int ret = 0;
	uint8_t buffer[4];

	if (
		(rows < BMS_DISP_ROWS_MIN) ||
		(BMS_DISP_ROWS_MAX < rows) ||
		(cols < BMS_DISP_COLS_MIN) ||
		(BMS_DISP_COLS_MAX < cols) ||
		((line_height_px * rows) < BMS_DISP_HEIGHT_PX_MIN) ||
		(BMS_DISP_HEIGHT_PX_MAX < (line_height_px * rows)) ||
		((column_width_px * cols) < BMS_DISP_WIDTH_PX_MIN) ||
		(BMS_DISP_WIDTH_PX_MAX < (column_width_px * cols))
	) {
		ret = 1;
		goto EXIT;
	}

	while (0 < _l) _send(buffer, _make_pxl_command(&_l, _mode, _r, _g, _b, buffer));

	_line_height_px = _lhpx = line_height_px;
	_column_width_px = _cwpx = column_width_px;
	if (_lhpx < 16) {
		_line_height_px = 16;
	}
	if (_cwpx < 16) {
		_column_width_px = 16;
	}
	_rows = rows;
	_cols = cols;
	_l = 0;
	_r = _g = _b = 0;
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
	/*
	 * | 1   0 |R10 R09 R08|C10 C09 C08|
	 * |R07 R06 R05 R04 R03 R02 R01 R00|
	 * |C07 C06 C05 C04 C03 C02 C01 C00|
	 * 
	 * R10 - R00: rows
	 * C10 - C00: cols
	 */

	uint8_t buffer[4];

	while (0 < _l) _send(buffer, _make_pxl_command(&_l, _mode, _r, _g, _b, buffer));

	if ((0 <= row) && (row < _rows) && (0 <= col) && (col < _cols)) {
		buffer[0] =
			0b10000000 |
			((row & 0b11100000000) >> 5) |
			((col & 0b11100000000) >> 8);
		buffer[1] = row & 0b11111111;
		buffer[2] = col & 0b11111111;

		_send(buffer, 3);

		_row = row;
		_col = col;
	} else {
		_row = -1;
		_col = -1;
	}
}

static void _put_chars(
	int fg_red, int fg_green, int fg_blue,
	int bg_red, int bg_green, int bg_blue,
	int bold, int underline, int reverse,
	int width,
	uint32_t const *chars, int max_num_chars
)
{
	/*
	 * - ex-length (0b10000000 < length)
	 * | 1   1   0 |Ln4 Ln3 Ln2 Ln1 Ln0|
	 * 
	 *     :                        :
	 *     :                        :
	 * L21 - L17: ((length - 1) >> 17) & 0b00011111
	 * L16 - L12: ((length - 1) >> 12) & 0b00011111
	 * L11 - L07: ((length - 1) >>  7) & 0b00011111
	 */
	/*
	 * - mode & 0b11 == 1
	 * | 0 |L06 L05 L04 L03 L02 L01 L00|
	 * |G07 G06 G05 G04 G03 G02 G01 G00|
	 * 
	 * L06 - L00: (length - 1) & 0b1111111
	 * G07 - G00: gray
	 * 
	 * - mode & 0b11 == 2
	 * | 0 |L06 L05 L04 L03 L02 L01 L00|
	 * |C07 C06 C05 C04 C03 C02 C01 C00|
	 * 
	 * L06 - L00: (length - 1) & 0b1111111
	 * C07 - C00: paletted
	 * 
	 * - mode & 0b11 == 3
	 * | 0 |L06 L05 L04 L03 L02 L01 L00|
	 * |R07 R06 R05 R04 R03 R02 R01 R00|
	 * |G07 G06 G05 G04 G03 G02 G01 G00|
	 * |B07 B06 B05 B04 B03 B02 B01 B00|
	 * 
	 * L06 - L00: (length - 1) & 0b1111111
	 * R07 - R00: red
	 * G07 - G00: green
	 * B07 - B00: blue
	 */

	int i;
	int j;
	int x;
	int y;
	char const *f;
	uint8_t buffer[4];
	int const bmh = _lhpx;
	int const bmw = _cwpx * width;
	int const bitmap_height = _line_height_px;
	int const bitmap_width = _column_width_px * width;
	uint8_t bitmap[bitmap_height * bitmap_width * 3];
	uint8_t fg_color[3];
	uint8_t bg_color[3];

	if (bold) {
		fg_red <<= 1;
		if (255 < fg_red) fg_red = 255;
		fg_green <<= 1;
		if (255 < fg_green) fg_green = 255;
		fg_blue <<= 1;
		if (255 < fg_blue) fg_blue = 255;
	}

	if (reverse == 0) {
		fg_color[BMS_FONT_R] = fg_red;
		fg_color[BMS_FONT_G] = fg_green;
		fg_color[BMS_FONT_B] = fg_blue;
		bg_color[BMS_FONT_R] = bg_red;
		bg_color[BMS_FONT_G] = bg_green;
		bg_color[BMS_FONT_B] = bg_blue;
	} else {
		fg_color[BMS_FONT_R] = bg_red;
		fg_color[BMS_FONT_G] = bg_green;
		fg_color[BMS_FONT_B] = bg_blue;
		bg_color[BMS_FONT_R] = fg_red;
		bg_color[BMS_FONT_G] = fg_green;
		bg_color[BMS_FONT_B] = fg_blue;
	}

	for (i = 0 ; i < (bitmap_height * bitmap_width * 3) ; i += 3) {
		memcpy (bitmap + i, bg_color, 3);
	}
	for (i = 0 ; (i < max_num_chars) && (chars[i] != 0) ; i++) {

		j = bms_fff->find(_fff, chars[i], &f);
		if (j == -1) {
			goto EXIT;
		}

		if ((strncmp(_font_file_name, f, sizeof(_font_file_name)) != 0) || (_font_file_index != j)) {
			if (_font_file_name[0] != '\0') {
				bms_font->finalize(_font);
			}
			if (bms_font->initialize(_font, f, j, bitmap_height * 3 / 4) != 0) {
				_font_file_name[0] = '\0';
				_font_file_index = -1;
				goto EXIT;
			}
			_font_file_name[sizeof(_font_file_name) - 1] = '\0';
#			pragma GCC diagnostic push
#			pragma GCC diagnostic ignored "-Wstringop-truncation"
			strncpy(_font_file_name, f, sizeof(_font_file_name));
#			pragma GCC diagnostic pop
			if (_font_file_name[sizeof(_font_file_name) - 1] != '\0') {
				bms_font->finalize(_font);
				_font_file_name[0] = '\0';
				_font_file_index = -1;
				goto EXIT;
			}
		}

		bms_font->get(_font, fg_color, bg_color, chars[i], bitmap, bitmap_height, bitmap_width);
	}

	if (underline != 0) {
		i = bitmap_height * 14 / 16;
		j = bitmap_height * 15 / 16;
		if (i == j) j = i + 1;
		for (y = i ; y < j ; y++) {
			for (x = 0 ; x < bitmap_width ; x++) {
				memcpy(bitmap + ((y * bitmap_width + x) * 3), fg_color, 3);
			}
		}
	}

	if ((bmh < bitmap_height) || (bmw < bitmap_width)) {
		for (i = 0 ; i < (bmh * bmw) ; i++) {
			x = i % bmw;
			y = i / bmw;
			memmove(bitmap + (i * 3), bitmap + ((y * bitmap_width + x) * 3), 3);
		}
	}

	for (x = 0 ; x < bmw ; x++) {
		for (y = 0 ; y < bmh ; y++) {
			i = (y * bmw + x) * 3;
			if (
				(_l == 0) ||
				(_r != bitmap[i + 0]) ||
				(_g != bitmap[i + 1]) ||
				(_b != bitmap[i + 2])
			) {
				while (0 < _l) _send(buffer, _make_pxl_command(&_l, _mode, _r, _g, _b, buffer));
				_r = bitmap[i + 0];
				_g = bitmap[i + 1];
				_b = bitmap[i + 2];
				_l = 1;
			} else {
				_l++;
			}
		}
	}

	_col += width;
	if (_col == _cols) {
		_col = 0;
		_row++;
		if (_row == _rows) {
			_row = 0;
		} else if (_rows < _row) {
			_row = -1;
		}
	} else if (_cols < _col) {
		_col = -1;
	}

EXIT:

	return;
}

static void _bell()
{
	/*
	 * | 1   1   1   1   1   1   1   0 |
	 */

	uint8_t buffer[1];

	buffer[0] = 0b11111110;

	_send(buffer, 1);
}

static void _sync()
{
	/*
	 * | 1   1   1   1   1   1   1   1 |
	 */

	uint8_t buffer[4];

	while (0 < _l) _send(buffer, _make_pxl_command(&_l, _mode, _r, _g, _b, buffer));

	if ((_mode & 0b10000000) == 0) {
		buffer[0] = 0b11111111;
		_send(buffer, 1);
	} else {
		memset(buffer, 0b11111111, sizeof(buffer));
		_send(buffer, sizeof(buffer));
	}
	_flush();
}
