/*

gbmhdr.c - Display General Bitmaps header

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

/*...vgbm\46\h:0:*/
/*...e*/

static char progname[] = "gbmhdr";

/*...susage:0:*/
static void usage(void)
	{
	int ft, n_ft;

	fprintf(stderr, "usage: %s [-g] [-s] [--] {fn.ext{,opt}}\n", progname);
	fprintf(stderr, "       -g            don't guess bitmap format, try each type\n");
	fprintf(stderr, "       -s            be silent about errors\n");
	fprintf(stderr, "       -c            show all contained bitmaps of multipage images\n");
	fprintf(stderr, "                     (this option discards all user options except ext_bpp)\n");
	fprintf(stderr, "       fn.ext{,opt}  input filenames (with any format specific options)\n");
	gbm_init();
	gbm_query_n_filetypes(&n_ft);
	for ( ft = 0; ft < n_ft; ft++ )
		{
		GBMFT gbmft;

		gbm_query_filetype(ft, &gbmft);
		fprintf(stderr, "                     %s when ext in [%s]\n",
			gbmft.short_name, gbmft.extensions);
		}
	gbm_deinit();

	fprintf(stderr, "       opt           bitmap format specific option to pass to bitmap reader\n");

	exit(1);
	}
/*...e*/
/*...smain:0:*/
static BOOLEAN guess     = TRUE;
static BOOLEAN silent    = FALSE;
static BOOLEAN multipage = FALSE;

/*...sshow_error:0:*/
static void show_error(const char *fn, const char *reason)
	{
	if ( !silent )
		fprintf(stderr, "%s: %s - %s\n", progname, fn, reason);
	}
/*...e*/
/*...sshow:0:*/
/*...sshow_guess:0:*/
static void show_guess(const char *fn, const char *opt, int fd)
{
    int ft, rc;
    int imgcount = 0;
    long filelen, datalen;
    GBMFT gbmft;
    GBM gbm;

    if ( gbm_guess_filetype(fn, &ft) != GBM_ERR_OK )
    {
        gbm_io_close(fd);
        show_error(fn, "can't guess bitmap file format from extension");
        return;
    }

    gbm_query_filetype(ft, &gbmft);

    /* ignore all user options */
    if (multipage)
    {
        int i;
        BOOLEAN ext_bpp = FALSE;

        if ( (rc = gbm_read_imgcount(fn, fd, ft, &imgcount)) != GBM_ERR_OK )
        {
            char s[100+1];
            gbm_io_close(fd);
            sprintf(s, "can't read file header: %s", gbm_err(rc));
            show_error(fn, s);
            return;
        }

        ext_bpp = (strstr(opt, "ext_bpp") != NULL);

        filelen = gbm_io_lseek(fd, 0L, GBM_SEEK_END);
        datalen = 0;

        for (i = 0; i < imgcount; i++)
        {
            char opt_index[50+1];

            sprintf(opt_index, "%s index=%d", ext_bpp ? "ext_bpp" : "", i);

            if ( (rc = gbm_read_header(fn, fd, ft, &gbm, opt_index)) != GBM_ERR_OK )
            {
                char s[100+1];
                gbm_io_close(fd);
                sprintf(s, "can't read file header: %s", gbm_err(rc));
                show_error(fn, s);
                return;
            }

            datalen += (gbm.w*gbm.h*gbm.bpp)/8;
            printf("Index %3d: %4dx%-4d %2dbpp %5ldKb %4d%% %-10s %s\n",
                   i,
                   gbm.w, gbm.h, gbm.bpp,
                   (filelen+1023)/1024,
                   (filelen*100)/datalen,
                   gbmft.short_name,
                   fn);
        }
        printf("Compression ratio: %3ld%%\n", filelen*100/datalen);
    }
    else
    {
        if ( (rc = gbm_read_header(fn, fd, ft, &gbm, opt)) != GBM_ERR_OK )
        {
            char s[100+1];
            gbm_io_close(fd);
            sprintf(s, "can't read file header: %s", gbm_err(rc));
            show_error(fn, s);
            return;
        }

        filelen = gbm_io_lseek(fd, 0L, GBM_SEEK_END);
        datalen = (gbm.w*gbm.h*gbm.bpp)/8;
        printf("%4dx%-4d %2dbpp %5ldKb %4d%% %-10s %s\n",
               gbm.w, gbm.h, gbm.bpp,
               (filelen+1023)/1024,
               (filelen*100)/datalen,
               gbmft.short_name,
               fn);
    }
}
/*...e*/
/*...sshow_noguess:0:*/
static void show_noguess(const char *fn, const char *opt, int fd)
	{
	int ft, n_ft;
	GBMFT gbmft;

	printf("%5ldKb %s\n",
		(gbm_io_lseek(fd, 0L, GBM_SEEK_END) + 1023) / 1024,
		fn);

	if ( gbm_guess_filetype(fn, &ft) == GBM_ERR_OK )
		{
		gbm_query_filetype(ft, &gbmft);
		printf("  file extension suggests bitmap format may be %-10s\n",
			gbmft.short_name);
		}

	gbm_query_n_filetypes(&n_ft);

	for ( ft = 0; ft < n_ft; ft++ )
		{
		GBM gbm;
		if ( gbm_read_header(fn, fd, ft, &gbm, opt) == GBM_ERR_OK )
			{
			gbm_query_filetype(ft, &gbmft);
			printf("  reading header suggests bitmap format may be %-10s - %4dx%-4d %2dbpp\n",
				gbmft.short_name, gbm.w, gbm.h, gbm.bpp);
			}
		}
	}
/*...e*/

static void show(const char *fn, const char *opt)
	{
	int	fd;
	struct stat buf;

	if ( stat(fn, &buf) != -1 && (buf.st_mode & S_IFDIR) == S_IFDIR )
		/* Is a directory */
		{
		show_error(fn, "is a directory");
		return;
		}

	if ( (fd = gbm_io_open(fn, GBM_O_RDONLY)) == -1 )
		{
		show_error(fn, "can't open");
		return;
		}

	if ( guess )
		show_guess(fn, opt, fd);
	else
		show_noguess(fn, opt, fd);

	gbm_io_close(fd);
	}
/*...e*/

int main(int argc, char *argv[])
	{
	int i;

/*...sprocess command line options:8:*/
if ( argc == 1 )
	usage();

for ( i = 1; i < argc; i++ )
	{
	if ( argv[i][0] != '-' )
		break;
	else if ( argv[i][1] == '-' )
		{ ++i; break; }
	switch ( argv[i][1] )
		{
		case 'g':	guess     = FALSE;	break;
		case 's':	silent    = TRUE;	break;
		case 'c':	multipage = TRUE;	break;
		default:	usage();	break;
		}
	}
/*...e*/

	for ( ; i < argc; i++ )
/*...shandle a filename argument:16:*/
{
char fn[500+1], *opt;
strncpy(fn, argv[i], 500);
if ( (opt = strchr(fn, ',')) != NULL )
	*opt++ = '\0';
else
	opt = "";
show(fn, opt);
}
/*...e*/
		
	return 0;
	}
/*...e*/
