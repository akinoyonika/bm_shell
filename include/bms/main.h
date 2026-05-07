#ifndef __BMS_MAIN_H__
#define __BMS_MAIN_H__

#define BMS_TIMEOUT_US (1000000)

int bms_main(int term_eventfd, int in_fd, int out_fd);

#endif /* __BMS_MAIN_H__ */
