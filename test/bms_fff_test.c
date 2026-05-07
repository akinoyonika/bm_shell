#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fontconfig/fontconfig.h>
#include "bms/fff.h"

void bms_fff_test()
{
	struct bms_fff fff[1];
	uint32_t code;
	char const *file_name;
	int index;

	assert(bms_fff->initialize(fff, "Noto Mono") == 0);

	code = 0x00000020;
	assert(0 <= (index = bms_fff->find(fff, code, &file_name)));
	printf("0x%08x: %s (%d)\n", code, file_name, index);
	code = 0x00004e00;
	assert(0 <= (index = bms_fff->find(fff, code, &file_name)));
	printf("0x%08x: %s (%d)\n", code, file_name, index);
	code = 0x0001f600;
	assert(0 <= (index = bms_fff->find(fff, code, &file_name)));
	printf("0x%08x: %s (%d)\n", code, file_name, index);
	code = 0x00002600;
	assert(0 <= (index = bms_fff->find(fff, code, &file_name)));
	printf("0x%08x: %s (%d)\n", code, file_name, index);

	bms_fff->finalize(fff);
}