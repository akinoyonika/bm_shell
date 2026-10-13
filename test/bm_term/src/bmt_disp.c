#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include "bmt/disp.h"

static int _begin(int w, int h);
static void _end(void);
static void _update(int x, int y, int w, int h, uint8_t *bitmap);
static void _present(void);
static void _ring(void);

struct bmt_disp_ops const bmt_disp[] = {{
	_begin,
	_end,
	_update,
	_present,
	_ring
}};

static SDL_Window *_window;
static SDL_Renderer *_renderer;
static SDL_Texture *_texture;

static int _begin(int w, int h)
{
	int ret = 0;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		goto SDL_INIT_FAILURE_EXIT;
	}
	_window = SDL_CreateWindow("bm_term", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
	if (_window == NULL) {
		ret = 1;
		goto SDL_CREATE_WINDOW_FAILURE_EXIT;
	}
	_renderer = SDL_CreateRenderer(_window, -1, 0);
	if (_renderer == NULL) {
		ret = 1;
		goto SDL_CREATE_RENDERER_FAILURE_EXIT;
	}
	_texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, w, h);
	if (_texture == NULL) {
		ret = 1;
		goto SDL_CREATE_TEXTURE_FAILURE_EXIT;
	}

SDL_CREATE_TEXTURE_FAILURE_EXIT:	
	if (ret != 0) SDL_DestroyRenderer(_renderer);
SDL_CREATE_RENDERER_FAILURE_EXIT:
	if (ret != 0) SDL_DestroyWindow(_window);
SDL_CREATE_WINDOW_FAILURE_EXIT:
	if (ret != 0) SDL_Quit();
SDL_INIT_FAILURE_EXIT:

	return ret;
}

static void _end()
{
	SDL_DestroyTexture(_texture);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();
}

static void _update(int x, int y, int w, int h, uint8_t *bitmap)
{
	SDL_Rect rect[1];

	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = h;
	SDL_UpdateTexture(_texture, rect, bitmap, w * 3);
}

static void _present()
{
	SDL_RenderCopy(_renderer, _texture, NULL, NULL);
	SDL_RenderPresent(_renderer);
}

static void _ring()
{
	char a[1] = {'\a'};
	int fd = open("/dev/tty", O_WRONLY);
	if (fd != -1) {
		while ((write(fd, a, 1) == -1) && (errno == EAGAIN));
		fsync(fd);
		close(fd);
	}
}
