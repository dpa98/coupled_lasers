#include "xpp.h"

#include <stdio.h>
#include <stdlib.h>

int xpp_available(void) {
    int rc = system("command -v xppaut >/dev/null 2>&1");
    return rc == 0;
}

int xpp_run(const char *ode_path, const char *out_dat_path) {
    char cmd[4096];
    int rc;
    snprintf(cmd, sizeof(cmd), "xppaut -silent '%s' -outfile '%s' > /dev/null 2>&1", ode_path, out_dat_path);
    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "xppaut failed (rc=%d) for %s\n", rc, ode_path);
        return -1;
    }
    return 0;
}
