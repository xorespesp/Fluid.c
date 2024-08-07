#pragma once
#include "common.h"
#include "pixel.h"

DECL_OBJECT(sim_obj_t);

typedef void(*sim_pixel_transfer_fn_t)(void* ctx, int32_t row, int32_t col, pixel_t clr);

sim_obj_t sim_create(int32_t box_size);
void sim_destroy(sim_obj_t*);
float sim_get_time_step(sim_obj_t);
void sim_set_time_step(sim_obj_t, float dt);
float sim_get_diffusion(sim_obj_t);
void sim_set_diffusion(sim_obj_t, float diff);
float sim_get_viscosity(sim_obj_t);
void sim_set_viscosity(sim_obj_t, float visc);
void sim_add_force(sim_obj_t, int32_t x, int32_t y, float fx, float fy);
void sim_add_density(sim_obj_t, int32_t x, int32_t y, float step);
void sim_fade_density(sim_obj_t, float step);
void sim_render_density(sim_obj_t, sim_pixel_transfer_fn_t cb, void* ctx, bool_t grayscale);
void sim_update(sim_obj_t);