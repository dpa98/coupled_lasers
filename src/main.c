#include "metrics.h"
#include "ode.h"
#include "sweep.h"
#include "ts.h"
#include "xpp.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void usage(void) {
    printf("Usage:\n");
    printf("  laser run --model M --out OUT.dat [--set k=v ...]\n");
    printf("  laser sweep1d --model M --param P --from A --to B --step S --out DIR [--i-from N --i-to N --resume] [--set k=v ...]\n");
    printf("  laser sweep2d --model M --x P A B S --y Q A B S --out DIR [--i-from N --i-to N --resume] [--set k=v ...]\n");
    printf("  laser analyze --in PATH --metric error|period|extrema --out OUT.csv [--transient N]\n");
}

static int parse_set(const char *arg, ParamSet *p) {
    const char *eq = strchr(arg, '=');
    size_t klen;
    if (!eq || eq == arg) return -1;
    klen = (size_t)(eq - arg);
    if (klen >= sizeof(p->key)) klen = sizeof(p->key) - 1;
    memcpy(p->key, arg, klen);
    p->key[klen] = '\0';
    p->value = atof(eq + 1);
    return 0;
}

static int is_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

static int ends_with(const char *s, const char *suf) {
    size_t ls = strlen(s), lsu = strlen(suf);
    if (lsu > ls) return 0;
    return strcmp(s + ls - lsu, suf) == 0;
}

static int cmp_strptr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}
