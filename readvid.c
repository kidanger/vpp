#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <glob.h>

#include "iio.h"
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
    assert(c == 3);

    // get filenames from the globbing expression
    int n;
    char** files = get_filenames(v[1], &n);
    assert(n > 0);

    // load the first frame so that we know the size of the frames
    int w, h, d;
    float* frame = iio_read_image_float_vec(files[0], &w, &h, &d);
    free(frame);

    // link to the pipeline
    FILE* out = vpp_init_output(v[2], w, h, d, n);
    assert(out);

    for (int i = 0; i < n; i++) {
        // load the frame and check dimensions
        int ow, oh, od;
        frame = iio_read_image_float_vec(files[i], &ow, &oh, &od);
        assert(ow == w);
        assert(oh == h);
        assert(od == d);

        // send the frame through the pipeline
        vpp_write_frame(out, frame, w, h, d);

        free(frame);
    }

    for (int i = 0; i < n; i++) {
        free(files[i]);
    }
    free(files);
    fclose(out);
    return 0;
}

