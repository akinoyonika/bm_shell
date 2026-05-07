#include <stdio.h>
#include <string.h>

extern void bms_fff_test(void);
extern void bms_font_test(void);
extern void bms_disp_test(void);
extern void bms_main_test(void);

int main (int argc, char **argv)
{
	int i;

	if (argc < 2) {
		bms_fff_test();
		bms_font_test();
		bms_disp_test();
		bms_main_test();
	} else {
		for (i = 1 ; i < argc ; i++) {
			if        (strcmp("fff", argv[i]) == 0)  {
				bms_fff_test();
			} else if (strcmp("font", argv[i]) == 0) {
				bms_font_test();
			} else if (strcmp("disp", argv[i]) == 0) {
				bms_disp_test();
			} else if (strcmp("main", argv[i]) == 0) {
				bms_main_test();
			}
		}
	}

	return 0;
}
