#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VPP_TAG "VPP"

FILE* vpp_init_input(const char* filename, int* w, int* h, int* d)
{
    char tag[4];
    FILE* in = !strcmp(filename, "-") ? stdin : fopen(filename, "rb");

    if (!in)
        goto err;
    if (!fread(tag, sizeof(tag), 1, in) || strncmp(tag, VPP_TAG, 4) != 0)
        goto err;
    if (   !fread(w, sizeof*w, 1, in)
        || !fread(h, sizeof*h, 1, in)
        || !fread(d, sizeof*d, 1, in))
        goto err;

    return in;
err:
    if (in)
        fclose(in);
    return NULL;
}

FILE* vpp_init_output(const char* filename, int w, int h, int d)
{
    char tag[4] = {VPP_TAG};
    FILE* out = !strcmp(filename, "-") ? stdout : fopen(filename, "wb");

    if (!out)
        goto err;
    if (!fwrite(tag, sizeof(tag), 1, out))
        goto err;
    if (   !fwrite(&w, sizeof w, 1, out)
        || !fwrite(&h, sizeof h, 1, out)
        || !fwrite(&d, sizeof d, 1, out))
        goto err;

    return out;
err:
    if (out)
        fclose(out);
    return NULL;
}

int vpp_read_frame(FILE* in, float* frame, int w, int h, int d)
{
    return fread(frame, sizeof*frame, w*h*d, in) == (size_t) w*h*d;
}

int vpp_write_frame(FILE* out, float* frame, int w, int h, int d)
{
    return fwrite(frame, sizeof*frame, w*h*d, out) == (size_t) w*h*d;
}

