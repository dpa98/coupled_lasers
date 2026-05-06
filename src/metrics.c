#include "metrics.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static double intensity_slave(const TimeSeries *ts, int i) {
    double erx = ts_get(ts, i, 4);
    double ery = ts_get(ts, i, 5);
    return erx * erx + ery * ery;
}

static double intensity_master(const TimeSeries *ts, int i) {
    double etx = ts_get(ts, i, 1);
    double ety = ts_get(ts, i, 2);
    return etx * etx + ety * ety;
}

MetricResult metric_error(const TimeSeries *ts, int transient) {
    MetricResult r = { -1, 0.0, 0.0, 0.0 };
    int start = transient > 0 ? transient : 0;
    int n = 0;
    int i;
    double sum = 0.0;
    double sumsq = 0.0;
    if (!ts || ts->rows <= start || ts->cols < 6) return r;
    for (i = start; i < ts->rows; i++) {
        double im = intensity_master(ts, i);
        double is = intensity_slave(ts, i);
        double ratio;
        if (im == 0.0) continue;
        ratio = is / im;
        sum += ratio;
        n++;
    }
    if (n == 0) return r;
    r.aux1 = sum / n; /* mean */
    for (i = start; i < ts->rows; i++) {
        double im = intensity_master(ts, i);
        double is;
        double ratio;
        if (im == 0.0) continue;
        is = intensity_slave(ts, i);
        ratio = is / im;
        sumsq += (ratio - r.aux1) * (ratio - r.aux1);
    }
    r.aux2 = (n > 0) ? sqrt(sumsq / n) / sqrt((double)n) : 0.0; /* stderr like legacy */
    r.value = r.aux1;
    r.code = 0;
    return r;
}

typedef struct {
    double value;
    int index;
    int is_max; /* 1 max, 0 min */
} Extremum;

static int collect_extrema(const TimeSeries *ts, int start, Extremum *out, int max_out) {
    int i;
    int n = 0;
    if (ts->rows < start + 3) return 0;
    for (i = start + 1; i < ts->rows - 1 && n < max_out; i++) {
        double a = intensity_slave(ts, i - 1);
        double b = intensity_slave(ts, i);
        double c = intensity_slave(ts, i + 1);
        if (a < b && b > c) {
            out[n].value = b;
            out[n].index = i;
            out[n].is_max = 1;
            n++;
        } else if (a > b && b < c) {
            out[n].value = b;
            out[n].index = i;
            out[n].is_max = 0;
            n++;
        }
    }
    return n;
}

MetricResult metric_period(const TimeSeries *ts, int transient) {
    MetricResult r = { -1, -1.0, 0.0, 0.0 };
    int start = transient > 0 ? transient : 0;
    int max_ext = 4096;
    Extremum *ext;
    int n, k, i, t;
    double tol = 10.0; /* amplitude tolerance, pragmatic default */

    if (!ts || ts->rows <= start + 2 || ts->cols < 6) return r;
    ext = (Extremum *)malloc((size_t)max_ext * sizeof(Extremum));
    if (!ext) return r;
    n = collect_extrema(ts, start, ext, max_ext);

    if (n < 4) {
        free(ext);
        r.code = -2;
        r.value = -2;
        return r;
    }

    /* fixed point / near-constant: all extrema very close */
    {
        int same = 1;
        for (i = 1; i < n; i++) {
            if (fabs(ext[i].value - ext[0].value) > tol) { same = 0; break; }
        }
        if (same) {
            free(ext);
            r.code = 0;
            r.value = 0;
            return r;
        }
    }

    for (k = 2; k < n / 2 && k < 200; k += 2) {
        t = 0;
        for (i = 0; i < k && i + k < n; i++) {
            if (fabs(ext[i].value - ext[i + k].value) <= tol) t++;
        }
        if (t == k) {
            free(ext);
            r.code = 0;
            r.value = k / 2.0;
            return r;
        }
    }

    free(ext);
    r.code = -1;
    r.value = -1;
    return r;
}

/* extrema output is handled by the analyze command */
