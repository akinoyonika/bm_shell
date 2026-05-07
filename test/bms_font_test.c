#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "bms/font.h"

void bms_font_test()
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	struct bms_font font[8];
	int const line_height = 64;
	int const font_size = line_height * 3 / 4;
	uint8_t fg_color[3];
	uint8_t bg_color[3];
	void put_code(
		uint8_t const *fg_color,
		uint8_t const *bg_color,
		uint32_t code,
		SDL_Renderer *renderer,
		SDL_Texture *texture,
		int line_height
	)
	{
		uint8_t bitmap[line_height * line_height * 3];
		int get(
			struct bms_font *this,
			uint8_t const *fg_color,
			uint8_t const *bg_color,
			uint32_t code,
			uint8_t *bitmap,
			int height,
			int width
		) {
			int i;
			for (i = 0 ; i < line_height * line_height * 3 ; i += 3) {
				memcpy(bitmap + i, bg_color, 3);
			}
			return bms_font->get(this, fg_color, bg_color, code, bitmap, height, width);
		}
		assert(
			(get(font + 0, fg_color, bg_color, code, bitmap, line_height, line_height) == 0) ||
			(get(font + 1, fg_color, bg_color, code, bitmap, line_height, line_height) == 0) ||
			(get(font + 2, fg_color, bg_color, code, bitmap, line_height, line_height) == 0) ||
			(get(font + 3, fg_color, bg_color, code, bitmap, line_height, line_height) == 0) ||
			(get(font + 4, fg_color, bg_color, code, bitmap, line_height, line_height) == 0) ||
			(get(font + 5, fg_color, bg_color, code, bitmap, line_height, line_height) == 0)
		);
		SDL_UpdateTexture(texture, NULL, bitmap, line_height * 3);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(100);
	}

	assert(SDL_Init(SDL_INIT_VIDEO) == 0);

	window = SDL_CreateWindow("mbs_font_test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, line_height, line_height, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, line_height, line_height);

	assert(bms_font->initialize(font + 0, "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 0, font_size) == 0);
	assert(bms_font->initialize(font + 1, "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", 0, font_size) == 0);
	assert(bms_font->initialize(font + 2, "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf", 0, font_size) == 0);
	assert(bms_font->initialize(font + 3, "/usr/share/fonts/truetype/noto/NotoSansSymbols-Regular.ttf", 0, font_size) == 0);
	assert(bms_font->initialize(font + 4, "/usr/share/fonts/truetype/noto/NotoSansSymbols2-Regular.ttf", 0, font_size) == 0);
	assert(bms_font->initialize(font + 5, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0, font_size) == 0);

	fg_color[BMS_FONT_R] = 0xff;
	fg_color[BMS_FONT_G] = 0xff;
	fg_color[BMS_FONT_B] = 0xff;
	bg_color[BMS_FONT_R] = 0x00;
	bg_color[BMS_FONT_G] = 0x00;
	bg_color[BMS_FONT_B] = 0x00;
	for (uint32_t code = 0x00000020 ; code < 0x0000007f ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x00004e00 ; code < 0x00004e80 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x0001f550 ; code < 0x0001f568 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x0001f600 ; code < 0x0001f650 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x00002600 ; code < 0x00002700 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);

	fg_color[BMS_FONT_R] = 0x00;
	fg_color[BMS_FONT_G] = 0x00;
	fg_color[BMS_FONT_B] = 0x00;
	bg_color[BMS_FONT_R] = 0xff;
	bg_color[BMS_FONT_G] = 0xff;
	bg_color[BMS_FONT_B] = 0xff;
	//for (uint32_t code = 0x00000020 ; code < 0x0000007f ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	for (uint32_t code = 0x00004e00 ; code < 0x00004e80 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x0001f550 ; code < 0x0001f568 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x0001f600 ; code < 0x0001f650 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x00002600 ; code < 0x00002700 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);

	fg_color[BMS_FONT_R] = 0xff;
	fg_color[BMS_FONT_G] = 0xff;
	fg_color[BMS_FONT_B] = 0xff;
	bg_color[BMS_FONT_R] = 0x00;
	bg_color[BMS_FONT_G] = 0xff;
	bg_color[BMS_FONT_B] = 0x00;
	//for (uint32_t code = 0x00000020 ; code < 0x0000007f ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x00004e00 ; code < 0x00004e80 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	for (uint32_t code = 0x0001f550 ; code < 0x0001f568 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	for (uint32_t code = 0x0001f600 ; code < 0x0001f650 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x00002600 ; code < 0x00002700 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);

	fg_color[BMS_FONT_R] = 0x00;
	fg_color[BMS_FONT_G] = 0xff;
	fg_color[BMS_FONT_B] = 0x00;
	bg_color[BMS_FONT_R] = 0xff;
	bg_color[BMS_FONT_G] = 0xff;
	bg_color[BMS_FONT_B] = 0xff;
	//for (uint32_t code = 0x00000020 ; code < 0x0000007f ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x00004e00 ; code < 0x00004e80 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x0001f550 ; code < 0x0001f568 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	//for (uint32_t code = 0x0001f600 ; code < 0x0001f650 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);
	for (uint32_t code = 0x00002600 ; code < 0x00002700 ; code++) put_code(fg_color, bg_color, code, renderer, texture, line_height);

	bms_font->finalize(font + 5);
	bms_font->finalize(font + 4);
	bms_font->finalize(font + 3);
	bms_font->finalize(font + 2);
	bms_font->finalize(font + 1);
	bms_font->finalize(font + 0);

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}
