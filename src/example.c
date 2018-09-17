#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vpp.h"

int main(int c, char** v) {
    if (c != 3)
        return fprintf(stderr, "usage: example input output\n"), 1;

    // link to the pipeline
    int w,h,d;
    FILE* in = vpp_init_input(v[1], &w, &h, &d);
    if (!in)
        return fprintf(stderr, "example: cannot initialize input '%s'\n", v[0]), 1;
    FILE* out = vpp_init_output(v[2], w, h, d);
    if (!out)
        return fprintf(stderr, "example: cannot initialize output '%s'\n", v[1]), 1;

    // allocate memory for the frame and the accumulation
    float* frame = malloc(w*h*d*sizeof*frame);
    float* buffer = malloc(w*h*d*sizeof*buffer);
    memset(buffer, 0, sizeof(*buffer)*w*h*d);

    int num = 0;
    while (vpp_read_frame(in, frame, w, h, d)) {
        // process the frame: recursive average
        for (int i = 0; i < w*h*d; i++) {
            buffer[i] = (frame[i] + num * buffer[i]) / (num + 1);
        }

        // send the frame to the next step
        if (!vpp_write_frame(out, buffer, w, h, d))
            break;
        num++;
    }

    free(frame);
    free(buffer);
    fclose(in);
    fclose(out);
    return 0;
}

