#ifndef __BMS_FFF_H__
#define __BMS_FFF_H__

struct bms_fff {
	FcPattern *fcp;
	FcFontSet *fcfs;
};

struct bms_fff_ops {
	int (*initialize)(struct bms_fff *this, char const *name);
	void (*finalize)(struct bms_fff *this);
	int (*find)(struct bms_fff *this, uint32_t code, char const **file_name);
};

extern struct bms_fff_ops const bms_fff[];

#endif /* __BMS_FFF_H__ */
