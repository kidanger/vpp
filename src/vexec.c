#include <stdlib.h>
#include <stdio.h>

#include "iio.h"

#define VPP_IMPLEMENTATION
#include "vpp.h"

int main(int c, char** v)
{
    c--; v++;
    if (c < 5)
        return fprintf(stderr, "usage: vexec input1 input1-file [input2 input2-files...] command output output-file\n"), 1;

    int ninputs = (c - 2) / 2;
    FILE* ins[ninputs];
    int ws[ninputs], hs[ninputs], ds[ninputs];
    const char* inputfiles[ninputs];
    for (int i = 0; i < ninputs; i++) {
        inputfiles[i] = v[i*2];
    }
    if (!vpp_init_inputs(ninputs, ins, inputfiles, ws, hs, ds))
        return fprintf(stderr, "vexec: cannot initialize one of the inputs\n"), 1;

    int w = ws[0];
    int h = hs[0];
    int d = ds[0];
    for (int i = 0; i < ninputs; i++) {
        if (w != ws[i] || h != hs[i] || d != ds[i])
            return fprintf(stderr, "vexec: input %d does not have the same dimensions\n", i+1), 1;
    }

    char* files[ninputs+1];
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
            if (!out)
                return fprintf(stderr, "vexec: cannot initialize output '%s'\n", v[c-3]), 1;
        } else if (ow != ow2 || oh != oh2 || od != od2) {
                return fprintf(stderr, "vexec: a frame does not have the same size\n"), 1;
        }

        if (!vpp_write_frame(out, result, ow, oh, od))
            break;
        free(result);
    }
}

