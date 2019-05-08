#ifndef OPENRCT2_UI_SWITCH_VIDEO_H
#define OPENRCT2_UI_SWITCH_VIDEO_H

#include <SDL2/SDL.h>

bool switch_changed_resolution(SDL_Window *window);
void switch_update_game_canvas_size(int width, int height);
void switch_get_resolution(int *width, int *height);

#endif //OPENRCT2_SWITCH_VIDEO_H
