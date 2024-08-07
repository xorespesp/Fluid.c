#pragma once
#include "common.h"

DECL_OBJECT(app_obj_t);

app_obj_t app_create();
void app_destroy(app_obj_t*);
void app_run(app_obj_t);