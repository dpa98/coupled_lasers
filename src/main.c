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

static int cmd_run(int argc, char **argv) {
    const char *model = NULL, *out = NULL;
    ParamSet sets[64];
    int nsets = 0;
    int i;
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) model = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out = argv[++i];
        else if (strcmp(argv[i], "--set") == 0 && i + 1 < argc) {
            if (nsets < 64 && parse_set(argv[++i], &sets[nsets]) == 0) nsets++;
        } else { fprintf(stderr, "unknown arg: %s\n", argv[i]); return 1; }
    }
    if (!model || !out) { usage(); return 1; }
    if (!xpp_available()) { fprintf(stderr, "error: xppaut not found in PATH\n"); return 1; }
    if (ensure_parent_dir(out) != 0) { fprintf(stderr, "error: cannot create output directory for %s\n", out); return 1; }
    {
        char tmp_ode[4096];
        snprintf(tmp_ode, sizeof(tmp_ode), "%s.tmp.ode", out);
        if (ode_apply_template(model, tmp_ode, sets, nsets) != 0) return 1;
        if (xpp_run(tmp_ode, out) != 0) { remove(tmp_ode); return 1; }
        remove(tmp_ode);
    }
    printf("wrote %s\n", out);
    return 0;
}

static int cmd_sweep1d(int argc, char **argv) {
    Sweep1D s;
    ParamSet sets[64];
    int nsets = 0;
    int i;
    memset(&s, 0, sizeof(s));
    s.i_from = 0; s.i_to = -1; s.resume = 0;
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) s.model = argv[++i];
        else if (strcmp(argv[i], "--param") == 0 && i + 1 < argc) s.param = argv[++i];
        else if (strcmp(argv[i], "--from") == 0 && i + 1 < argc) s.from = atof(argv[++i]);
        else if (strcmp(argv[i], "--to") == 0 && i + 1 < argc) s.to = atof(argv[++i]);
        else if (strcmp(argv[i], "--step") == 0 && i + 1 < argc) s.step = atof(argv[++i]);
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) s.out_dir = argv[++i];
        else if (strcmp(argv[i], "--i-from") == 0 && i + 1 < argc) s.i_from = atoi(argv[++i]);
        else if (strcmp(argv[i], "--i-to") == 0 && i + 1 < argc) s.i_to = atoi(argv[++i]);
        else if (strcmp(argv[i], "--resume") == 0) s.resume = 1;
        else if (strcmp(argv[i], "--set") == 0 && i + 1 < argc) {
            if (nsets < 64 && parse_set(argv[++i], &sets[nsets]) == 0) nsets++;
        } else { fprintf(stderr, "unknown arg: %s\n", argv[i]); return 1; }
    }
    if (!s.model || !s.param || !s.out_dir || s.step == 0.0) { usage(); return 1; }
    if (!xpp_available()) { fprintf(stderr, "error: xppaut not found in PATH\n"); return 1; }
    s.sets = sets; s.nsets = nsets;
    return sweep1d_run(&s) == 0 ? 0 : 1;
}

static int cmd_sweep2d(int argc, char **argv) {
    Sweep2D s;
    ParamSet sets[64];
    int nsets = 0;
    int i;
    memset(&s, 0, sizeof(s));
    s.i_from = 0; s.i_to = -1; s.resume = 0;
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) s.model = argv[++i];
        else if (strcmp(argv[i], "--x") == 0 && i + 4 < argc) {
            s.x_param = argv[++i]; s.x_from = atof(argv[++i]); s.x_to = atof(argv[++i]); s.x_step = atof(argv[++i]);
        } else if (strcmp(argv[i], "--y") == 0 && i + 4 < argc) {
            s.y_param = argv[++i]; s.y_from = atof(argv[++i]); s.y_to = atof(argv[++i]); s.y_step = atof(argv[++i]);
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) s.out_dir = argv[++i];
        else if (strcmp(argv[i], "--i-from") == 0 && i + 1 < argc) s.i_from = atoi(argv[++i]);
        else if (strcmp(argv[i], "--i-to") == 0 && i + 1 < argc) s.i_to = atoi(argv[++i]);
        else if (strcmp(argv[i], "--resume") == 0) s.resume = 1;
        else if (strcmp(argv[i], "--set") == 0 && i + 1 < argc) {
            if (nsets < 64 && parse_set(argv[++i], &sets[nsets]) == 0) nsets++;
        } else { fprintf(stderr, "unknown arg: %s\n", argv[i]); return 1; }
    }
    if (!s.model || !s.x_param || !s.y_param || !s.out_dir || s.x_step == 0.0 || s.y_step == 0.0) { usage(); return 1; }
    if (!xpp_available()) { fprintf(stderr, "error: xppaut not found in PATH\n"); return 1; }
    s.sets = sets; s.nsets = nsets;
    return sweep2d_run(&s) == 0 ? 0 : 1;
}

typedef struct {
    char file[256];
    char params[1024]; /* comma-separated values in index.csv order, excluding index,file */
} IndexEntry;

typedef struct {
    char param_header[1024]; /* e.g. "alpha_t,gama" or "gama" */
    IndexEntry *entries;
    int n;
} IndexMap;

static const char *path_basename(const char *path) {
    const char *s = strrchr(path, '/');
    return s ? s + 1 : path;
}

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) { s[n - 1] = '\0'; n--; }
}

static int load_index_map(const char *dir, IndexMap *m) {
    char path[4096];
    FILE *f;
    char line[4096];
    char *p;
    int cap = 0;
    m->param_header[0] = '\0';
    m->entries = NULL;
    m->n = 0;
    snprintf(path, sizeof(path), "%s/index.csv", dir);
    f = fopen(path, "r");
    if (!f) return -1;

    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    trim_newline(line);
    /* header: index,file,<params...> */
    p = line;
    /* skip index */
    p = strchr(p, ',');
    if (!p) { fclose(f); return -1; }
    p++;
    /* skip file */
    p = strchr(p, ',');
    if (p) {
        p++;
        snprintf(m->param_header, sizeof(m->param_header), "%s", p);
    } else {
        m->param_header[0] = '\0';
    }

    while (fgets(line, sizeof(line), f)) {
        char *c1, *c2;
        IndexEntry e;
        trim_newline(line);
        if (line[0] == '\0') continue;
        c1 = strchr(line, ',');
        if (!c1) continue;
        c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        *c1 = '\0';
        *c2 = '\0';
        snprintf(e.file, sizeof(e.file), "%s", c1 + 1);
        snprintf(e.params, sizeof(e.params), "%s", c2 + 1);
        if (m->n == cap) {
            int ncap = cap == 0 ? 32 : cap * 2;
            IndexEntry *ne = (IndexEntry *)realloc(m->entries, (size_t)ncap * sizeof(IndexEntry));
            if (!ne) break;
            m->entries = ne;
            cap = ncap;
        }
        m->entries[m->n++] = e;
    }
    fclose(f);
    return 0;
}

static const IndexEntry *index_lookup(const IndexMap *m, const char *path) {
    const char *base = path_basename(path);
    int i;
    if (!m || !m->entries) return NULL;
    for (i = 0; i < m->n; i++) {
        if (strcmp(m->entries[i].file, base) == 0) return &m->entries[i];
    }
    return NULL;
}

static void free_index_map(IndexMap *m) {
    free(m->entries);
    m->entries = NULL;
    m->n = 0;
}

static int analyze_file(const char *path, const char *metric, int transient, FILE *out, const char *extra_header, const char *extra_values) {
    TimeSeries ts;
    const char *eh = (extra_header && extra_header[0]) ? extra_header : NULL;
    const char *ev = (extra_values && extra_values[0]) ? extra_values : NULL;
    if (ts_load(path, &ts) != 0) {
        fprintf(stderr, "failed to load %s\n", path);
        return -1;
    }
    if (strcmp(metric, "error") == 0) {
        MetricResult r = metric_error(&ts, transient);
        if (eh) fprintf(out, "%s,%s,%.15g,%.15g,%d\n", path, ev ? ev : "", r.aux1, r.aux2, r.code);
        else fprintf(out, "%s,%.15g,%.15g,%d\n", path, r.aux1, r.aux2, r.code);
    } else if (strcmp(metric, "period") == 0) {
        MetricResult r = metric_period(&ts, transient);
        if (eh) fprintf(out, "%s,%s,%.15g,%d\n", path, ev ? ev : "", r.value, r.code);
        else fprintf(out, "%s,%.15g,%d\n", path, r.value, r.code);
    } else if (strcmp(metric, "extrema") == 0) {
        int max_ext = 16384;
        typedef struct { double value; int index; int is_max; } E;
        E *ext = (E *)malloc((size_t)max_ext * sizeof(E));
        int n = 0, i;
        if (ext) {
            int start = transient > 0 ? transient : 0;
            for (i = start + 1; i < ts.rows - 1 && n < max_ext; i++) {
                double a = ts_get(&ts, i - 1, 4) * ts_get(&ts, i - 1, 4) + ts_get(&ts, i - 1, 5) * ts_get(&ts, i - 1, 5);
                double b = ts_get(&ts, i, 4) * ts_get(&ts, i, 4) + ts_get(&ts, i, 5) * ts_get(&ts, i, 5);
                double c = ts_get(&ts, i + 1, 4) * ts_get(&ts, i + 1, 4) + ts_get(&ts, i + 1, 5) * ts_get(&ts, i + 1, 5);
                if (a < b && b > c) { ext[n].value = b; ext[n].index = i; ext[n].is_max = 1; n++; }
                else if (a > b && b < c) { ext[n].value = b; ext[n].index = i; ext[n].is_max = 0; n++; }
            }
            for (i = 0; i < n; i++) {
                if (eh) fprintf(out, "%s,%s,%s,%.15g,%d\n", path, ev ? ev : "", ext[i].is_max ? "max" : "min", ext[i].value, ext[i].index);
                else fprintf(out, "%s,%s,%.15g,%d\n", path, ext[i].is_max ? "max" : "min", ext[i].value, ext[i].index);
            }
            free(ext);
        }
    }
    ts_free(&ts);
    return 0;
}
