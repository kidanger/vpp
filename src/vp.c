#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "tinyexpr.c"
#include "bigbuf.c"

#define VPP_IMPLEMENTATION
#include "vpp.h"

void dup_(int c, char** v)
{
    signal(SIGPIPE, SIG_IGN);

    if (c != 3)
        return (void) fprintf(stderr, "usage: dup input output1 output2\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "dup: cannot initialize input '%s'\n", v[0]);
    int n = 2;
    FILE* outs[n];
    outs[0] = vpp_init_output(v[1], w, h, d);
    outs[1] = vpp_init_output(v[2], w, h, d);
    if (!outs[0])
        return (void) fprintf(stderr, "dup: cannot initialize output '%s'\n", v[1]);
    if (!outs[1])
        return (void) fprintf(stderr, "dup: cannot initialize output '%s'\n", v[2]);

    struct bigbuf buffers[2] = {
        bigbuf_init(1<<20),
        bigbuf_init(1<<20),
    };
    int fds[2] = { fileno(outs[0]),  fileno(outs[1]) };
    int fdmax = (fds[0] > fds[1] ? fds[0] : fds[1]) + 1;

    /* pipes are write-blocking even if ready from select
     * except in non-block mode (and with n > PIPE_BUF) */
    for (int i = 0; i < 2; i++) {
        int flags = fcntl(fds[i], F_GETFL, 0);
        fcntl(fds[i], F_SETFL, flags | O_NONBLOCK);
    }

    float* frame = malloc(w*h*d*sizeof*frame);
    while (n) {
        if (in) {
            // as long as we have no backlog for one output, we can read a frame
            if (!vpp_read_frame(in, frame, w, h, d)) {
                fclose(in);
                in = NULL;
            } else {
                for (int i = 0; i < n; i++) {
                    bigbuf_write(&buffers[i], (char*) frame, w*h*d*sizeof(float));
                }
            }
        } else {
            // someone is starving, but there is no more frames
            // this means we need to close it
            for (int i = 0; i < n; i++) {
                if (!bigbuf_has_data(&buffers[i])) {
                    close(fds[i]);
                    fds[i] = -1;
                    bigbuf_free(&buffers[i]);
                    memmove(fds+i, fds+i+1, (n-i-1)*sizeof(*fds));
                    memmove(buffers+i, buffers+i+1, (n-i-1)*sizeof(*buffers));
                    n--;
                }
            }
        }

        while (n) {
            // if someone is starving, go get some frames
            int starving = 0;
            for (int i = 0; i < n; i++) {
                starving |= !bigbuf_has_data(&buffers[i]);
            }
            if (starving)
                break;

            // wait for some pipes to be ready
            fd_set wset;
            FD_ZERO(&wset);
            for (int i = 0; i < n; i++) {
                FD_SET(fds[i], &wset);
            }
            if (select(fdmax, NULL, &wset, NULL, NULL) < 0)
                return;

            for (int i = 0; i < n; i++) {
                if (FD_ISSET(fds[i], &wset)) {
                    char* data;
                    size_t size = bigbuf_ask_read(&buffers[i], &data);
                    ssize_t writes;
                    if ((writes = write(fds[i], data, size)) < 0) {
                        // one of the reader has finished, remove it from the list
                        if (errno == EPIPE) {
                            close(fds[i]);
                            fds[i] = -1;
                            bigbuf_free(&buffers[i]);
                            memmove(fds+i, fds+i+1, (n-i-1)*sizeof(*fds));
                            memmove(buffers+i, buffers+i+1, (n-i-1)*sizeof(*buffers));
                            n--;
                        } else {
                            return;
                        }
                    } else {
                        bigbuf_commit_read(&buffers[i], writes);
                    }
                }
            }
        }
    }
}

void buf(int c, char** v)
{
    if (c != 2)
        return (void) fprintf(stderr, "usage: buf input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "buf: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "buf: cannot initialize output '%s'\n", v[1]);

    int fdin = fileno(in);
    int fdout = fileno(out);
    int fdmax = (fdin > fdout ? fdin : fdout) + 1;

    struct bigbuf buf = bigbuf_init(1<<20);

    float* frame = malloc(w*h*d*sizeof*frame);
    while (fdin != -1 || bigbuf_has_data(&buf)) {
        fd_set rset, wset;
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        if (fdin != -1)
            FD_SET(fdin, &rset);
        if (bigbuf_has_data(&buf))
            FD_SET(fdout, &wset);

        if (select(fdmax, &rset, &wset, NULL, NULL) < 0)
            break;

        if (fdin != -1 && FD_ISSET(fdin, &rset)) {
            char* data;
            size_t size = bigbuf_ask_write(&buf, &data);
            ssize_t s = read(fdin, data, size);
            if (s < 0)
                break;

            if (s == 0) { // end of file
                fclose(in);
                fdin = -1;
            } else {
                bigbuf_commit_write(&buf, s);
            }
        }

        if (bigbuf_has_data(&buf) && FD_ISSET(fdout, &wset)) {
            char* data;
            size_t size = bigbuf_ask_read(&buf, &data);
            ssize_t writes;
            if ((writes = write(fdout, data, size)) < 0)
                break;

            bigbuf_commit_read(&buf, writes);
        }
    }
    bigbuf_free(&buf);
}

void take(int c, char** v)
{
    if (c != 3)
        return (void) fprintf(stderr, "usage: take input output n\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "take: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    int n = atoi(v[2]);
    if (!out)
        return (void) fprintf(stderr, "take: cannot initialize output '%s'\n", v[1]);

    float* frame = malloc(w*h*d*sizeof*frame);

    for (int i = 0; i < n; i++) {
        if (!vpp_read_frame(in, frame, w, h, d)
            || !vpp_write_frame(out, frame, w, h, d))
            break;
    }
    fclose(in);
    fclose(out);
}

void repeat(int c, char** v)
{
    if (c != 2)
        return (void) fprintf(stderr, "usage: repeat input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "repeat: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "repeat: cannot initialize output '%s'\n", v[1]);

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
    if (c != 2)
        return (void) fprintf(stderr, "usage: first input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "first: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "first: cannot initialize output '%s'\n", v[1]);

    float* frame = malloc(w*h*d*sizeof*frame);

    if (!vpp_read_frame(in, frame, w, h, d))
        return;
    vpp_write_frame(out, frame, w, h, d);
}

void last(int c, char** v)
{
    if (c != 2)
        return (void) fprintf(stderr, "usage: last input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "last: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "last: cannot initialize output '%s'\n", v[1]);

    float* frame = malloc(w*h*d*sizeof*frame);

    while (vpp_read_frame(in, frame, w, h, d))
        ;
    vpp_write_frame(out, frame, w, h, d);
}

void skip(int c, char** v)
{
    if (c != 3)
        return (void) fprintf(stderr, "usage: skip input output n\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "skip: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    int n = atoi(v[2]);
    if (!out)
        return (void) fprintf(stderr, "skip: cannot initialize output '%s'\n", v[1]);

    float* frame = malloc(w*h*d*sizeof*frame);

    for (int i = 0; i < n; i++) {
        if (!vpp_read_frame(in, frame, w, h, d)) {
            goto end;
        }
    }

    while (vpp_read_frame(in, frame, w, h, d)
           && vpp_write_frame(out, frame, w, h, d))
        ;
end:
    fclose(in);
    fclose(out);
}

void concat(int c, char** v)
{
    if (c != 3)
        return (void) fprintf(stderr, "usage: concat input1 input2 output\n");

    int ws[2], hs[2], ds[2];
    FILE* ins[2];
    if (!vpp_init_inputs(2, ins, (const char**)v, ws, hs, ds))
        return (void) fprintf(stderr, "concat: cannot initialize one of the inputs\n");
    if (ws[0] != ws[1] || hs[0] != hs[1] || ds[0] != ds[1])
        return (void) fprintf(stderr, "concat: inputs should be of the same dimensions\n");

    int w = ws[0];
    int h = hs[0];
    int d = ds[0];
    FILE* out = vpp_init_output(v[2], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "concat: cannot initialize output '%s'\n", v[1]);

    float* frame = malloc(w*h*d*sizeof*frame);

    while (vpp_read_frame(ins[0], frame, w, h, d))
        if (!vpp_write_frame(out, frame, w, h, d))
            return;
    while (vpp_read_frame(ins[1], frame, w, h, d)
           && vpp_write_frame(out, frame, w, h, d))
        ;
}

void timeinterval(int c, char** v)
{
    if (c != 1)
        return (void) fprintf(stderr, "usage: timeinterval input\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "timeinterval: cannot initialize input '%s'\n", v[0]);

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
    if (c != 2)
        return (void) fprintf(stderr, "usage: average input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "average: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "average: cannot initialize output '%s'\n", v[1]);

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
    if (c != 2)
        return (void) fprintf(stderr, "usage: count input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "count: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], 1, 1, 1);
    if (!out)
        return (void) fprintf(stderr, "count: cannot initialize output '%s'\n", v[1]);

    float* frame = malloc(w*h*d*sizeof*frame);

    float num = 0;
    while (vpp_read_frame(in, frame, w, h, d)) {
        num++;
    }
    vpp_write_frame(out, &num, 1, 1, 1);
}

void max(int c, char** v)
{
    if (c != 2)
        return (void) fprintf(stderr, "usage: max input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "max: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "max: cannot initialize output '%s'\n", v[1]);

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
    if (c != 2)
        return (void) fprintf(stderr, "usage: min input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "min: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "min: cannot initialize output '%s'\n", v[1]);

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
    if (c != 2)
        return (void) fprintf(stderr, "usage: sum input output\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "sum: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "sum: cannot initialize output '%s'\n", v[1]);

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
    if (c != 3)
        return (void) fprintf(stderr, "usage: map input output expression\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "map: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "map: cannot initialize output '%s'\n", v[1]);

    double x;
    te_variable vars[] = {{"x", &x, 0, 0}};
    int err;
    te_expr *n = te_compile(v[2], vars, 1, &err);
    if (!n)
        return (void) fprintf(stderr, "map: parsing error in expression '%s'\n", v[2]);

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
    if (c != 3)
        return (void) fprintf(stderr, "usage: reduce input output expression\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "reduce: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "reduce: cannot initialize output '%s'\n", v[1]);

    double x, y;
    te_variable vars[] = {{"x", &x, 0, 0}, {"y", &y, 0, 0}};
    int err;
    te_expr *n = te_compile(v[2], vars, 2, &err);
    if (!n)
        return (void) fprintf(stderr, "reduce: parsing error in expression '%s'\n", v[2]);

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
    if (c != 3)
        return (void) fprintf(stderr, "usage: scan input output expression\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "scan: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "scan: cannot initialize output '%s'\n", v[1]);

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

void zip(int c, char** v)
{
    if (c != 4)
        return (void) fprintf(stderr, "usage: zip input1 input2 output expression\n");

    int n = 2;
    int ws[n], hs[n], ds[n];
    FILE *ins[n];
    if (!vpp_init_inputs(n, ins, (const char**)v, ws, hs, ds))
        return (void) fprintf(stderr, "zip: cannot initialize one of the inputs\n");

    int w = ws[0];
    int h = hs[0];
    int d = ds[0];

    float* frames[n];
    for (int i = 0; i < n; i++) {
        frames[i] = malloc(w*h*d*sizeof(float));
        if (w != ws[i] || h != hs[i] || d != ds[i])
            return (void) fprintf(stderr, "zip: input %d does not have the same dimensions\n", i+1);
    }

    FILE* out = vpp_init_output(v[2], w, h, d);
    if (!out)
        return (void) fprintf(stderr, "zip: cannot initialize output '%s'\n", v[2]);

    double x, y;
    te_variable vars[] = {{"x", &x, 0, 0}, {"y", &y, 0, 0}};
    int err;
    te_expr *expr = te_compile(v[3], vars, 2, &err);
    if (!expr) {
        fprintf(stderr, "parsing error in zip\n");
        return;
    }

    while (1) {
        for (int i = 0; i < n; i++) {
            if (!vpp_read_frame(ins[i], frames[i], w, h, d)) {
                return;
            }
        }
        for (int i = 0; i < w*h*d; i++) {
            x = frames[0][i];
            y = frames[1][i];
            frames[0][i] = te_eval(expr);
        }
        if (!vpp_write_frame(out, frames[0], w, h, d))
            break;
    }
    te_free(expr);
}

void framereduce(int c, char** v)
{
    if (c != 3)
        return (void) fprintf(stderr, "usage: framereduce input output expression\n");

    int w,h,d;
    FILE* in = vpp_init_input(v[0], &w, &h, &d);
    if (!in)
        return (void) fprintf(stderr, "framereduce: cannot initialize input '%s'\n", v[0]);
    FILE* out = vpp_init_output(v[1], 1, 1, 1);
    if (!out)
        return (void) fprintf(stderr, "framereduce: cannot initialize output '%s'\n", v[1]);

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
    if (c < 2)
        return fprintf(stderr, "usage: vp command [args...]\n"), 1;

    char* name = v[1];
    c -= 2;
    v += 2;
    // many operators are inspired by http://reactivex.io/documentation/operators.html
    if (0) {
    } else if (!strcmp(name, "buf")) {
        return buf(c, v), 0;
    } else if (!strcmp(name, "dup")) {
        return dup_(c, v), 0;
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
    } else if (!strcmp(name, "zip")) {
        return zip(c, v), 0;
    } else if (!strcmp(name, "framereduce")) {
        return framereduce(c, v), 0;
    }

    // TODO: pmap, preduce, pscan: plambda

    fprintf(stderr, "%s: unrecognized program '%s'\n", v[-2], name);
    return 0;
}

