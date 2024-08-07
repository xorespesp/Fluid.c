#include "app.h"
#include "vis.h"
#include "misc.h"

static void _vis_test_key_cb(vis_obj_t vis, 
    vis_key_e key, 
    int scancode, 
    vis_action_e action, 
    vis_modkey_e mods)
{
    UNUSED_PARAM(vis);
    printf("%s() -> key %d, scan 0x%08X, action %d, mods 0x%08X\n"
        , __func__
        , key
        , scancode
        , action
        , mods
    );
}

static void _vis_test_mousebtn_cb(vis_obj_t vis,
    vis_mousebtn_e button, 
    vis_action_e action, 
    vis_modkey_e mods)
{
    UNUSED_PARAM(vis);
    printf("%s() -> mousebtn %d, action %d, mods 0x%08X\n"
        , __func__
        , button
        , action
        , mods
    );
}

static void _vis_test_cursorenter_cb(vis_obj_t vis, bool_t entered)
{
    UNUSED_PARAM(vis);
    printf("%s() -> entered %d\n"
        , __func__
        , entered
    );
}

static void _vis_test_cursorpos_cb(vis_obj_t vis, int32_t xpos, int32_t ypos)
{
    UNUSED_PARAM(vis);
    printf("%s() -> xpos %d, ypos %d\n"
        , __func__
        , xpos
        , ypos
    );
}

void vis_test_main()
{
    vis_obj_t vis = vis_create(80, 50, "vis test");
    vis_set_key_cb(vis, _vis_test_key_cb);
    vis_set_mousebtn_cb(vis, _vis_test_mousebtn_cb);
    vis_set_cursorenter_cb(vis, _vis_test_cursorenter_cb);
    vis_set_cursorpos_cb(vis, _vis_test_cursorpos_cb);

    while (!vis_should_close(vis)) {
        for (int32_t row = 0; row < vis_get_rows(vis); ++row) {
            for (int32_t col = 0; col < vis_get_cols(vis); ++col) {
                const uint8_t y = rand() % 256;
                vis_draw(vis, col, row, (pixel_t) {
                    .r=y, .g=y, .b=y, .a=255
                });
            }
        }

        vis_draw(vis, 4, 2, (pixel_t) { .r=255, .g=0, .b=0, .a=255 });
        vis_draw(vis, 5, 3, (pixel_t) { .r=0, .g=255, .b=0, .a=255 });
        vis_draw(vis, 6, 4, (pixel_t) { .a=255, .r=0, .g=0, .b=255 });

        vis_update(vis);
        vis_poll(vis);
    }

    vis_destroy(&vis);
    system("pause");
}

int main()
{
    //vis_test_main();

    app_obj_t app = app_create();
    if (app) {
        app_run(app);
        app_destroy(&app);
    } else {
        fprintf(stderr, "faild to create app!\n");
        return -1;
    }

    return 0;
}