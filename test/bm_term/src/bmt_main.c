#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "bmt/main.h"

static uint32_t _get_tick_us()
{
	struct timespec t;
	unsigned long tv;
	clock_gettime(CLOCK_MONOTONIC, &t);
	tv = (unsigned long)(t.tv_sec) * 1000000UL + (unsigned long)(t.tv_nsec) / 1000UL;
	return (uint32_t)(tv & 0xffffffff);
}

static ssize_t _read(int fd, uint8_t *buffer, size_t buffer_size)
{
	ssize_t sst;
	errno = 0;
	sst = read(fd, buffer, buffer_size);
	if (sst == -1) {
		if (errno == EAGAIN) {
			sst = 0;
		}
	}
	return sst;
}
#define _is_rfd_close(ret) (((ret) == 0) && (errno != EAGAIN))
#define _is_rfd_readable(ret) (((ret) == 0) && (errno != EAGAIN)) // ret = _read(fd, NULL, 0);

static ssize_t _write(int fd, uint8_t const *buffer, size_t buffer_size)
{
	size_t st;
	ssize_t sst;
	uint32_t start_time;
	errno = 0;
	if (buffer_size == 0) {
		sst = write(fd, NULL, 0);
		if (sst == -1) {
			if (errno == EAGAIN) {
				sst = 0;
			}
		}
		goto EXIT;
	}
	for (
		st = 0, errno = 0, start_time = _get_tick_us() ;
		(st < buffer_size) && ((_get_tick_us() - start_time) < BMT_TIMEOUT_US) ;
		st += sst
	) {
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
EXIT:
	return sst;
}
#define _is_wfd_writable(ret) (((ret) == 0) && (errno != EAGAIN)) // ret = _write(fd, NULL, 0);

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

struct _term_arg {
	struct bmt_settings *settings;
	char font[64];
	int mode;
	int lhpx;
	int cwpx;
	int rows;
	int cols;
	int user;
	int pass;
};

static int _pterm(int term_eventfd, int in_fd, int out_fd, struct _term_arg *arg)
{
	int is_running = 1;
	struct pollfd fds[2];
	ssize_t sst;
	uint8_t buffer[8];
	size_t buffer_size = 0;
	char line[80];
	size_t line_len = 0;
	int term = 0;
	int lgin = 0;
	uint64_t re;

	arg->font[0] = 0;
	arg->mode    = 0;
	arg->lhpx    = 0;
	arg->cwpx    = 0;
	arg->rows    = 0;
	arg->cols    = 0;
	arg->user    = 0;
	arg->pass    = 0;

	while (!lgin && is_running) {
		sst = _read(in_fd, NULL, 0);
		if (!_is_rfd_readable(sst)) {
			(fds + 0)->fd = term_eventfd;
			(fds + 0)->events = POLLIN;
			(fds + 0)->revents = 0;
			(fds + 1)->fd = in_fd;
			(fds + 1)->events = POLLIN;
			(fds + 1)->revents = 0;

			poll(fds, 2, -1);
		}

		sst = _read(in_fd, buffer, sizeof(buffer));
		if ((sst == -1) || _is_rfd_close(sst)) {
			sst = 0;
			is_running = 0;
		}
		buffer_size = sst;

		if (
			(term         == 0) ||
			(arg->font[0] == 0) ||
			(arg->mode    == 0) ||
			(arg->lhpx    == 0) ||
			(arg->cwpx    == 0) ||
			(arg->rows    == 0) ||
			(arg->cols    == 0) ||
			(arg->user    == 0) ||
			(arg->pass    == 0) ||
			(lgin         == 0)
		) {
			for (size_t st = 0 ; st < buffer_size ; st++) {
				line[line_len++] = buffer[st];
				if (line[line_len - 1] == '\n') {
					line[line_len - 1] = '\0';
					if (term == 0) {
						if (strncmp(line, "TERM: ", 6) == 0) {
							term = 1;
						}
						if (term != 0) line_len = 0;
					}
					if (arg->font[0] == 0) {
						if (strncmp(line, "FONT: ", 6) == 0) {
							arg->font[sizeof(arg->font) - 1] = 0;
#							pragma GCC diagnostic push
#							pragma GCC diagnostic ignored "-Wstringop-truncation"
							strncpy(arg->font, line + 6, sizeof(arg->font));
#							pragma GCC diagnostic pop
							if (arg->font[sizeof(arg->font) - 1] != 0) {
								arg->font[0] = 0;
							}
						}
						if (arg->font[0] != 0) line_len = 0;
					}
					if (arg->mode == 0) {
						if (sscanf(line, "MODE: %d", &(arg->mode)) != 1) {
							arg->mode = 0;
						}
						if (arg->mode < 0) arg->mode = 0;
						if (arg->mode != 0) line_len = 0;
					}
					if (arg->lhpx == 0) {
						if (sscanf(line, "LHPX: %d", &(arg->lhpx)) != 1) {
							arg->lhpx = 0;
						}
						if (arg->lhpx < 0) arg->lhpx = 0;
						if (arg->lhpx != 0) line_len = 0;
					}
					if (arg->cwpx == 0) {
						if (sscanf(line, "CWPX: %d", &(arg->cwpx)) != 1) {
							arg->cwpx = 0;
						}
						if (arg->cwpx < 0) arg->cwpx = 0;
						if (arg->cwpx != 0) line_len = 0;
					}
					if (arg->rows == 0) {
						if (sscanf(line, "ROWS: %d", &(arg->rows)) != 1) {
							arg->rows = 0;
						}
						if (arg->rows < 0) arg->rows = 0;
						if (arg->rows != 0) line_len = 0;
					}
					if (arg->cols == 0) {
						if (sscanf(line, "COLS: %d", &(arg->cols)) != 1) {
							arg->cols = 0;
						}
						if (arg->cols < 0) arg->cols = 0;
						if (arg->cols != 0) line_len = 0;
					}
					if (arg->user == 0) {
						if (strncmp(line, "USER: ", 6) == 0) {
							arg->user = 1;
						}
						if (arg->user != 0) line_len = 0;
					}
					if (arg->pass == 0) {
						if (strncmp(line, "PASS: ", 6) == 0) {
							arg->pass = 1;
						}
						if (arg->pass != 0) line_len = 0;
					}
					if (lgin == 0) {
						if (strncmp(line, "login", 5) == 0) {
							lgin = 1;
						}
						if (lgin != 0) line_len = 0;
					}
					if (0 < line_len) {
						fprintf(stderr, "%s\n", line);
						line_len = 0;
					}
				} else if (line[line_len - 1] < '\x20') {
					line_len--;
				} else if ((arg->font[0] == 0) && (line_len == 6) && (strncmp(line, "FONT: ", 6) == 0)) {
					if (arg->settings->font == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->font), strlen(arg->settings->font)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->mode    == 0) && (line_len == 6) && (strncmp(line, "MODE: ", 6) == 0)) {
					if (arg->settings->mode == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->mode), strlen(arg->settings->mode)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->lhpx    == 0) && (line_len == 6) && (strncmp(line, "LHPX: ", 6) == 0)) {
					if (arg->settings->lhpx == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->lhpx), strlen(arg->settings->lhpx)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->cwpx    == 0) && (line_len == 6) && (strncmp(line, "CWPX: ", 6) == 0)) {
					if (arg->settings->cwpx == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->cwpx), strlen(arg->settings->cwpx)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->rows    == 0) && (line_len == 6) && (strncmp(line, "ROWS: ", 6) == 0)) {
					if (arg->settings->rows == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->rows), strlen(arg->settings->rows)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->cols    == 0) && (line_len == 6) && (strncmp(line, "COLS: ", 6) == 0)) {
					if (arg->settings->cols == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->cols), strlen(arg->settings->cols)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->user    == 0) && (line_len == 6) && (strncmp(line, "USER: ", 6) == 0)) {
					if (arg->settings->user == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->user), strlen(arg->settings->user)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				} else if ((arg->pass    == 0) && (line_len == 6) && (strncmp(line, "PASS: ", 6) == 0)) {
					if (arg->settings->pass == NULL) {
						if (_write(out_fd, (uint8_t *)"\n", 1) == -1) {
							is_running = 0;
						}
					} else {
						if (
							(_write(out_fd, (uint8_t *)(arg->settings->pass), strlen(arg->settings->pass)) == -1) ||
							(_write(out_fd, (uint8_t *)"\n", 1) == -1)
						) {
							is_running = 0;
						}
					}
				}
				if (sizeof(line) <= line_len) line_len = sizeof(line) - 1;
			}
		}

		/*if (out_fd != STDOUT_FILENO) {
			sst = _write(STDOUT_FILENO, buffer, buffer_size);
			if (sst == -1) {
				is_running = 0;
			} else {
				fsync(STDOUT_FILENO);
			}
		}*/

		buffer_size = 0;

		if (_read(term_eventfd, (uint8_t *)&re, sizeof(re)) == sizeof(re)) is_running = 0;
	}

	return is_running;
}

static int _bterm(int term_eventfd, int in_fd, int out_fd, struct _term_arg *arg)
{
	int is_running = 0;
	int const mode = arg->mode;
	int const line_height = arg->lhpx;
	int const col_width = arg->cwpx;
	int const rows = arg->rows;
	int const cols = arg->cols;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	int is_texture_dirty;
	SDL_Event event[1];
	pthread_t key_th;
	struct pollfd fds[3];
	uint8_t buffer[4]; // 4 = maximum command length
	size_t buffer_size = 0;
	size_t expect_buffer_size = 0;
	ssize_t sst;
	uint8_t bitmap[line_height * col_width * 3];
	size_t bitmap_size = 0;
	int x = 0;
	int y = 0;
	int l = 0;
	uint64_t re;
	int command_len(uint8_t command, int mode)
	{
		int ret;
		switch (command & 0b10000000) {
		case 0b00000000:
			switch (mode & 0b11) {
			case 1:
				ret = 2;
				break;
			case 2:
				ret = 2;
				break;
			default:
				ret = 4;
				break;
			}
			break;
		default:
			switch (command & 0b11000000) {
			case 0b10000000:
				ret = 3;
				break;
			default:
				switch (command & 0b11100000) {
				case 0b11000000:
					ret = 1;
					break;
				default:
					switch (command & 0b11110000) {
					case 0b11100000:
						ret = 0;
						break;
					default:
						switch (command & 0b11111000) {
						case 0b11110000:
							ret = 0;
							break;
						default:
							switch (command & 0b11111100) {
							case 0b11111000:
								ret = 0;
								break;
							default:
								switch (command & 0b11111110) {
								case 0b11111100:
									ret = 0;
									break;
								default:
									switch (command & 0b11111111) {
									case 0b11111110:
										ret = 1;
										break;
									default:
										ret = 1;
										break;
									}
								}
								break;
							}
							break;
						}
						break;
					}
					break;
				}
				break;
			}
			break;
		}
		return ret;
	}
	void do_command(
		uint8_t const *buffer,
		int mode,
		int cols, int rows,
		int line_height, int col_width,
		int *y, int *x, int *l,
		uint8_t *bitmap, size_t *bitmap_size,
		SDL_Renderer *renderer, SDL_Texture *texture, int *is_texture_dirty
	)
	{
		int i;
		int j;
		uint8_t r;
		uint8_t g;
		uint8_t b;
		SDL_Rect rect[1];
		switch (buffer[0] & 0b10000000) {
		case 0b00000000:
			*l = (((*l) << 7) | ((int)(buffer[0]) & 0b01111111)) + 1;
			if ((*l < 0) || ((rows * cols * line_height * col_width) < *l)) *l = 0;
			switch (mode & 0b11) {
			case 1:
				r = buffer[1];
				g = buffer[1];
				b = buffer[1];
				break;
			case 2:
				i = _indexed_to_rgb((int)buffer[1]);
				r = (i >> 16) & 0xff;
				g = (i >>  8) & 0xff;
				b = (i >>  0) & 0xff;
				break;
			default:
				r = buffer[1];
				g = buffer[2];
				b = buffer[3];
				break;
			}
			for (i = 0 ; i < *l ; i++) {
				j = *bitmap_size / 3;
				j = (j % line_height) * col_width + j / line_height;
				j *= 3;
				bitmap[j++] = r;
				bitmap[j++] = g;
				bitmap[j++] = b;
				*bitmap_size += 3;
				if (*bitmap_size == (line_height * col_width * 3)) {
					rect->x = *x * col_width;
					rect->y = *y * line_height;
					rect->w = col_width;
					rect->h = line_height;
					SDL_UpdateTexture(texture, rect, bitmap, col_width * 3);
					*is_texture_dirty = 1;
					*bitmap_size = 0;
					(*x)++;
					if (cols <= *x) {
						*x = 0;
						(*y)++;
						if (rows <= *y) {
							*y = 0;
						}
					}
				}
			}
			*l = 0;
			break;
		default:
			switch (buffer[0] & 0b11000000) {
			case 0b10000000:
				*bitmap_size = 0;
				i = (((int)(buffer[0]) & 0b00111000) << 5) | (int)(buffer[1]);
				j = (((int)(buffer[0]) & 0b00000111) << 8) | (int)(buffer[2]);
				if ((0 <= i) && (i < rows) && (0 <= j) && (j < cols)) {
					*y = i;
					*x = j;
				}
				break;
			default:
				switch (buffer[0] & 0b11100000) {
				case 0b11000000:
					*l = ((*l) << 5) | ((int)buffer[0] & 0b00011111);
					break;
				default:
					switch (buffer[0] & 0b11110000) {
					case 0b11100000:
						break;
					default:
						switch (buffer[0] & 0b11111000) {
						case 0b11110000:
							break;
						default:
							switch (buffer[0] & 0b11111100) {
							case 0b11111000:
								break;
							default:
								switch (buffer[0] & 0b11111110) {
								case 0b11111100:
									break;
								default:
									switch (buffer[0] & 0b11111111) {
									case 0b11111110:
										i = open("/dev/tty", O_WRONLY);
										if (i != -1) {
											_write(i, (uint8_t *)"\a", 1);
											fsync(i);
											close(i);
										}
										break;
									default:
										if (*is_texture_dirty) {
											SDL_RenderCopy(renderer, texture, NULL, NULL);
											SDL_RenderPresent(renderer);
											*is_texture_dirty = 0;
										}
										if (0 < *bitmap_size) {
											*bitmap_size = 0;
											(*x)++;
											if (cols <= *x) {
												*x = 0;
												(*y)++;
												if (rows <= *y) {
													*y = 0;
												}
											}
										}
										break;
									}
									break;
								}
								break;
							}
							break;
						}
						break;
					}
					break;
				}
				break;
			}
			break;
		}
	}
	struct key_arg {
		int term_event_type;
		int out_fd;
		int winexp_eventfd;
	} key_arg[1];
	void *key(void *param)
	{
		struct key_arg *arg = (struct key_arg *)param;
		int is_running = 1;
		SDL_Event event[1];
		uint64_t const se = 1;
		while (is_running) {
			if (!SDL_WaitEvent(event)) continue;
			if (event->type == arg->term_event_type) is_running = 0;
			if (
				(event->type == SDL_WINDOWEVENT) &&
				(event->window.event == SDL_WINDOWEVENT_EXPOSED)
			) {
				_write(arg->winexp_eventfd, (uint8_t *)&se, sizeof(se));
			}
			if (event->type == SDL_TEXTINPUT) {
				_write(arg->out_fd, (uint8_t *)(event->text.text), strlen(event->text.text));
			}
			if (event->type == SDL_KEYDOWN) {
				char *s;
				switch (event->key.keysym.sym) {
				case SDLK_BACKSPACE:
					s = "\x08";
					break;
				case SDLK_TAB:
					s = "\x09";
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					s = "\x0d";
					break;
				case SDLK_ESCAPE:
					s = "\x1b";
					break;
				case SDLK_INSERT:
					s = "\x1b[2~";
					break;
				case SDLK_DELETE:
					s = "\x1b[3~";
					break;
				case SDLK_HOME:
					s = "\x1b[H";
					break;
				case SDLK_END:
					s = "\x1b[F";
					break;
				case SDLK_PAGEUP:
					s = "\x1b[5~";
					break;
				case SDLK_PAGEDOWN:
					s = "\x1b[6~";
					break;
				case SDLK_UP:
					s = "\x1b[A";
					break;
				case SDLK_DOWN:
					s = "\x1b[B";
					break;
				case SDLK_RIGHT:
					s = "\x1b[C";
					break;
				case SDLK_LEFT:
					s = "\x1b[D";
					break;
				case SDLK_F1:
					s = "\x1b[OP";
					break;
				case SDLK_F2:
					s = "\x1b[OQ";
					break;
				case SDLK_F3:
					s = "\x1b[OR";
					break;
				case SDLK_F4:
					s = "\x1b[OS";
					break;
				case SDLK_F5:
					s = "\x1b[15~";
					break;
				case SDLK_F6:
					s = "\x1b[17~";
					break;
				case SDLK_F7:
					s = "\x1b[18~";
					break;
				case SDLK_F8:
					s = "\x1b[19~";
					break;
				case SDLK_F9:
					s = "\x1b[20~";
					break;
				case SDLK_F10:
					s = "\x1b[21~";
					break;
				case SDLK_F11:
					s = "\x1b[23~";
					break;
				case SDLK_F12:
					s = "\x1b[24~";
					break;
				default:
					s = "";
					if (event->key.keysym.mod & KMOD_CTRL) {
						switch (event->key.keysym.sym) {
						case SDLK_a:
							s = "\x01";
							break;
						case SDLK_b:
							s = "\x02";
							break;
						case SDLK_c:
							s = "\x03";
							break;
						case SDLK_d:
							s = "\x04";
							break;
						case SDLK_e:
							s = "\x05";
							break;
						case SDLK_f:
							s = "\x06";
							break;
						case SDLK_g:
							s = "\x07";
							break;
						case SDLK_h:
							s = "\x08";
							break;
						case SDLK_i:
							s = "\x09";
							break;
						case SDLK_j:
							s = "\x0a";
							break;
						case SDLK_k:
							s = "\x0b";
							break;
						case SDLK_l:
							s = "\x0c";
							break;
						case SDLK_m:
							s = "\x0d";
							break;
						case SDLK_n:
							s = "\x0e";
							break;
						case SDLK_o:
							s = "\x0f";
							break;
						case SDLK_p:
							s = "\x10";
							break;
						case SDLK_q:
							s = "\x11";
							break;
						case SDLK_r:
							s = "\x12";
							break;
						case SDLK_s:
							s = "\x13";
							break;
						case SDLK_t:
							s = "\x14";
							break;
						case SDLK_u:
							s = "\x15";
							break;
						case SDLK_v:
							s = "\x16";
							break;
						case SDLK_w:
							s = "\x17";
							break;
						case SDLK_x:
							s = "\x18";
							break;
						case SDLK_y:
							s = "\x19";
							break;
						case SDLK_z:
							s = "\x1a";
							break;
						default:
							s = "";
							break;
						}
					}
					break;
				}
				_write(arg->out_fd, (uint8_t *)s, strlen(s));
			}
		}
		return NULL;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		goto SDL_INIT_FAILURE_EXIT;
	}

	window = SDL_CreateWindow("bm_term", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, col_width * cols, line_height * rows, 0);
	if (window == NULL) {
		goto SDL_CREATE_WINDOW_FAILURE_EXIT;
	}
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == NULL) {
		goto SDL_CREATE_RENDERER_FAILURE_EXIT;
	}
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, col_width * cols, line_height * rows);
	if (texture == NULL) {
		goto SDL_CREATE_TEXTURE_FAILURE_EXIT;
	}
	SDL_StartTextInput();
	is_texture_dirty = 0;

	key_arg->term_event_type = SDL_RegisterEvents(1);
	if (key_arg->term_event_type == 0xffffffff) {
		goto SDL_REGISTER_EVENT_FAILURE_EXIT;
	}
	key_arg->winexp_eventfd = eventfd(0, EFD_NONBLOCK);
	if (key_arg->winexp_eventfd == -1) {
		goto WINEXP_EVENTFD_FAILURE_EXIT;
	}
	key_arg->out_fd = out_fd;
	if (pthread_create(&key_th, NULL, key, key_arg) != 0) {
		goto PTHREAD_CREATE_FAILURE_EXIT;
	}

	is_running = 1;

	while (is_running) {
		sst = _read(in_fd, NULL, 0);
		if (!_is_rfd_readable(sst)) {
			(fds + 0)->fd = term_eventfd;
			(fds + 0)->events = POLLIN;
			(fds + 0)->revents = 0;
			(fds + 1)->fd = key_arg->winexp_eventfd;
			(fds + 1)->events = POLLIN;
			(fds + 1)->revents = 0;
			(fds + 2)->fd = in_fd;
			(fds + 2)->events = POLLIN;
			(fds + 2)->revents = 0;

			if (buffer_size == 0) {
				poll(fds, 3, -1);
			} else {
				if (poll(fds, 3, BMT_TIMEOUT_US / 1000) == 0) {
					buffer_size = 0;
					expect_buffer_size = 0;
				}
			}
		}

		if (buffer_size == 0) {
			sst = _read(in_fd, buffer, 1);
			if ((sst == -1) || _is_rfd_close(sst)) {
				sst = 0;
				is_running = 0;
			}
			buffer_size += sst;
		}

		if ((expect_buffer_size == 0) && (buffer_size == 1)) {
			expect_buffer_size = command_len(buffer[0], mode);
			if (expect_buffer_size == 0) {
				buffer_size = 0;
			}
		}

		if ((0 < expect_buffer_size) && (buffer_size < expect_buffer_size)) {
			sst = _read(in_fd, buffer + buffer_size, expect_buffer_size - buffer_size);
			if ((sst == -1) || _is_rfd_close(sst)) {
				sst = 0;
				is_running = 0;
			}
			buffer_size += sst;
		}

		if ((0 < expect_buffer_size) && (buffer_size == expect_buffer_size)) {
			do_command(
				buffer,
				mode,
				cols, rows,
				line_height, col_width,
				&y, &x, &l,
				bitmap, &bitmap_size,
				renderer, texture, &is_texture_dirty
			);
			buffer_size = 0;
			expect_buffer_size = 0;
		}

		if ((_read(key_arg->winexp_eventfd, (uint8_t *)&re, sizeof(re)) == sizeof(re)) && is_texture_dirty) {
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
			is_texture_dirty = 0;
		}

		if (_read(term_eventfd, (uint8_t *)&re, sizeof(re)) == sizeof(re)) is_running = 0;
	}

	SDL_zero(*event);
	event->type = key_arg->term_event_type;
	SDL_PushEvent(event);
	pthread_join(key_th, NULL);
PTHREAD_CREATE_FAILURE_EXIT:
	close(key_arg->winexp_eventfd);
WINEXP_EVENTFD_FAILURE_EXIT:
SDL_REGISTER_EVENT_FAILURE_EXIT:
	SDL_StopTextInput();
	SDL_DestroyTexture(texture);
SDL_CREATE_TEXTURE_FAILURE_EXIT:	
	SDL_DestroyRenderer(renderer);
SDL_CREATE_RENDERER_FAILURE_EXIT:
	SDL_DestroyWindow(window);
SDL_CREATE_WINDOW_FAILURE_EXIT:

	SDL_Quit();
SDL_INIT_FAILURE_EXIT:

	return is_running;
}

int bmt_main(int term_eventfd, int in_fd, int out_fd, struct bmt_settings *settings)
{
	int ret = 0;
	struct _term_arg arg[1] = {{0}};
	int in_fd_fl;
	int out_fd_fl;

	in_fd_fl = fcntl(in_fd, F_GETFL);
	if ((in_fd_fl == -1) || (fcntl(in_fd, F_SETFL, in_fd_fl | O_NONBLOCK) != 0)) {
		ret = 1;
		goto FCNTL_IN_FD_FAILURE_EXIT;
	}

	out_fd_fl = fcntl(out_fd, F_GETFL);
	if ((out_fd_fl == -1) || (fcntl(out_fd, F_SETFL, out_fd_fl | O_NONBLOCK) != 0)) {
		ret = 1;
		goto FCNTL_OUT_FD_FAILURE_EXIT;
	}

	arg->settings = settings;
	if (!_pterm(term_eventfd, in_fd, out_fd, arg)) {
		ret = 1;
		goto PTERM_ABORT_EXIT;
	}

	_bterm(term_eventfd, in_fd, out_fd, arg);

PTERM_ABORT_EXIT:
	fcntl(out_fd, F_SETFL, in_fd_fl);
FCNTL_OUT_FD_FAILURE_EXIT:
	fcntl(in_fd, F_SETFL, in_fd_fl);
FCNTL_IN_FD_FAILURE_EXIT:

	return ret;
}
