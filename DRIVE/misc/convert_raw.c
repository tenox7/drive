#include <stdio.h>

main(int argc, char **argv)
{
    int w, h;
    FILE *inf, *outf;
    int i, j, r, g, b, n;

    if (argc != 5) {
	fprintf(stderr, "Usage: convert_raw in_name out_name w h\n");
	exit(1);
    }

    inf = fopen(argv[1], "rb");
    outf = fopen(argv[2], "wb");
    if (!(inf && outf)) {
	fprintf(stderr, "Error opening files\n");
	exit(1);
    }
    w = atoi(argv[3]);
    h = atoi(argv[4]);

    fprintf(outf, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < h; i++) {
	for (j = 0; j < w; j++) {
	    n = getc(inf);
	    r = (n >> 5) & 7;
	    g = (n >> 2) & 7;
	    b = (n     ) & 3;
	    r = (r << 5) | (r << 2) | (r >> 1);
	    g = (g << 5) | (g << 2) | (g >> 1);
	    b = (b << 6) | (b << 4) | (b << 2) | b;
	    fputc(r, outf);
	    fputc(g, outf);
	    fputc(b, outf);
	}
    }
}
