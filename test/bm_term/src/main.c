#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <signal.h>
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

struct bmt_settings arg_settings[1];
char const *arg_filename;

static int scan_arg(int argc, char const **argv)
{
	int ret = 0;
	char const *p;
	int scan(int argc, char const **argv)
	{
		int ret = 0;
		int i;
		char c = '\0';
		static char buffer[256];
		int fargc;
		char const *fargv[20];
		FILE *file;
		size_t st;
		char *p;
		char quot;

		for (i = 1 ; i < argc ; i++) {
			if (argv[i][0] == '-') {
				c = argv[i][1];
				switch (c) {
				case 'f':
				case 'm':
				case 'h':
				case 'w':
				case 'r':
				case 'c':
				case 'u':
				case 'p':
					break;
				default:
					ret = 1;
					break;
				}
				if (ret != 0) break;
				continue;
			}
			switch (c) {
			case 'f':
				arg_settings->font = argv[i];
				c = '\0';
				break;
			case 'm':
				arg_settings->mode = argv[i];
				c = '\0';
				break;
			case 'h':
				arg_settings->lhpx = argv[i];
				c = '\0';
				break;
			case 'w':
				arg_settings->cwpx = argv[i];
				c = '\0';
				break;
			case 'r':
				arg_settings->rows = argv[i];
				c = '\0';
				break;
			case 'c':
				arg_settings->cols = argv[i];
				c = '\0';
				break;
			case 'u':
				arg_settings->user = argv[i];
				c = '\0';
				break;
			case 'p':
				arg_settings->pass = argv[i];
				c = '\0';
				break;
			default:
				if (arg_filename == NULL) {
					arg_filename = argv[i];
					file = fopen(arg_filename, "r");
					if (file == NULL) {
						ret = 1;
						break;
					}
					st = fread(buffer, 1, sizeof(buffer) - 1, file);
					fclose(file);
					if (st == 0) {
						ret = 1;
						break;
					}
					buffer[st] = '\0';

					for (p = buffer, fargc = 1, fargv[0] = NULL, quot = '\0' ; (p - buffer) < sizeof(buffer) ; p++) {
						if (*p == '\0') {
							break;
						} else if ((*p == '\'') || (*p == '"')) {
							if (quot == '\0') {
								quot = *p;
								*p = '\0';
							} else if (quot == *p) {
								quot = '\0';
								*p = '\0';
							}
						} else if (*p <= ' ') {
							if (quot == '\0') {
								*p = '\0';
							}
						}

						if (
							((p == buffer) || (*(p - 1) == '\0')) &&
							(*p != '\0')
						) {
							if (fargc < (sizeof(fargv) / sizeof(fargv[0]))) {
								fargv[fargc++] = p;
							}
						}
					}

					ret = scan(fargc, fargv);
					arg_filename = NULL;
				}
				break;
			}
			if (ret != 0) break;
		}

		return ret;
	}

	arg_settings->font = "mono";
	arg_settings->mode = "3";
	arg_settings->lhpx = "16";
	arg_settings->cwpx = "8";
	arg_settings->rows = "24";
	arg_settings->cols = "80";
	arg_settings->user = "";
	arg_settings->pass = "";
	arg_filename = NULL;

	ret = scan(argc, argv);

	if (ret != 0) {
		p = strrchr(argv[0], '/');
		if (p == NULL) {
			p = argv[0];
		} else {
			p++;
		}
		printf(
			"%s "
				"[<option file name>] "
				"[-f <font>] "
				"[-m < 1 | 2 | 3 | 128 | 129 | 130 >] "
				"[-h <line height>] "
				"[-w <char width>] "
				"[-r <rows>] "
				"[-c <columns>] "
				"[-u <user name>] "
				"[-p <password>]\n",
			p
		);
		printf("%s (C) 2026 akinoyonika\n", p);
	}

	return ret;
}

int main(int argc, char const **argv)
{
	int ret = 0;

	if (scan_arg(argc, argv) != 0) {
		ret = 1;
		goto EXIT;
	}

	_term_eventfd = eventfd(0, EFD_NONBLOCK);
	if (_term_eventfd == -1) {
		ret = 1;
		goto EXIT;
	}

	signal(SIGINT, _sigh);
	signal(SIGTERM, _sigh);

	ret = bmt_main(_term_eventfd, STDIN_FILENO, STDOUT_FILENO, arg_settings);

	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	close(_term_eventfd);

EXIT:

	return ret;
}
