#include "openrct2-ui/switch_input.h"
#include "openrct2-ui/switch_touch.h"
#include <openrct2-ui/switch_keyboard.h>
#include "openrct2-ui/switch.h"

#include <switch.h>
#include <math.h>

#define NO_MAPPING 255

enum {
    SWITCH_PAD_A        = 0,
    SWITCH_PAD_B        = 1,
    SWITCH_PAD_X        = 2,
    SWITCH_PAD_Y        = 3,
    SWITCH_PAD_LSTICK   = 4,
    SWITCH_PAD_RSTICK   = 5,
    SWITCH_PAD_L        = 6,
    SWITCH_PAD_R        = 7,
    SWITCH_PAD_ZL       = 8,
    SWITCH_PAD_ZR       = 9,
    SWITCH_PAD_PLUS     = 10,
    SWITCH_PAD_MINUS    = 11,
    SWITCH_PAD_LEFT     = 12,
    SWITCH_PAD_UP       = 13,
    SWITCH_PAD_RIGHT    = 14,
    SWITCH_PAD_DOWN     = 15,
    SWITCH_NUM_BUTTONS  = 16
};

int last_mouse_x = 0;
int last_mouse_y = 0;
int touch_mode = TOUCH_MODE_TOUCHPAD;
static bool can_change_touch_mode = true;
static bool rjoy_previous_up = false;
static bool rjoy_previous_down = false;
static bool rjoy_previous_left = false;
static bool rjoy_previous_right = false;
static uint64_t last_joy_update_time = 0;
static SDL_Joystick *joy = NULL;
static int hires_dx = 0; // sub-pixel-precision counters to allow slow pointer motion of <1 pixel per frame
static int hires_dy = 0;
static int vkbd_requested = 0;
static int fast_mouse = 0;
static int slow_mouse = 0;
static int holding_modifier = 0;
static SDL_Keycode map_switch_button_to_sdlkey[SWITCH_NUM_BUTTONS][2] =
{
    // entry 0: not holding modifier
    // entry 1: holding modifier
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_A
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_B
    SDLK_LCTRL,     SDLK_LCTRL,             // SWITCH_PAD_X
    SDLK_LSHIFT,    SDLK_LSHIFT,            // SWITCH_PAD_Y
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_LSTICK
    SDLK_LALT,      SDLK_LALT,              // SWITCH_PAD_RSTICK this triggers alt-ctrl-c for cheat menu
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_L
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_R
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_ZL
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_ZR
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_PLUS
    SDLK_UNKNOWN,   SDLK_UNKNOWN,           // SWITCH_PAD_MINUS
    SDLK_z,         SDLK_BACKSPACE,         // SWITCH_PAD_LEFT
    SDLK_PAGEUP,    SDLK_PAGEUP,            // SWITCH_PAD_UP
    SDLK_RETURN,    SDLK_ESCAPE,            // SWITCH_PAD_RIGHT
    SDLK_PAGEDOWN,  SDLK_PAGEDOWN,          // SWITCH_PAD_DOWN
};

static SDL_Scancode map_switch_button_to_sdlscancode[SWITCH_NUM_BUTTONS][2] =
{
    // entry 0: not holding modifier
    // entry 1: holding modifier
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_A
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_B
    SDL_SCANCODE_LCTRL,     SDL_SCANCODE_LCTRL,    // SWITCH_PAD_X
    SDL_SCANCODE_LSHIFT,    SDL_SCANCODE_LSHIFT,   // SWITCH_PAD_Y
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_LSTICK
    SDL_SCANCODE_LALT,      SDL_SCANCODE_LALT,     // SWITCH_PAD_RSTICK this triggers alt-ctrl-c for cheat menu
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_L
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_R
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_ZL
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_ZR
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_PLUS
    SDL_SCANCODE_UNKNOWN,   SDL_SCANCODE_UNKNOWN,  // SWITCH_PAD_MINUS
    SDL_SCANCODE_Z,         SDL_SCANCODE_BACKSPACE,// SWITCH_PAD_LEFT
    SDL_SCANCODE_PAGEUP,    SDL_SCANCODE_PAGEUP,   // SWITCH_PAD_UP
    SDL_SCANCODE_RETURN,    SDL_SCANCODE_ESCAPE,   // SWITCH_PAD_RIGHT
    SDL_SCANCODE_PAGEDOWN,  SDL_SCANCODE_PAGEDOWN, // SWITCH_PAD_DOWN
};

static uint8_t map_switch_button_to_sdlmousebutton[SWITCH_NUM_BUTTONS] =
{
    SDL_BUTTON_LEFT,    // SWITCH_PAD_A
    SDL_BUTTON_RIGHT,   // SWITCH_PAD_B
    NO_MAPPING,         // SWITCH_PAD_X
    NO_MAPPING,         // SWITCH_PAD_Y
    NO_MAPPING,         // SWITCH_PAD_LSTICK
    NO_MAPPING,         // SWITCH_PAD_RSTICK
    SDL_BUTTON_RIGHT,   // SWITCH_PAD_L
    SDL_BUTTON_LEFT,    // SWITCH_PAD_R
    NO_MAPPING,         // SWITCH_PAD_ZL
    NO_MAPPING,         // SWITCH_PAD_ZR
    NO_MAPPING,         // SWITCH_PAD_PLUS
    NO_MAPPING,         // SWITCH_PAD_MINUS
    NO_MAPPING,         // SWITCH_PAD_LEFT
    NO_MAPPING,         // SWITCH_PAD_UP
    NO_MAPPING,         // SWITCH_PAD_RIGHT
    NO_MAPPING          // SWITCH_PAD_DOWN
};

static void switch_start_text_input(char *initial_text, int multiline);
static void switch_rescale_analog(int *x, int *y, int dead);
static void switch_button_to_sdlkey_event(int switch_button, SDL_Event *event, uint32_t event_type);
static void switch_button_to_sdlmouse_event(int switch_button, SDL_Event *event, uint32_t event_type);
static void switch_create_and_push_sdlkey_event(uint32_t event_type, SDL_Scancode scan, SDL_Keycode key);

int switch_poll_event(SDL_Event *event)
{
    int ret = SDL_PollEvent(event);
    if (event != NULL) {
        if (touch_mode != TOUCH_MODE_ORIGINAL) {
            switch_handle_touch(event);
        }
        switch (event->type) {
            case SDL_MOUSEMOTION:
                // update joystick / touch mouse coords
                last_mouse_x = event->motion.x;
                last_mouse_y = event->motion.y;
                break;
            case SDL_JOYBUTTONDOWN:
                if (event->jbutton.which != 0) { // Only Joystick 0 controls the game
                    break;
                }
                switch (event->jbutton.button) {
                    case SWITCH_PAD_Y:
                    case SWITCH_PAD_X:
                    case SWITCH_PAD_UP:
                    case SWITCH_PAD_DOWN:
                    case SWITCH_PAD_LEFT:
                    case SWITCH_PAD_RIGHT: // intentional fallthrough
                        switch_button_to_sdlkey_event(event->jbutton.button, event, SDL_KEYDOWN);
                        break;
                    case SWITCH_PAD_B:
                    case SWITCH_PAD_R:
                    case SWITCH_PAD_A:
                    case SWITCH_PAD_L: // intentional fallthrough
                        switch_button_to_sdlmouse_event(event->jbutton.button, event, SDL_MOUSEBUTTONDOWN);
                        break;
                    case SWITCH_PAD_RSTICK:
                        switch_button_to_sdlkey_event(event->jbutton.button, event, SDL_KEYDOWN);
                        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_LCTRL, SDLK_LCTRL);
                        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_C, SDLK_c);
                        break;
                    case SWITCH_PAD_ZL:
                        holding_modifier = 1;
                        fast_mouse = 1;
                        hires_dx = 0;
                        hires_dy = 0;
                        break;
                    case SWITCH_PAD_ZR:
                        slow_mouse = 1;
                        hires_dx = 0;
                        hires_dy = 0;
                        break;
                    case SWITCH_PAD_PLUS:
                        vkbd_requested = 1;
                        break;
                    case SWITCH_PAD_MINUS:
                        if (can_change_touch_mode) {
                            touch_mode++;
                            touch_mode %= NUM_TOUCH_MODES;
                            can_change_touch_mode = false;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case SDL_JOYBUTTONUP:
                if (event->jbutton.which != 0) { // Only Joystick 0 controls the game
                    break;
                }
                switch (event->jbutton.button) {
                    case SWITCH_PAD_Y:
                    case SWITCH_PAD_X:
                    case SWITCH_PAD_UP:
                    case SWITCH_PAD_DOWN:
                    case SWITCH_PAD_LEFT:
                    case SWITCH_PAD_RIGHT: // intentional fallthrough
                        switch_button_to_sdlkey_event(event->jbutton.button, event, SDL_KEYUP);
                        break;
                    case SWITCH_PAD_B:
                    case SWITCH_PAD_R:
                    case SWITCH_PAD_A:
                    case SWITCH_PAD_L: // intentional fallthrough
                        switch_button_to_sdlmouse_event(event->jbutton.button, event, SDL_MOUSEBUTTONUP);
                        break;
                    case SWITCH_PAD_RSTICK:
                        switch_button_to_sdlkey_event(event->jbutton.button, event, SDL_KEYUP);
                        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_C, SDLK_c);
                        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_LCTRL, SDLK_LCTRL);
                        break;
                    case SWITCH_PAD_ZL:
                        holding_modifier = 0;
                        fast_mouse = 0;
                        hires_dx = 0;
                        hires_dy = 0;
                        break;
                    case SWITCH_PAD_ZR:
                        slow_mouse = 0;
                        hires_dx = 0;
                        hires_dy = 0;
                        break;
                    case SWITCH_PAD_MINUS:
                        can_change_touch_mode = true;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }
    return ret;
}

void switch_handle_analog_sticks()
{
    if (!joy) {
        joy = SDL_JoystickOpen(0);
    }

    int left_x = SDL_JoystickGetAxis(joy, 0);
    int left_y = SDL_JoystickGetAxis(joy, 1);
    switch_rescale_analog(&left_x, &left_y, 2000);

    // scale joystick mouse velocity so it is independent of fps (nominal fps is 40 in this game)
    if (last_joy_update_time == 0) {
        last_joy_update_time = svcGetSystemTick();
    }
    uint64_t current_time = svcGetSystemTick();
    uint64_t delta_t = (current_time - last_joy_update_time);
    last_joy_update_time = current_time;
    float factor = (float) delta_t / (float) (19200 * 25);
    if (factor > 10.0 || factor < 0.1) {
        factor = 1.0;
    }
    left_x = left_x * factor;
    left_y = left_y * factor;

    hires_dx += left_x; // sub-pixel precision to allow slow mouse motion at speeds < 1 pixel/frame
    hires_dy += left_y;

    int slowdown = 4096;

    if (fast_mouse) {
        slowdown /= 3;
    }

    if (slow_mouse) {
        slowdown *= 8;
    }

    if (hires_dx != 0 || hires_dy != 0) {
        int xrel = hires_dx / slowdown;
        int yrel = hires_dy / slowdown;
        hires_dx %= slowdown;
        hires_dy %= slowdown;
        if (xrel != 0 || yrel != 0) {
            // limit joystick mouse to screen coords, same as physical mouse
            int x = last_mouse_x + xrel;
            int y = last_mouse_y + yrel;
            if (x < 0) {
                x = 0;
                xrel = 0 - last_mouse_x;
            }
            if (x > SWITCH_DISPLAY_WIDTH) {
                x = SWITCH_DISPLAY_WIDTH;
                xrel = SWITCH_DISPLAY_WIDTH - last_mouse_x;
            }
            if (y < 0) {
                y = 0;
                yrel = 0 - last_mouse_y;
            }
            if (y > SWITCH_DISPLAY_HEIGHT) {
                y = SWITCH_DISPLAY_HEIGHT;
                yrel = SWITCH_DISPLAY_HEIGHT - last_mouse_y;
            }
            SDL_Event event;
            event.type = SDL_MOUSEMOTION;
            event.motion.x = x;
            event.motion.y = y;
            event.motion.xrel = xrel;
            event.motion.yrel = yrel;
            SDL_PushEvent(&event);
        }
    }

    // map right stick to cursor keys
    float right_x = SDL_JoystickGetAxis(joy, 2);
    float right_y = -1 * SDL_JoystickGetAxis(joy, 3);
    float right_joy_dead_zone_squared = 10240.0*10240.0;
    float slope = 0.414214f; // tangent of 22.5 degrees for size of angular zones

    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;

    if ((right_x * right_x + right_y * right_y) > right_joy_dead_zone_squared) {
        if (right_y > 0 && right_x > 0) {
            // upper right quadrant
            if (right_y > slope * right_x) {
                up = 1;
            }
            if (right_x > slope * right_y) {
                right = 1;
            }
        } else if (right_y > 0 && right_x <= 0) {
            // upper left quadrant
            if (right_y > slope * (-right_x)) {
                up = 1;
            }
            if ((-right_x) > slope * right_y) {
                left = 1;
            }
        } else if (right_y <= 0 && right_x > 0) {
            // lower right quadrant
            if ((-right_y) > slope * right_x) {
                down = 1;
            }
            if (right_x > slope * (-right_y)) {
                right = 1;
            }
        } else if (right_y <= 0 && right_x <= 0) {
            // lower left quadrant
            if ((-right_y) > slope * (-right_x)) {
                down = 1;
            }
            if ((-right_x) > slope * (-right_y)) {
                left = 1;
            }
        }
    }

    if (up && !rjoy_previous_up) {
        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_UP, SDLK_UP);
    }
    else if (!up && rjoy_previous_up) {
        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_UP, SDLK_UP);
    }

    if (down && !rjoy_previous_down) {
        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_DOWN, SDLK_DOWN);
    }
    else if (!down && rjoy_previous_down) {
        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_DOWN, SDLK_DOWN);
    }

    if (left && !rjoy_previous_left) {
        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_LEFT, SDLK_LEFT);
    }
    else if (!left && rjoy_previous_left) {
        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_LEFT, SDLK_LEFT);
    }

    if (right && !rjoy_previous_right) {
        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_RIGHT, SDLK_RIGHT);
    }
    else if (!right && rjoy_previous_right) {
        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_RIGHT, SDLK_RIGHT);
    }

    rjoy_previous_up = up;
    rjoy_previous_down = down;
    rjoy_previous_left = left;
    rjoy_previous_right = right;
}

void switch_handle_virtual_keyboard(void)
{
    if (vkbd_requested) {
        vkbd_requested = 0;
        switch_start_text_input((char *) "", 0);
    }
}

static int get_utf8_character_bytes(const uint8_t *uc)
{
    if (uc[0] < 0x80) {
        return 1;
    } else if ((uc[0] & 0xe0) == 0xc0 && (uc[1] & 0xc0) == 0x80) {
        return 2;
    } else if ((uc[0] & 0xf0) == 0xe0 && (uc[1] & 0xc0) == 0x80 && (uc[2] & 0xc0) == 0x80) {
        return 3;
    } else if ((uc[0] & 0xf8) == 0xf0 && (uc[1] & 0xc0) == 0x80 && (uc[2] & 0xc0) == 0x80 && (uc[3] & 0xc0) == 0x80) {
        return 4;
    } else {
        return 1;
    }
}

static void switch_start_text_input(char *initial_text, int multiline)
{
    char text[601] = {'\0'};
    switch_keyboard_get((char *) "Enter New Text:", initial_text, 600, multiline, text);
    if (text == nullptr) {
        return;
    }
    for (int i = 0; i < 600; i++) {
        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE);
        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE);
    }
    for (int i = 0; i < 600; i++) {
        switch_create_and_push_sdlkey_event(SDL_KEYDOWN, SDL_SCANCODE_DELETE, SDLK_DELETE);
        switch_create_and_push_sdlkey_event(SDL_KEYUP, SDL_SCANCODE_DELETE, SDLK_DELETE);
    }
    const uint8_t *utf8_text = (uint8_t*) text;
    for (int i = 0; i < 599 && utf8_text[i];) {
        int bytes_in_char = get_utf8_character_bytes(&utf8_text[i]);
        SDL_Event textinput_event;
        textinput_event.type = SDL_TEXTINPUT;
        for (int n = 0; n < bytes_in_char; n++) {
            textinput_event.text.text[n] = text[i + n];
        }
        textinput_event.text.text[bytes_in_char] = 0;
        SDL_PushEvent(&textinput_event);
        i += bytes_in_char;
    }
}

static void switch_rescale_analog(int *x, int *y, int dead)
{
    //radial and scaled dead_zone
    //http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
    //input and output values go from -32767...+32767;

    //the maximum is adjusted to account for SCE_CTRL_MODE_DIGITALANALOG_WIDE
    //where a reported maximum axis value corresponds to 80% of the full range
    //of motion of the analog stick

    if (dead == 0) {
        return;
    }
    if (dead >= 32767) {
        *x = 0;
        *y = 0;
        return;
    }

    const float max_axis = 32767.0f;
    float analog_x = (float) *x;
    float analog_y = (float) *y;
    float dead_zone = (float) dead;

    float magnitude = sqrtf(analog_x * analog_x + analog_y * analog_y);
    if (magnitude >= dead_zone) {
        //adjust maximum magnitude
        float abs_analog_x = fabs(analog_x);
        float abs_analog_y = fabs(analog_y);
        float max_x;
        float max_y;
        if (abs_analog_x > abs_analog_y) {
            max_x = max_axis;
            max_y = (max_axis * analog_y) / abs_analog_x;
        } else {
            max_x = (max_axis * analog_x) / abs_analog_y;
            max_y = max_axis;
        }
        float maximum = sqrtf(max_x * max_x + max_y * max_y);
        if (maximum > 1.25f * max_axis) maximum = 1.25f * max_axis;
        if (maximum < magnitude) maximum = magnitude;

        // find scaled axis values with magnitudes between zero and maximum
        float scalingFactor = maximum / magnitude * (magnitude - dead_zone) / (maximum - dead_zone);
        analog_x = (analog_x * scalingFactor);
        analog_y = (analog_y * scalingFactor);

        // clamp to ensure results will never exceed the max_axis value
        float clamping_factor = 1.0f;
        abs_analog_x = fabs(analog_x);
        abs_analog_y = fabs(analog_y);
        if (abs_analog_x > max_axis || abs_analog_y > max_axis){
            if (abs_analog_x > abs_analog_y) {
                clamping_factor = max_axis / abs_analog_x;
            } else {
                clamping_factor = max_axis / abs_analog_y;
            }
        }
        *x = (int) (clamping_factor * analog_x);
        *y = (int) (clamping_factor * analog_y);
    } else {
        *x = 0;
        *y = 0;
    }
}

static void switch_button_to_sdlkey_event(int switch_button, SDL_Event *event, uint32_t event_type)
{
    SDL_Scancode scan;
    SDL_Keycode key;
    if (holding_modifier) {
        scan = map_switch_button_to_sdlscancode[switch_button][1];
        key = map_switch_button_to_sdlkey[switch_button][1];
    } else {
        scan = map_switch_button_to_sdlscancode[switch_button][0];
        key = map_switch_button_to_sdlkey[switch_button][0];
    }

    event->type = event_type;
    event->key.keysym.scancode = scan;
    event->key.keysym.sym = key;
    event->key.keysym.mod = 0;
    event->key.repeat = 0;

    if (event_type == SDL_KEYDOWN) {
        const uint8_t *state = SDL_GetKeyboardState(nullptr);
        const_cast<uint8_t *>(state)[scan] = 1;
    }
    if (event_type == SDL_KEYUP) {
        const uint8_t *state = SDL_GetKeyboardState(nullptr);
        const_cast<uint8_t *>(state)[scan] = 0;
    }
}

static void switch_button_to_sdlmouse_event(int switch_button, SDL_Event *event, uint32_t event_type)
{
    event->type = event_type;
    event->button.button = map_switch_button_to_sdlmousebutton[switch_button];
    if (event_type == SDL_MOUSEBUTTONDOWN) {
        event->button.state = SDL_PRESSED;
    }
    if (event_type == SDL_MOUSEBUTTONUP) {
        event->button.state = SDL_RELEASED;
    }
    event->button.x = last_mouse_x;
    event->button.y = last_mouse_y;
}

static void switch_create_and_push_sdlkey_event(uint32_t event_type, SDL_Scancode scan, SDL_Keycode key)
{
    SDL_Event event;
    event.type = event_type;
    event.key.keysym.scancode = scan;
    event.key.keysym.sym = key;
    event.key.keysym.mod = 0;
    SDL_PushEvent(&event);

    // Updates the state of the keys, otherwise scrolling doesn't work
    if (event_type == SDL_KEYDOWN) {
        const uint8_t *state = SDL_GetKeyboardState(nullptr);
        const_cast<uint8_t *>(state)[scan] = 1;
    }
    else if (event_type == SDL_KEYUP) {
        const uint8_t *state = SDL_GetKeyboardState(nullptr);
        const_cast<uint8_t *>(state)[scan] = 0;
    }
}
