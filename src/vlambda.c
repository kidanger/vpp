#include <stdio.h>
#include "vpp.h"

#define HIDE_ALL_MAINS
#include "plambda.c"

static int main_videos(int c, char **v)
{
	if (c < 2) {
		fprintf(stderr, "usage:\n\t%s in1 in2 ... \"plambda\"\n", *v);
		//                          0 1   2         c-1
		return EXIT_FAILURE;
	}
	char *filename_out = pick_option(&c, &v, "o", "-");

	struct plambda_program p[1];

	plambda_compile_program(p, v[c-1]);

	int n = c - 2;
	if (n > 0 && p->var->n == 0) {
		int maxplen = n*10 + strlen(v[c-1]) + 100;
		char newprogram[maxplen];
		add_hidden_variables(newprogram, maxplen, n, v[c-1]);
		plambda_compile_program(p, newprogram);
	}
	if (n != p->var->n && !(n == 1 && p->var->n == 0))
		fail("the program expects %d variables but %d images "
			 "were given", p->var->n, n);
	int w[n], h[n], pd[n];
	FILE *ins[n];
	float *x[n];
	FORI(n) {
		ins[i] = vpp_init_input(v[i+1], w + i, h + i, pd + i);
		assert(ins[i]);
		x[i] = malloc(w[i]*h[i]*pd[i]*sizeof(float));
	}

	if (n>1) FORI(n) if (!strstr(p->var->t[i], "hidden"))
		fprintf(stderr, "plambda correspondence \"%s\" = \"%s\"\n",
				p->var->t[i], v[i+1]);

	xsrand(100+SRAND());

	int pdreal = eval_dim(p, x, pd);

	FILE* fileout = vpp_init_output(filename_out, *w, *h, pdreal);
	assert(fileout);
	float *out = xmalloc(*w * *h * pdreal * sizeof*out);

	while (1) {
		FORI(n) {
			if (!vpp_read_frame(ins[i], x[i], w[i], h[i], pd[i]))
				return 0;
		}

		int opd = run_program_vectorially(out, pdreal, p, x, w, h, pd);
		assert(opd == pdreal);
		if (!vpp_write_frame(fileout, out, *w, *h, opd))
			break;
	}

	FORI(n) free(x[i]);
	free(out);
	collection_of_varnames_end(p->var);

	return EXIT_SUCCESS;
}

int main(int c, char** v)
{
	if (c == 1) return print_help(*v, 0);
	if (c == 2 && 0 == strcmp(v[1], "-h")) return print_help(*v,0);
	if (c == 2 && 0 == strcmp(v[1], "--help")) return print_help(*v,1);
	if (c == 2 && 0 == strcmp(v[1], "--version")) return print_version();
	if (c == 2 && 0 == strcmp(v[1], "--man")) return do_man();

	int (*f)(int, char**) = **v=='c' ? main_calc : main_videos;
	if (f == main_videos && c > 2 && 0 == strcmp(v[1], "-c")) {
		for (int i = 1; i <= c; i++)
			v[i] = v[i+1];
		f = main_calc;
		c -= 1;
	}
	if (f == main_videos && c == 2) {
		char *vv[3] = { v[0], "-", v[1] };
		return f(3, vv);
	}
	return f(c,v);
}

