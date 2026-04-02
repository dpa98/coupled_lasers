#ifndef TS_H
#define TS_H

#include <stddef.h>

typedef struct {
    double *data;   /* row-major: rows * cols */
    int rows;
    int cols;
} TimeSeries;

int ts_load(const char *path, TimeSeries *ts);
void ts_free(TimeSeries *ts);
double ts_get(const TimeSeries *ts, int row, int col);

#endif
