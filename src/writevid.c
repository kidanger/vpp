#include <limits.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>

#include "iio.h"
#include "vpp.h"

int main(int c, char** v)
{
    if (c != 3)
        return fprintf(stderr, "usage: readvid input format\n"), 1;
    const char* formatexpr = v[2];

    // link to the pipeline
    int w,h,d;
    FILE* in = vpp_init_input(v[1], &w, &h, &d);
    if (!in)
        return fprintf(stderr, "writevid: cannot initialize input '%s'\n", v[1]), 1;

    // allocate memory for the frame
    float* frame = malloc(w*h*d*sizeof*frame);

    int i = 0;
    while (vpp_read_frame(in, frame, w, h, d)) {
        // format the filename for the current frame
        char file[PATH_MAX];
        snprintf(file, PATH_MAX, formatexpr, i);

        // save the frame
        iio_write_image_float_vec(file, frame, w, h, d);
        i++;
    }

    free(frame);
    fclose(in);
    return 0;
}

