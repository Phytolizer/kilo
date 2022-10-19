/*
BSD 3-Clause License

Copyright (c) 2020,2021, Maxim Konakov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "str/str.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define STRNICMP _strnicmp
#else
#include <strings.h>
#define STRNICMP strncasecmp
#endif

// compatibility
#ifndef _GNU_SOURCE
static inline void* mempcpy(void* dest, const void* src, const size_t n) {
    char* d = memcpy(dest, src, n);
    return d + n;
}
#endif

static inline void str_mem_free(void* p) {
    if (p) {
        free(p);
    }
}

// string deallocation
void str_free(const str s) {
    if (str_is_owner(s)) {
        str_mem_free((void*)s.ptr);
    }
}

// memory allocation helpers
#define REALLOC(p, n) \
    ({ \
        void* const s___ = (p); \
        void* const p___ = realloc(s___, (n)); \
        if (!p___) { \
            str_mem_free(s___); \
            return ENOMEM; \
        } \
        p___; \
    })

// errno checker
#define EINTR_RETRY(expr) \
    while ((expr) < 0) { \
        do { \
            const int __err = errno; \
            if (__err != EINTR) { \
                return __err; \
            } \
        } while (0); \
    }

// swap
void str_swap(str* const s1, str* const s2) {
    const str tmp = *s1;

    *s1 = *s2;
    *s2 = tmp;
}

// string comparison
// --------------------------------------------------------------------- compare
// two strings lexicographically
int str_cmp(const str s1, const str s2) {
    const size_t n1 = s1.len;
    const size_t n2 = s2.len;

    // either string may be missing a null terminator, hence "memcmp"
    const int res = memcmp(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

    if (res != 0 || n1 == n2) {
        return res;
    }

    return (n1 < n2) ? -1 : 1;
}

// case-insensitive comparison
int str_cmp_ci(const str s1, const str s2) {
    const size_t n1 = s1.len;
    const size_t n2 = s2.len;

    // either string may be missing a null terminator, hence "strNcasecmp"
    const int res = STRNICMP(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

    if (res != 0 || n1 == n2) {
        return res;
    }

    return (n1 < n2) ? -1 : 1;
}

// test for prefix
bool str_has_prefix(const str s, const str prefix) {
    const size_t n = prefix.len;

    return (n == 0) || (s.len >= n && memcmp(str_ptr(s), str_ptr(prefix), n) == 0);
}

// test for suffix
bool str_has_suffix(const str s, const str suffix) {
    const size_t n = suffix.len;

    return (n == 0) || (s.len >= n && memcmp(str_end(s) - n, str_ptr(suffix), n) == 0);
}

// string constructors
// ----------------------------------------------------------------- create a
// reference to the given range of chars
str str_ref_chars(const char* const s, const size_t n) {
    return (s && n > 0) ? ((str){s, n, false}) : str_null;
}

str str_ref_from_ptr_(const char* const s) {
    return s ? str_ref_chars(s, strlen(s)) : str_null;
}

// take ownership of the given range of chars
str str_acquire_chars(const char* const s, const size_t n) {
    if (!s) {
        return str_null;
    }

    return (str){s, n, true};
}

// take ownership of the given C string
str str_acquire(const char* const s) {
    return s ? str_acquire_chars(s, strlen(s)) : str_null;
}

static str str_vprintf(const char* const fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);

    const int n = vsnprintf(NULL, 0, fmt, ap2);

    va_end(ap2);

    if (n < 0) {
        return str_null;
    }

    char* const s = malloc((size_t)n + 1);

    if (!s) {
        return str_null;
    }

    (void)vsnprintf(s, (size_t)n + 1, fmt, ap);

    return str_acquire_chars(s, (size_t)n);
}

str str_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    str s = str_vprintf(fmt, args);
    va_end(args);
    return s;
}

str str_trim_left(str s, str char_set) {
    if (s.len == 0) {
        return s;
    }

    size_t p = 0;

    while (p < s.len && memchr(str_ptr(char_set), s.ptr[p], char_set.len)) {
        ++p;
    }

    return str_after(s, p);
}

str str_trim_right(str s, str char_set) {
    if (s.len == 0) {
        return s;
    }

    size_t p = s.len;

    while (p > 0 && memchr(str_ptr(char_set), s.ptr[p - 1], char_set.len)) {
        --p;
    }

    return str_upto(s, p);
}

str str_trim(str s, str char_set) {
    return str_trim_left(str_trim_right(s, char_set), char_set);
}

// allocate a copy of the given string
int str_cpy(str* const dest, const str s) {
    const size_t n = s.len;

    if (n == 0) {
        str_clear(dest);
    } else {
        char* const p = memcpy(malloc(n + 1), str_ptr(s), n);

        p[n] = 0;
        str_assign(dest, str_acquire_chars(p, n));
    }

    return 0;
}

str str_dup(str s) {
    str s2 = str_null;
    str_cpy(&s2, s);
    return s2;
}

// string composition
// -----------------------------------------------------------------------
// append string
static inline char* append_str(char* p, const str s) {
    return mempcpy(p, str_ptr(s), s.len);
}

static size_t total_length(const str* src, size_t count) {
    size_t sum = 0;

    for (; count > 0; --count) {
        sum += (src++)->len;
    }

    return sum;
}

// concatenate strings
int str_cat_range(str* const dest, const str* src, size_t count) {
    if (!src) {
        str_clear(dest);
        return 0;
    }

    // calculate total length
    const size_t num = total_length(src, count);

    if (num == 0) {
        str_clear(dest);
        return 0;
    }

    // allocate
    char* const buff = malloc(num + 1);

    // copy bytes
    char* p = buff;

    for (; count > 0; --count) {
        p = append_str(p, *src++);
    }

    // null-terminate and assign
    *p = 0;
    str_assign(dest, str_acquire_chars(buff, num));
    return 0;
}

str str_cat_ret_range(const str* src, size_t count) {
    str s = str_null;
    str_cat_range(&s, src, count);
    return s;
}

// join strings
int str_join_range(str* const dest, const str sep, const str* src, size_t count) {
    // test for simple cases
    if (str_is_empty(sep)) {
        return str_cat_range(dest, src, count);
    }

    if (!src || count == 0) {
        str_clear(dest);
        return 0;
    }

    if (count == 1) {
        return str_cpy(dest, *src);
    }

    // calculate total length
    const size_t num = total_length(src, count) + sep.len * (count - 1);

    // allocate
    char* const buff = malloc(num + 1);

    // copy bytes
    char* p = append_str(buff, *src++);

    while (--count > 0) {
        p = append_str(append_str(p, sep), *src++);
    }

    // null-terminate and assign
    *p = 0;
    str_assign(dest, str_acquire_chars(buff, num));
    return 0;
}

str str_join_ret_range(str sep, const str* src, size_t count) {
    str s = str_null;
    str_join_range(&s, sep, src, count);
    return s;
}

// searching and sorting
// -------------------------------------------------------------------- string
// partitioning
#ifndef _GNU_SOURCE
// an implementation of memmem(3) for the str_partition() function below
static const void* memmem(const void* s, const size_t len, const void* patt, size_t patt_len) {
    switch (patt_len) {
        case 0:
            return s;
        case 1:
            return memchr(s, *(const unsigned char*)patt, len);
    }

    const char* cs = s;
    const char* cpatt = patt;

    const char* const end = cs + len;
    const int c = *(const unsigned char*)cpatt++;

    --patt_len;

    for (cs = memchr(cs, c, len); cs && cs + patt_len < end;
         ++cs, cs = memchr(cs, c, (size_t)(end - cs))) {
        if (memcmp(cs + 1, cpatt, patt_len) == 0) {
            return cs;
        }
    }

    return NULL;
}
#endif  // ifndef _GNU_SOURCE

bool str_partition(const str src, const str patt, str* const prefix, str* const suffix) {
    const size_t patt_len = patt.len;

    if (patt_len > 0 && !str_is_empty(src)) {
        const char* s = memmem(str_ptr(src), src.len, str_ptr(patt), patt_len);

        if (s) {
            if (prefix) {
                str_assign(prefix, str_ref_chars(str_ptr(src), (size_t)(s - str_ptr(src))));
            }

            if (suffix) {
                s += patt_len;
                str_assign(suffix, str_ref_chars(s, (size_t)(str_end(src) - s)));
            }

            return true;
        }
    }

    if (prefix) {
        str_assign(prefix, str_ref(src));
    }

    if (suffix) {
        str_clear(suffix);
    }

    return false;
}

str_find_result str_find_last_char(str src, char ch) {
    size_t i = src.len;

    while (i > 0) {
        if (src.ptr[--i] == ch) {
            return (str_find_result){.found = true, .pos = i};
        }
    }

    return (str_find_result){.found = false};
}

str_find_result str_find_last_str(str src, str patt) {
    if (patt.len > 0 && !str_is_empty(src)) {
        const char* s = str_end(src) - patt.len;

        while (s >= src.ptr) {
            if (memcmp(s, patt.ptr, patt.len) == 0) {
                return (str_find_result){.found = true, .pos = (size_t)(s - src.ptr)};
            }

            --s;
        }
    }

    return (str_find_result){.found = false};
}

str_find_result str_find_char(str src, char ch) {
    const char* s = memchr(str_ptr(src), ch, src.len);

    if (s) {
        return (str_find_result){.found = true, .pos = (size_t)(s - str_ptr(src))};
    }

    return (str_find_result){.found = false};
}

str_find_result str_find_str(str src, str patt) {
    if (patt.len > 0 && !str_is_empty(src)) {
        const char* s = memmem(str_ptr(src), src.len, str_ptr(patt), patt.len);

        if (s) {
            return (str_find_result){.found = true, .pos = (size_t)(s - str_ptr(src))};
        }
    }

    return (str_find_result){.found = false};
}

str_find_result str_find_any(str src, str char_set) {
    for (size_t i = 0; i < str_len(src); i++) {
        if (str_find_char(char_set, src.ptr[i]).found) {
            return (str_find_result){.found = true, .pos = i};
        }
    }

    return (str_find_result){.found = false};
}

// comparison functions
int str_order_asc(const void* const s1, const void* const s2) {
    return str_cmp(*(const str*)s1, *(const str*)s2);
}

int str_order_desc(const void* const s1, const void* const s2) {
    return -str_cmp(*(const str*)s1, *(const str*)s2);
}

int str_order_asc_ci(const void* const s1, const void* const s2) {
    return str_cmp_ci(*(const str*)s1, *(const str*)s2);
}

int str_order_desc_ci(const void* const s1, const void* const s2) {
    return -str_cmp_ci(*(const str*)s1, *(const str*)s2);
}

// sorting
void str_sort_range(const str_cmp_func cmp, str* const array, const size_t count) {
    if (array && count > 1) {
        qsort(array, count, sizeof(array[0]), cmp);
    }
}

// searching
const str* str_search_range(const str key, const str* const array, const size_t count) {
    if (!array || count == 0) {
        return NULL;
    }

    if (count == 1) {
        return str_eq(key, array[0]) ? array : NULL;
    }

    return bsearch(&key, array, count, sizeof(str), str_order_asc);
}

// partitioning
size_t str_partition_range(bool (*pred)(const str), str* const array, const size_t count) {
    if (!array) {
        return 0;
    }

    const str* const end = array + count;
    str* p = array;

    while (p < end && pred(*p)) {
        ++p;
    }

    for (str* s = p + 1; s < end; ++s) {
        if (pred(*s)) {
            str_swap(p++, s);
        }
    }

    return (size_t)(p - array);
}

// unique partitioning
size_t str_unique_range(str* const array, const size_t count) {
    if (!array || count == 0) {
        return 0;
    }

    if (count == 1) {
        return 1;
    }

    str_sort_range(str_order_asc, array, count);

    const str* const end = array + count;
    str* p = array;

    for (str* s = array + 1; s < end; ++s) {
        if (!str_eq(*p, *s) && (++p < s)) {
            str_swap(p, s);
        }
    }

    return (size_t)(p + 1 - array);
}

// tokeniser
static inline bool is_delim(const str_tok_state* const state, const char c) {
    return state->bits[(unsigned char)c >> 3] & (1 << (c & 0x7));
}

static inline void set_bit(str_tok_state* const state, const char c) {
    state->bits[(unsigned char)c >> 3] |= (1 << (c & 0x7));
}

void str_tok_delim(str_tok_state* const state, const str delim_set) {
    memset(state->bits, 0, sizeof(state->bits));

    const char* const end = str_end(delim_set);

    for (const char* s = str_ptr(delim_set); s < end; ++s) {
        set_bit(state, *s);
    }
}

void str_tok_init(str_tok_state* const state, const str src, const str delim_set) {
    state->src = str_ptr(src);
    state->end = str_end(src);

    str_tok_delim(state, delim_set);
}

bool str_tok(str* const dest, str_tok_state* const state) {
    // token start
    const char* begin = state->src;

    while (begin < state->end && is_delim(state, *begin)) {
        ++begin;
    }

    if (begin == state->end) {
        str_clear(dest);
        return false;
    }

    // token end
    const char* end = begin + 1;

    while (end < state->end && !is_delim(state, *end)) {
        ++end;
    }

    state->src = end;
    str_assign(dest, str_ref_chars(begin, (size_t)(end - begin)));

    return true;
}
