#include "str/strtox.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

static bool char_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static char char_to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

static bool char_is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

#define MKTAB(f) \
    { \
        f(2U), f(3U), f(4U), f(5U), f(6U), f(7U), f(8U), f(9U), f(10U), f(11U), f(12U), f(13U), \
            f(14U), f(15U), f(16U), f(17U), f(18U), f(19U), f(20U), f(21U), f(22U), f(23U), \
            f(24U), f(25U), f(26U), f(27U), f(28U), f(29U), f(30U), f(31U), f(32U), f(33U), \
            f(34U), f(35U), f(36U) \
    }

static uint64_t str2u64_cutoff(unsigned int i) {
    return UINT64_MAX / i;
}

static uint64_t str2u64_cutlim(unsigned int i) {
    return UINT64_MAX % i;
}

Str2U64Result str2u64(str in, int base) {
    const uint64_t cutoff_tab[] = MKTAB(str2u64_cutoff);
    const uint64_t cutlim_tab[] = MKTAB(str2u64_cutlim);

    if (base < 0 || base == 1 || base > 36) {
        return (Str2U64Result){.err = EINVAL};
    }

    size_t save = 0;
    size_t s = save;
    while (char_is_space(str_getc(in, s))) {
        s++;
    }
    if (s == str_len(in)) {
        // no number
        return (Str2U64Result){.endptr = in.ptr};
    }

    if (str_getc(in, s) == '+') {
        s++;
    }

    if (str_getc(in, s) == '0') {
        if ((base == 0 || base == 16) && char_to_upper(str_getc(in, s + 1)) == 'X') {
            base = 16;
            s += 2;
        } else if (base == 0) {
            base = 8;
        }
    } else if (base == 0) {
        base = 10;
    }

    save = s;

    size_t end = str_len(in);
    uint64_t cutoff = cutoff_tab[base - 2];
    uint64_t cutlim = cutlim_tab[base - 2];
    bool overflow = false;
    char c = str_getc(in, s);

    uint64_t i = 0;

    while (c != '\0') {
        if (s == end) {
            break;
        }

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (char_is_letter(c)) {
            c = (char)(char_to_upper(c) - 'A' + 10);
        } else {
            break;
        }

        if (c >= base) {
            break;
        }

        if (i > cutoff || (i == cutoff && (uint8_t)c > cutlim)) {
            overflow = true;
        } else {
            i *= (uint64_t)base;
            i += (uint64_t)c;
        }

        s++;
        c = str_getc(in, s);
    }

    if (s == save) {
        // no number
        return (Str2U64Result){.endptr = in.ptr};
    }

    end = s;

    if (overflow) {
        return (Str2U64Result){.err = ERANGE};
    }

    return (Str2U64Result){.value = i, .endptr = str_ptr(in) + end};
}

Str2I64Result str2i64(str s, int base) {
    Str2U64Result r = str2u64(s, base);
    if (r.err != 0) {
        return (Str2I64Result){.err = r.err};
    }
    if (r.value > INT64_MAX) {
        return (Str2I64Result){.err = ERANGE};
    }
    return (Str2I64Result){.value = (int64_t)r.value, .endptr = r.endptr};
}
