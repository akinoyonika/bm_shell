#ifndef __BMT_MAIN_H__
#define __BMT_MAIN_H__

#define BMT_TIMEOUT_US (1000000)

struct bmt_settings {
	char const *font;
	char const *mode;
	char const *lhpx;
	char const *cwpx;
	char const *rows;
	char const *cols;
	char const *user;
	char const *pass;
};

int bmt_main(int term_eventfd, int in_fd, int out_fd, struct bmt_settings *settings);

#endif /* __BMT_MAIN_H__ */
