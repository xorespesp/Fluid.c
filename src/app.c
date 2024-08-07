#include "app.h"
#include "vis.h"
#include "sim.h"
#include "misc.h"
#include "perf.h"

#define DIFF_MIN  (0.0f)
#define DIFF_MAX  (1e-3f)
#define DIFF_STEP ((DIFF_MAX - DIFF_MIN) / 200.0f)

#define VISC_MIN  (1e-6f)
#define VISC_MAX  (1e-3f)
#define VISC_STEP ((VISC_MAX - VISC_MIN) / 200.0f)

struct _app_obj_t {
    float d_add_step;
    float d_fade_step;
    float f_add_scale;
    float diff_factor, visc_factor;
    bool_t fl_grayscale;
    bool_t fl_render_overlay;

    struct {
        bool_t fl_lmouse_pressed;
        bool_t fl_rmouse_pressed;
        bool_t fl_cursor_entered;
        bool_t fl_cursor_first_moving;
        int32_t last_cursor_xpos, last_cursor_ypos;
        int32_t last_cursor_xdelta, last_cursor_ydelta;
    } vis_state;

    char overlay_buff[256];
    double curr_frame_time, curr_acc_frame_time;
    size_t curr_fps, frame_counter;

    perf_obj_t perf;
    sim_obj_t sim;
    vis_obj_t vis;
};

static void _app_vis_key_cb(vis_obj_t vis, 
    vis_key_e key, 
    int scancode, 
    vis_action_e action, 
    vis_modkey_e mods)
{
    UNUSED_PARAM(scancode);
    UNUSED_PARAM(mods);

    app_obj_t self = (app_obj_t)vis_get_user_context(vis);
    assert(self);

    if (action == VIS_ACTION_RELEASE)
    {
        switch (key) {
        case VIS_KEY_F1:
            self->fl_grayscale = !self->fl_grayscale;
            break;
        case VIS_KEY_F12:
            self->fl_render_overlay = !self->fl_render_overlay;
            vis_set_overlay_visibility(self->vis, self->fl_render_overlay);
            break;
        }
    }
    else
    {
        switch (key) {
        case VIS_KEY_LEFT:
        case VIS_KEY_RIGHT:
            self->diff_factor = clamp(self->diff_factor + DIFF_STEP * ((key == VIS_KEY_LEFT) ? -1 : 1), DIFF_MIN, DIFF_MAX);
            sim_set_diffusion(self->sim, self->diff_factor);
            break;
        case VIS_KEY_UP:
        case VIS_KEY_DOWN:
            self->visc_factor = clamp(self->visc_factor + VISC_STEP * ((key == VIS_KEY_UP) ? 1 : -1), VISC_MIN, VISC_MAX);
            sim_set_viscosity(self->sim, self->visc_factor);
            break;
        }
    }
}

static void _app_vis_mousebtn_cb(vis_obj_t vis, 
    vis_mousebtn_e button, 
    vis_action_e action,
    vis_modkey_e mods)
{
    UNUSED_PARAM(mods);

    app_obj_t self = (app_obj_t)vis_get_user_context(vis);
    assert(self);

    const int pressed = (action == VIS_ACTION_PRESS);
    switch (button) {
    case VIS_MOUSEBTN_L:
        self->vis_state.fl_lmouse_pressed = pressed;
        break;
    case VIS_MOUSEBTN_R:
        self->vis_state.fl_rmouse_pressed = pressed;
        break;
    }
}

static void _app_vis_cursorenter_cb(vis_obj_t vis, bool_t entered)
{
    app_obj_t self = (app_obj_t)vis_get_user_context(vis);
    assert(self);

    self->vis_state.fl_cursor_first_moving = entered && (self->vis_state.fl_cursor_entered ^ entered);
    self->vis_state.fl_cursor_entered = entered;
    if (!entered) {
        self->vis_state.fl_lmouse_pressed = self->vis_state.fl_rmouse_pressed = FALSE;
    }
}

static void _app_vis_cursorpos_cb(vis_obj_t vis, int32_t xpos, int32_t ypos)
{
    app_obj_t self = (app_obj_t)vis_get_user_context(vis);
    assert(self);

    if (self->vis_state.fl_cursor_first_moving) {
        self->vis_state.last_cursor_xdelta = self->vis_state.last_cursor_ydelta = 0; // reset
        self->vis_state.last_cursor_xpos = xpos;
        self->vis_state.last_cursor_ypos = ypos;
    } else {
        self->vis_state.last_cursor_xdelta = xpos - self->vis_state.last_cursor_xpos;
        self->vis_state.last_cursor_ydelta = ypos - self->vis_state.last_cursor_ypos;
        self->vis_state.last_cursor_xpos = xpos;
        self->vis_state.last_cursor_ypos = ypos;
    }
    self->vis_state.fl_cursor_first_moving = FALSE; // reset
}

static inline void _app_poll(app_obj_t self)
{
    assert(self);

    if (self->fl_render_overlay) {
        const int32_t cch = snprintf(
            self->overlay_buff, 
            sizeof(self->overlay_buff),
            "Frame time: %05.2fms (%zufps)\n"
            "Diffusion: %f (%.1f%%)\n"
            "Viscosity: %f (%.1f%%)\n"
            "Render mode: %s"
            , self->curr_frame_time
            , self->curr_fps
            , self->diff_factor
            , 100.0f * ((self->diff_factor - DIFF_MIN) / (DIFF_MAX - DIFF_MIN))
            , self->visc_factor
            , 100.0f * ((self->visc_factor - VISC_MIN) / (VISC_MAX - VISC_MIN))
            , self->fl_grayscale ? "gray" : "color"
        );

        vis_set_overlay_text(self->vis, self->overlay_buff, cch);
    }

    sim_fade_density(self->sim, self->d_fade_step);

    if (self->vis_state.fl_cursor_entered) {
        const int32_t
            xpos = self->vis_state.last_cursor_xpos,
            ypos = self->vis_state.last_cursor_ypos,
            xdelta = self->vis_state.last_cursor_xdelta,
            ydelta = self->vis_state.last_cursor_ydelta;

        if (self->vis_state.fl_lmouse_pressed) {
            sim_add_density(self->sim,
                xpos,
                ypos,
                self->d_add_step
            );
        }

        if (xdelta || ydelta) {
            const float scale = self->f_add_scale;
            sim_add_force(self->sim,
                xpos,
                ypos,
                (float)xdelta * scale,
                (float)ydelta * scale
            );
            self->vis_state.last_cursor_xdelta = self->vis_state.last_cursor_ydelta = 0; // reset
        }
    }

    sim_update(self->sim);
    sim_render_density(self->sim, vis_draw, self->vis, self->fl_grayscale);

    vis_update(self->vis);
    vis_poll(self->vis);
}

app_obj_t app_create() {
    app_obj_t newobj = (app_obj_t)calloc(1, sizeof(struct _app_obj_t));
    if (!newobj) {
        return NULL;
    }

    const int32_t box_size = 80;

    newobj->d_add_step = (float)box_size * 10.0f;
    newobj->d_fade_step = newobj->d_add_step * 1e-04f;
    newobj->f_add_scale = 0.5f;
    newobj->diff_factor = DIFF_MIN;
    newobj->visc_factor = VISC_MIN;
    newobj->fl_grayscale = FALSE;
    newobj->fl_render_overlay = TRUE;

    newobj->perf = perf_create();
    if (!newobj->perf) {
        app_destroy(&newobj);
        return NULL;
    }

    newobj->sim = sim_create(box_size);
    if (!newobj->sim) {
        app_destroy(&newobj);
        return NULL;
    }

    sim_set_diffusion(newobj->sim, newobj->diff_factor);
    sim_set_viscosity(newobj->sim, newobj->visc_factor);

    newobj->vis = vis_create(box_size, box_size, "Fluid.c");
    if (!newobj->vis) {
        app_destroy(&newobj);
        return NULL;
    }

    vis_set_user_context(newobj->vis, newobj);
    vis_set_key_cb(newobj->vis, _app_vis_key_cb);
    vis_set_mousebtn_cb(newobj->vis, _app_vis_mousebtn_cb);
    vis_set_cursorenter_cb(newobj->vis, _app_vis_cursorenter_cb);
    vis_set_cursorpos_cb(newobj->vis, _app_vis_cursorpos_cb);
    vis_set_overlay_visibility(newobj->vis, newobj->fl_render_overlay);

    return newobj;
}

void app_destroy(app_obj_t* pself) {
    if (pself && *pself) {
        vis_destroy(&(*pself)->vis);
        sim_destroy(&(*pself)->sim);
        perf_destroy(&(*pself)->perf);
        SAFE_FREE(*pself);
    }
}

void app_run(app_obj_t self) {
    assert(self);
    while (!vis_should_close(self->vis)) {
        perf_begin(self->perf);
        _app_poll(self);
        perf_end(self->perf);
        const double delta_ms = perf_get_delta_ms(self->perf);
        const double smoothing = 0.9; // NOTE: Larger smoothing value  -> gives a slower smoother changing value.
                                      //       Smaller smoothing value -> gives a quicker changing value.
        self->curr_frame_time = (self->curr_frame_time * smoothing) + (delta_ms * (1.0 - smoothing)); // calculate smoothed average
        self->curr_acc_frame_time += delta_ms;
        ++self->frame_counter;
        if (self->curr_acc_frame_time > 1000.0) { // every second
            self->curr_fps = self->frame_counter;
            self->curr_acc_frame_time -= 1000.0;
            self->frame_counter = 0;
        }
    }
}