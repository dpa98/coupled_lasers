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
