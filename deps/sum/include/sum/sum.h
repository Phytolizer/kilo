#pragma once

#include <stdbool.h>

#define SUM_MAYBE_TYPE(T) \
    struct { \
        bool present; \
        T value; \
    }

#define SUM_JUST(v) \
    { .present = true, .value = v, }

#define SUM_NOTHING \
    { .present = false, }

#define SUM_RESULT_TYPE(T, E) \
    struct { \
        bool ok; \
        union { \
            T value; \
            E error; \
        } get; \
    }

#define SUM_EITHER_TYPE(L, R) \
    struct { \
        bool left; \
        union { \
            L left; \
            R right; \
        } get; \
    }

#define SUM_OK(v) \
    { .ok = true, .get.value = (v), }

#define SUM_ERR(e) \
    { .ok = false, .get.error = (e), }

#define SUM_LEFT(v) \
    { .left = true, .get.left = (v), }

#define SUM_RIGHT(v) \
    { .left = false, .get.right = (v), }
