#include <unistd.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include "bmt/key.h"

static int _begin(void (*write)(uint8_t const *buffer, size_t buffer_size, void *param), void *write_param);
static void _end(void);

struct bmt_key_ops const bmt_key[] = {{
	_begin,
	_end
}};

static pthread_t _key_th;
static struct _key_arg {
	int term_event_type;
	void (*write)(uint8_t const *buffer, size_t buffer_size, void *param);
	void *write_param;
} _key_arg[1];

static void *_key(void *param)
{
	struct _key_arg *arg = (struct _key_arg *)param;
	int is_running = 1;
	SDL_Event event[1];
	while (is_running) {
		if (!SDL_WaitEvent(event)) continue;
		if (event->type == arg->term_event_type) is_running = 0;
		if (
			(event->type == SDL_WINDOWEVENT) &&
			(event->window.event == SDL_WINDOWEVENT_EXPOSED)
		) {
			arg->write(NULL, 0, arg->write_param);
		}
		if (event->type == SDL_TEXTINPUT) {
			arg->write((uint8_t *)(event->text.text), strlen(event->text.text), arg->write_param);
		}
		if (event->type == SDL_KEYDOWN) {
			char *s;
			switch (event->key.keysym.sym) {
			case SDLK_BACKSPACE:
				s = "\x08";
				break;
			case SDLK_TAB:
				s = "\x09";
				break;
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				s = "\x0d";
				break;
			case SDLK_ESCAPE:
				s = "\x1b";
				break;
			case SDLK_INSERT:
				s = "\x1b[2~";
				break;
			case SDLK_DELETE:
				s = "\x1b[3~";
				break;
			case SDLK_HOME:
				s = "\x1b[H";
				break;
			case SDLK_END:
				s = "\x1b[F";
				break;
			case SDLK_PAGEUP:
				s = "\x1b[5~";
				break;
			case SDLK_PAGEDOWN:
				s = "\x1b[6~";
				break;
			case SDLK_UP:
				s = "\x1b[A";
				break;
			case SDLK_DOWN:
				s = "\x1b[B";
				break;
			case SDLK_RIGHT:
				s = "\x1b[C";
				break;
			case SDLK_LEFT:
				s = "\x1b[D";
				break;
			case SDLK_F1:
				s = "\x1b[OP";
				break;
			case SDLK_F2:
				s = "\x1b[OQ";
				break;
			case SDLK_F3:
				s = "\x1b[OR";
				break;
			case SDLK_F4:
				s = "\x1b[OS";
				break;
			case SDLK_F5:
				s = "\x1b[15~";
				break;
			case SDLK_F6:
				s = "\x1b[17~";
				break;
			case SDLK_F7:
				s = "\x1b[18~";
				break;
			case SDLK_F8:
				s = "\x1b[19~";
				break;
			case SDLK_F9:
				s = "\x1b[20~";
				break;
			case SDLK_F10:
				s = "\x1b[21~";
				break;
			case SDLK_F11:
				s = "\x1b[23~";
				break;
			case SDLK_F12:
				s = "\x1b[24~";
				break;
			default:
				s = "";
				if (event->key.keysym.mod & KMOD_CTRL) {
					switch (event->key.keysym.sym) {
					case SDLK_a:
						s = "\x01";
						break;
					case SDLK_b:
						s = "\x02";
						break;
					case SDLK_c:
						s = "\x03";
						break;
					case SDLK_d:
						s = "\x04";
						break;
					case SDLK_e:
						s = "\x05";
						break;
					case SDLK_f:
						s = "\x06";
						break;
					case SDLK_g:
						s = "\x07";
						break;
					case SDLK_h:
						s = "\x08";
						break;
					case SDLK_i:
						s = "\x09";
						break;
					case SDLK_j:
						s = "\x0a";
						break;
					case SDLK_k:
						s = "\x0b";
						break;
					case SDLK_l:
						s = "\x0c";
						break;
					case SDLK_m:
						s = "\x0d";
						break;
					case SDLK_n:
						s = "\x0e";
						break;
					case SDLK_o:
						s = "\x0f";
						break;
					case SDLK_p:
						s = "\x10";
						break;
					case SDLK_q:
						s = "\x11";
						break;
					case SDLK_r:
						s = "\x12";
						break;
					case SDLK_s:
						s = "\x13";
						break;
					case SDLK_t:
						s = "\x14";
						break;
					case SDLK_u:
						s = "\x15";
						break;
					case SDLK_v:
						s = "\x16";
						break;
					case SDLK_w:
						s = "\x17";
						break;
					case SDLK_x:
						s = "\x18";
						break;
					case SDLK_y:
						s = "\x19";
						break;
					case SDLK_z:
						s = "\x1a";
						break;
					default:
						s = "";
						break;
					}
				}
				break;
			}
			arg->write((uint8_t *)s, strlen(s), arg->write_param);
		}
	}
	return NULL;
}

static int _begin(void (*write)(uint8_t const *buffer, size_t buffer_size, void *param), void *write_param)
{
	int ret = 0;

	SDL_StartTextInput();

	_key_arg->term_event_type = SDL_RegisterEvents(1);
	if (_key_arg->term_event_type == 0xffffffff) {
		ret = 1;
		goto SDL_REGISTER_EVENT_FAILURE_EXIT;
	}
	_key_arg->write_param = write_param;
	_key_arg->write = write;
	if (pthread_create(&_key_th, NULL, _key, _key_arg) != 0) {
		ret = 1;
		goto PTHREAD_CREATE_FAILURE_EXIT;
	}

PTHREAD_CREATE_FAILURE_EXIT:
SDL_REGISTER_EVENT_FAILURE_EXIT:
	if (ret != 0) SDL_StopTextInput();

	return ret;
}

static void _end()
{
	SDL_Event event[1];

	SDL_zero(*event);
	event->type = _key_arg->term_event_type;
	SDL_PushEvent(event);
	pthread_join(_key_th, NULL);
	SDL_StopTextInput();
}
