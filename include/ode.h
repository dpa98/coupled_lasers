#ifndef ODE_H
#define ODE_H

typedef struct {
    char key[64];
    double value;
} ParamSet;

int ode_apply_template(const char *template_path, const char *out_path,
                       const ParamSet *params, int nparams);

#endif
