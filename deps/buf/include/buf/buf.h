#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BUF_ASSERT(x) \
    do { \
        if (!(x)) { \
            (void)fprintf(stderr, "Assertion failed: %s:%d: %s\n", __FILE__, __LINE__, #x); \
            abort(); \
        } \
    } while (0)

#define BUF(T) \
    struct { \
        T* ptr; \
        uint64_t len; \
        uint64_t cap; \
        bool ref; \
    }

#define BUF_NAMED(name, T) \
    struct name { \
        T* ptr; \
        uint64_t len; \
        uint64_t cap; \
        bool ref; \
    }

#define BUF_EMPTY \
    { .ptr = NULL, .len = 0, .cap = 0, .ref = false }

#define BUF_REF(p, sz) \
    { .ptr = (p), .len = (sz), .cap = (sz), .ref = true }

#define BUF_AS_REF(buf) BUF_REF((buf).ptr, (buf).len)

#define BUF_ARRAY(a) BUF_REF((a), sizeof(a) / sizeof((a)[0]))

#define BUF_OWNER(p, sz) \
    { .ptr = (p), .len = (sz), .cap = (sz), .ref = false }

#define BUF_NEW \
    { .ptr = NULL, .len = 0, .cap = 0, .ref = false }

#define BUF_FREE(buf) \
    do { \
        if (!(buf).ref) { \
            free((buf).ptr); \
        } \
    } while (false)

#define BUF_PUSH(buf, val) \
    do { \
        if ((buf)->len == (buf)->cap) { \
            (buf)->cap = (buf)->cap ? (buf)->cap * 2 : 1; \
            (buf)->ptr = realloc((buf)->ptr, (buf)->cap * sizeof(*(buf)->ptr)); \
            BUF_ASSERT((buf)->ptr != NULL); \
        } \
        (buf)->ptr[(buf)->len++] = (val); \
    } while (false)

#define BUF_POP_FIRST(buf) \
    do { \
        if ((buf)->len > 0) { \
            (buf)->len--; \
            memmove((buf)->ptr, (buf)->ptr + 1, (buf)->len * sizeof(*(buf)->ptr)); \
        } \
    } while (false)

#define BUF_POP(buf) ((buf)->ptr[--(buf)->len])

#define BUF_FIND(buf, val, eq, target) \
    do { \
        *(target) = NULL; \
        for (uint64_t i = 0; i < (buf).len; i++) { \
            if (eq((buf).ptr[i], (val))) { \
                *(target) = &(buf).ptr[i]; \
                break; \
            } \
        } \
    } while (false)

#define BUF_INDEX(buf, val, eq, target) \
    do { \
        *(target) = (buf).len; \
        for (uint64_t i = 0; i < (buf).len; i++) { \
            if (eq((buf).ptr[i], (val))) { \
                *(target) = i; \
                break; \
            } \
        } \
    } while (false)

#define BUF_LAST(buf) ((buf).ptr[(buf).len - 1])

#define BUF_CONCAT(buf, other) \
    do { \
        for (uint64_t i = 0; i < (other).len; i++) { \
            BUF_PUSH(buf, (other).ptr[i]); \
        } \
    } while (false)

#define BUF_SHIFT(buf, n) \
    do { \
        BUF_ASSERT((buf)->ref); \
        (buf)->ptr += (n); \
        (buf)->len -= (n); \
        (buf)->cap -= (n); \
    } while (false)

#define BUF_SHIFTED(buf, n) BUF_REF((buf).ptr + (n), (buf).len - (n))

#define BUF_BEFORE(buf, n) BUF_REF((buf).ptr, (n))
#define BUF_AFTER(buf, n) BUF_SHIFTED(buf, n)
#define BUF_SUB(buf, start, len) BUF_REF((buf).ptr + (start), (len))
#define BUF_SUB_BOUNDS(buf, start, end) BUF_REF((buf).ptr + (start), (end) - (start))
