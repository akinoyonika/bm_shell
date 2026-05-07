#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <time.h>
#include <pty.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <vterm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "bms/font.h"
#include "bms/term.h"
#include "bms/disp.h"
#include "bms/main.h"

#define _TERM "xterm-256color"
#define _INIT_ROWS (8)
#define _INIT_COLS (40)
#define _EXEC_FILE "script"
#define _EXEC_ARGS                                                                 \
	"script",                                                                      \
	"-qc",                                                                         \
	"                      read -t 10  -p \"TERM: \" TERM; r=$?\n"                 \
	"if [ $r -eq 0 ]; then read -t 10  -p \"FONT: \" FONT; r=$?; fi\n"             \
	"if [ $r -eq 0 ]; then read -t 10  -p \"MODE: \" OPERATION_MODE; r=$?; fi\n"   \
	"if [ $r -eq 0 ]; then read -t 10  -p \"LHPX: \" LINE_HEIGHT_PX; r=$?; fi\n"   \
	"if [ $r -eq 0 ]; then read -t 10  -p \"CWPX: \" COLUMN_WIDTH_PX; r=$?; fi\n"  \
	"if [ $r -eq 0 ]; then read -t 10  -p \"ROWS: \" LINES; r=$?; fi\n"            \
	"if [ $r -eq 0 ]; then read -t 10  -p \"COLS: \" COLUMNS; r=$?; fi\n"          \
	"if [ $r -eq 0 ]; then read -t 10  -p \"USER: \" user; r=$?; fi\n"             \
	"if [ $r -eq 0 ]; then read -t 10 -sp \"PASS: \" pass; r=$?; echo; fi\n"       \
	"if [ $r -ne 0 ]; then exit 1; fi\n"                                           \
	"export TERM\n"                                                                \
	"export LINES\n"                                                               \
	"export COLUMNS\n"                                                             \
	"export user\n"                                                                \
	"export pass\n"                                                                \
	"expect -c \"\n"                                                               \
	"	log_user 0\n"                                                              \
	"	set timeout 10\n"                                                          \
	"	spawn env LANG=C su - \\$env(user)\n"                                      \
	"	expect {\n"                                                                \
	"		\\\"Password: \\\" {\n"                                                \
	"			send \\\"\\$env(pass)\\\\n\\\"\n"                                  \
	"		}\n"                                                                   \
	"		eof {\n"                                                               \
	"			send_user \\\"user \\$env(user) does not exist\\\\n\\\"\n"         \
	"			exit 1\n"                                                          \
	"		}\n"                                                                   \
	"	}\n"                                                                       \
	"	expect {\n"                                                                \
	"		-re \\\".*\\[\\$#\\](\\\\x1b\\\\\\\\\\[\\[0-9;\\]*m)* \\$\\\" {\n"     \
	"			send_user \\\"login\\\"\n"                                         \
	"			send \\\" stty rows \\$env(LINES) cols \\$env(COLUMNS)\\\\n\\\"\n" \
	"		}\n"                                                                   \
	"		eof {\n"                                                               \
	"			send_user \\\"authentication failure\\\\n\\\"\n"                   \
	"			exit 1\n"                                                          \
	"		}\n"                                                                   \
	"	}\n"                                                                       \
	"	expect {\n"                                                                \
	"		-re \\\".*\\[\\$#\\](\\\\x1b\\\\\\\\\\[\\[0-9;\\]*m)* \\$\\\" {\n"     \
	"			log_user 1\n"                                                      \
	"			send \\\"\\\\n\\\"\n"                                              \
	"			interact\n"                                                        \
	"		}\n"                                                                   \
	"	}\n"                                                                       \
	"\"\n",                                                                        \
	"/dev/null"
#define _BUFFER_SIZE (4 * 1024)

static int _is_in_primitive_mode = 1;

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
		(st < buffer_size) && ((_get_tick_us() - start_time) < BMS_TIMEOUT_US) ;
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

int bms_main(int term_eventfd, int in_fd, int out_fd)
{
	int ret;
	int is_running = 1;
	int fd;
	int pid;
	struct pollfd fds[3];
	char buffer[_BUFFER_SIZE];
	char line[80];
	ssize_t sst;
	size_t st;
	size_t buffer_size;
	size_t line_len;
	struct bms_term tm[1];
	char const *term_ent = _TERM "\n";
	char term[64];
	char font[64];
	int mode;
	int lhpx;
	int cwpx;
	int rows;
	int cols;
	int user;
	int pass;
	int lgin;
	uint64_t re;
	int fd_fl;
	int in_fd_fl;

	pid = forkpty(&fd, NULL, NULL, NULL);
	if (pid == -1) {
		ret = 1;
		goto FORK_FAILURE_EXIT;
	}
	if (pid == 0) {
		execlp(_EXEC_FILE, _EXEC_ARGS, NULL);
		exit(1);
	}
	ret = 0;

	fd_fl = fcntl(fd, F_GETFL);
	if ((fd_fl == -1) || (fcntl(fd, F_SETFL, fd_fl | O_NONBLOCK)) != 0) {
		ret = 1;
		goto FCNTL_FD_FAILURE_EXIT;
	}

	in_fd_fl = fcntl(in_fd, F_GETFL);
	if ((in_fd_fl == -1) || (fcntl(in_fd, F_SETFL, in_fd_fl | O_NONBLOCK) != 0)) {
		ret = 1;
		goto FCNTL_IN_FD_FAILURE_EXIT;
	}

	term[0] = 0;
	font[0] = 0;
	mode    = 0;
	lhpx    = 0;
	cwpx    = 0;
	rows    = 0;
	cols    = 0;
	user    = 0;
	pass    = 0;
	lgin    = 0;
	line_len = 0;
	while (is_running) {
		if (term[0] == 0) {
			(fds + 0)->fd = term_eventfd;
			(fds + 0)->events = POLLIN;
			(fds + 0)->revents = 0;
			(fds + 1)->fd = fd;
			(fds + 1)->events = POLLIN;
			(fds + 1)->revents = 0;

			poll(fds, 2, -1);

			buffer_size = 0;
		} else {
			(fds + 0)->fd = term_eventfd;
			(fds + 0)->events = POLLIN;
			(fds + 0)->revents = 0;
			(fds + 1)->fd = in_fd;
			(fds + 1)->events = POLLIN;
			(fds + 1)->revents = 0;
			(fds + 2)->fd = fd;
			(fds + 2)->events = POLLIN;
			(fds + 2)->revents = 0;

			poll(fds, 3, -1);

			sst = _read(in_fd, (uint8_t *)buffer, _BUFFER_SIZE);
			if ((sst == -1) || _is_rfd_close(sst)) {
				sst = 0;
				is_running = 0;
			}
			buffer_size = sst;

			sst = _write(fd, (uint8_t *)buffer, buffer_size);
			if (sst == -1) {
				is_running = 0;
			} else {
				fsync(fd);
			}
			buffer_size = 0;
		}

		sst = _read(fd, (uint8_t *)buffer, _BUFFER_SIZE);
		if ((sst == -1) || _is_rfd_close(sst)) {
			sst = 0;
			is_running = 0;
		}
		buffer_size = sst;

		if (_is_in_primitive_mode) {
			if (
				(term[0] == 0) ||
				(font[0] == 0) ||
				(mode    == 0) ||
				(lhpx    == 0) ||
				(cwpx    == 0) ||
				(rows    == 0) ||
				(cols    == 0) ||
				(user    == 0) ||
				(pass    == 0) ||
				(lgin    == 0)
			) {
				for (st = 0 ; st < buffer_size ; st++) {
					line[line_len++] = buffer[st];
					if (line[line_len - 1] == '\n') {
						line[line_len - 1] = '\0';
						if (term[0] == 0) {
							if (strncmp(line, "TERM: ", 6) == 0) {
								term[sizeof(term) - 1] = 0;
#								pragma GCC diagnostic push
#								pragma GCC diagnostic ignored "-Wstringop-truncation"
								strncpy(term, line + 6, sizeof(term));
#								pragma GCC diagnostic pop
								if (term[sizeof(term) - 1] != 0) {
									term[0] = 0;
								}
							}
							if (term[0] != 0) line_len = 0;
						}
						if (font[0] == 0) {
							if (strncmp(line, "FONT: ", 6) == 0) {
								font[sizeof(font) - 1] = 0;
#								pragma GCC diagnostic push
#								pragma GCC diagnostic ignored "-Wstringop-truncation"
								strncpy(font, line + 6, sizeof(font));
#								pragma GCC diagnostic pop
								if (font[sizeof(font) - 1] != 0) {
									font[0] = 0;
								}
							}
							if (font[0] != 0) line_len = 0;
						}
						if (mode == 0) {
							if (sscanf(line, "MODE: %d", &mode) != 1) {
								mode = 0;
							}
							if (mode < 0) mode = 0;
							if (mode != 0) line_len = 0;
						}
						if (lhpx == 0) {
							if (sscanf(line, "LHPX: %d", &lhpx) != 1) {
								lhpx = 0;
							}
							if (lhpx < 0) lhpx = 0;
							if (lhpx != 0) line_len = 0;
						}
						if (cwpx == 0) {
							if (sscanf(line, "CWPX: %d", &cwpx) != 1) {
								cwpx = 0;
							}
							if (cwpx < 0) cwpx = 0;
							if (cwpx != 0) line_len = 0;
						}
						if (rows == 0) {
							if (sscanf(line, "ROWS: %d", &rows) != 1) {
								rows = 0;
							}
							if (rows < 0) rows = 0;
							if (rows != 0) line_len = 0;
						}
						if (cols == 0) {
							if (sscanf(line, "COLS: %d", &cols) != 1) {
								cols = 0;
							}
							if (cols < 0) cols = 0;
							if (cols != 0) line_len = 0;
						}
						if (user == 0) {
							if (strncmp(line, "USER: ", 6) == 0) {
								user = 1;
							}
							if (user != 0) line_len = 0;
						}
						if (pass == 0) {
							if (strncmp(line, "PASS: ", 6) == 0) {
								pass = 1;
							}
							if (pass != 0) line_len = 0;
						}
						if (lgin == 0) {
							if (strncmp(line, "login", 5) == 0) {
								lgin = 1;
							}
							if (lgin != 0) line_len = 0;
						}
						if (0 < line_len) {
							line_len = 0;
						}
					} else if (line[line_len - 1] < '\x20') {
						line_len--;
					} else if ((term[0] == 0) && (line_len == 6) && (strncmp(line, "TERM: ", 6) == 0)) {
						sst = _write(fd, (uint8_t *)term_ent, strlen(term_ent));
						if (sst == -1) {
							is_running = 0;
						} else {
							fsync(fd);
						}
					}
					if (sizeof(line) <= line_len) line_len = sizeof(line) - 1;
				}
			}

			if (lgin == 0) {
				sst = _write(out_fd, (uint8_t *)buffer, buffer_size);
				if (sst == -1) {
					is_running = 0;
				} else {
					fsync(out_fd);
				}
				buffer_size = 0;
			} else {
				for (st = 0 ; st < buffer_size ; st += sst) {
					sst = _write(out_fd, (uint8_t *)buffer + st, 1);
					if (sst == -1) break;
					if (buffer[st] == '\n') {
						st++;
						break;
					}
				}
				if (buffer[st - 1] != '\n') {
					is_running = 0;
					buffer_size = 0;
				} else {
					fsync(out_fd);
					_is_in_primitive_mode = 0;
					if (bms_disp->initialize(out_fd, font, mode, lhpx, cwpx, rows, cols) != 0) {
						ret = 1;
						goto DISP_INIT_FAILURE_EXIT;
					}
					if (
						bms_term->initialize(
							tm,
							rows, cols,
							bms_disp->get_cursor_pos,
							bms_disp->set_cursor_pos,
							bms_disp->put_chars,
							bms_disp->bell
						) != 0
					) {
						ret = 1;
						goto TERM_INIT_FAILURE_EXIT;
					}
					if (st < buffer_size) {
						bms_term->write(tm, buffer + st, buffer_size - st);
						bms_disp->sync();
					}
					buffer_size = 0;
				}
			}
		} else {
			if (0 < buffer_size) {
				bms_term->write(tm, buffer, buffer_size);
				bms_disp->sync();
				buffer_size = 0;
			}
			/*
			 * screen resize sample code
			 */
#			if 0
				struct winsize ws[1] = {{0}};
				bms_disp->set_size(lhpx, cwpx, rows, cols);
				bms_term->set_size(tm, rows, cols);
				ws->ws_row = rows;
				ws->ws_col = cols;
				ioctl(fd, TIOCSWINSZ, &ws);
#			endif
		}

		if (_read(term_eventfd, (uint8_t *)&re, sizeof(re)) == sizeof(re)) {
			is_running = 0;
		}
	}

	if (!_is_in_primitive_mode) bms_term->finalize(tm);
TERM_INIT_FAILURE_EXIT:
	if (!_is_in_primitive_mode) bms_disp->finalize();
DISP_INIT_FAILURE_EXIT:
	fcntl(in_fd, F_SETFL, in_fd_fl);
FCNTL_IN_FD_FAILURE_EXIT:
	fcntl(fd, F_SETFL, fd_fl);
FCNTL_FD_FAILURE_EXIT:
	close(fd);
	if (waitpid(pid, NULL, WNOHANG) == 0) {
		kill(pid, SIGTERM);
		waitpid(pid, NULL, 0);
	}
FORK_FAILURE_EXIT:

	return ret;
}
