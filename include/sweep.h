#ifndef SWEEP_H
#define SWEEP_H

#include "ode.h"

typedef struct {
    const char *model;
    const char *param;
    double from;
    double to;
    double step;
    const char *out_dir;
    int i_from;
    int i_to;
    int resume;
    const ParamSet *sets;
    int nsets;
} Sweep1D;

typedef struct {
    const char *model;
    const char *x_param;
    double x_from;
    double x_to;
    double x_step;
    const char *y_param;
    double y_from;
    double y_to;
    double y_step;
    const char *out_dir;
    int i_from;
    int i_to;
    int resume;
    const ParamSet *sets;
    int nsets;
} Sweep2D;

int sweep1d_run(const Sweep1D *s);
int sweep2d_run(const Sweep2D *s);

#endif
