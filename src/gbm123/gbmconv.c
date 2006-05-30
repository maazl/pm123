/*

gbmconv.c - Converts one Bitmap file supported by Generalized Bitmap Module to another one

*/

/*...sincludes:0:*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#if defined(AIX) || defined(LINUX) || defined(SUN) || defined(MAC)
#include <unistd.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#ifdef MAC
#include <types.h>
#include <stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "gbm.h"


static char progname[] = "gbmconv";

static void fatal(const char *fmt, ...)
{
    va_list    vars;
    char s[256+1];

    va_start(vars, fmt);
    vsprintf(s, fmt, vars);
    va_end(vars);
    fprintf(stderr, "%s: %s\n", progname, s);
    exit(1);
}

static void usage(void)
{
    int ft, n_ft;

    fprintf(stderr, "usage: %s fn1.ext{,opt} fn2.ext{,opt}\n", progname);
    fprintf(stderr, "       fn1.ext{,opt}  input  filename (with any format specific options)\n");
    fprintf(stderr, "       fn2.ext{,opt}  output filename (with any format specific options)\n");
    fprintf(stderr, "                      ext's are used to deduce desired bitmap file formats\n");

    gbm_init();
    gbm_query_n_filetypes(&n_ft);
    for ( ft = 0; ft < n_ft; ft++ )
    {
        GBMFT gbmft;

        gbm_query_filetype(ft, &gbmft);
        fprintf(stderr, "                      %s when ext in [%s]\n",
            gbmft.short_name, gbmft.extensions);
    }
    gbm_deinit();

    fprintf(stderr, "       opt's          bitmap format specific options\n");

    exit(1);
}

/* ------------------------------ */

int main(int argc, char *argv[])
{
    char     fn_src[500+1], fn_dst[500+1], *opt_src, *opt_dst;
    int      fd, ft_src, ft_dst, stride, flag;
    GBM_ERR  rc;
    GBMFT    gbmft;
    GBM      gbm;
    GBMRGB   gbmrgb[0x100];
    byte    *data;

    if ( argc < 3 )
        usage();

    strcpy(fn_src, argv[1]);
    strcpy(fn_dst, argv[2]);

    if ( (opt_src = strchr(fn_src, ',')) != NULL )
        *opt_src++ = '\0';
    else
        opt_src = "";

    if ( (opt_dst = strchr(fn_dst, ',')) != NULL )
        *opt_dst++ = '\0';
    else
        opt_dst = "";

    gbm_init();

    if ( gbm_guess_filetype(fn_src, &ft_src) != GBM_ERR_OK )
        fatal("can't guess bitmap file format for %s", fn_src);

    if ( gbm_guess_filetype(fn_dst, &ft_dst) != GBM_ERR_OK )
        fatal("can't guess bitmap file format for %s", fn_dst);

    if ( (fd = gbm_io_open(fn_src, GBM_O_RDONLY)) == -1 )
        fatal("can't open %s", fn_src);

    if ( (rc = gbm_read_header(fn_src, fd, ft_src, &gbm, opt_src)) != GBM_ERR_OK )
    {
        gbm_io_close(fd);
        fatal("can't read header of %s: %s", fn_src, gbm_err(rc));
    }

    gbm_query_filetype(ft_dst, &gbmft);
    switch ( gbm.bpp )
    {
        case 64: flag = GBM_FT_W64; break;
        case 48: flag = GBM_FT_W48; break;
        case 32: flag = GBM_FT_W32; break;
        case 24: flag = GBM_FT_W24; break;
        case  8: flag = GBM_FT_W8;  break;
        case  4: flag = GBM_FT_W4;  break;
        case  1: flag = GBM_FT_W1;  break;
        default: flag = 0;          break;
    }
    if ( (gbmft.flags & flag) == 0 )
    {
        gbm_io_close(fd);
        fatal("output bitmap format %s does not support writing %d bpp data",
            gbmft.short_name, gbm.bpp);
    }

    if ( (rc = gbm_read_palette(fd, ft_src, &gbm, gbmrgb)) != GBM_ERR_OK )
    {
        gbm_io_close(fd);
        fatal("can't read palette of %s: %s", fn_src, gbm_err(rc));
    }

    stride = ( ((gbm.w * gbm.bpp + 31)/32) * 4 );
    if ( (data = malloc((size_t) (stride * gbm.h))) == NULL )
    {
        gbm_io_close(fd);
        fatal("out of memory allocating %d bytes for input bitmap", stride * gbm.h);
    }

    if ( (rc = gbm_read_data(fd, ft_src, &gbm, data)) != GBM_ERR_OK )
    {
        free(data);
        gbm_io_close(fd);
        fatal("can't read bitmap data of %s: %s", fn_src, gbm_err(rc));
    }

    gbm_io_close(fd);

    if ( (fd = gbm_io_create(fn_dst, GBM_O_WRONLY)) == -1 )
    {
        free(data);
        fatal("can't create %s", fn_dst);
    }

    if ( (rc = gbm_write(fn_dst, fd, ft_dst, &gbm, gbmrgb, data, opt_dst)) != GBM_ERR_OK )
    {
        free(data);
        gbm_io_close(fd);
        remove(fn_dst);
        fatal("can't write %s: %s", fn_dst, gbm_err(rc));
    }

    free(data);

    gbm_io_close(fd);
    gbm_deinit();

    return 0;
}


