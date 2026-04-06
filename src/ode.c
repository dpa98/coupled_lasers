#include "ode.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_ident_char(int c) {
    return isalnum((unsigned char)c) || c == '_';
}

static int replace_key_value(char *line, size_t cap, const char *key, double value) {
    char buf[4096];
    const char *src = line;
    char *dst = buf;
    size_t keylen = strlen(key);
    int replaced = 0;

    while (*src && (size_t)(dst - buf) < cap - 1) {
        if (strncmp(src, key, keylen) == 0) {
            char before = (src == line) ? ' ' : *(src - 1);
            char after = *(src + keylen);
            if (!is_ident_char(before) && after == '=') {
                int n = snprintf(dst, cap - (size_t)(dst - buf), "%s=%.15g", key, value);
                if (n < 0) return -1;
                dst += n;
                src += keylen + 1;
                while (*src && !isspace((unsigned char)*src) && *src != ',' && *src != ')' && *src != ']') src++;
                replaced = 1;
                continue;
            }
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    if (replaced) {
        snprintf(line, cap, "%s", buf);
    }
    return replaced;
}
