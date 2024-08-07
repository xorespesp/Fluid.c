#pragma once
#include "common.h"

DECL_OBJECT(perf_obj_t);

perf_obj_t perf_create();
void perf_destroy(perf_obj_t*);
void perf_begin(perf_obj_t);
void perf_end(perf_obj_t);
double perf_get_delta_ms(perf_obj_t);