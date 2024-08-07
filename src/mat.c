#include "mat.h"
#include "misc.h"

struct _mat2f_obj_t {
    int32_t rows, cols, size;
    float* data_f32;
};

mat2f_obj_t mat2f_create(int32_t rows, int32_t cols) {
    mat2f_obj_t newobj = (mat2f_obj_t)calloc(1, sizeof(struct _mat2f_obj_t));
    if (!newobj) {
        return NULL;
    }

    newobj->rows = rows;
    newobj->cols = cols;
    newobj->size = rows * cols;
    newobj->data_f32 = (float*)calloc(newobj->size, sizeof(float));
    if (!newobj->data_f32) {
        mat2f_destroy(&newobj);
        return NULL;
    }

    return newobj;
}

void mat2f_destroy(mat2f_obj_t* pself) {
    if (pself && *pself) {
        SAFE_FREE((*pself)->data_f32);
        SAFE_FREE(*pself);
    }
}

int32_t mat2f_get_rows(mat2f_obj_t self) {
    assert(self);
    return self->rows;
}

int32_t mat2f_get_cols(mat2f_obj_t self) {
    assert(self);
    return self->cols;
}

int32_t mat2f_get_size(mat2f_obj_t self) {
    assert(self);
    return self->size;
}

bool_t mat2f_is_empty(mat2f_obj_t self) {
    assert(self);
    return !self->size;
}

bool_t mat2f_is_shape_eq(mat2f_obj_t self, mat2f_obj_t other) {
    assert(self);
    return 
        self->cols == other->cols &&
        self->rows == other->rows;
}

float* mat2f_at_index(mat2f_obj_t self, int32_t idx) {
    assert(self);
    assert(0 <= idx && idx < self->size);
    return self->data_f32 + idx;
}

float* mat2f_at_coord(mat2f_obj_t self, int32_t row, int32_t col) {
    assert(self);
    const int32_t 
        rows = self->rows, 
        cols = self->cols,
        idx = (clamp(row, 0, rows - 1) * cols) + clamp(col, 0, cols - 1);
    assert(0 <= idx && idx < self->size);
    return self->data_f32 + idx;
}