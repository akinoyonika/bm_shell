#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include "bms/main.h"

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

int arg_is_test;

static int scan_arg(int argc, char const **argv)
{
	int i;
	char const *p;
	int ret = 0;
	char c = '\0';

	arg_is_test = 0;

	for (i = 1 ; i < argc ; i++) {
		if (argv[i][0] == '-') {
			c = argv[i][1];
			switch (c) {
			case 't':
				arg_is_test = 1;
				c = '\0';
				break;
			default:
				ret = 1;
				break;
			}
			if (ret != 0) break;
			continue;
		}
		switch (c) {
		default:
			ret = 1;
			break;
		}
		if (ret != 0) break;
	}

	if (ret != 0) {
		p = strrchr(argv[0], '/');
		if (p == NULL) {
			p = argv[0];
		} else {
			p++;
		}
		printf("%s [-t (test mode)]\n", p);
		printf("%s (C) 2026 akinoyonika\n", p);
	}

	return ret;
}
int main(int argc, char const **argv)
{
	int ret;
	int out_fd;
	struct termios termios[1];
	struct termios org_termios[1];

	if (scan_arg(argc, argv) != 0) {
		ret = 1;
		goto EXIT;
	}

	out_fd = STDOUT_FILENO;
	if (arg_is_test) {
		out_fd = 0;
		tcgetattr(STDIN_FILENO, org_termios);
		memcpy(termios, org_termios, sizeof(*termios));
		termios->c_lflag &= ~ISIG;
		termios->c_lflag &= ~ICANON;
		termios->c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, termios);
	}

	_term_eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (_term_eventfd == -1) {
		ret = 1;
		goto EXIT;
	}

	signal(SIGINT, _sigh);
	signal(SIGTERM, _sigh);
	signal(SIGPIPE, SIG_IGN);

	ret = bms_main(_term_eventfd, STDIN_FILENO, out_fd);

	signal(SIGPIPE, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	close(_term_eventfd);

EXIT:

	if (arg_is_test) {
		tcsetattr(STDIN_FILENO, TCSANOW, org_termios);
	}

	return ret;
}