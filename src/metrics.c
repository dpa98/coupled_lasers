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
