#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>

#include "iio.h"
#define VPP_IMPLEMENTATION
#include "vpp.h"

static char** get_filenames(const char* globexpr, int* n)
{
    glob_t res;
    glob(globexpr, GLOB_TILDE | GLOB_BRACE, NULL, &res);

    char** files = malloc(sizeof*files * res.gl_pathc);
    for(unsigned int j = 0; j < res.gl_pathc; j++) {
        files[j] = strdup(res.gl_pathv[j]);
    }

    globfree(&res);
    *n = res.gl_pathc;
    return files;
}

int main(int c, char** v)
{
    if (c != 3)
        return fprintf(stderr, "usage: readvid glob output\n"), 1;

    // get filenames from the globbing expression
    int n;
    char** files = get_filenames(v[1], &n);
    if (n <= 0)
        return fprintf(stderr, "readvid: no images found from globbing\n"), 1;

    // load the first frame so that we know the size of the frames
    int w, h, d;
    float* frame = iio_read_image_float_vec(files[0], &w, &h, &d);
    free(frame);

    // link to the pipeline
    FILE* out = vpp_init_output(v[2], w, h, d);
    if (!out)
        return fprintf(stderr, "readvid: cannot initialize output '%s'\n", v[2]), 1;

    for (int i = 0; i < n; i++) {
        // load the frame and check dimensions
        int ow, oh, od;
        frame = iio_read_image_float_vec(files[i], &ow, &oh, &od);
        if (ow != w || oh != h || od != d)
            return fprintf(stderr, "readvid: image '%s' does not have the same size\n", files[i]), 1;

        // send the frame through the pipeline
        if (!vpp_write_frame(out, frame, w, h, d))
            break;

        free(frame);
    }

    for (int i = 0; i < n; i++) {
        free(files[i]);
    }
    free(files);
    fclose(out);
    return 0;
}

