#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <termios.h>
#include "bms/main.h"
#include "bmt/main.h"

static char _FONT_[] = "NotoSansMono";
static char _MODE_[] = "3";
static char _LHPX_[] = "16";
static char _CWPX_[] = "8";
static char _ROWS_[] = "24";
static char _COLS_[] = "80";

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

void bms_main_test()
{
	struct termios termios[1];
	struct termios org_termios[1];
	pthread_t th;
	int from_fd[2];
	int to_fd[2];
	uint64_t const se = 1;
	struct th_arg {
		struct bmt_settings settings[1];
		int term_eventfd;
		int in_fd;
		int out_fd;
	} th_arg[1];
	void *term(void *param)
	{
		struct th_arg *arg = (struct th_arg *)param;
		bmt_main(arg->term_eventfd, arg->in_fd, arg->out_fd, arg->settings);
		return NULL;
	}

	th_arg->settings->font = _FONT_;
	th_arg->settings->mode = _MODE_;
	th_arg->settings->lhpx = _LHPX_;
	th_arg->settings->cwpx = _CWPX_;
	th_arg->settings->rows = _ROWS_;
	th_arg->settings->cols = _COLS_;
	printf("USER: ");
	if (scanf("%m[^\n]s", &(th_arg->settings->user)) < 1) th_arg->settings->user = NULL;
	(void)getchar();
	printf("PASS: ");
	tcgetattr(STDIN_FILENO, org_termios);
	memcpy(termios, org_termios, sizeof(*termios));
	termios->c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, termios);
	if (scanf("%m[^\n]s", &(th_arg->settings->pass)) < 1) th_arg->settings->pass = NULL;
	(void)getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, org_termios);
	printf("\n");

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

	assert(bms_main(_term_eventfd, to_fd[0], from_fd[1]) == 0);

	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	close(_term_eventfd);

	_write(th_arg->term_eventfd, (uint8_t *)&se, sizeof(se));
	pthread_join(th, NULL);

	close(from_fd[0]);
	close(from_fd[1]);
	close(to_fd[0]);
	close(to_fd[1]);
	if (th_arg->settings->user != NULL) free((void *)(th_arg->settings->user));
	if (th_arg->settings->pass != NULL) free((void *)(th_arg->settings->pass));
}
