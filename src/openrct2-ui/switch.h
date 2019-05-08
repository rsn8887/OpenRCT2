#ifndef OPENRCT2_UI_SWITCH_H
#define OPENRCT2_UI_SWITCH_H

// These widths represent the game resolution (window size set in config.ini)
#define SWITCH_DISPLAY_WIDTH game_canvas_width
#define SWITCH_DISPLAY_HEIGHT game_canvas_height

enum {
    TOUCH_MODE_TOUCHPAD             = 0, // drag the pointer and tap-click like on a touchpad (default)
    TOUCH_MODE_DIRECT               = 1, // pointer jumps to finger but doesn't click on tap
    TOUCH_MODE_ORIGINAL             = 2, // original julius touch mode as on any other system: pointer jumps to finger and clicks on tap
    NUM_TOUCH_MODES                 = 3
};

extern int game_canvas_width; // defined in switch_video.cpp
extern int game_canvas_height; // defined in switch_video.cpp
extern int last_mouse_x; // defined in switch_input.cpp
extern int last_mouse_y; // defined in switch_input.cpp
extern int touch_mode; // defined in switch_input.cpp

#endif // OPENRCT2_UI_SWITCH_H