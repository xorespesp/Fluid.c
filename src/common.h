#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef _Bool bool_t;

#if !defined(TRUE)
#  define TRUE  1
#endif

#if !defined(FALSE)
#  define FALSE 0
#endif

#define DECL_OBJECT(X) typedef struct _ ## X * X;

// `[[maybe_unused]]` feature(since C23) is currently unavailable, so we use the classic method.
#define UNUSED_PARAM(X) ((void)(X))

#define SAFE_FREE(X) \
    if (X) { \
        free(X); \
        (X) = NULL; \
    }

#define SAFE_DISPATCH(X, ...) \
    if ((X)) { \
        (X)(__VA_ARGS__); \
    }