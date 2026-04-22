#include "ts.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int count_cols(const char *line) {
    int cols = 0;
    const char *p = line;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        cols++;
        while (*p && !isspace((unsigned char)*p)) p++;
    }
    return cols;
}

int ts_load(const char *path, TimeSeries *ts) {
    FILE *f = fopen(path, "r");
    char line[4096];
    int rows = 0, cols = 0;
    int r, c;

    if (!f || !ts) {
        if (f) fclose(f);
        return -1;
    }
    ts->data = NULL;
    ts->rows = 0;
    ts->cols = 0;

    while (fgets(line, sizeof(line), f)) {
        int n = count_cols(line);
        if (n <= 0) continue;
        if (cols == 0) cols = n;
        if (n == cols) rows++;
    }
    if (rows == 0 || cols == 0) {
        fclose(f);
        return -1;
    }

    ts->data = (double *)malloc((size_t)rows * (size_t)cols * sizeof(double));
    if (!ts->data) {
        fclose(f);
        return -1;
    }

    rewind(f);
    r = 0;
    while (fgets(line, sizeof(line), f) && r < rows) {
        char *p = line;
        int n = count_cols(line);
        if (n != cols) continue;
        for (c = 0; c < cols; c++) {
            char *end = NULL;
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            ts->data[(size_t)r * (size_t)cols + (size_t)c] = strtod(p, &end);
            p = end ? end : p + 1;
        }
        r++;
    }
    fclose(f);
    ts->rows = r;
    ts->cols = cols;
    return 0;
}

void ts_free(TimeSeries *ts) {
    if (!ts) return;
    free(ts->data);
    ts->data = NULL;
    ts->rows = 0;
    ts->cols = 0;
}

double ts_get(const TimeSeries *ts, int row, int col) {
    if (!ts || !ts->data || row < 0 || col < 0 || row >= ts->rows || col >= ts->cols) return 0.0;
    return ts->data[(size_t)row * (size_t)ts->cols + (size_t)col];
}
