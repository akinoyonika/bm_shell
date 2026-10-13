#ifndef __BMT_DISP_H__
#define __BMT_DISP_H__

struct bmt_disp_ops {
	int (*begin)(int w, int h);
	void (*end)(void);
	void (*update)(int x, int y, int w, int h, uint8_t *bitmap);
	void (*present)(void);
	void (*ring)(void);
};

extern struct bmt_disp_ops const bmt_disp[];

#endif /* __BMT_DISP_H__ */
