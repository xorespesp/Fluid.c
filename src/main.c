#include "app.h"

int main() {
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