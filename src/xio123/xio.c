/*
 * Copyright 2006 Dmitry A.Steklenev
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <nerrno.h>
#include <netdb.h>
#include <sys/socket.h>

#include "xio.h"
#include "xio_protocol.h"
#include "xio_buffer.h"
#include "xio_file.h"
#include "xio_ftp.h"
#include "xio_http.h"
#include "xio_socket.h"
#include "utilfct.h"
#include "decoder_plug.h"

static char   http_proxy_host[XIO_MAX_HOSTNAME];
static int    http_proxy_port;
static char   http_proxy_user[XIO_MAX_USERNAME];
static char   http_proxy_pass[XIO_MAX_PASSWORD];
static u_long http_proxy_addr;

static int  buffer_size = 32768;
static int  buffer_wait = 0;

/* Serializes access to the library's global data. */
static HMTX mutex;

/* Cleanups the file structure. */
static void
xio_terminate( XFILE* x )
{
  if( x->buffer ) {
    // At end the buffer itself will clear all others.
    buffer_terminate( x );
  } else {
    if( x->protocol ) {
      x->protocol->clean( x );
    }
    free( x );
  }
}

/* Open file. Returns a pointer to a file structure that can be used
   to access the open file. A NULL pointer return value indicates an
   error. */
XFILE* PM123_ENTRY
xio_fopen( const char* filename, const char* mode )
{
  XFILE* x;

  if( !filename || !*filename ) {
    errno = ENOENT;
    return NULL;
  }
  if( !( x = calloc( 1, sizeof( XFILE )))) {
    return NULL;
  }

  if( strnicmp( filename, "http:", 5 ) == 0 )
  {
    x->scheme   = XIO_HTTP;
    x->protocol = http_initialize( x );
  }
  else if( strnicmp( filename, "ftp:", 4 ) == 0 )
  {
    if( *http_proxy_host ) {
      x->scheme   = XIO_HTTP;
      x->protocol = http_initialize( x );
    } else {
      x->scheme   = XIO_FTP;
      x->protocol = ftp_initialize( x );
    }
  } else {
    x->scheme   = XIO_FILE;
    x->protocol = file_initialize( x );
  }

  if( !x->protocol ) {
    xio_terminate( x );
    return NULL;
  }

  if( strchr( mode, 'r' )) {
    if( strchr( mode, '+' )) {
      x->oflags = XO_READ | XO_WRITE;
    } else {
      x->oflags = XO_READ;
    }
  } else if( strchr( mode, 'w' )) {
    if( strchr( mode, '+' )) {
      x->oflags = XO_WRITE | XO_CREATE | XO_TRUNCATE | XO_READ;
    } else {
      x->oflags = XO_WRITE | XO_CREATE | XO_TRUNCATE;
    }
  } else if( strchr( mode, 'a' )) {
    if( strchr( mode, '+' )) {
      x->oflags = XO_WRITE | XO_CREATE | XO_APPEND | XO_READ;
    } else {
      x->oflags = XO_WRITE | XO_CREATE | XO_APPEND;
    }
  } else {
    xio_terminate( x );
    errno = EINVAL;
    return NULL;
  }

  if((( x->oflags & XO_READ   ) &&
      ( x->oflags & XO_WRITE  ) && !( x->protocol->supports & XS_CAN_READWRITE )) ||
     (( x->oflags & XO_READ   ) && !( x->protocol->supports & XS_CAN_READ      )) ||
     (( x->oflags & XO_WRITE  ) && !( x->protocol->supports & XS_CAN_WRITE     )) ||
     (( x->oflags & XO_CREATE ) && !( x->protocol->supports & XS_CAN_CREATE    ))  )
  {
    xio_terminate( x );
    errno = EINVAL;
    return NULL;
  }

  if( x->protocol->open( x, filename, x->oflags ) != 0 ) {
    xio_terminate( x );
    return NULL;
  }

  buffer_initialize( x );
  return x;
}

/* Reads specified chunk of the data and notifies an attached
   observer about streaming metadata. Returns the number of
   bytes placed in result. The return value 0 indicates an attempt
   to read at end-of-file. A return value -1 indicates an error. */
static int
xio_read_and_notify( XFILE* x, char* result, unsigned int count )
{
  int read_size;
  int read_done;
  int done;
  int i;

  unsigned char metahead;
  int           metasize;
  char*         metabuff;
  char*         titlepos;

  // Note: x->protocol->s_metaint and x->protocol->s_metapos
  // do not changed by a read-ahead thread.

  if( !x->protocol->s_metaint ) {
    return buffer_read( x, result, count );
  }

  read_done = 0;

  while( read_done < count ) {
    if( x->protocol->s_metapos == 0 )
    {
      // Time to read metadata from a input stream.
      metahead = 0;
      done = buffer_read( x, &metahead, 1 );

      if( done ) {
        if( metahead ) {
          metasize = metahead * 16;

          if(( metabuff = malloc( metasize + 1 )) == NULL ) {
            return -1;
          }
          if(( done = buffer_read( x, metabuff, metasize )) != metasize ) {
            return -1;
          }

          metabuff[done] = 0;
          DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );

          if(( titlepos = strstr( metabuff, "StreamTitle='" )) != NULL )
          {
            titlepos += 13;
            for( i = 0; i < sizeof( x->protocol->s_title ) - 1 && *titlepos
                        && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
            {
              x->protocol->s_title[i] = *titlepos++;
            }

            x->protocol->s_title[i] = 0;
          }

          if( x->protocol->s_observer &&
              x->protocol->s_metabuff )
          {
            strlcpy( x->protocol->s_metabuff, metabuff, x->protocol->s_metasize );
            WinPostMsg( x->protocol->s_observer, WM_METADATA,
                        MPFROMP( x->protocol->s_metabuff ), 0 );
          }

          if( x->protocol->supports & XS_USE_SPOS ) {
            x->protocol->s_pos -= ( metasize + 1 );
          }

          DosReleaseMutexSem( x->protocol->mtx_access );
        }
        x->protocol->s_metapos = x->protocol->s_metaint;
      }
    }

    // Determines the maximum size of the data chunk for reading.
    read_size = count - read_done;
    read_size = min( read_size, x->protocol->s_metapos );

    done = buffer_read( x, result + read_done, read_size );

    if( !done || done == -1 ) {
      break;
    }

    read_done += done;
    x->protocol->s_metapos -= done;
  }
  return read_done;
}

/* Reads up to count items of size length from the input file and
   stores them in the given buffer. The position in the file increases
   by the number of bytes read. Returns the number of full items
   successfully read, which can be less than count if an error occurs
   or if the end-of-file is met before reaching count. */
size_t PM123_ENTRY
xio_fread( void* buffer, size_t size, size_t count, XFILE* x )
{
  int done;
  int read = count * size;
  int rc   = 0;

  DosRequestMutexSem( x->protocol->mtx_file, SEM_INDEFINITE_WAIT );

  if(!( x->oflags & XO_READ )) {
    errno = EINVAL;
  } else {
    done = xio_read_and_notify( x, buffer, read );

    if( done == read ) {
      rc = count;
    } else if( done >= 0 ) {
      buffer_seek( x, -( done % size ), XIO_SEEK_CUR );
      rc = done / size;
    }
  }

  DosReleaseMutexSem( x->protocol->mtx_file );
  return rc;
}

/* Writes up to count items, each of size bytes in length, from buffer
   to the output file. Returns the number of full items successfully
   written, which can be fewer than count if an error occurs. */
size_t PM123_ENTRY
xio_fwrite( void* buffer, size_t size, size_t count, XFILE* x )
{
  int   write  = count * size;
  int   rc     = 0;
  int   done;

  DosRequestMutexSem( x->protocol->mtx_file, SEM_INDEFINITE_WAIT );

  if(!( x->oflags & XO_WRITE )) {
    errno = EBADF;
  } else {
    done = buffer_write( x, buffer, write );

    if( done == write ) {
      rc = count;
    } else if( done >= 0 ) {
      buffer_seek( x, -( done % size ), XIO_SEEK_CUR );
      rc = done / size;
    }
  }

  DosReleaseMutexSem( x->protocol->mtx_file );
  return rc;
}

/* Closes a file pointed to by x. Returns 0 if it successfully closes
   the file, or -1 if any errors were detected. */
int PM123_ENTRY
xio_fclose( XFILE* x )
{
  DosRequestMutexSem( x->protocol->mtx_file, SEM_INDEFINITE_WAIT );

  if( x->protocol->abort || x->protocol->close( x ) == 0 ) {
    xio_terminate( x );
    return 0;
  }

  DosReleaseMutexSem( x->protocol->mtx_file );
  xio_terminate( x );
  return -1;
}

/* Causes an abnormal termination of all current read/write
   operations of the file. All current and subsequent calls can
   raise an error. */
void PM123_ENTRY
xio_fabort( XFILE* x )
{
  if( !x->protocol->abort ) {
    x->protocol->close( x );
    x->protocol->abort = 1;
  }
}

/* Finds the current position of the file. Returns the current file
   position. On error, returns -1L and sets errno to a nonzero value. */
long PM123_ENTRY
xio_ftell( XFILE* x ) {
  return buffer_tell( x );
}

/* Changes the current file position to a new location within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
int PM123_ENTRY
xio_fseek( XFILE* x, long int offset, int origin )
{
  int rc = -1;

  DosRequestMutexSem( x->protocol->mtx_file, SEM_INDEFINITE_WAIT );

  if( x->protocol->s_metaint ) {
    errno = EINVAL;
  } else if( buffer_seek( x, offset, origin ) != -1 ) {
    rc = 0;
  }

  DosReleaseMutexSem( x->protocol->mtx_file );
  return rc;
}

/* Repositions the file pointer associated with stream to the beginning
   of the file. A call to xio_rewind is the same as:
   (void)xio_fseek( x, 0L, XIO_SEEK_SET )
   except that xio_rewind also clears the error indicator for
   the stream. */
void PM123_ENTRY
xio_rewind( XFILE* x )
{
  if( !x->protocol->s_metaint ) {
    xio_fseek( x, 0, XIO_SEEK_SET );
  }
  errno = 0;
}

/* Returns the size of the file. A return value of -1L indicates an
   error or an unknown size. */
long PM123_ENTRY
xio_fsize( XFILE* x ) {
  return buffer_filesize( x );
}

/* Reads bytes from the current file position up to and including the
   first new-line character (\n), up to the end of the file, or until
   the number of bytes read is equal to n-1, whichever comes first.
   Stores the result in string and adds a null character (\0) to the
   end of the string. The string includes the new-line character, if
   read. If n is equal to 1, the string is empty. Returns a pointer
   to the string buffer if successful. A NULL return value indicates
   an error or an end-of-file condition. */
char* PM123_ENTRY
xio_fgets( char* string, int n, XFILE* x )
{
  int done = 0;

  DosRequestMutexSem( x->protocol->mtx_file, SEM_INDEFINITE_WAIT );

  while( done < n - 1 ) {
    if( xio_read_and_notify( x, string, 1 ) == 1 ) {
      if( *string == '\r' ) {
        continue;
      } else if( *string == '\n' ) {
        ++string;
        ++done;
        break;
      } else {
        ++string;
        ++done;
      }
    } else {
      break;
    }
  }

  DosReleaseMutexSem( x->protocol->mtx_file );

  *string = 0;
  return done ? string : NULL;
}

/* Copies string to the output file at the current position.
   It does not copy the null character (\0) at the end of the string.
   Returns -1 if an error occurs; otherwise, it returns a non-negative
   value. */
int PM123_ENTRY
xio_fputs( const char* string, XFILE* x )
{
  char* cr = "\r";
  int   rc = 0;

  DosRequestMutexSem( x->protocol->mtx_file, SEM_INDEFINITE_WAIT );

  while( *string ) {
    if( *string == '\n' ) {
      if( buffer_write( x, cr, 1 ) != 1 ) {
        rc = -1;
        break;
      }
    }
    if( buffer_write( x, string++, 1 ) != 1 ) {
      rc = -1;
      break;
    }
  }

  DosReleaseMutexSem( x->protocol->mtx_file );
  return rc;
}

/* Returns the last error code set by a library call in the current
   thread. Subsequent calls do not reset this error code. */
int PM123_ENTRY
xio_errno( void ) {
  return errno;
}

/* Maps the error number in errnum to an error message string. */
const char* PM123_ENTRY
xio_strerror( int errnum )
{
  if( errnum >= FTPBASEERR ) {
    return ftp_strerror( errnum );
  } else if( errnum >= HTTPBASEERR ) {
    return http_strerror( errnum );
  } else if( errnum >= HBASEERR ) {
    return h_strerror( errnum - HBASEERR );
  #ifdef SOCBASEERR
  } else if( errnum >= SOCBASEERR ) {
    return sock_strerror( errnum );
  #endif
  } else {
    return strerror( errnum );
  }
}

/* Sets a handle of a window that are to be notified of changes
   in the state of the library. */
void PM123_ENTRY
xio_set_observer( XFILE* x, unsigned long window,
                            char* buffer, int buffer_size )
{
  DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );
  x->protocol->s_observer = window;
  x->protocol->s_metabuff = buffer;
  x->protocol->s_metasize = buffer_size;
  DosReleaseMutexSem( x->protocol->mtx_access );
}

/* Returns a specified meta information if it is
   provided by associated stream. */
char* PM123_ENTRY
xio_get_metainfo( XFILE* x, int type, char* result, int size )
{
  DosRequestMutexSem( x->protocol->mtx_access, SEM_INDEFINITE_WAIT );

  switch( type ) {
    case XIO_META_GENRE :  strlcpy( result, x->protocol->s_genre, size ); break;
    case XIO_META_NAME  :  strlcpy( result, x->protocol->s_name , size ); break;
    case XIO_META_TITLE :  strlcpy( result, x->protocol->s_title, size ); break;
    default:
      *result = 0;
  }
  DosReleaseMutexSem( x->protocol->mtx_access );
  return result;
}

/* Returns XIO_NOT_SEEK (0) on streams incapable of seeking,
   XIO_CAN_SEEK (1) on streams capable of seeking and returns
   XIO_CAN_SEEK_FAST (2) on streams capable of fast seeking. */
int PM123_ENTRY
xio_can_seek( XFILE* x )
{
  if( !x->protocol->s_metaint ) {
    if( x->protocol->supports & XS_CAN_SEEK_FAST ) {
      return XIO_CAN_SEEK_FAST;
    } else if( x->protocol->supports & XS_CAN_SEEK ) {
      return XIO_CAN_SEEK;
    }
  }
  return XIO_NOT_SEEK;
}

/* Returns the read-ahead buffer size. */
int PM123_ENTRY
xio_buffer_size( void )
{
  int size;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  size = buffer_size;
  DosReleaseMutexSem( mutex );
  return size;
}

/* Returns fills the buffer before reading state. */
int PM123_ENTRY
xio_buffer_wait( void )
{
  int wait;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  wait = buffer_wait;
  DosReleaseMutexSem( mutex );
  return wait;
}

/* Sets the read-ahead buffer size. */
void PM123_ENTRY
xio_set_buffer_size( int size )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  buffer_size = size;
  DosReleaseMutexSem( mutex );
}

/* Sets fills the buffer before reading state. */
void PM123_ENTRY
xio_set_buffer_wait( int wait )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  buffer_wait = wait;
  DosReleaseMutexSem( mutex );
}

/* Returns the name of the proxy server. */
void PM123_ENTRY
xio_http_proxy_host( char* hostname, int size )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( hostname, http_proxy_host, size );
  DosReleaseMutexSem( mutex );
}

/* Returns the port number of the proxy server. */
int PM123_ENTRY
xio_http_proxy_port( void )
{
  int port;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  port = http_proxy_port;
  DosReleaseMutexSem( mutex );
  return port;
}

/* Returns the user name of the proxy server. */
void PM123_ENTRY
xio_http_proxy_user( char* username, int size )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( username, http_proxy_user, size );
  DosReleaseMutexSem( mutex );
}

/* Returns the user password of the proxy server. */
void PM123_ENTRY
xio_http_proxy_pass( char* password, int size )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( password, http_proxy_pass, size );
  DosReleaseMutexSem( mutex );
}

/* Sets the name of the proxy server. */
void PM123_ENTRY
xio_set_http_proxy_host( char* hostname )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( http_proxy_host, hostname, sizeof( http_proxy_host ));
  http_proxy_addr = 0;
  DosReleaseMutexSem( mutex );
}

/* Sets the port number of the proxy server. */
void PM123_ENTRY
xio_set_http_proxy_port( int port )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  http_proxy_port = port;
  DosReleaseMutexSem( mutex );
}

/* Sets the user name of the proxy server. */
void PM123_ENTRY
xio_set_http_proxy_user( char* username )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( http_proxy_user, username, sizeof( http_proxy_user ));
  DosReleaseMutexSem( mutex );
}

/* Sets the user password of the proxy server. */
void PM123_ENTRY
xio_set_http_proxy_pass( char* password )
{
  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( http_proxy_pass, password, sizeof( http_proxy_pass ));
  DosReleaseMutexSem( mutex );
}

/* Returns an internet address of the proxy server.
   Returns 0 if the proxy server is not defined or -1 if
   an error occurs */
unsigned long PM123_ENTRY
xio_http_proxy_addr( void )
{
  char host[XIO_MAX_HOSTNAME];
  u_long address;

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  strlcpy( host, http_proxy_host, sizeof( host ));
  address = http_proxy_addr;
  DosReleaseMutexSem( mutex );

  if( address != 0 && address != -1 ) {
    return address;
  }
  if( !*host ) {
    return 0;
  }

  address = so_get_address( host );

  DosRequestMutexSem( mutex, SEM_INDEFINITE_WAIT );
  http_proxy_addr = address;
  DosReleaseMutexSem( mutex );
  return address;
}

int INIT_ATTRIBUTE __dll_initialize( void )
{
  if( DosCreateMutexSem( NULL, &mutex, 0, FALSE ) == NO_ERROR ) {
    return 1;
  } else {
    return 0;
  }
}

int TERM_ATTRIBUTE __dll_terminate( void )
{
  if( DosCloseMutexSem( mutex ) == NO_ERROR ) {
    return 1;
  } else {
    return 0;
  }
}

#if defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag       )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return __dll_initialize();
  } else {
    #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
    #endif
    __dll_terminate();
    _CRT_term();
    return 1UL;
  }
}
#endif
