#define  INCL_DOS
#include <os2.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <process.h>
#include <string.h>

#ifndef TCPV40HDRS
#include <unistd.h>
#include <arpa/inet.h>
#endif

#ifdef OS2
  #include <errno.h>
#else
  #include <sys/errno.h>
#endif

#include "utilfct.h"
#include "httpget.h"
#include "sockfile.h"

size_t internal_fread(void *buffer, size_t size, size_t count, FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   switch(sockfile->sockmode)
   {
      case HTTP:
      {
         int temp = readbuffer(buffer, size*count, sockfile->handle);
         sockfile->position += temp;
         return temp;
      }
      default:
         return fread(buffer,size,count,sockfile->stream);
   }
}

#define CHUNK_SIZE 512

void TFNENTRY readahead_thread(void *arg)
{
   SOCKFILE *sockfile = arg;
   ULONG resetcount;
   BOOL finished = FALSE;

   do
   {
      DosWaitEventSem(sockfile->fillbuffer, -1);
      DosResetEventSem(sockfile->fillbuffer,&resetcount);

      while(sockfile->available+CHUNK_SIZE <= sockfile->buffersize && !finished)
      {
         int read = 0;
         char chunk[CHUNK_SIZE];

         if(!DosRequestMutexSem(sockfile->accessfile, -1))
         {
            read = internal_fread(chunk,1,CHUNK_SIZE,(FILE *) sockfile);
            DosReleaseMutexSem(sockfile->accessfile);
         }

         if(read && !DosRequestMutexSem(sockfile->accessbuffer, -1))
         {
            if(sockfile->bufferpos+read > sockfile->buffersize)
            {
               int endspace = sockfile->buffersize-sockfile->bufferpos;
               memcpy(&sockfile->buffer[sockfile->bufferpos], chunk, endspace);
               memcpy(&sockfile->buffer[0],chunk+endspace,read-endspace);
            }
            else
               memcpy(&sockfile->buffer[sockfile->bufferpos],chunk,read);

            sockfile->bufferpos += read;
            if(sockfile->bufferpos >= sockfile->buffersize)
               sockfile->bufferpos -= sockfile->buffersize;
            sockfile->available += read;
            DosReleaseMutexSem(sockfile->accessbuffer);
            DosPostEventSem(sockfile->bufferchanged);
         }
         else
         {
            DosReleaseMutexSem(sockfile->accessbuffer);
            DosPostEventSem(sockfile->bufferchanged);
            break; // end of stream or something
         }
      }
      DosPostEventSem(sockfile->bufferfilled);

   } while(!finished);

   sockfile->bufferthread = 0;
   DosPostEventSem(sockfile->bufferchanged);

   return;
}

int PM123_ENTRY sockfile_errno(int sockmode)
{
   if(sockmode)
      return 1000+sockmode;
   else
      return errno;
}

int PM123_ENTRY sockfile_bufferstatus(FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;
   return sockfile->available;
}

int PM123_ENTRY sockfile_abort(FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(!sockfile) return -1;

   switch(sockfile->sockmode)
   {
      case HTTP:
      {
         int handle = sockfile->handle;
         return so_cancel(handle);
      }

      default:
         return 0;
   }
}


int PM123_ENTRY sockfile_nobuffermode(FILE *stream, int setnobuffermode)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(sockfile->buffersize && !DosRequestMutexSem(sockfile->accessfile, -1)
                           && !DosRequestMutexSem(sockfile->accessbuffer, -1))
   {
      if(setnobuffermode)
      {
        /* reposition file */
         if(!sockfile->sockmode)
            fseek(sockfile->stream, -sockfile->available, SEEK_CUR);
         sockfile->nobuffermode = TRUE;
         sockfile->available = 0;
      }
      else
      {
         sockfile->nobuffermode = FALSE;
         DosPostEventSem(sockfile->fillbuffer);
      }
      DosReleaseMutexSem(sockfile->accessbuffer);
      DosReleaseMutexSem(sockfile->accessfile);
      return TRUE;
   }
   return FALSE;
}

size_t PM123_ENTRY xio_fread(void *buffer, size_t size, size_t count, FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(sockfile->buffersize && !sockfile->nobuffermode
      && !DosWaitEventSem(sockfile->bufferfilled, -1))
   {
      int read = 0;
      int toread = size*count;
      int i, e;
      ULONG resetcount;

      DosResetEventSem(sockfile->bufferchanged,&resetcount);
      while(sockfile->available < toread)
      {
         int prevavailable = sockfile->available;

         DosPostEventSem(sockfile->fillbuffer);
         DosWaitEventSem(sockfile->bufferchanged, -1);
         DosResetEventSem(sockfile->bufferchanged,&resetcount);

         if(sockfile->available == prevavailable)
         {
            toread = sockfile->available;
            break;
         }
      }

      if(toread && !DosRequestMutexSem(sockfile->accessbuffer, -1))
      {
         for(i = toread-1, e = sockfile->bufferpos-1-(sockfile->available-toread); i >= 0 ; i--, e--)
         {
            if(e < 0)
               e = sockfile->buffersize+e;

            ((char *) buffer)[i] = sockfile->buffer[e];
         }
         read = toread;
         sockfile->available -= read;
         DosReleaseMutexSem(sockfile->accessbuffer);
         DosPostEventSem(sockfile->fillbuffer);
      }
      return read;
   }
   else
      return internal_fread(buffer,size,count,stream);
}

long PM123_ENTRY xio_ftell(FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(sockfile->sockmode)
      return sockfile->position-sockfile->available;
   else
      return ftell(sockfile->stream)-sockfile->available;
}

size_t PM123_ENTRY xio_fsize(FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if( sockfile->sockmode == HTTP ) {
      return sockfile->http_info.length;
   } else if( sockfile->sockmode == FTP ) {
      return 0;
   } else {
      struct stat fi = {0};
      fstat( fileno( sockfile->stream ), &fi );
      return fi.st_size;
   }
}

int PM123_ENTRY xio_fseek(FILE *stream, long offset, int origin)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(sockfile->sockmode)
      return -1;
   else
   {
      if(sockfile->buffersize && !sockfile->nobuffermode
         && !DosRequestMutexSem(sockfile->accessfile, -1)
         && !DosRequestMutexSem(sockfile->accessbuffer, -1))
      {
         int rc;
         if(origin == SEEK_CUR) offset -= sockfile->available;
         sockfile->available = 0;
         rc = fseek (sockfile->stream, offset, origin);
         sockfile->justseeked = 1;
         DosReleaseMutexSem(sockfile->accessbuffer);
         DosReleaseMutexSem(sockfile->accessfile);
         DosPostEventSem(sockfile->fillbuffer);
         return rc;
      }
      else
         return fseek (sockfile->stream, offset, origin);
   }
}

void PM123_ENTRY xio_rewind (FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(!sockfile->sockmode)
   {
      if(sockfile->buffersize
         && !DosRequestMutexSem(sockfile->accessfile, -1)
         && !DosRequestMutexSem(sockfile->accessbuffer, -1))
      {
         sockfile->available = 0;
         rewind(sockfile->stream);
         sockfile->justseeked = 1;
         DosReleaseMutexSem(sockfile->accessbuffer);
         DosReleaseMutexSem(sockfile->accessfile);
         DosPostEventSem(sockfile->fillbuffer);
      }
      else
         rewind(sockfile->stream);
   }
}

FILE* PM123_ENTRY xio_fopen (const char *fname, const char *mode, int sockmode, int buffersize, int bufferwait)
{
   SOCKFILE *sockfile = calloc(1,sizeof(SOCKFILE));

   switch(sockmode)
   {
      case HTTP:
      {
         sockfile->handle = http_open(fname,&sockfile->http_info);
         if(!sockfile->handle)
         {
            free(sockfile);
            return NULL;
         }
         break;
      }

      default:
      {
/* VAC++ fopen() doesn't work well with sharing */
#if defined(__IBMC__)
         int oflag, shflag;

         if(strchr(mode,'r'))
            oflag = O_RDONLY;
         else if(strchr(mode,'w'))
            oflag = O_CREAT | O_TRUNC | O_WRONLY;
         else if(strchr(mode,'a'))
            oflag = O_CREAT | O_APPEND | O_WRONLY;

         if(strchr(mode,'+'))
            oflag |= O_RDWR & ~O_RDONLY & ~O_WRONLY;
         if(strchr(mode,'b'))
            oflag |= O_BINARY;
         else
            oflag |= O_TEXT;

         if(oflag & O_RDONLY)
            shflag = SH_DENYNO;
         else if((oflag & O_WRONLY) || (oflag & O_RDWR))
            shflag = SH_DENYWR;

         sockfile->stream = fdopen(_sopen(fname,oflag,shflag,S_IWRITE),mode);
#else
         sockfile->stream = fopen(fname, mode);
#endif
         if(!sockfile->stream)
         {
            free(sockfile);
            return NULL;
         }
         break;
      }

   }

   sockfile->sockmode = sockmode;
   if((sockfile->buffersize = buffersize) != 0)
   {
      sockfile->buffer = (char *) malloc(buffersize);
      if(sockfile->buffer)
      {
         sockfile->available = 0;
         sockfile->bufferpos = 0;
         sockfile->nobuffermode = FALSE;
         DosCreateEventSem(NULL,&sockfile->fillbuffer,0,TRUE);
         DosCreateEventSem(NULL,&sockfile->bufferfilled,0,!bufferwait);
         DosCreateEventSem(NULL,&sockfile->bufferchanged,0,FALSE);
         DosCreateMutexSem(NULL,&sockfile->accessbuffer,0,FALSE);
         DosCreateMutexSem(NULL,&sockfile->accessfile,0,FALSE);
         sockfile->bufferthread = _beginthread(readahead_thread,0,64*1024,(void *) sockfile);
         DosSetPriority(PRTYS_THREAD,3,0,sockfile->bufferthread);
      }
      else
      {
         sockfile->buffersize = 0;
         sockfile->available = 0;
      }
   }
   return (FILE *) sockfile;
}

int PM123_ENTRY sockfile_httpinfo(FILE *stream, HTTP_INFO *http_info)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;
   *http_info = sockfile->http_info;
   return http_info->length;
}

int PM123_ENTRY xio_fclose (FILE *stream)
{
   SOCKFILE *sockfile = (SOCKFILE *) stream;

   if(!sockfile) return EOF;

   if(sockfile->buffersize)
   {
      DosKillThread(sockfile->bufferthread);
      free(sockfile->buffer);
      DosCloseEventSem(sockfile->fillbuffer);
      DosCloseEventSem(sockfile->bufferfilled);
      DosCloseEventSem(sockfile->bufferchanged);
      DosCloseMutexSem(sockfile->accessbuffer);
      DosCloseMutexSem(sockfile->accessfile);
   }

   switch(sockfile->sockmode)
   {
      case HTTP:
      {
         int handle = sockfile->handle;
         free(sockfile);
         return soclose(handle);
      }

      default:
      {
         FILE *stream = sockfile->stream;
         free(sockfile);
         return fclose(stream);
      }
   }

}
