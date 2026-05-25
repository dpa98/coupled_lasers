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

static int mkdir_p(const char *path) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", path);
    return system(cmd) == 0 ? 0 : -1;
}

static int ensure_parent_dir(const char *path) {
    char tmp[4096];
    char *slash;
    snprintf(tmp, sizeof(tmp), "%s", path);
    slash = strrchr(tmp, '/');
    if (!slash) return 0;
    if (slash == tmp) return 0;
    *slash = '\0';
    return mkdir_p(tmp);
}

static char **collect_dat_files(const char *dir, int *count) {
    DIR *d = opendir(dir);
    struct dirent *e;
    char **files = NULL;
    int cap = 0, n = 0;
    if (!d) return NULL;
    while ((e = readdir(d)) != NULL) {
        char path[4096];
        if (!ends_with(e->d_name, ".dat")) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        if (is_dir(path)) continue;
        if (n == cap) {
            int ncap = cap == 0 ? 32 : cap * 2;
            char **nf = (char **)realloc(files, (size_t)ncap * sizeof(char *));
            if (!nf) { closedir(d); *count = n; return files; }
            files = nf;
            cap = ncap;
        }
        files[n] = strdup(path);
        if (!files[n]) { closedir(d); *count = n; return files; }
        n++;
    }
    closedir(d);
    qsort(files, (size_t)n, sizeof(char *), cmp_strptr);
    *count = n;
    return files;
}
