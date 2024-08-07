#pragma once
#include <stdint.h>

#define __DECL_CLAMP_FUNC(TYPE, SUFFIX) \
    TYPE clamp_##SUFFIX(TYPE v, TYPE lo, TYPE hi);

#define __IMPL_CLAMP_FUNC(TYPE, SUFFIX) \
    TYPE clamp_##SUFFIX(TYPE v, TYPE lo, TYPE hi) { \
        return (v < lo) ? lo : ((v > hi) ? hi : v); \
    }

__DECL_CLAMP_FUNC(int8_t,   i8)
__DECL_CLAMP_FUNC(int16_t,  i16)
__DECL_CLAMP_FUNC(int32_t,  i32)
__DECL_CLAMP_FUNC(int64_t,  i64)
__DECL_CLAMP_FUNC(uint8_t,  u8)
__DECL_CLAMP_FUNC(uint16_t, u16)
__DECL_CLAMP_FUNC(uint32_t, u32)
__DECL_CLAMP_FUNC(uint64_t, u64)
__DECL_CLAMP_FUNC(float,    f32)
__DECL_CLAMP_FUNC(double,   f64)

#define clamp(V, LO, HI) _Generic((V), \
    int8_t:   clamp_i8, \
    int16_t:  clamp_i16, \
    int32_t:  clamp_i32, \
    int64_t:  clamp_i64, \
    uint8_t:  clamp_u8, \
    uint16_t: clamp_u16, \
    uint32_t: clamp_u32, \
    uint64_t: clamp_u64, \
    float:    clamp_f32, \
    double:   clamp_f64 \
)(V, LO, HI)