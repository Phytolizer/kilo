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

#pragma once

#include <hedley/hedley.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// string type
// ----------------------------------------------------------------------------
typedef struct {
    const char* ptr;
    size_t len;
    bool owner;
} str;

// NULL string
#define str_null ((str){0, 0, false})

// string properties
// ---------------------------------------------------------------------- length
// of the string
static inline size_t str_len(const str s) {
    return s.len;
}

// pointer to the string
static inline const char* str_ptr(const str s) {
    return s.ptr ? s.ptr : "";
}

// end of the string
static inline const char* str_end(const str s) {
    return str_ptr(s) + str_len(s);
}

static inline char str_getc(const str s, size_t i) {
    return (char)(i < str_len(s) ? str_ptr(s)[i] : '\0');
}

// test if the string is empty
static inline bool str_is_empty(const str s) {
    return str_len(s) == 0;
}

// test if the string is allocated on the heap
static inline bool str_is_owner(const str s) {
    return s.owner;
}

// test if the string is a reference
static inline bool str_is_ref(const str s) {
    return !s.owner;
}

// string memory control
// ------------------------------------------------------------------- free
// memory allocated for the string
void str_free(str s);

// string movements
// ----------------------------------------------------------------------- free
// target string, then assign the new value to it
static inline void str_assign(str* const ps, const str s) {
    str_free(*ps);
    *ps = s;
}

// move the string, resetting the source to str_null
static inline str str_move(str* const ps) {
    const str t = *ps;
    *ps = str_null;
    return t;
}

// pass ownership of the string
static inline str str_pass(str* const ps) {
    const str t = *ps;
    ps->owner = false;
    return t;
}

// swap two string objects
void str_swap(str* s1, str* s2);

// string helpers
// --------------------------------------------------------------------------
// reset the string to str_null
static inline void str_clear(str* const ps) {
    str_assign(ps, str_null);
}

// compare two strings lexicographically
int str_cmp(str s1, str s2);

// test if two strings match
static inline bool str_eq(const str s1, const str s2) {
    return str_cmp(s1, s2) == 0;
}

// case-insensitive comparison
int str_cmp_ci(str s1, str s2);

// case-insensitive match
static inline bool str_eq_ci(const str s1, const str s2) {
    return str_cmp_ci(s1, s2) == 0;
}

// test for prefix
bool str_has_prefix(str s, str prefix);

// test for suffix
bool str_has_suffix(str s, str suffix);

// string composition
// ------------------------------------------------------------------
int str_cpy(str* dest, str s);

str str_dup(str s);

int str_cat_range(str* dest, const str* src, size_t count);
str str_cat_ret_range(const str* src, size_t count);

// concatenate string arguments
#define ARGS_LEN_(...) (sizeof((str[]){__VA_ARGS__}) / sizeof(str))
#define ARRAYOF_(...) ((str[]){__VA_ARGS__}), ARGS_LEN_(__VA_ARGS__)

#define str_cat(dest, ...) str_cat_range((dest), ARRAYOF_(__VA_ARGS__))
#define str_cat_ret(...) str_cat_ret_range(ARRAYOF_(__VA_ARGS__))

#define str_append(dest, ...) \
    do { \
        str temp = str_null; \
        str_cat(&temp, *dest, __VA_ARGS__); \
        str_free(*(dest)); \
        *(dest) = temp; \
    } while (0)

// implementation helpers
int str_join_range(str* dest, str sep, const str* src, size_t count);
str str_join_ret_range(str sep, const str* src, size_t count);

// join string arguments around the separator
#define str_join(dest, sep, ...) str_join_range((dest), (sep), ARRAYOF_(__VA_ARGS__))
#define str_join_ret(sep, ...) str_join_ret_range((sep), ARRAYOF_(__VA_ARGS__))

#define path_join(dest, ...) str_join((dest), str_lit("/"), __VA_ARGS__)

// constructors
// ----------------------------------------------------------------------------
// string reference from a string literal
#define str_lit(s) ((str){"" s, sizeof(s) - 1, false})
#define str_lit_c(s) \
    { "" s, sizeof(s) - 1, false }

static inline str str_ref_(const str s) {
    return (str){s.ptr, s.len, false};
}

str str_ref_from_ptr_(const char* s);

// string reference from anything
#define str_ref(s) \
    _Generic((s), str : str_ref_, char* : str_ref_from_ptr_, const char* : str_ref_from_ptr_)(s)

// create a reference to the given range of chars
str str_ref_chars(const char* s, size_t n);

// take ownership of the given range of chars
str str_acquire_chars(const char* s, size_t n);

// take ownership of the given string
str str_acquire(const char* s);

str str_printf(const char* fmt, ...) HEDLEY_PRINTF_FORMAT(1, 2);

#define str_after(s, i) str_ref_chars(str_ptr(s) + (i), str_len(s) - (i))
#define str_upto(s, i) str_ref_chars(str_ptr(s), (i))
#define str_substr(s, i, n) str_ref_chars(str_ptr(s) + (i), (n))
#define str_substr_bounds(s, i, j) str_substr((s), (i), (j) - (i))

str str_trim_left(str s, str char_set);
str str_trim_right(str s, str char_set);
str str_trim(str s, str char_set);

// searching and sorting
// -------------------------------------------------------------------- string
// partitioning (substring search)
bool str_partition(str src, str patt, str* prefix, str* suffix);

typedef struct {
    size_t pos;
    bool found;
} str_find_result;

str_find_result str_find_last_char(str src, char ch);
str_find_result str_find_last_str(str src, str patt);

#define str_find_last(s, val) \
    _Generic((val), char \
             : str_find_last_char, int \
             : str_find_last_char, str \
             : str_find_last_str)(s, val)

str_find_result str_find_char(str src, char ch);
str_find_result str_find_str(str src, str patt);

#define str_find(s, val) \
    _Generic((val), char : str_find_char, int : str_find_char, str : str_find_str)(s, val)

str_find_result str_find_any(str src, str char_set);

// comparison functions
typedef int (*str_cmp_func)(const void*, const void*);

int str_order_asc(const void* s1, const void* s2);
int str_order_desc(const void* s1, const void* s2);
int str_order_asc_ci(const void* s1, const void* s2);
int str_order_desc_ci(const void* s1, const void* s2);

// sort array of strings
void str_sort_range(str_cmp_func cmp, str* array, size_t count);

// searching
const str* str_search_range(str key, const str* array, size_t count);

// partitioning
size_t str_partition_range(bool (*pred)(str), str* array, size_t count);

// unique partitioning
size_t str_unique_range(str* array, size_t count);

// tokeniser
// --------------------------------------------------------------------------------
typedef struct {
    unsigned char bits[32];  // 256 / 8
    const char *src, *end;
} str_tok_state;

void str_tok_init(str_tok_state* state, str src, str delim_set);
bool str_tok(str* dest, str_tok_state* state);
void str_tok_delim(str_tok_state* state, str delim_set);

#define str_fmt "%.*s"
#define str_arg(s) (int)(s).len, str_ptr(s)

#ifdef __cplusplus
}
#endif
