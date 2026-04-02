#ifndef METRICS_H
#define METRICS_H

#include "ts.h"

typedef struct {
    int code;        /* 0 ok, <0 error/classification */
    double value;
    double aux1;
    double aux2;
} MetricResult;

MetricResult metric_error(const TimeSeries *ts, int transient);
MetricResult metric_period(const TimeSeries *ts, int transient);

#endif
