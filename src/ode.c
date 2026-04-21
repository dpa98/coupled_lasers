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

int ode_apply_template(const char *template_path, const char *out_path,
                       const ParamSet *params, int nparams) {
    FILE *in = fopen(template_path, "r");
    FILE *out;
    char line[4096];
    int *seen;
    int i;

    if (!in) return -1;
    out = fopen(out_path, "w");
    if (!out) {
        fclose(in);
        return -1;
    }

    seen = (int *)calloc((size_t)nparams > 0 ? (size_t)nparams : 1, sizeof(int));
    if (!seen) {
        fclose(in);
        fclose(out);
        return -1;
    }

    while (fgets(line, sizeof(line), in)) {
        for (i = 0; i < nparams; i++) {
            int r = replace_key_value(line, sizeof(line), params[i].key, params[i].value);
            if (r > 0) seen[i] = 1;
        }
        fputs(line, out);
    }

    for (i = 0; i < nparams; i++) {
        if (!seen[i]) {
            fprintf(stderr, "error: parameter '%s' not found in template\n", params[i].key);
            free(seen);
            fclose(in);
            fclose(out);
            return -1;
        }
    }

    free(seen);
    fclose(in);
    fclose(out);
    return 0;
}
