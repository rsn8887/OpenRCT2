#include "openrct2-ui/switch_video.h"
#include "openrct2-ui/switch.h"
#include <switch.h>

int game_canvas_width = 960;
int game_canvas_height = 540;
static int currently_docked = -1;
static int display_width;
static int display_height;

static int isDocked();

int isDocked()
{
    switch (appletGetOperationMode()) {
        case AppletOperationMode_Handheld:
            return 0;
        case AppletOperationMode_Console:
            return 1;
        default:
            return 0;
    }
}

bool switch_changed_resolution(SDL_Window *window)
{
    int docked = isDocked();
    if ((docked && !currently_docked) || (!docked && currently_docked)) {
        // docked mode has changed, update window size etc.
        if (docked) {
            display_width = 1920;
            display_height = 1080;
            currently_docked = 1;
        } else {
            display_width = 1280;
            display_height = 720;
            currently_docked = 0;
        }
        if (window) {
            SDL_SetWindowSize(window, display_width, display_height);
        }
        return true;
    } else {
        return false;
    }
}

void switch_get_resolution(int *width, int *height)
{
    if (currently_docked == -1) {
        if (isDocked()) {
            display_width = 1920;
            display_height = 1080;
            currently_docked = 1;
        } else {
            display_width = 1280;
            display_height = 720;
            currently_docked = 0;
        }
    }
    *width = display_width;
    *height = display_height;
}

void switch_update_game_canvas_size(int width, int height)
{
    // touch input and joystick input needs these values
    game_canvas_width = width;
    game_canvas_height = height;
}