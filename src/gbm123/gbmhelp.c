/*

gbmhelp.c - Helpers for GBM file I/O stuff

*/

/*...sincludes:0:*/
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
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

/*...sgbm_same:0:*/
BOOLEAN gbm_same(const char *s1, const char *s2, int n)
	{
	for ( ; n--; s1++, s2++ )
		if ( tolower(*s1) != tolower(*s2) )
			return FALSE;
	return TRUE;
	}
/*...e*/
/*...sgbm_find_word:0:*/
const char *gbm_find_word(const char *str, const char *substr)
	{
	char buf[100+1], *s;
	int  len = strlen(substr);

	for ( s  = strtok(strcpy(buf, str), " \t,");
	      s != NULL;
	      s  = strtok(NULL, " \t,") )
		if ( gbm_same(s, substr, len) && s[len] == '\0' )
			{
			int inx = s - buf;
			return str + inx;
				/* Avoid referencing buf in the final return.
				   lcc and a Mac compiler see the buf, and then
				   warn about possibly returning the address
				   of an automatic variable! */
			}
	return NULL;
	}
/*...e*/
/*...sgbm_find_word_prefix:0:*/
const char *gbm_find_word_prefix(const char *str, const char *substr)
	{
	char buf[100+1], *s;
	int  len = strlen(substr);

	for ( s  = strtok(strcpy(buf, str), " \t,");
	      s != NULL;
	      s  = strtok(NULL, " \t,") )
		if ( gbm_same(s, substr, len) )
			{
			int inx = s - buf;
			return str + inx;
				/* Avoid referencing buf in the final return.
				   lcc and a Mac compiler see the buf, and then
				   warn about possibly returning the address
				   of an automatic variable! */
			}
	return NULL;
	}
/*...e*/


/* map supported public defines to compiler specific ones */
static int get_checked_internal_open_mode(int mode)
{
   const static int supported_open_modes = GBM_O_RDONLY |
                                           GBM_O_WRONLY |
                                           GBM_O_RDWR   |
                                           GBM_O_EXCL   |
                                           GBM_O_NOINHERIT;

   /* internal, compiler specific open mode */
   int open_mode = 0;

   /* map supported public defines to compiler specific ones */

   /* check if only supported modes are provided */
   if (!(mode & supported_open_modes))
   {
      return 0xffffffff;
   }

   /* mask external binary mode bit */
   mode &= ~GBM_O_BINARY;

   if (mode & GBM_O_RDONLY)
   {
      open_mode |= O_RDONLY;
   }
   else if (mode & GBM_O_WRONLY)
   {
      open_mode |= O_WRONLY;
   }
   else if (mode & GBM_O_RDWR)
   {
      open_mode |= O_RDWR;
   }
   else if (mode & GBM_O_EXCL)
   {
      open_mode |= O_EXCL;
   }
   else
   {
      return 0xffffffff;
   }

   if (mode & GBM_O_NOINHERIT)
   {
      open_mode |= O_NOINHERIT;
   }

   /* force binary mode if necessary */
   #ifdef O_BINARY
     open_mode |= O_BINARY;
   #endif

   return open_mode;
}

/*...sgbm_file_\42\:0:*/
/* Looking at this, you might think that the gbm_file_* function pointers
   could be made to point straight at the regular read,write etc..
   If we do this then we get into problems with different calling conventions
   (for example read is _Optlink under C-Set++ on OS/2), and also where
   function arguments differ (the length field to read is unsigned on OS/2).
   This simplest thing to do is simply to use the following veneers. */

static int GBMENTRY def_open(const char *fn, int mode)
{
   const int internal_mode = get_checked_internal_open_mode(mode);

   /* In case of a mapping error we get 0xffffffff which is an illegal mode.
    * So let the OS take care for correct error reporting.
    */
   return open(fn, internal_mode);
}


static int GBMENTRY def_create(const char *fn, int mode)
{
   const int internal_mode = get_checked_internal_open_mode(mode);

   /* In case of a mapping error we get 0xffffffff which is an illegal mode.
    * So let the OS take care for correct error reporting.
    */

#ifdef MAC
   return open(fn, O_CREAT | O_TRUNC | internal_mode);
		/* S_IREAD and S_IWRITE won't exist on the Mac until MacOS/X */
#else
   return open(fn, O_CREAT | O_TRUNC | internal_mode, S_IREAD | S_IWRITE);
#endif
}

static void GBMENTRY def_close(int fd)
	{ close(fd); }

static long GBMENTRY def_lseek(int fd, long pos, int whence)
{
   int internal_whence = -1;

   switch(whence)
   {
      case GBM_SEEK_SET:
         internal_whence = SEEK_SET;
         break;

      case GBM_SEEK_CUR:
         internal_whence = SEEK_CUR;
         break;

      case GBM_SEEK_END:
         internal_whence = SEEK_END;
         break;

      default:
         /* as we provide an illegal whence value, the OS takes care
          * for correct error reporting
          */
         break;
   }

   return lseek(fd, pos, internal_whence);
}


static int GBMENTRY def_read(int fd, void *buf, int len)
	{ return read(fd, buf, len); }

static int GBMENTRY def_write(int fd, const void *buf, int len)
#ifdef MAC
	/* Prototype for write is missing a 'const' */
	{ return write(fd, (void *) buf, len); }
#else
	{ return write(fd, buf, len); }
#endif

int  (GBMENTRYP gbm_file_open  )(const char *fn, int mode)         = def_open  ;
int  (GBMENTRYP gbm_file_create)(const char *fn, int mode)         = def_create;
void (GBMENTRYP gbm_file_close )(int fd)                           = def_close ;
long (GBMENTRYP gbm_file_lseek )(int fd, long pos, int whence)     = def_lseek ;
int  (GBMENTRYP gbm_file_read  )(int fd, void *buf, int len)       = def_read  ;
int  (GBMENTRYP gbm_file_write )(int fd, const void *buf, int len) = def_write ;

/* Reset the remappable I/O functions to the initial ones. */
void gbm_restore_file_io(void)
{
  gbm_file_open   = def_open  ;
  gbm_file_create = def_create;
  gbm_file_close  = def_close ;
  gbm_file_lseek  = def_lseek ;
  gbm_file_read   = def_read  ;
  gbm_file_write  = def_write ;
}


/*...e*/
/*...sreading ahead:0:*/
#define	AHEAD_BUF 0x4000

typedef struct
	{
	byte buf[AHEAD_BUF];
	int inx, cnt;
	int fd;
	} AHEAD;

AHEAD *gbm_create_ahead(int fd)
	{
	AHEAD *ahead;

	if ( (ahead = malloc((size_t) sizeof(AHEAD))) == NULL )
		return NULL;

	ahead->inx = 0;
	ahead->cnt = 0;
	ahead->fd  = fd;

	return ahead;
	}

void gbm_destroy_ahead(AHEAD *ahead)
	{
	free(ahead);
	}	

int gbm_read_ahead(AHEAD *ahead)
	{
	if ( ahead->inx >= ahead->cnt )
		{
		ahead->cnt = gbm_file_read(ahead->fd, (char *) ahead->buf, AHEAD_BUF);
		if ( ahead->cnt <= 0 )
			return -1;
		ahead->inx = 0;
		}
	return (int) (unsigned int) ahead->buf[ahead->inx++];
	}
/*...e*/
