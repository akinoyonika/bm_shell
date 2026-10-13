#ifndef __BMT_KEY_H__
#define __BMT_KEY_H__

struct bmt_key_ops {
	int (*begin)(void (*write)(uint8_t const *buffer, size_t buffer_size, void *param), void *write_param);
	void (*end)(void);
};

extern struct bmt_key_ops const bmt_key[];

#endif /* __BMT_KEY_H__ */
