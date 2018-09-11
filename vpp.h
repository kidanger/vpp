#ifndef VPP_HEADER
#define VPP_HEADER

FILE* vpp_init_input(const char* filename, int* w, int* h, int* d, int* n);

FILE* vpp_init_output(const char* filename, int w, int h, int d, int n);

int vpp_read_frame(FILE* in, float* frame, int w, int h, int d);

int vpp_write_frame(FILE* out, float* frame, int w, int h, int d);

#endif

