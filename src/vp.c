#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tinyexpr.c"

#include "vpp.h"

// many operators are inspired by http://reactivex.io/documentation/operators.html

void take(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    int n = atoi(v[2]);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);

    for (int i = 0; i < n; i++) {
        if (!vpp_read_frame(in, frame, w, h, d)
            || !vpp_write_frame(out, frame, w, h, d))
            break;
    }
}

void repeat(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);

    if (!vpp_read_frame(in, frame, w, h, d))
        return;
    fclose(in);

    while (1) {
        if (!vpp_write_frame(out, frame, w, h, d))
            break;
    }
}

void first(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);

    if (!vpp_read_frame(in, frame, w, h, d))
        return;
    vpp_write_frame(out, frame, w, h, d);
}

void last(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);

    while (vpp_read_frame(in, frame, w, h, d))
        ;
    vpp_write_frame(out, frame, w, h, d);
}

void skip(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    int n = atoi(v[2]);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);

    for (int i = 0; i < n; i++) {
        if (!vpp_read_frame(in, frame, w, h, d))
            return;
    }

    while (vpp_read_frame(in, frame, w, h, d)
           && vpp_write_frame(out, frame, w, h, d))
        ;
}

void concat(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    int w2,h2,d2;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* in2 = vpp_init_input(v[1], &w2, &h2, &d2);
    FILE* out = vpp_init_output(v[2], w, h, d);
    assert(in && in2 && out);
    assert(w == w2 && h == h2 && d == d2);

    float* frame = malloc(w*h*d*sizeof*frame);

    while (vpp_read_frame(in, frame, w, h, d))
        if (!vpp_write_frame(out, frame, w, h, d))
            return;
    while (vpp_read_frame(in2, frame, w, h, d)
           && vpp_write_frame(out, frame, w, h, d))
        ;
}

void timeinterval(int c, char** v)
{
    assert(c == 1);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    assert(in);

    float* frame = malloc(w*h*d*sizeof*frame);
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    while (vpp_read_frame(in, frame, w, h, d)) {
        clock_gettime(CLOCK_REALTIME, &end);

        double diff = (end.tv_sec - start.tv_sec)
            + (end.tv_nsec - start.tv_nsec)/1000000000.;
        printf("%fs\n", diff);
        start = end;
    }
}

void average(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    memset(buffer, 0, sizeof(*buffer)*w*h*d);

    int num = 0;
    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            buffer[i] = (frame[i] + num * buffer[i]) / (num + 1);
        }
        num++;
    }
    vpp_write_frame(out, frame, w, h, d);
}

void count(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], 1, 1, 1);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);

    float num = 0;
    while (vpp_read_frame(in, frame, w, h, d)) {
        num++;
    }
    vpp_write_frame(out, &num, 1, 1, 1);
}

void max(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    if (!vpp_read_frame(in, buffer, w, h, d))
        return;

    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            buffer[i] = fmaxf(buffer[i], frame[i]);
        }
    }
    vpp_write_frame(out, buffer, w, h, d);
}

void min(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    if (!vpp_read_frame(in, buffer, w, h, d))
        return;

    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            buffer[i] = fminf(buffer[i], frame[i]);
        }
    }
    vpp_write_frame(out, buffer, w, h, d);
}

void sum(int c, char** v)
{
    assert(c == 2);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    if (!vpp_read_frame(in, buffer, w, h, d))
        return;

    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            buffer[i] += frame[i];
        }
    }
    vpp_write_frame(out, buffer, w, h, d);
}

void map(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    double x;
    te_variable vars[] = {{"x", &x, 0, 0}};
    int err;
    te_expr *n = te_compile(v[2], vars, 1, &err);
    if (!n) {
        fprintf(stderr, "parsing error in map\n");
        return;
    }

    float* frame = malloc(w*h*d*sizeof*frame);
    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            x = frame[i];
            frame[i] = te_eval(n);
        }
        if (!vpp_write_frame(out, frame, w, h, d))
            break;
    }
    te_free(n);
}

void reduce(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    double x, y;
    te_variable vars[] = {{"x", &x, 0, 0}, {"y", &y, 0, 0}};
    int err;
    te_expr *n = te_compile(v[2], vars, 2, &err);
    if (!n) {
        fprintf(stderr, "parsing error in reduce\n");
        return;
    }

    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    if (!vpp_read_frame(in, buffer, w, h, d))
        return;

    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            x = buffer[i];
            y = frame[i];
            buffer[i] = te_eval(n);
        }
    }
    vpp_write_frame(out, buffer, w, h, d);
    te_free(n);
}

void scan(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], w, h, d);
    assert(in && out);

    double x, y;
    te_variable vars[] = {{"x", &x, 0, 0}, {"y", &y, 0, 0}};
    int err;
    te_expr *n = te_compile(v[2], vars, 2, &err);
    if (!n) {
        fprintf(stderr, "parsing error in scan\n");
        return;
    }

    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    if (!vpp_read_frame(in, buffer, w, h, d))
        return;

    while (vpp_read_frame(in, frame, w, h, d)) {
        for (int i = 0; i < w*h*d; i++) {
            x = buffer[i];
            y = frame[i];
            buffer[i] = te_eval(n);
        }
        if (!vpp_write_frame(out, buffer, w, h, d))
            break;
    }
    te_free(n);
}

void framereduce(int c, char** v)
{
    assert(c == 3);
    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    FILE* out = vpp_init_output(v[1], 1, 1, 1);
    assert(in && out);

    double x, y;
    te_variable vars[] = {{"x", &x, 0, 0}, {"y", &y, 0, 0}};
    int err;
    te_expr *n = te_compile(v[2], vars, 2, &err);
    if (!n) {
        fprintf(stderr, "parsing error in reduce\n");
        return;
    }

    float* frame = malloc(w*h*d*sizeof*frame);
    while (vpp_read_frame(in, frame, w, h, d)) {
        x = frame[0];
        for (int i = 1; i < w*h*d; i++) {
            y = frame[i];
            x = te_eval(n);
        }
        float val = x;
        if (!vpp_write_frame(out, &val, 1, 1, 1))
            break;
    }
    te_free(n);
}

int main(int c, char** v)
{
    assert(c >= 2);

    char* name = v[1];
    c -= 2;
    v += 2;
    if (0) {
    } else if (!strcmp(name, "take")) {
        return take(c, v), 0;
    } else if (!strcmp(name, "repeat")) {
        return repeat(c, v), 0;
    } else if (!strcmp(name, "first")) {
        return first(c, v), 0;
    } else if (!strcmp(name, "last")) {
        return last(c, v), 0;
    } else if (!strcmp(name, "skip")) {
        return skip(c, v), 0;
    } else if (!strcmp(name, "concat")) {
        return concat(c, v), 0;
    } else if (!strcmp(name, "timeinterval")) {
        return timeinterval(c, v), 0;
    } else if (!strcmp(name, "average")) {
        return average(c, v), 0;
    } else if (!strcmp(name, "count")) {
        return count(c, v), 0;
    } else if (!strcmp(name, "max")) {
        return max(c, v), 0;
    } else if (!strcmp(name, "min")) {
        return min(c, v), 0;
    } else if (!strcmp(name, "sum")) {
        return sum(c, v), 0;
    } else if (!strcmp(name, "map")) {
        return map(c, v), 0;
    } else if (!strcmp(name, "reduce")) {
        return reduce(c, v), 0;
    } else if (!strcmp(name, "scan")) {
        return scan(c, v), 0;
    } else if (!strcmp(name, "framereduce")) {
        return framereduce(c, v), 0;
    }

    // TODO: pmap, preduce, pscan: plambda

    fprintf(stderr, "%s: unrecognized program '%s'\n", v[-2], name);
    return 0;
}

