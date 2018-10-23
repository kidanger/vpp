#include <stdio.h>
#include <string.h>
#include <fcntl.h> // open, fnctl

#include "vpp.h"

#define VPP_TAG "VPP"

#ifdef VPP_OUTPUT_DOT
// taken from iio
// this compatible only with some OSes
#include <stdlib.h>
#include <unistd.h>
static const char *emptystring = "";
static const char *myname(void)
{
#define n 0x29a
    static char buf[n];
    long p = getpid();
    snprintf(buf, n, "/proc/%ld/cmdline", p);
    FILE *f = fopen(buf, "r");
    if (!f) return emptystring;
    int c, i = 0;
    while ((c = fgetc(f)) != EOF && i < n) {
#undef n
        buf[i] = c ? c : ' ';
        i += 1;
    }
    if (i) buf[i-1] = '\0';
    fclose(f);
    return buf;
}
#endif

static FILE* open_input(const char* filename)
{
    // open the file is non blocking mode in case this is a pipe
    // we don't want to block on opening pipes since the other end
    // will block until we open it (causing deadlocks with 2 pipes)
    int fd = !strcmp(filename, "-") ? 0 : open(filename, O_RDONLY, O_NONBLOCK);
    if (fd == -1)
        return NULL;

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    FILE* in = fdopen(fd, "rb");
    if (in)
        setvbuf(in, NULL, _IONBF, 0);
    return in;
}

static int read_header(FILE* in, int* w, int* h, int*d )
{
    char tag[4];
    if (!fread(tag, sizeof(tag), 1, in) || strncmp(tag, VPP_TAG, 4) != 0)
        return 0;
    if (   !fread(w, sizeof*w, 1, in)
        || !fread(h, sizeof*h, 1, in)
        || !fread(d, sizeof*d, 1, in))
        return 0;
    if (*w <= 0 || *h <= 0 || *d <= 0)
        return 0;

#ifdef VPP_OUTPUT_DOT
    if (getenv("VPP_OUTPUT_DOT")) {
        char parent[2048];
        char* p = parent;
        do {
            if (p - parent >= (long)sizeof(parent) || fread(p, 1, 1, in) != 1)
                return 0;
        } while (*p && p++);
        FILE* dot = fopen(getenv("VPP_OUTPUT_DOT"), "a");
        if (dot) {
            fprintf(dot, "\"%s\" -> \"%s\" [label=\"%dx%dx%d\"];\n",
                    parent, myname(), *w, *h, *d);
            fclose(dot);
        }
    }
#endif

    return 1;
}

FILE* vpp_init_input(const char* filename, int* w, int* h, int* d)
{
    FILE* in = open_input(filename);
    if (in && !read_header(in, w, h, d)) {
        fclose(in);
        return NULL;
    }
    return in;
}

int vpp_init_inputs(int n, FILE** files, const char** filenames, int* w, int* h, int* d)
{
    for (int i = 0; i < n; i++) {
        files[i] = NULL;
    }

    for (int i = 0; i < n; i++) {
        if (!(files[i] = open_input(filenames[i])))
            goto err;
    }

    for (int i = 0; i < n; i++) {
        if (!read_header(files[i], w+i, h+i, d+i)) {
            goto err;
        }
    }

    return 1;
err:
    for (int i = 0; i < n; i++) {
        if (files[i])
            fclose(files[i]);
    }
    return 0;
}

FILE* vpp_init_output(const char* filename, int w, int h, int d)
{
    char tag[4] = {VPP_TAG};
    FILE* out = !strcmp(filename, "-") ? stdout : fopen(filename, "wb");
    setvbuf(out, NULL, _IONBF, 0);

    if (!out)
        goto err;
    if (!fwrite(tag, sizeof(tag), 1, out))
        goto err;
    if (   !fwrite(&w, sizeof w, 1, out)
        || !fwrite(&h, sizeof h, 1, out)
        || !fwrite(&d, sizeof d, 1, out))
        goto err;

#ifdef VPP_OUTPUT_DOT
    if (getenv("VPP_OUTPUT_DOT")) {
        const char* n = myname();
        size_t len = strlen(n) + 1;
        if (fwrite(n, 1, len, out) != len)
            goto err;
    }
#endif

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

