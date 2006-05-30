/* test of fft2f.c */

#include <math.h>
#include <stdio.h>
#include "fft2f.c"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MAX(x,y) ((x) > (y) ? (x) : (y))

/* random number generator, 0 <= RND < 1 */
#define RND(p) ((*(p) = (*(p) * 7141 + 54773) % 259200) * (1.0 / 259200))

#define NMAX 8192

main()
{
    void cdft(int, double, double, double *);
    void rdft(int, double, double, double *);
    void ddct(int, double, double, double *);
    void ddst(int, double, double, double *);
    void dfct(int, double, double, double *);
    void dfst(int, double, double, double *);
    void putdata(int nini, int nend, double *a);
    double errorcheck(int nini, int nend, double scale, double *a);
    int n;
    double a[NMAX + 1], err;

    printf("data length n=? \n");
    scanf("%d", &n);

    /* check of CDFT */
    putdata(0, n - 1, a);
    cdft(n, cos(2 * M_PI / n), sin(2 * M_PI / n), a);
    cdft(n, cos(2 * M_PI / n), -sin(2 * M_PI / n), a);
    err = errorcheck(0, n - 1, 2.0 / n, a);
    printf("cdft err= %lg \n", err);

    /* check of RDFT */
    putdata(0, n - 1, a);
    rdft(n, cos(M_PI / n), sin(M_PI / n), a);
    rdft(n, cos(M_PI / n), -sin(M_PI / n), a);
    err = errorcheck(0, n - 1, 2.0 / n, a);
    printf("rdft err= %lg \n", err);

    /* check of DDCT */
    putdata(0, n - 1, a);
    ddct(n, cos(M_PI / (2 * n)), sin(M_PI / (2 * n)), a);
    ddct(n, cos(M_PI / (2 * n)), -sin(M_PI / (2 * n)), a);
    a[0] *= 0.5;
    err = errorcheck(0, n - 1, 2.0 / n, a);
    printf("ddct err= %lg \n", err);

    /* check of DDST */
    putdata(0, n - 1, a);
    ddst(n, cos(M_PI / (2 * n)), sin(M_PI / (2 * n)), a);
    ddst(n, cos(M_PI / (2 * n)), -sin(M_PI / (2 * n)), a);
    a[0] *= 0.5;
    err = errorcheck(0, n - 1, 2.0 / n, a);
    printf("ddst err= %lg \n", err);

    /* check of DFCT */
    putdata(0, n, a);
    a[0] *= 0.5;
    a[n] *= 0.5;
    dfct(n, cos(M_PI / n), sin(M_PI / n), a);
    a[0] *= 0.5;
    a[n] *= 0.5;
    dfct(n, cos(M_PI / n), sin(M_PI / n), a);
    err = errorcheck(0, n, 2.0 / n, a);
    printf("dfct err= %lg \n", err);

    /* check of DFST */
    putdata(1, n - 1, a);
    dfst(n, cos(M_PI / n), sin(M_PI / n), a);
    dfst(n, cos(M_PI / n), sin(M_PI / n), a);
    err = errorcheck(1, n - 1, 2.0 / n, a);
    printf("dfst err= %lg \n", err);
}


void putdata(int nini, int nend, double *a)
{
    int j, seed = 0;

    for (j = nini; j <= nend; j++) {
        a[j] = RND(&seed);
    }
}


double errorcheck(int nini, int nend, double scale, double *a)
{
    int j, seed = 0;
    double err = 0, e;

    for (j = nini; j <= nend; j++) {
        e = RND(&seed) - a[j] * scale;
        err = MAX(err, fabs(e));
    }
    return err;
}

