#ifndef OPENRCT2_UI_SWITCH_INPUT_H
#define OPENRCT2_UI_SWITCH_INPUT_H

#include <SDL2/SDL.h>

int switch_poll_event(SDL_Event *event);
void switch_handle_analog_sticks(void);
void switch_handle_virtual_keyboard(void);
void switch_handle_repeat_keys(void);

#endif /* OPENRCT2_UI_SWITCH_INPUT_H */
