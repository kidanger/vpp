#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "iio.h"

#include "vpp.h"

int main(int c, char** v)
{
    c--; v++;
    assert(c >= 5); // assumes one or more inputs and one output

    int w,h,d;
    int ninputs = (c - 2) / 2;
    char* files[ninputs+1];
    FILE* ins[ninputs];
    ins[0] = vpp_init_input(v[0], &w, &h, &d);
    assert(ins[0]);
    for (int i = 1; i < ninputs; i++) {
        int w2,h2,d2;
        ins[i] = vpp_init_input(v[i*2], &w2, &h2, &d2);
        assert(ins[i]);
        assert(w == w2 && h == h2 && d == d2);
    }

    for (int i = 0; i < ninputs+1; i++) {
        files[i] = v[1+i*2];
    }

    char* prog = v[c-1];

    float* frames[ninputs];
    for (int i = 0; i < ninputs; i++) {
        frames[i] = malloc(w*h*d*sizeof(float));
    }

    FILE* out = NULL;
    int ow,oh,od;
    while (1) {
        for (int i = 0; i < ninputs; i++) {
            if (!vpp_read_frame(ins[i], frames[i], w, h, d))
                return 0;
        }

        for (int i = 0; i < ninputs; i++) {
            iio_write_image_float_vec(files[i], frames[i], w, h, d);
        }

        if (system(prog))
            break;

        int ow2, oh2, od2;
        float* result = iio_read_image_float_vec(files[ninputs], &ow2, &oh2, &od2);
        if (!out) {
            ow = ow2;
            oh = oh2;
            od = od2;
            out = vpp_init_output(v[c-3], ow, oh, od);
            assert(out);
        }
        assert(ow == ow2 && oh == oh2 && od == od2);

        if (!vpp_write_frame(out, result, ow, oh, od))
            break;
        free(result);
    }
}

