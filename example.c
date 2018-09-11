#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vpp.h"

int main(int c, char** v) {
    assert(c == 3);

    // link to the pipeline
    int w,h,d,n;
    FILE* in = vpp_init_input(v[1], &w, &h, &d, &n);
    FILE* out = vpp_init_output(v[2], w, h, d, n);
    assert(in && out);

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
        vpp_write_frame(out, buffer, w, h, d);
        num++;
    }

    free(frame);
    free(buffer);
    fclose(in);
    fclose(out);
    return 0;
}

