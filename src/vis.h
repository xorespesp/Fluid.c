#pragma once
#include "common.h"
#include "pixel.h"

DECL_OBJECT(vis_obj_t);

typedef enum {
    VIS_ACTION_PRESS,
    VIS_ACTION_RELEASE,
    VIS_ACTION_REPEAT,
} vis_action_e;

typedef enum {
    VIS_KEY_UNKNOWN,

    VIS_KEY_A, VIS_KEY_B, VIS_KEY_C, VIS_KEY_D, VIS_KEY_E,
    VIS_KEY_F, VIS_KEY_G, VIS_KEY_H, VIS_KEY_I, VIS_KEY_J, 
    VIS_KEY_K, VIS_KEY_L, VIS_KEY_M, VIS_KEY_N, VIS_KEY_O, 
    VIS_KEY_P, VIS_KEY_Q, VIS_KEY_R, VIS_KEY_S, VIS_KEY_T, 
    VIS_KEY_U, VIS_KEY_V, VIS_KEY_W, VIS_KEY_X, VIS_KEY_Y, 
    VIS_KEY_Z,

    VIS_KEY_0, VIS_KEY_1, VIS_KEY_2, VIS_KEY_3, VIS_KEY_4, 
    VIS_KEY_5, VIS_KEY_6, VIS_KEY_7, VIS_KEY_8, VIS_KEY_9,

    VIS_KEY_F1, VIS_KEY_F2, VIS_KEY_F3, VIS_KEY_F4,
    VIS_KEY_F5, VIS_KEY_F6, VIS_KEY_F7, VIS_KEY_F8,
    VIS_KEY_F9, VIS_KEY_F10, VIS_KEY_F11, VIS_KEY_F12,

    VIS_KEY_ESCAPE,
    VIS_KEY_BACK,
    VIS_KEY_RETURN,
    VIS_KEY_SPACE,
    VIS_KEY_LEFT,
    VIS_KEY_UP,
    VIS_KEY_RIGHT,
    VIS_KEY_DOWN,
    VIS_KEY_MULTIPLY,
    VIS_KEY_ADD,
    VIS_KEY_SUBTRACT,
    VIS_KEY_DIVIDE,
    // ...
} vis_key_e;

typedef enum {
    VIS_MOUSEBTN_L,
    VIS_MOUSEBTN_R,
} vis_mousebtn_e;

typedef enum {
    VIS_MODKEY_SHIFT = 1 << 0,
    VIS_MODKEY_CTRL = 1 << 1,
    VIS_MODKEY_ALT = 1 << 2,
    VIS_MODKEY_CAPSLOCK = 1 << 3,
    VIS_MODKEY_NUMLOCK = 1 << 4,
} vis_modkey_e;

typedef void(*vis_key_fn_t)(vis_obj_t vis, vis_key_e key, int scancode, vis_action_e action, vis_modkey_e mods);
typedef void(*vis_mousebtn_fn_t)(vis_obj_t vis, vis_mousebtn_e button, vis_action_e action, vis_modkey_e mods);
typedef void(*vis_cursorenter_fn_t)(vis_obj_t vis, bool_t entered);
typedef void(*vis_cursorpos_fn_t)(vis_obj_t vis, int32_t xpos, int32_t ypos);

vis_obj_t vis_create(int32_t cols, int32_t rows, const char* title);
void vis_destroy(vis_obj_t*);
void vis_close(vis_obj_t);
bool_t vis_poll(vis_obj_t);
bool_t vis_should_close(vis_obj_t);
void vis_set_user_context(vis_obj_t, void* user_ctx);
void* vis_get_user_context(vis_obj_t);
void vis_set_key_cb(vis_obj_t, vis_key_fn_t cb);
void vis_set_mousebtn_cb(vis_obj_t, vis_mousebtn_fn_t cb);
void vis_set_cursorenter_cb(vis_obj_t, vis_cursorenter_fn_t cb);
void vis_set_cursorpos_cb(vis_obj_t, vis_cursorpos_fn_t cb);
void vis_set_overlay_visibility(vis_obj_t, int visible);
void vis_set_overlay_text(vis_obj_t, const char* str, int32_t cch); // NOTE: `vis` object does not own string memory, just holds a pointer. so user must keep the lifetime of string memory valid until the frame is rendered.
int32_t vis_get_cols(vis_obj_t);
int32_t vis_get_rows(vis_obj_t);
void vis_draw(vis_obj_t, int32_t col, int32_t row, pixel_t clr);
void vis_update(vis_obj_t);