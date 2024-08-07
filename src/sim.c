#include "sim.h"
#include "mat.h"
#include "misc.h"
#include <math.h>

struct _sim_obj_t {
	float dt; // time step
	float diff; // diffusion rate of the fluid
	float visc; // viscosity of the fluid
    int32_t solve_iter_size; // solve iteration size
    mat2f_obj_t m_vx0, m_vx; // prev, curr x-velocity
    mat2f_obj_t m_vy0, m_vy; // prev, curr y-velocity
    mat2f_obj_t m_d0,  m_d; // prev, curr density
};

static inline pixel_t _sim_hsl2rgb(
    float H/* ∈ [0, 360] */,
    float S/* ∈ [0, 1] */,
    float L/* ∈ [0, 1] */)
{
    float R, G, B;/* ∈ [0, 1] */
    float P, Q, T, fract;

    (H == 360.0f) ? (H = 0.0f) : (H /= 60.0f);
    fract = H - floorf(H);

    P = L * (1.0f - S);
    Q = L * (1.0f - S * fract);
    T = L * (1.0f - S * (1.0f - fract));

    if (0.0f <= H && H < 1.0f) {
        R = L, G = T, B = P;
    } else if (1.0f <= H && H < 2.0f) {
        R = Q, G = L, B = P;
    } else if (2.0f <= H && H < 3.0f) {
        R = P, G = L, B = T;
    } else if (3.0f <= H && H < 4.0f) {
        R = P, G = Q, B = L;
    } else if (4.0f <= H && H < 5.0f) {
        R = T, G = P, B = L;
    } else if (5.0f <= H && H < 6.0f) {
        R = L, G = P, B = Q;
    } else {
        R = 0.0f, G = 0.0f, B = 0.0f;
    }

    return (pixel_t) { 
        .a = 0xff,
        .r = (uint8_t)roundf(R * 255.0f),
        .g = (uint8_t)roundf(G * 255.0f),
        .b = (uint8_t)roundf(B * 255.0f),
    };
}

static inline void _sim_set_bounds(
    const int32_t b,
    const mat2f_obj_t m_x/* inout */)
{
    assert(!mat2f_is_empty(m_x) 
        && mat2f_get_rows(m_x) == mat2f_get_cols(m_x)
    );

    const int32_t N = mat2f_get_rows(m_x);

    for (int32_t j = 1; j <= N; ++j) {
        *mat2f_at_coord(m_x, j, 0) = b == 1 ? -(*mat2f_at_coord(m_x, j, 1)) : (*mat2f_at_coord(m_x, j, 1));
        *mat2f_at_coord(m_x, j, N-1) = b == 1 ? -(*mat2f_at_coord(m_x, j, N-2)) : (*mat2f_at_coord(m_x, j, N-2));
    }

    for (int32_t i = 1; i <= N; ++i) {
        *mat2f_at_coord(m_x, 0, i) = b == 2 ? -(*mat2f_at_coord(m_x, 1, i)) : (*mat2f_at_coord(m_x, 1, i));
        *mat2f_at_coord(m_x, N-1, i) = b == 2 ? -(*mat2f_at_coord(m_x, N-2, i)) : (*mat2f_at_coord(m_x, N-2, i));
    }

    *mat2f_at_coord(m_x, 0, 0) = 0.5f * (
        (*mat2f_at_coord(m_x, 0, 1)) +
        (*mat2f_at_coord(m_x, 1, 0))
    );

    *mat2f_at_coord(m_x, N-1, 0) = 0.5f * (
        (*mat2f_at_coord(m_x, N-1, 1)) +
        (*mat2f_at_coord(m_x, N-2, 0))
    );

    *mat2f_at_coord(m_x, 0, N-1) = 0.5f * (
        (*mat2f_at_coord(m_x, 0, N-2)) +
        (*mat2f_at_coord(m_x,  1, N-1))
    );

    *mat2f_at_coord(m_x, N-1, N-1) = 0.5f * (
        (*mat2f_at_coord(m_x, N-1, N-2)) +
        (*mat2f_at_coord(m_x, N-2, N-1))
    );
}

static inline void _sim_solve_gauss_seidel(
    const int32_t b,
    const mat2f_obj_t m_x/* inout */,
    const mat2f_obj_t m_x0,
    const float a,
    const float c,
    const int32_t iter_size)
{
    assert(!mat2f_is_empty(m_x) 
        && mat2f_get_rows(m_x) == mat2f_get_cols(m_x)
        && mat2f_is_shape_eq(m_x, m_x0)
    );

    const int32_t N = mat2f_get_rows(m_x);

    const float c_recip = 1.0f / c;
    for (int32_t k = 0; k < iter_size; ++k) {
        for (int32_t j = 1; j <= N; ++j) {
            for (int32_t i = 1; i <= N; ++i) {
                *mat2f_at_coord(m_x, j, i) = c_recip * (
                    (*mat2f_at_coord(m_x0, j, i)) + a * (
                        (*mat2f_at_coord(m_x, j, i-1)) +
                        (*mat2f_at_coord(m_x, j, i+1)) +
                        (*mat2f_at_coord(m_x, j-1, i)) +
                        (*mat2f_at_coord(m_x, j+1, i))
                    )
                );
            }
        }
        _sim_set_bounds(b, m_x);
    }
}

static inline void _sim_diffuse(
    const int32_t b,
    const mat2f_obj_t m_x/* inout */,
    const mat2f_obj_t m_x0,
    const float diff,
    const float dt,
    const int32_t solve_iter_size)
{
    assert(!mat2f_is_empty(m_x) 
        && mat2f_get_rows(m_x) == mat2f_get_cols(m_x)
        && mat2f_is_shape_eq(m_x, m_x0)
    );

    const int32_t N = mat2f_get_rows(m_x);

    const float 
        a = dt * diff * (N-2) * (N-2), 
        c = 1 + 4 * a;

    _sim_solve_gauss_seidel(
        b, 
        m_x, m_x0, 
        a, 
        c, 
        solve_iter_size
    );
}

static inline void _sim_project(
    const mat2f_obj_t m_vx/* inout */,
    const mat2f_obj_t m_vy/* inout */,
    const mat2f_obj_t m_p/* inout */,
    const mat2f_obj_t m_div/* inout */,
    const int32_t solve_iter_size)
{
    assert(!mat2f_is_empty(m_vx)
        && mat2f_get_rows(m_vx) == mat2f_get_cols(m_vx)
        && mat2f_is_shape_eq(m_vx, m_vy)
        && mat2f_is_shape_eq(m_vx, m_p)
        && mat2f_is_shape_eq(m_vx, m_div)
    );

    const int32_t N = mat2f_get_rows(m_vx);
    const float N_f32 = (float)N;
    const float N_f32_recip = 1.0f / N_f32;

    for (int32_t j = 1; j <= N; ++j) {
        for (int32_t i = 1; i <= N; ++i) {
            *mat2f_at_coord(m_div, j, i) = -0.5f * N_f32_recip * (
                (*mat2f_at_coord(m_vx, j, i+1)) -
                (*mat2f_at_coord(m_vx, j, i-1)) +
                (*mat2f_at_coord(m_vy, j+1, i)) -
                (*mat2f_at_coord(m_vy, j-1, i))
            );
            
            *mat2f_at_coord(m_p, j, i) = 0;
        }
    }

    _sim_set_bounds(0, m_div);
    _sim_set_bounds(0, m_p);
    _sim_solve_gauss_seidel(
        0, 
        m_p, m_div, 
        1, 
        4, 
        solve_iter_size
    );

    for (int32_t j = 1; j <= N; ++j) {
        for (int32_t i = 1; i <= N; ++i) {
            *mat2f_at_coord(m_vx, j, i) -= 0.5f * N_f32 * (*mat2f_at_coord(m_p, j, i+1) - *mat2f_at_coord(m_p, j, i-1));
            *mat2f_at_coord(m_vy, j, i) -= 0.5f * N_f32 * (*mat2f_at_coord(m_p, j+1, i) - *mat2f_at_coord(m_p, j-1, i));
        }
    }
    _sim_set_bounds(1, m_vx);
    _sim_set_bounds(2, m_vy);
}

static inline void _sim_advect(
    const int32_t b,
    const mat2f_obj_t m_d/* inout */,
    const mat2f_obj_t m_d0,
    const mat2f_obj_t m_vx,
    const mat2f_obj_t m_vy,
    const float dt)
{
    assert(!mat2f_is_empty(m_d) 
        && mat2f_get_rows(m_d) == mat2f_get_cols(m_d)
        && mat2f_is_shape_eq(m_d, m_d0)
        && mat2f_is_shape_eq(m_d, m_vx)
        && mat2f_is_shape_eq(m_d, m_vy)
    );

    const int32_t N = mat2f_get_rows(m_d);
    const float
        dt_x = dt * (N-2),
        dt_y = dt * (N-2),
        N_f32 = (float)N;

    for (int32_t j = 1; j <= N; ++j) {
        for (int32_t i = 1; i <= N; ++i) {
            const float
                x = clamp((float)i - (dt_x * (*mat2f_at_coord(m_vx, j, i))), 0.5f, N_f32 + 0.5f),
                y = clamp((float)j - (dt_y * (*mat2f_at_coord(m_vy, j, i))), 0.5f, N_f32 + 0.5f);

            const float
                i0 = floorf(x),
                i1 = 1.0f + i0,
                j0 = floorf(y),
                j1 = 1.0f + j0;

            const float 
                s1 = x - i0,
                s0 = 1.0f - s1,
                t1 = y - j0,
                t0 = 1.0f - t1;

            const int32_t 
                i0_i32 = (int32_t)i0, 
                i1_i32 = (int32_t)i1,
                j0_i32 = (int32_t)j0,
                j1_i32 = (int32_t)j1;
            
            *mat2f_at_coord(m_d, j, i) =
                s0 * (t0 * (*mat2f_at_coord(m_d0, j0_i32, i0_i32)) + t1 * (*mat2f_at_coord(m_d0, j1_i32, i0_i32))) +
                s1 * (t0 * (*mat2f_at_coord(m_d0, j0_i32, i1_i32)) + t1 * (*mat2f_at_coord(m_d0, j1_i32, i1_i32)));
        }
    }

    _sim_set_bounds(b, m_d);
}

static inline void _sim_step_velocity(
    const mat2f_obj_t m_vx/* inout */,
    const mat2f_obj_t m_vy/* inout */,
    const mat2f_obj_t m_vx0/* inout */,
    const mat2f_obj_t m_vy0/* inout */,
    const float visc, 
    const float dt,
    const int32_t solve_iter_size)
{
    assert(!mat2f_is_empty(m_vx)
        && mat2f_get_rows(m_vx) == mat2f_get_cols(m_vx)
        && mat2f_is_shape_eq(m_vx, m_vy)
        && mat2f_is_shape_eq(m_vx, m_vx0)
        && mat2f_is_shape_eq(m_vx, m_vy0)
    );

    _sim_diffuse(
        1, 
        m_vx0, m_vx, // swapped
        visc, 
        dt, 
        solve_iter_size
    );

    _sim_diffuse(
        2,
        m_vy0, m_vy, // swapped
        visc, 
        dt, 
        solve_iter_size
    );

    _sim_project(
        m_vx0, m_vy0,
        m_vx, 
        m_vy,
        solve_iter_size
    );

    _sim_advect(
        1, 
        m_vx, m_vx0, 
        m_vx0, m_vy0, 
        dt
    );

    _sim_advect(
        2, 
        m_vy, m_vy0, 
        m_vx0, m_vy0, 
        dt
    );

    _sim_project(
        m_vx, m_vy, 
        m_vx0, 
        m_vy0, 
        solve_iter_size
    );
}

static inline void _sim_step_density(
    const mat2f_obj_t m_d/* inout */,
    const mat2f_obj_t m_d0/* inout */,
    const mat2f_obj_t m_vx,
    const mat2f_obj_t m_vy,
    const float diff,
    const float dt,
    const int32_t solve_iter_size)
{
    assert(!mat2f_is_empty(m_d)
        && mat2f_get_rows(m_d) == mat2f_get_cols(m_d)
        && mat2f_is_shape_eq(m_d, m_d0)
        && mat2f_is_shape_eq(m_d, m_vx)
        && mat2f_is_shape_eq(m_d, m_vy)
    );

    _sim_diffuse(
        0, 
        m_d0, m_d, // swapped
        diff, 
        dt, 
        solve_iter_size
    );

    _sim_advect(
        0, 
        m_d, m_d0, 
        m_vx, m_vy, 
        dt
    );
}

static inline void _sim_fade_density(
    const mat2f_obj_t m_d/* inout */,
    const float step)
{
    assert(!mat2f_is_empty(m_d) 
        && mat2f_get_rows(m_d) == mat2f_get_cols(m_d)
    );

    for (int32_t i = 0; i < mat2f_get_size(m_d); ++i) {
        float* const pd = mat2f_at_index(m_d, i);
        *pd = max(*pd - step, 0.0f);
    }
}

sim_obj_t sim_create(int32_t box_size) {
    if (box_size < 10) {
        return NULL;
    }

    sim_obj_t newobj = (sim_obj_t)calloc(1, sizeof(struct _sim_obj_t));
    if (!newobj) {
        return NULL;
    }

    newobj->dt = 0.35f;
    newobj->diff = 0.0f;
    newobj->visc = 1e-06f;
    newobj->solve_iter_size = 12; // 20

    const int32_t rows = box_size, cols = box_size;
    newobj->m_vx0 = mat2f_create(rows, cols);
    newobj->m_vx = mat2f_create(rows, cols);
    newobj->m_vy0 = mat2f_create(rows, cols);
    newobj->m_vy = mat2f_create(rows, cols);
    newobj->m_d0 = mat2f_create(rows, cols);
    newobj->m_d = mat2f_create(rows, cols);
    
    if (
        !newobj->m_vx0 ||
        !newobj->m_vx ||
        !newobj->m_vy0 ||
        !newobj->m_vy ||
        !newobj->m_d0 ||
        !newobj->m_d
        )
    {
        sim_destroy(&newobj);
        return NULL;
    }

    return newobj;
}

void sim_destroy(sim_obj_t* pself) {
    if (pself && *pself) {
        mat2f_destroy(&(*pself)->m_vx0);
        mat2f_destroy(&(*pself)->m_vy0);
        mat2f_destroy(&(*pself)->m_vx);
        mat2f_destroy(&(*pself)->m_vy);
        mat2f_destroy(&(*pself)->m_d);
        mat2f_destroy(&(*pself)->m_d0);
        SAFE_FREE(*pself);
    }
}

float sim_get_time_step(sim_obj_t self) {
    assert(self);
    return self->dt;
}

void sim_set_time_step(sim_obj_t self, float dt) {
    assert(self);
    self->dt = dt;
}

float sim_get_diffusion(sim_obj_t self) {
    assert(self);
    return self->diff;
}

void sim_set_diffusion(sim_obj_t self, float diff) {
    assert(self);
    self->diff = diff;
}

float sim_get_viscosity(sim_obj_t self) {
    assert(self);
    return self->visc;
}

void sim_set_viscosity(sim_obj_t self , float visc) {
    assert(self);
    self->visc = visc;
}

void sim_add_force(sim_obj_t self, int32_t x, int32_t y, float fx, float fy) {
    assert(self);
    *mat2f_at_coord(self->m_vx, y, x) += fx;
    *mat2f_at_coord(self->m_vy, y, x) += fy;
}

void sim_add_density(sim_obj_t self, int32_t x, int32_t y, float step) {
    assert(self);
    *mat2f_at_coord(self->m_d, y, x) += step;
}

void sim_fade_density(sim_obj_t self, float step) {
    assert(self);
    _sim_fade_density(self->m_d, step);
}

void sim_render_density(sim_obj_t self, sim_pixel_transfer_fn_t cb, void* ctx, bool_t grayscale) {
    assert(self);
    assert(cb);
    float const h_offset = 235.0f;
    const mat2f_obj_t m_d = self->m_d;
    for (int32_t row = 0; row < mat2f_get_rows(m_d); ++row) {
        for (int32_t col = 0; col < mat2f_get_cols(m_d); ++col) {
            const float d = *mat2f_at_coord(m_d, col, row);
            cb(ctx, row, col,
                _sim_hsl2rgb(
                    (float)((int32_t)(h_offset + d) % 361),
                    grayscale ? 0.0f : 0.7f,
                    min(d, grayscale ? 200.0f : 255.0f) * (1.0f / 255.0f)
                ));
        }
    }
}

void sim_update(sim_obj_t self) {
    assert(self);

    const float
        dt = self->dt,
        diff = self->diff,
        visc = self->visc;

    const int32_t
        solve_iter_size = self->solve_iter_size;

    const mat2f_obj_t
        m_vx0 = self->m_vx0,
        m_vx = self->m_vx,
        m_vy = self->m_vy,
        m_vy0 = self->m_vy0,
        m_d = self->m_d,
        m_d0 = self->m_d0;

    _sim_step_density(
        m_d, m_d0,
        m_vx, m_vy, 
        diff, 
        dt, 
        solve_iter_size
    );

    _sim_step_velocity(
        m_vx, m_vy, 
        m_vx0, m_vy0, 
        visc, 
        dt, 
        solve_iter_size
    );
}