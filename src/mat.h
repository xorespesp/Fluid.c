#pragma once
#include "common.h"

DECL_OBJECT(mat2f_obj_t);

mat2f_obj_t mat2f_create(int32_t rows, int32_t cols);
void mat2f_destroy(mat2f_obj_t*);
int32_t mat2f_get_rows(mat2f_obj_t);
int32_t mat2f_get_cols(mat2f_obj_t);
int32_t mat2f_get_size(mat2f_obj_t);
bool_t mat2f_is_empty(mat2f_obj_t);
bool_t mat2f_is_shape_eq(mat2f_obj_t, mat2f_obj_t other);
float* mat2f_at_index(mat2f_obj_t, int32_t idx);
float* mat2f_at_coord(mat2f_obj_t, int32_t row, int32_t col);