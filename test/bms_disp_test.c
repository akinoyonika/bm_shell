#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <signal.h>
#include "bms/disp.h"
#include "bmt/main.h"

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

static int _term_eventfd = -1;

static void _sigh(int signum)
{
	uint64_t const se = 1;
	_write(_term_eventfd, (uint8_t *)&se, sizeof(se));
}

void bms_disp_test()
{
	char const *font = "Noto Mono";
	int const mode = 3;
	int const line_height = 16;
	int const col_width = 8;
	int const rows = 24;
	int const cols = 80;
	int row = 0;
	int col = 0;
	int x = 0;
	int y = 0;
	pthread_t th;
	int from_fd[2];
	int to_fd[2];
	uint64_t const se = 1;
	char line[80];
	int stdin_fd_fl;
	int stdout_fd_fl;
	struct th_arg {
		int term_eventfd;
		int in_fd;
		int out_fd;
	} th_arg[1];
	void *term(void *param)
	{
		struct th_arg *arg = (struct th_arg *)param;
		struct bmt_settings settings[1];
		settings->font = NULL;
		settings->mode = NULL;
		settings->lhpx = NULL;
		settings->cwpx = NULL;
		settings->rows = NULL;
		settings->cols = NULL;
		settings->user = NULL;
		settings->pass = NULL;
		bmt_main(arg->term_eventfd, arg->in_fd, arg->out_fd, settings);
		return NULL;
	}
	void disp_codes(
		uint32_t from, uint32_t to, int width,
		int mode,
		int rows, int cols,
		int line_height, int col_width,
		int fg_red, int fg_green, int fg_blue,
		int bg_red, int bg_green, int bg_blue,
		int *row, int *col,
		int *y, int *x
	)
	{
		uint32_t code;
		uint32_t const sp = 0x00000020;

		for (code = from ; code < to ; code++) {
			if (cols < (*col + width)) {
				bms_disp->put_chars(fg_red, fg_green, fg_blue, bg_red, bg_green, bg_blue, 0, 0, 0, 1, &sp, 1);
				*col = 0;
				(*row)++;
				if (*row == rows) {
					*row = 0;
				}
			}
			if (*col == 0) {
				bms_disp->set_cursor_pos(*row, *col);
			}

			bms_disp->put_chars(fg_red, fg_green, fg_blue, bg_red, bg_green, bg_blue, 0, 0, 0, width, &code, 1);

			(*col) += width;
			if (*col == cols) {
				*col = 0;
				(*row)++;
				if (*row == rows) {
					*row = 0;
				}
			}
		}
	
		bms_disp->sync();
	}

	stdin_fd_fl = fcntl(STDIN_FILENO, F_GETFL);
	assert(stdin_fd_fl != -1);
	stdout_fd_fl = fcntl(STDOUT_FILENO, F_GETFL);
	assert(stdout_fd_fl != -1);
	assert(fcntl(STDIN_FILENO, F_SETFL, stdin_fd_fl | O_NONBLOCK) == 0);
	assert(fcntl(STDOUT_FILENO, F_SETFL, stdout_fd_fl | O_NONBLOCK) == 0);

	assert(pipe2(from_fd, O_NONBLOCK) == 0);
	assert(pipe2(to_fd, O_NONBLOCK) == 0);

	th_arg->term_eventfd = eventfd(0, EFD_NONBLOCK);
	assert(th_arg->term_eventfd != -1);
	th_arg->in_fd = from_fd[0];
	th_arg->out_fd = to_fd[1];
	assert(pthread_create(&th, NULL, term, th_arg) == 0);
	
	_term_eventfd = eventfd(0, EFD_NONBLOCK);
	assert(_term_eventfd != -1);

	signal(SIGINT, _sigh);
	signal(SIGTERM, _sigh);

	snprintf(line, sizeof(line), "TERM: xterm-256color\n");
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "FONT: %s\n", font);
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "MODE: %d\n", mode);
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "LHPX: %d\n", line_height);
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "CWPX: %d\n", col_width);
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "ROWS: %d\n", rows);
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "COLS: %d\n", cols);
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "USER: user\n");
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "PASS: pass\n");
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));
	snprintf(line, sizeof(line), "login\n");
	assert(_write(from_fd[1], (uint8_t *)line, strlen(line)) == strlen(line));

	assert(bms_disp->initialize(from_fd[1], font, mode, line_height, col_width, rows, cols) == 0);

	disp_codes(
		0x00000020, 0x0000007f, 1,
		mode,
		rows, cols,
		line_height, col_width,
		255, 255, 255,
		0, 0, 0,
		&row, &col,
		&y, &x
	);
	disp_codes(
		0x00004e00, 0x00004e80, 2,
		mode,
		rows, cols,
		line_height, col_width,
		0, 0, 0,
		255, 255, 255,
		&row, &col,
		&y, &x
	);
	disp_codes(
		0x0001f550, 0x0001f568, 2,
		mode,
		rows, cols,
		line_height, col_width,
		255, 255, 255,
		0, 255, 0,
		&row, &col,
		&y, &x
	);
	disp_codes(
		0x0001f600, 0x0001f650, 2,
		mode,
		rows, cols,
		line_height, col_width,
		255, 255, 255,
		0, 255, 0,
		&row, &col,
		&y, &x
	);
	disp_codes(
		0x00002600, 0x00002700, 2,
		mode,
		rows, cols,
		line_height, col_width,
		0, 255, 0,
		255, 255, 255,
		&row, &col,
		&y, &x
	);

	sleep(10);

	bms_disp->finalize();

	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	close(_term_eventfd);

	_write(th_arg->term_eventfd, (uint8_t *)&se, sizeof(se));
	pthread_join(th, NULL);

	close(from_fd[0]);
	close(from_fd[1]);
	close(to_fd[0]);
	close(to_fd[1]);

	
	fcntl(STDOUT_FILENO, F_SETFL, stdout_fd_fl);
	fcntl(STDIN_FILENO, F_SETFL, stdin_fd_fl);
}
