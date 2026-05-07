#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <fontconfig/fontconfig.h>
#include "bms/fff.h"

static int _initialize(struct bms_fff *this, char const *name);
static void _finalize(struct bms_fff *this);
static int _find(struct bms_fff *this, uint32_t code, char const **file_name);

struct bms_fff_ops const bms_fff[] = {{
	_initialize,
	_finalize,
	_find
}};

static unsigned num_init = 0;

static int _initialize(struct bms_fff *this, char const *name)
{
	int ret = 0;
	FcResult result;

	if (num_init == 0) {
		if (!FcInit()) {
			ret = 1;
			goto INIT_FAIL_EXIT;
		}
	}
	num_init++;

	this->fcp = FcNameParse((const FcChar8*)name);
	if (this->fcp == NULL) {
		ret = 1;
		goto PARSE_FAIL_EXIT;
	}

	this->fcfs = FcFontSort(NULL, this->fcp, FcTrue, NULL, &result);
	if (this->fcfs == NULL) {
		ret = 1;
		goto SORT_FAIL_EXIT;
	}

	if (result != FcResultMatch) {
		ret = 1;
	}

	if (ret != 0) FcFontSetDestroy(this->fcfs);
SORT_FAIL_EXIT:
	if (ret != 0) FcPatternDestroy(this->fcp);
PARSE_FAIL_EXIT:
	if ((ret != 0) && (0 < num_init) && ((--num_init) == 0)) FcFini();
INIT_FAIL_EXIT:
	return ret;
}

static void _finalize(struct bms_fff *this)
{
	if (0 < num_init) {
		FcFini();
		num_init--;
	}
}

static int _find(struct bms_fff *this, uint32_t code, char const **file_name)
{
	int ret = -1;
	int i;
	FcChar8 *fn;
	int idx;
	FcCharSet *cs;

	for (i = 0 ; i < this->fcfs->nfont ; i++) {
		if (
			(FcPatternGetString(this->fcfs->fonts[i], FC_FILE, 0, &fn) == FcResultMatch) &&
			(FcPatternGetInteger(this->fcfs->fonts[i], FC_INDEX, 0, &idx) == FcResultMatch) &&
			(FcPatternGetCharSet(this->fcfs->fonts[i], FC_CHARSET, 0, &cs) == FcResultMatch) &&
			FcCharSetHasChar(cs, code)
		) {
			*file_name = (char *)fn;
			ret = idx;
			break;
		}
	}

	return ret;
}
