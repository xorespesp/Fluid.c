#include "perf.h"
#include "misc.h"
#include <Windows.h>

struct _perf_obj_t {
    LARGE_INTEGER freq; // timer frequency
    LARGE_INTEGER tp_begin, tp_end;
};

perf_obj_t perf_create() {
    perf_obj_t newobj = (perf_obj_t)calloc(1, sizeof(struct _perf_obj_t));
    if (!newobj) {
        return NULL;
    }

    if (!QueryPerformanceFrequency(&newobj->freq)) {
        perf_destroy(&newobj);
        return NULL;
    }

    return newobj;
}

void perf_destroy(perf_obj_t* pself) {
    if (pself && *pself) {
        SAFE_FREE(*pself);
    }
}

void perf_begin(perf_obj_t self) {
    assert(self);
    QueryPerformanceCounter(&self->tp_begin);
}

void perf_end(perf_obj_t self) {
    assert(self);
    QueryPerformanceCounter(&self->tp_end);
}

double perf_get_delta_ms(perf_obj_t self) {
    assert(self->tp_end.QuadPart >= self->tp_begin.QuadPart);

    double delta = (double)(self->tp_end.QuadPart - self->tp_begin.QuadPart);

    //
    // We now have the elapsed number of ticks, along with the
    // number of ticks-per-second. We use these values
    // to convert to the number of elapsed milliseconds.
    // To guard against loss-of-precision, we convert
    // to milliseconds *before* dividing by ticks-per-second.
    //

    delta *= 1000.0; // Convert unit: [sec] -> [ms]
    delta /= (double)self->freq.QuadPart;

    return delta;
}