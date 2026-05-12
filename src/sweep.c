#include "sweep.h"

#include "ode.h"
#include "xpp.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int mkdir_p(const char *path) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", path);
    return system(cmd) == 0 ? 0 : -1;
}

static int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static int count_steps(double from, double to, double step) {
    int n;
    if (step == 0.0) return 0;
    n = (int)floor((to - from) / step + 0.0000001) + 1;
    if (n < 0) n = 0;
    return n;
}

static void dat_name(char *buf, size_t cap, const char *out_dir, int index) {
    snprintf(buf, cap, "%s/%06d.dat", out_dir, index);
}

int sweep1d_run(const Sweep1D *s) {
    char tmp_dir[4096];
    char tmp_ode[4096];
    char tmp_dat[4096];
    char out_dat[4096];
    char index_path[4096];
    FILE *idx;
    int nsteps, i, start, end;
    ParamSet *params;
    int nparams;

    if (!s || !s->model || !s->param || !s->out_dir || s->step == 0.0) return -1;
    if (mkdir_p(s->out_dir) != 0) return -1;
    snprintf(tmp_dir, sizeof(tmp_dir), "%s/.tmp", s->out_dir);
    if (mkdir_p(tmp_dir) != 0) return -1;

    nsteps = count_steps(s->from, s->to, s->step);
    start = s->i_from > 0 ? s->i_from : 0;
    end = s->i_to >= 0 ? s->i_to : nsteps - 1;
    if (end >= nsteps) end = nsteps - 1;

    snprintf(index_path, sizeof(index_path), "%s/index.csv", s->out_dir);
    idx = fopen(index_path, "w");
    if (!idx) return -1;
    fprintf(idx, "index,file,%s\n", s->param);
    for (i = 0; i < nsteps; i++) {
        double v = s->from + (double)i * s->step;
        fprintf(idx, "%d,%06d.dat,%.15g\n", i, i, v);
    }
    fclose(idx);

    nparams = s->nsets + 1;
    params = (ParamSet *)malloc((size_t)nparams * sizeof(ParamSet));
    if (!params) return -1;
    for (i = 0; i < s->nsets; i++) params[i] = s->sets[i];

    for (i = start; i <= end; i++) {
        double v = s->from + (double)i * s->step;
        dat_name(out_dat, sizeof(out_dat), s->out_dir, i);
        if (s->resume && file_exists(out_dat)) {
            printf("skip %d (%s=%.15g)\n", i, s->param, v);
            continue;
        }
        snprintf(params[nparams - 1].key, sizeof(params[nparams - 1].key), "%s", s->param);
        params[nparams - 1].value = v;

        snprintf(tmp_ode, sizeof(tmp_ode), "%s/run_%06d.ode", tmp_dir, i);
        snprintf(tmp_dat, sizeof(tmp_dat), "%s/run_%06d.dat", tmp_dir, i);

        printf("run %d/%d %s=%.15g\n", i, end, s->param, v);
        if (ode_apply_template(s->model, tmp_ode, params, nparams) != 0) {
            fprintf(stderr, "failed to create ode for index %d\n", i);
            free(params);
            return -1;
        }
        if (xpp_run(tmp_ode, tmp_dat) != 0) {
            fprintf(stderr, "xpp failed at index %d\n", i);
            free(params);
            return -1;
        }
        {
            char mv[8192];
            snprintf(mv, sizeof(mv), "mv '%s' '%s'", tmp_dat, out_dat);
            if (system(mv) != 0) {
                fprintf(stderr, "failed to move output for index %d\n", i);
                free(params);
                return -1;
            }
        }
        remove(tmp_ode);
    }

    free(params);
    return 0;
}
